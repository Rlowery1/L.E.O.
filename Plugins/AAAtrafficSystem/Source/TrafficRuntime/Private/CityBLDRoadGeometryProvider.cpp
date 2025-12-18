#include "CityBLDRoadGeometryProvider.h"

#include "Components/SplineComponent.h"
#include "Components/DynamicMeshComponent.h"
#include "VectorTypes.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TrafficGeometrySmoothing.h"
#include "TrafficCityBLDAdapterSettings.h"
#include "TrafficRoadFamilySettings.h"
#include "TrafficRoadMetadataComponent.h"
#include "TrafficRuntimeModule.h"
#include "TrafficRoadTypes.h"
#include "Algo/Reverse.h"

namespace
{
	static TAutoConsoleVariable<int32> CVarTrafficCityBLDWarnMissingCenterlineSplineTag(
		TEXT("aaa.Traffic.CityBLD.WarnMissingCenterlineSplineTag"),
		1,
		TEXT("If non-zero, warns when a CityBLD road actor has multiple splines but none are tagged with RoadSplineTag (e.g. CityBLD_Centerline)."),
		ECVF_Default);

	static bool IsCityBLDRoadCandidate(const AActor* Actor, const UTrafficCityBLDAdapterSettings* AdapterSettings)
	{
		if (!Actor)
		{
			return false;
		}

		if (AdapterSettings && !AdapterSettings->RoadActorTag.IsNone() && Actor->ActorHasTag(AdapterSettings->RoadActorTag))
		{
			return true;
		}

		const FString ClassName = Actor->GetClass()->GetName();

		// Back-compat: CityBLD roads are BP_MeshRoad actors by convention.
		if (ClassName.Contains(TEXT("BP_MeshRoad"), ESearchCase::IgnoreCase))
		{
			return true;
		}

		if (AdapterSettings)
		{
			for (const FString& Contains : AdapterSettings->RoadClassNameContains)
			{
				if (!Contains.IsEmpty() && ClassName.Contains(Contains, ESearchCase::IgnoreCase))
				{
					return true;
				}
			}
		}

		return false;
	}

	static bool ShouldWarnOncePerActor(const AActor* Actor)
	{
		if (!Actor)
		{
			return false;
		}

		static TSet<FString> WarnedActorPaths;
		const FString Key = Actor->GetPathName();
		if (WarnedActorPaths.Contains(Key))
		{
			return false;
		}

		WarnedActorPaths.Add(Key);
		return true;
	}

	static void SampleControlSpline(const USplineComponent* SplineComp, const TArray<FVector>& Vertices, TArray<FVector>& OutPoints)
	{
		if (!SplineComp)
		{
			return;
		}

		const int32 NumVerts = Vertices.Num();
		const float Length = SplineComp->GetSplineLength();
		const int32 SampleCount = FMath::Clamp(static_cast<int32>(Length / 200.f), 10, 200);
		OutPoints.Reserve(SampleCount + 1);
		const FVector Up(0.f, 0.f, 1.f);
		const float ApproxStep = (SampleCount > 0) ? (Length / static_cast<float>(SampleCount)) : 200.f;
		const float SliceHalfLength = FMath::Clamp(ApproxStep * 1.5f, 100.f, 600.f);

		for (int32 i = 0; i <= SampleCount; ++i)
		{
			const float Dist = Length * static_cast<float>(i) / static_cast<float>(SampleCount);
			const FVector Pos = SplineComp->GetLocationAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::World);

			if (NumVerts >= 2)
			{
				FVector Tangent = SplineComp->GetTangentAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::World);
				Tangent.Z = 0.f;
				if (Tangent.Normalize())
				{
					FVector Right = FVector::CrossProduct(Tangent, Up);
					Right.Normalize();

					float MinDist = FLT_MAX;
					float MaxDist = -FLT_MAX;
					float MaxZ = -FLT_MAX;
					int32 InSlice = 0;
					for (const FVector& V : Vertices)
					{
						const FVector Delta = V - Pos;
						const float Along = FVector::DotProduct(Delta, Tangent);
						if (FMath::Abs(Along) > SliceHalfLength)
						{
							continue;
						}

						const float D = FVector::DotProduct(Delta, Right);
						MinDist = FMath::Min(MinDist, D);
						MaxDist = FMath::Max(MaxDist, D);
						MaxZ = FMath::Max(MaxZ, V.Z);
						++InSlice;
					}

					if (InSlice >= 16 && MinDist < MaxDist)
					{
						const float Mid = 0.5f * (MinDist + MaxDist);
						FVector Fitted = Pos + Right * Mid;
						if (MaxZ > -FLT_MAX)
						{
							// Many modular kits keep the control spline at a constant Z even when the generated mesh/collision is elevated.
							// Use the mesh vertex Z in the local slice so centerline points match the drivable surface (prevents "lane Z=0 while road Z>0").
							Fitted.Z = MaxZ;
						}
						OutPoints.Add(Fitted);
						continue;
					}
				}
			}

