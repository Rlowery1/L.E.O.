#include "CityBLDRoadGeometryProvider.h"

#include "Components/SplineComponent.h"
#include "Components/DynamicMeshComponent.h"
#include "VectorTypes.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TrafficGeometrySmoothing.h"
#include "TrafficRuntimeModule.h"
#include "TrafficRoadTypes.h"

namespace
{
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
						++InSlice;
					}

					if (InSlice >= 16 && MinDist < MaxDist)
					{
						const float Mid = 0.5f * (MinDist + MaxDist);
						OutPoints.Add(Pos + Right * Mid);
						continue;
					}
				}
			}

			OutPoints.Add(Pos);
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

	int32 NextRoadId = 0;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}

		if (!Actor->GetClass()->GetName().Contains(TEXT("BP_MeshRoad")))
		{
			continue;
		}

		TArray<FVector> Centerline;
		if (!BuildCenterlineFromCityBLDRoad(Actor, Centerline) || Centerline.Num() < 2)
		{
			continue;
		}

		FTrafficRoad Road;
		Road.RoadId = NextRoadId++;
		Road.FamilyId = 0;
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

	// Only handle CityBLD BP_MeshRoad here; let other providers handle non-CityBLD roads.
	if (!RoadActor->GetClass()->GetName().Contains(TEXT("BP_MeshRoad"), ESearchCase::IgnoreCase))
	{
		return false;
	}

	// This uses the full CityBLD smoothing pipeline already implemented in BuildCenterlineFromCityBLDRoad.
	return BuildCenterlineFromCityBLDRoad(RoadActor, OutPoints) && OutPoints.Num() >= 2;
}

bool UCityBLDRoadGeometryProvider::BuildCenterlineFromCityBLDRoad(const AActor* Actor, TArray<FVector>& OutPoints) const
{
	OutPoints.Reset();

	if (!Actor)
	{
		return false;
	}

	UE_LOG(LogTraffic, Verbose, TEXT("[CityBLD] Processing actor %s"), *Actor->GetName());

	const USplineComponent* ControlSpline = nullptr;
	TArray<USplineComponent*> Splines;
	Actor->GetComponents<USplineComponent>(Splines);
	for (USplineComponent* Spline : Splines)
	{
		if (Spline && Spline->GetName().Contains(TEXT("Control")))
		{
			ControlSpline = Spline;
			break;
		}
	}
	if (!ControlSpline && Splines.Num() > 0)
	{
		ControlSpline = Splines[0];
	}
	if (!ControlSpline)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[CityBLD] No SplineComponent found on %s"), *Actor->GetName());
		return false;
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
	UE_LOG(LogTraffic, Verbose, TEXT("[CityBLD] %s produced %d centreline points"), *Actor->GetName(), OutPoints.Num());
	return OutPoints.Num() >= 2;
}