			OutPoints.Add(Pos);
		}
	}

	static TAutoConsoleVariable<int32> CVarTrafficCityBLDProjectCenterlineZToCollision(
		TEXT("aaa.Traffic.CityBLD.ProjectCenterlineZToCollision"),
		1,
		TEXT("If non-zero, projects sampled centerline points onto the owning road actor's collision (fixes kits where the control spline Z is not at the drivable surface).\n")
		TEXT("Default: 1"),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarTrafficCityBLDProjectCenterlineZTraceHalfHeightCm(
		TEXT("aaa.Traffic.CityBLD.ProjectCenterlineZTraceHalfHeightCm"),
		20000.0f,
		TEXT("Half-height (cm) for the vertical trace used when projecting centerline points onto road collision.\n")
		TEXT("Default: 20000"),
		ECVF_Default);

	static void ProjectCenterlineZToActorCollision(const AActor& Actor, TArray<FVector>& Points)
	{
		if (CVarTrafficCityBLDProjectCenterlineZToCollision.GetValueOnAnyThread() == 0)
		{
			return;
		}

		UWorld* World = Actor.GetWorld();
		if (!World || Points.Num() == 0)
		{
			return;
		}

		const float HalfHeight = FMath::Max(0.f, CVarTrafficCityBLDProjectCenterlineZTraceHalfHeightCm.GetValueOnAnyThread());
		if (HalfHeight <= 0.f)
		{
			return;
		}

		FCollisionQueryParams Params(SCENE_QUERY_STAT(AAA_Traffic_CityBLD_ProjectZ), /*bTraceComplex=*/false);
		Params.bReturnPhysicalMaterial = false;

		for (FVector& P : Points)
		{
			TArray<FHitResult> Hits;
			const FVector Start(P.X, P.Y, P.Z + HalfHeight);
			const FVector End(P.X, P.Y, P.Z - HalfHeight);
			if (!World->LineTraceMultiByChannel(Hits, Start, End, ECC_WorldStatic, Params))
			{
				continue;
			}

			for (const FHitResult& Hit : Hits)
			{
				if (!Hit.bBlockingHit)
				{
					continue;
				}
				if (Hit.GetActor() != &Actor)
				{
					continue;
				}
				if (Hit.ImpactNormal.Z < 0.2f)
				{
					continue;
				}

				P.Z = Hit.ImpactPoint.Z;
				break;
			}
		}
	}

	static void BuildSmoothCenterline(const USplineComponent* ControlSpline, const TArray<FVector>& CollectedVerts, TArray<FVector>& OutPoints)
	{
		OutPoints.Reset();
		if (!ControlSpline)
		{
			return;
		}

		// 1. Sample the control spline and (when available) fit its midline to the generated road mesh.
		TArray<FVector> GuideSamples;
		SampleControlSpline(ControlSpline, CollectedVerts, GuideSamples);
		if (GuideSamples.Num() < 2)
		{
			OutPoints = GuideSamples;
			return;
		}

		// 2. Detect sharp corners and replace them with smooth Bezier segments.
		const float SpikeThreshold = FMath::DegreesToRadians(35.f); // threshold for curvature spikes
		TArray<FIntPoint> SpikeIntervals;
		TrafficGeometrySmoothing::DetectCurvatureSpikes(GuideSamples, SpikeThreshold, SpikeIntervals);

		TArray<FVector> GuideFixed;
		TrafficGeometrySmoothing::ReplaceSpikeRegions(GuideSamples, SpikeIntervals, GuideFixed);

		// 3. Apply Chaikin smoothing to reduce jitter.
		TArray<FVector> Smoothed;
		TrafficGeometrySmoothing::ChaikinSmooth(GuideFixed, 1, Smoothed);

		// 4. Convert to cubic Bezier segments and resample for a smooth final centreline.
		TArray<TrafficGeometrySmoothing::FBezierSegment> Segments;
		TrafficGeometrySmoothing::CatmullRomToBezier(Smoothed, Segments);
		TrafficGeometrySmoothing::SampleBezierSegments(Segments, 5, OutPoints);
	}
}

void UCityBLDRoadGeometryProvider::CollectRoads(UWorld* World, TArray<FTrafficRoad>& OutRoads)
{
	OutRoads.Reset();

	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[CityBLDRoadGeometryProvider] World is null."));
		return;
	}

	const UTrafficRoadFamilySettings* FamSettings = GetDefault<UTrafficRoadFamilySettings>();
	const TArray<FRoadFamilyDefinition>* FamiliesPtr = FamSettings ? &FamSettings->Families : nullptr;
	const int32 FamilyCount = FamiliesPtr ? FamiliesPtr->Num() : 0;
	const UTrafficCityBLDAdapterSettings* AdapterSettings = GetDefault<UTrafficCityBLDAdapterSettings>();

	int32 NextRoadId = 0;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}

		if (!IsCityBLDRoadCandidate(Actor, AdapterSettings))
		{
			continue;
		}

		UTrafficRoadMetadataComponent* Meta = Actor->FindComponentByClass<UTrafficRoadMetadataComponent>();
		if (Meta && !Meta->bIncludeInTraffic)
		{
			continue;
		}

		TArray<FVector> Centerline;
		if (!BuildCenterlineFromCityBLDRoad(Actor, Centerline) || Centerline.Num() < 2)
		{
			continue;
		}

		if (Meta && Meta->bReverseCenterlineDirection)
		{
			Algo::Reverse(Centerline);
		}

		int32 FamilyId = 0;
		if (FamSettings && FamilyCount > 0 && Meta && !Meta->FamilyName.IsNone())
		{
			const FRoadFamilyDefinition* Found = FamSettings->FindFamilyByName(Meta->FamilyName);
			if (Found)
			{
				const FRoadFamilyDefinition* Base = FamiliesPtr->GetData();
				const ptrdiff_t Offset = Found - Base;
				FamilyId = (Offset >= 0 && Offset < FamilyCount) ? static_cast<int32>(Offset) : 0;
			}
			else if (ShouldWarnOncePerActor(Actor))
			{
				UE_LOG(LogTraffic, Warning,
					TEXT("[CityBLDRoadGeometryProvider] Actor %s has unknown FamilyName '%s'. Using index 0 ('%s')."),
					*Actor->GetName(),
					*Meta->FamilyName.ToString(),
					(FamilyCount > 0) ? *(*FamiliesPtr)[0].FamilyName.ToString() : TEXT("None"));
			}
		}

		FTrafficRoad Road;
		Road.RoadId = NextRoadId++;
		Road.FamilyId = (FamilyCount > 0) ? FMath::Clamp(FamilyId, 0, FamilyCount - 1) : 0;
		Road.SourceActor = Actor;
		Road.CenterlinePoints = MoveTemp(Centerline);

		// Provide a simple lane setup; calibration can refine later.
		const int32 LaneCount = 2;
		const float LaneWidth = 350.f;
		for (int32 LaneIdx = 0; LaneIdx < LaneCount; ++LaneIdx)
		{
			FTrafficLane Lane;
			Lane.LaneId = Road.Lanes.Num();
			Lane.RoadId = Road.RoadId;
			Lane.SideIndex = LaneIdx;
			Lane.Width = LaneWidth;
			Lane.CenterlinePoints = Road.CenterlinePoints;
			Road.Lanes.Add(Lane);
		}

		OutRoads.Add(MoveTemp(Road));
	}

	UE_LOG(LogTraffic, Log,
		TEXT("[CityBLDRoadGeometryProvider] Collected %d CityBLD roads."),
		OutRoads.Num());
}

bool UCityBLDRoadGeometryProvider::GetDisplayCenterlineForActor(AActor* RoadActor, TArray<FVector>& OutPoints) const
{
	OutPoints.Reset();
	if (!RoadActor)
	{
		return false;
	}

	const UTrafficCityBLDAdapterSettings* AdapterSettings = GetDefault<UTrafficCityBLDAdapterSettings>();

	// Only handle CityBLD road candidates here; let other providers handle unrelated roads.
	if (!IsCityBLDRoadCandidate(RoadActor, AdapterSettings))
	{
		return false;
	}

	// This uses the full CityBLD smoothing pipeline already implemented in BuildCenterlineFromCityBLDRoad.
	const bool bOk = BuildCenterlineFromCityBLDRoad(RoadActor, OutPoints) && OutPoints.Num() >= 2;
	if (!bOk)
	{
		return false;
	}

	if (const UTrafficRoadMetadataComponent* Meta = RoadActor->FindComponentByClass<UTrafficRoadMetadataComponent>())
	{
		if (Meta->bReverseCenterlineDirection)
		{
			Algo::Reverse(OutPoints);
		}
	}

	return OutPoints.Num() >= 2;
}

bool UCityBLDRoadGeometryProvider::BuildCenterlineFromCityBLDRoad(const AActor* Actor, TArray<FVector>& OutPoints) const
{
	OutPoints.Reset();

	if (!Actor)
	{
		return false;
	}

	UE_LOG(LogTraffic, Verbose, TEXT("[CityBLD] Processing actor %s"), *Actor->GetName());

	const UTrafficCityBLDAdapterSettings* AdapterSettings = GetDefault<UTrafficCityBLDAdapterSettings>();

	const USplineComponent* ControlSpline = nullptr;
	TArray<USplineComponent*> Splines;
	Actor->GetComponents<USplineComponent>(Splines);

	// Prefer an explicitly tagged centerline spline when present (CityBLD road kits typically tag this).
	if (AdapterSettings && !AdapterSettings->RoadSplineTag.IsNone())
	{
		for (USplineComponent* Spline : Splines)
		{
			if (Spline && Spline->ComponentHasTag(AdapterSettings->RoadSplineTag))
			{
				ControlSpline = Spline;
				break;
			}
		}
	}

	// Heuristics fallback when tags aren't set.
	if (!ControlSpline)
	{
		for (USplineComponent* Spline : Splines)
		{
			if (Spline && Spline->GetName().Contains(TEXT("Centerline"), ESearchCase::IgnoreCase))
			{
				ControlSpline = Spline;
				break;
			}
		}
	}

	if (!ControlSpline)
	{
		// Many kits expose a generic "Spline" component as the actual road path.
		// Prefer an exact-name match before selecting "ControlSpline" helpers.
		for (USplineComponent* Spline : Splines)
		{
			if (Spline && Spline->GetName().Equals(TEXT("Spline"), ESearchCase::IgnoreCase))
			{
				ControlSpline = Spline;
				break;
			}
		}
	}

	if (!ControlSpline)
	{
		for (USplineComponent* Spline : Splines)
		{
			if (Spline && Spline->GetName().Contains(TEXT("Control"), ESearchCase::IgnoreCase))
			{
				ControlSpline = Spline;
				break;
			}
		}
	}

	// If multiple splines exist and none matched tags/names, pick the longest (usually the road path).
	if (!ControlSpline && Splines.Num() > 0)
	{
		float BestLen = -1.f;
		for (USplineComponent* Spline : Splines)
		{
			if (!Spline)
			{
				continue;
			}
			const float Len = Spline->GetSplineLength();
			if (Len > BestLen)
			{
				BestLen = Len;
				ControlSpline = Spline;
			}
		}
	}

	if (!ControlSpline)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[CityBLD] No SplineComponent found on %s"), *Actor->GetName());
		return false;
	}

	// Warn when multiple splines exist but we couldn't find the tagged centerline spline; this is the #1 cause of "spaghetti" overlays.
	if (CVarTrafficCityBLDWarnMissingCenterlineSplineTag.GetValueOnGameThread() != 0 &&
		AdapterSettings && !AdapterSettings->RoadSplineTag.IsNone() &&
		Splines.Num() > 1 && !ControlSpline->ComponentHasTag(AdapterSettings->RoadSplineTag) &&
		ShouldWarnOncePerActor(Actor))
	{
		TArray<FString> Candidates;
		Candidates.Reserve(Splines.Num());
		for (USplineComponent* Spline : Splines)
		{
			if (!Spline)
			{
				continue;
			}
			const FString TagHit = Spline->ComponentHasTag(AdapterSettings->RoadSplineTag) ? TEXT("TAG_MATCH") : TEXT("no_tag");
			Candidates.Add(FString::Printf(TEXT("%s(%s)"), *Spline->GetName(), *TagHit));
		}

		UE_LOG(LogTraffic, Warning,
			TEXT("[CityBLD] %s has %d spline components but none are tagged '%s'. Using '%s'. ")
			TEXT("Calibration overlay may be wrong until the correct spline component is tagged. ")
			TEXT("Tag the intended centerline spline component with '%s' to avoid heuristics. Candidates: [%s]"),
			*Actor->GetName(),
			Splines.Num(),
			*AdapterSettings->RoadSplineTag.ToString(),
			*ControlSpline->GetName(),
			*AdapterSettings->RoadSplineTag.ToString(),
			*FString::Join(Candidates, TEXT(", ")));
	}

	if (AdapterSettings && AdapterSettings->bUseControlSplineForDisplayCenterline)
	{
		// For calibration overlay, the control spline is the most stable "ground truth" for the road path.
		// Mesh-based fitting can be thrown off by nearby segments/overlaps in complex test tracks.
		const float Length = ControlSpline->GetSplineLength();
		const int32 SampleCount = FMath::Clamp(static_cast<int32>(Length / 200.f), 10, 200);
		OutPoints.Reserve(SampleCount + 1);

		for (int32 i = 0; i <= SampleCount; ++i)
		{
			const float Dist = (SampleCount > 0) ? (Length * static_cast<float>(i) / static_cast<float>(SampleCount)) : 0.f;
			OutPoints.Add(ControlSpline->GetLocationAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::World));
		}

		ProjectCenterlineZToActorCollision(*Actor, OutPoints);

		UE_LOG(LogTraffic, Verbose, TEXT("[CityBLD] %s used control spline samples (%d points)"), *Actor->GetName(), OutPoints.Num());
		return OutPoints.Num() >= 2;
	}

	TArray<FVector> CollectedVerts;
	TArray<UDynamicMeshComponent*> DynMeshComps;
	Actor->GetComponents<UDynamicMeshComponent>(DynMeshComps);
	for (UDynamicMeshComponent* Comp : DynMeshComps)
	{
		if (!Comp || !Comp->GetDynamicMesh())
		{
			continue;
		}

		const FTransform Xform = Comp->GetComponentTransform();
		const UDynamicMesh* DynMesh = Comp->GetDynamicMesh();
		const UE::Geometry::FDynamicMesh3* Mesh = DynMesh ? DynMesh->GetMeshPtr() : nullptr;
		if (!Mesh)
		{
			continue;
		}

		for (int32 Vid : Mesh->VertexIndicesItr())
		{
			const FVector3d PosD = Mesh->GetVertex(Vid);
			CollectedVerts.Add(Xform.TransformPosition(static_cast<FVector>(PosD)));
		}
	}
	UE_LOG(LogTraffic, Verbose, TEXT("[CityBLD] %s collected %d dynamic mesh vertices"),
		*Actor->GetName(), CollectedVerts.Num());

	BuildSmoothCenterline(ControlSpline, CollectedVerts, OutPoints);
	ProjectCenterlineZToActorCollision(*Actor, OutPoints);
	UE_LOG(LogTraffic, Verbose, TEXT("[CityBLD] %s produced %d centreline points"), *Actor->GetName(), OutPoints.Num());
	return OutPoints.Num() >= 2;
}
