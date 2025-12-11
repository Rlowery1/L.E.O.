#include "CityBLDRoadGeometryProvider.h"
#include "TrafficCityBLDAdapterSettings.h"
#include "TrafficRoadFamilySettings.h"
#include "TrafficRoadMetadataComponent.h"
#include "TrafficRoadMeshTagComponent.h"
#include "TrafficRuntimeModule.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "DrawDebugHelpers.h"
#include "Math/BoxSphereBounds.h"
#include "UObject/SoftObjectPath.h"
#include "Materials/MaterialInterface.h"

namespace
{
	static bool ShouldIncludeMeshForTraffic(
		UStaticMeshComponent* Comp,
		AActor* RoadActor,
		const UTrafficCityBLDAdapterSettings* Settings)
	{
		if (!Comp || !RoadActor)
		{
			return false;
		}

		// Heuristic: skip invisible or non-colliding components to limit noise.
		if (!Comp->IsVisible())
		{
			return false;
		}

		UStaticMesh* Mesh = Comp->GetStaticMesh();
		if (!Mesh)
		{
			return false;
		}

		FBoxSphereBounds LocalBounds = Comp->CalcBounds(FTransform::Identity);

		bool bInclude = true;

		bool bHasTagComponent = false;
		TArray<UTrafficRoadMeshTagComponent*> TagComponents;
		RoadActor->GetComponents(TagComponents);
		for (UTrafficRoadMeshTagComponent* TagComp : TagComponents)
		{
			if (TagComp->TargetMesh && TagComp->TargetMesh != Comp)
			{
				continue;
			}
			bHasTagComponent = true;
			bInclude = TagComp->bUseForTraffic;
			break;
		}

		if (bHasTagComponent && !bInclude)
		{
			return false;
		}

		if (bInclude && Settings && Settings->DrivableMaterialKeywords.Num() > 0)
		{
			bool bMaterialMatch = false;
			if (Mesh)
			{
				for (const FStaticMaterial& StaticMat : Mesh->GetStaticMaterials())
				{
					UMaterialInterface* Mat = StaticMat.MaterialInterface;
					if (!Mat)
					{
						continue;
					}

					const FString MatName = Mat->GetName();
					for (const FString& Keyword : Settings->DrivableMaterialKeywords)
					{
						if (!Keyword.IsEmpty() && MatName.Contains(Keyword))
						{
							bMaterialMatch = true;
							break;
						}
					}

					if (bMaterialMatch)
					{
						break;
					}
				}
			}

			bInclude = bMaterialMatch;
		}

		if (bInclude && Settings && Settings->MaxMeshHeightCm > 0.f)
		{
			const float HeightCm = LocalBounds.BoxExtent.Z * 2.f;
			if (HeightCm > Settings->MaxMeshHeightCm)
			{
				bInclude = false;
			}
		}

		if (bInclude && Settings && Settings->ExcludedMeshNameKeywords.Num() > 0)
		{
			const FString CompName = Comp->GetName();
			const FString MeshName = Mesh->GetName();
			for (const FString& ExKey : Settings->ExcludedMeshNameKeywords)
			{
				if (!ExKey.IsEmpty() &&
					(CompName.Contains(ExKey, ESearchCase::IgnoreCase) ||
						MeshName.Contains(ExKey, ESearchCase::IgnoreCase)))
				{
					bInclude = false;
					break;
				}
			}
		}

		if (bInclude && Settings && Settings->MinMeshAspectRatio > 0.f)
		{
			const FVector BoxExtent = LocalBounds.BoxExtent;
			const float Length = 2.f * FMath::Max(BoxExtent.X, BoxExtent.Y);
			const float Width = 2.f * FMath::Min(BoxExtent.X, BoxExtent.Y);
			const float Aspect = (Width > KINDA_SMALL_NUMBER) ? (Length / Width) : Length;
			if (Aspect < Settings->MinMeshAspectRatio)
			{
				bInclude = false;
			}
		}

		if (bInclude && Settings && Settings->MaxMeshLateralOffsetCm > 0.f)
		{
			const FTransform& Xform = Comp->GetComponentTransform();
			const FVector WorldCenter = Xform.TransformPosition(LocalBounds.Origin);
			const FVector Forward = RoadActor->GetActorForwardVector();
			const FVector ApproxPerp = FVector(-Forward.Y, Forward.X, 0.f).GetSafeNormal();

			if (!ApproxPerp.IsNearlyZero())
			{
				const float LateralOffset = FMath::Abs(FVector::DotProduct(WorldCenter - RoadActor->GetActorLocation(), ApproxPerp));
				bInclude = LateralOffset <= Settings->MaxMeshLateralOffsetCm;
			}
		}

		if (bInclude)
		{
			// Filter meshes whose normals are far from the road plane.
			const FVector ApproxTangent = RoadActor->GetActorForwardVector().GetSafeNormal();
			const FVector ApproxRight = FVector(-ApproxTangent.Y, ApproxTangent.X, 0.f).GetSafeNormal();
			const FVector ApproxUp = FVector::CrossProduct(ApproxTangent, ApproxRight).GetSafeNormal();

			if (ApproxUp.IsNearlyZero())
			{
				bInclude = false;
			}
			else if (const FStaticMeshRenderData* RenderData = Mesh->GetRenderData())
			{
				if (RenderData->LODResources.Num() > 0)
				{
					const FStaticMeshLODResources& LOD = RenderData->LODResources[0];
					const FStaticMeshVertexBuffer& VertexBuffer = LOD.VertexBuffers.StaticMeshVertexBuffer;
					if (VertexBuffer.GetNumVertices() > 0)
					{
						const FVector Normal = FVector(VertexBuffer.VertexTangentZ(0)).GetSafeNormal();
						const float CosAngle = FVector::DotProduct(Normal, ApproxUp);
						// Reject if tilt exceeds ~60 degrees.
						if (CosAngle < 0.5f)
						{
							bInclude = false;
						}
					}
				}
			}
		}

		return bInclude;
	}

	// Sample the spline at uniform distance intervals in world space.
	static void SampleSplineUniformDistance(
		const USplineComponent* Spline,
		float SampleStepCm,
		TArray<FVector>& OutWorldPoints)
	{
		OutWorldPoints.Reset();

		if (!Spline)
		{
			return;
		}

		const float TotalLength = Spline->GetSplineLength();
		if (TotalLength <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		const int32 NumSamples = FMath::Max(2, FMath::CeilToInt(TotalLength / SampleStepCm));

		for (int32 Index = 0; Index <= NumSamples; ++Index)
		{
			const float Dist = FMath::Min(Index * SampleStepCm, TotalLength);
			const FVector Pos = Spline->GetLocationAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::World);
			OutWorldPoints.Add(Pos);
		}
	}

	// Simple moving-average smoothing for a polyline.
	static void SmoothPolyline(
		const TArray<FVector>& InPoints,
		int32 Radius,
		TArray<FVector>& OutPoints)
	{
		OutPoints.Reset();

		const int32 Num = InPoints.Num();
		if (Num == 0 || Radius <= 0)
		{
			OutPoints = InPoints;
			return;
		}

		OutPoints.SetNum(Num);

		for (int32 i = 0; i < Num; ++i)
		{
			FVector Accum(0.f, 0.f, 0.f);
			int32 Count = 0;

			const int32 Start = FMath::Max(0, i - Radius);
			const int32 End = FMath::Min(Num - 1, i + Radius);

			for (int32 j = Start; j <= End; ++j)
			{
				Accum += InPoints[j];
				++Count;
			}

			if (Count > 0)
			{
				OutPoints[i] = Accum / static_cast<float>(Count);
			}
			else
			{
				OutPoints[i] = InPoints[i];
			}
		}
	}

	static void CollectAsphaltVertices(AActor* RoadActor, TArray<FVector>& OutVertices)
	{
		OutVertices.Reset();

		const UTrafficCityBLDAdapterSettings* AdaptSettings = GetDefault<UTrafficCityBLDAdapterSettings>();

		if (!RoadActor)
		{
			return;
		}

		TArray<UStaticMeshComponent*> MeshComponents;
		RoadActor->GetComponents(MeshComponents);

		for (UStaticMeshComponent* Comp : MeshComponents)
		{
			if (!Comp)
			{
				continue;
			}

			if (!ShouldIncludeMeshForTraffic(Comp, RoadActor, AdaptSettings))
			{
				continue;
			}

			UStaticMesh* Mesh = Comp->GetStaticMesh();
			if (!Mesh || !Mesh->GetRenderData() || Mesh->GetRenderData()->LODResources.Num() == 0)
			{
				continue;
			}

			const FStaticMeshLODResources& LOD = Mesh->GetRenderData()->LODResources[0];
			const FPositionVertexBuffer& PosBuffer = LOD.VertexBuffers.PositionVertexBuffer;
			const int32 VertexCount = PosBuffer.GetNumVertices();

			const FTransform& Xform = Comp->GetComponentTransform();
			for (int32 Idx = 0; Idx < VertexCount; ++Idx)
			{
				const FVector LocalPos = FVector(PosBuffer.VertexPosition(Idx));
				const FVector WorldPos = Xform.TransformPosition(LocalPos);
				OutVertices.Add(WorldPos);
			}
		}
	}

	static void ComputePCAAxis2D(const TArray<FVector>& Vertices, FVector& OutOrigin, FVector& OutDir, FVector& OutPerp)
	{
		OutOrigin = FVector::ZeroVector;
		OutDir = FVector::ForwardVector;
		OutPerp = FVector::RightVector;

		const int32 Num = Vertices.Num();
		if (Num <= 2)
		{
			return;
		}

		double MeanX = 0.0;
		double MeanY = 0.0;
		double MeanZ = 0.0;
		for (const FVector& V : Vertices)
		{
			MeanX += V.X;
			MeanY += V.Y;
			MeanZ += V.Z;
		}
		MeanX /= static_cast<double>(Num);
		MeanY /= static_cast<double>(Num);
		MeanZ /= static_cast<double>(Num);

		double CovXX = 0.0;
		double CovXY = 0.0;
		double CovYY = 0.0;
		for (const FVector& V : Vertices)
		{
			const double DX = static_cast<double>(V.X) - MeanX;
			const double DY = static_cast<double>(V.Y) - MeanY;
			CovXX += DX * DX;
			CovXY += DX * DY;
			CovYY += DY * DY;
		}

		const double Trace = CovXX + CovYY;
		const double Det = CovXX * CovYY - CovXY * CovXY;
		const double Disc = FMath::Sqrt(FMath::Max(0.0, Trace * Trace * 0.25 - Det));

		const double Lambda1 = Trace * 0.5 + Disc;

		double VX = 1.0;
		double VY = 0.0;

		if (FMath::Abs(CovXY) > KINDA_SMALL_NUMBER)
		{
			VX = Lambda1 - CovYY;
			VY = CovXY;
		}
		else
		{
			if (CovXX >= CovYY)
			{
				VX = 1.0;
				VY = 0.0;
			}
			else
			{
				VX = 0.0;
				VY = 1.0;
			}
		}

		const double Len = FMath::Sqrt(VX * VX + VY * VY);
		if (Len > KINDA_SMALL_NUMBER)
		{
			VX /= Len;
			VY /= Len;
		}

		OutDir = FVector(static_cast<float>(VX), static_cast<float>(VY), 0.f).GetSafeNormal();
		OutPerp = FVector(-OutDir.Y, OutDir.X, 0.f);
		OutOrigin = FVector(static_cast<float>(MeanX), static_cast<float>(MeanY), static_cast<float>(MeanZ));
	}

	static TArray<float> ComputeCurvatures(const TArray<FVector>& Points)
	{
		TArray<float> Curvatures;
		const int32 Num = Points.Num();
		if (Num == 0)
		{
			return Curvatures;
		}

		Curvatures.Init(0.0f, Num);

		for (int32 i = 1; i < Num - 1; ++i)
		{
			const FVector V1 = (Points[i] - Points[i - 1]).GetSafeNormal();
			const FVector V2 = (Points[i + 1] - Points[i]).GetSafeNormal();
			Curvatures[i] = FVector::CrossProduct(V1, V2).Size();
		}

		return Curvatures;
	}

	static bool BuildMeshDrivenCenterline(AActor* RoadActor, TArray<FVector>& OutPoints)
	{
		OutPoints.Reset();

		TArray<FVector> Vertices;
		CollectAsphaltVertices(RoadActor, Vertices);
		if (Vertices.Num() < 2)
		{
			return false;
		}

		FVector Origin;
		FVector Dir;
		FVector Perp;
		ComputePCAAxis2D(Vertices, Origin, Dir, Perp);

		struct FProjectedVertex
		{
			float S = 0.f;
			FVector WorldPos = FVector::ZeroVector;
		};

		TArray<FProjectedVertex> Projected;
		Projected.Reserve(Vertices.Num());
		for (const FVector& V : Vertices)
		{
			FProjectedVertex PV;
			PV.WorldPos = V;
			PV.S = FVector::DotProduct(V - Origin, Dir);
			Projected.Add(PV);
		}

		Projected.Sort([](const FProjectedVertex& A, const FProjectedVertex& B)
			{
				return A.S < B.S;
			});

		if (Projected.Num() < 2)
		{
			return false;
		}

		TArray<FVector> SortedPoints;
		SortedPoints.Reserve(Projected.Num());
		for (const FProjectedVertex& PV : Projected)
		{
			SortedPoints.Add(PV.WorldPos);
		}

		const TArray<float> Curvatures = ComputeCurvatures(SortedPoints);
		const float BaseStep = 50.f;
		const float CurvatureThreshold = 0.01f;

		TArray<FVector> AdaptiveSamples;
		AdaptiveSamples.Reserve(SortedPoints.Num());

		AdaptiveSamples.Add(SortedPoints[0]);
		int32 StartIndex = 0;
		float AccumulatedLength = 0.f;

		for (int32 i = 1; i < SortedPoints.Num(); ++i)
		{
			const float SegmentLength = (SortedPoints[i] - SortedPoints[i - 1]).Size();
			AccumulatedLength += SegmentLength;

			const bool bHitLength = AccumulatedLength >= BaseStep;
			const bool bHitCurvature = Curvatures.IsValidIndex(i) && Curvatures[i] > CurvatureThreshold;

			if (bHitLength || bHitCurvature)
			{
				FVector Mean = FVector::ZeroVector;
				int32 Count = 0;
				for (int32 j = StartIndex; j <= i; ++j)
				{
					Mean += SortedPoints[j];
					++Count;
				}
				if (Count > 0)
				{
					Mean /= static_cast<float>(Count);
					AdaptiveSamples.Add(Mean);
				}

				StartIndex = i;
				AccumulatedLength = 0.f;
			}
		}

		if (StartIndex < SortedPoints.Num() - 1)
		{
			FVector Mean = FVector::ZeroVector;
			int32 Count = 0;
			for (int32 j = StartIndex; j < SortedPoints.Num(); ++j)
			{
				Mean += SortedPoints[j];
				++Count;
			}
			if (Count > 0)
			{
				Mean /= static_cast<float>(Count);
				AdaptiveSamples.Add(Mean);
			}
		}

		AdaptiveSamples.Add(SortedPoints.Last());

		if (AdaptiveSamples.Num() < 2)
		{
			return false;
		}

		TArray<FVector> SmoothedSamples;
		SmoothPolyline(AdaptiveSamples, 5, SmoothedSamples);

		if (SmoothedSamples.Num() >= 2 && AdaptiveSamples.Num() >= 2)
		{
			SmoothedSamples[0] = AdaptiveSamples[0];
			SmoothedSamples.Last() = AdaptiveSamples.Last();
		}

		OutPoints = SmoothedSamples;

#if WITH_EDITOR
		if (UWorld* World = RoadActor ? RoadActor->GetWorld() : nullptr)
		{
			for (int32 i = 1; i < OutPoints.Num(); ++i)
			{
				DrawDebugLine(World, OutPoints[i - 1], OutPoints[i], FColor::Magenta, false, 5.0f, 0, 5.0f);
			}
		}
#endif

		return OutPoints.Num() >= 2;
	}
} // namespace

bool UCityBLDRoadGeometryProvider::IsRoadActor(
	AActor* Actor,
	const UTrafficCityBLDAdapterSettings* Settings) const
{
	if (!Actor || !Settings)
	{
		return false;
	}

	const bool bHasSpline = Actor->FindComponentByClass<USplineComponent>() != nullptr;
	bool bIsRoad = false;

	// Metadata is authoritative when present.
	if (UTrafficRoadMetadataComponent* Meta = Actor->FindComponentByClass<UTrafficRoadMetadataComponent>())
	{
		if (!Meta->bIncludeInTraffic)
		{
			bIsRoad = false;
		}
		else
		{
			bIsRoad = true;
		}
	}
	else if (!Settings->RoadActorTag.IsNone())
	{
		if (Actor->ActorHasTag(Settings->RoadActorTag))
		{
			bIsRoad = true;
		}
	}
	else if (Settings->RoadClassNameContains.Num() > 0)
	{
		const FString ClassPath = Actor->GetClass()->GetPathName();
		for (const FString& Substring : Settings->RoadClassNameContains)
		{
			if (!Substring.IsEmpty() && ClassPath.Contains(Substring))
			{
				bIsRoad = true;
				break;
			}
		}
	}
	else
	{
		bIsRoad = bHasSpline;
	}

	UE_LOG(LogTraffic, Log,
		TEXT("[UCityBLDRoadGeometryProvider] IsRoadActor? Actor=%s HasSpline=%s Result=%s"),
		*Actor->GetName(),
		bHasSpline ? TEXT("Yes") : TEXT("No"),
		bIsRoad ? TEXT("Yes") : TEXT("No"));

	return bIsRoad;
}

USplineComponent* UCityBLDRoadGeometryProvider::FindRoadSpline(
	AActor* Actor,
	const UTrafficCityBLDAdapterSettings* Settings) const
{
	if (!Actor)
	{
		return nullptr;
	}

	TArray<USplineComponent*> SplineComponents;
	Actor->GetComponents(SplineComponents);

	if (SplineComponents.Num() == 0)
	{
		return nullptr;
	}

	if (Settings && !Settings->RoadSplineTag.IsNone())
	{
		for (USplineComponent* Spline : SplineComponents)
		{
			if (Spline && Spline->ComponentHasTag(Settings->RoadSplineTag))
			{
				return Spline;
			}
		}
	}

	return SplineComponents[0];
}

int32 UCityBLDRoadGeometryProvider::ResolveFamilyIdForActor(AActor* Actor) const
{
	const UTrafficRoadFamilySettings* FamSettings = GetDefault<UTrafficRoadFamilySettings>();
	if (!FamSettings || FamSettings->Families.Num() == 0)
	{
		return 0;
	}

	const TArray<FRoadFamilyDefinition>& Families = FamSettings->Families;
	const UTrafficCityBLDAdapterSettings* CitySettings = GetDefault<UTrafficCityBLDAdapterSettings>();
	FName DesiredFamilyName = CitySettings ? CitySettings->DefaultFamilyName : NAME_None;

	if (Actor)
	{
		if (UTrafficRoadMetadataComponent* Meta = Actor->FindComponentByClass<UTrafficRoadMetadataComponent>())
		{
			if (!Meta->FamilyName.IsNone())
			{
				DesiredFamilyName = Meta->FamilyName;
			}
		}
	}

	if (DesiredFamilyName.IsNone())
	{
		return 0;
	}

	const FRoadFamilyDefinition* Found = FamSettings->FindFamilyByName(DesiredFamilyName);
	if (Found)
	{
		const FRoadFamilyDefinition* Base = Families.GetData();
		const ptrdiff_t Offset = Found - Base;
		if (Offset >= 0 && Offset < Families.Num())
		{
			return (int32)Offset;
		}
	}

	return 0;
}

bool UCityBLDRoadGeometryProvider::GetDisplayCenterlineForActor(AActor* RoadActor, TArray<FVector>& OutPoints) const
{
	OutPoints.Reset();

	const UTrafficCityBLDAdapterSettings* Settings = GetDefault<UTrafficCityBLDAdapterSettings>();
	if (!IsRoadActor(RoadActor, Settings))
	{
		return false;
	}

	const bool bUsedMesh = BuildMeshDrivenCenterline(RoadActor, OutPoints);
	bool bUsedSpline = false;

	if (!bUsedMesh)
	{
		if (USplineComponent* Spline = FindRoadSpline(RoadActor, Settings))
		{
			if (GIsAutomationTesting)
			{
				const int32 NumPts = Spline->GetNumberOfSplinePoints();
				for (int32 i = 0; i < NumPts; ++i)
				{
					OutPoints.Add(Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World));
				}
				bUsedSpline = OutPoints.Num() >= 2;
			}
			else
			{
				SampleSplineUniformDistance(Spline, 100.0f, OutPoints);

				if (OutPoints.Num() >= 2)
				{
					TArray<FVector> Smoothed;
					SmoothPolyline(OutPoints, 3, Smoothed);
					if (Smoothed.Num() == OutPoints.Num())
					{
						OutPoints = MoveTemp(Smoothed);
					}
					bUsedSpline = true;
				}
			}
		}
	}

	const bool bSuccess = bUsedMesh || bUsedSpline;

	UE_LOG(LogTraffic, Log,
		TEXT("[UCityBLDRoadGeometryProvider] DisplayCenterline: Actor=%s Path=%s NumPoints=%d"),
		RoadActor ? *RoadActor->GetName() : TEXT("None"),
		bUsedMesh ? TEXT("mesh") : (bUsedSpline ? TEXT("spline") : TEXT("none")),
		OutPoints.Num());

	return bSuccess;
}

void UCityBLDRoadGeometryProvider::CollectRoads(UWorld* World, TArray<FTrafficRoad>& OutRoads)
{
	OutRoads.Reset();

	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[UCityBLDRoadGeometryProvider] World is null."));
		return;
	}

	const UTrafficCityBLDAdapterSettings* Settings = GetDefault<UTrafficCityBLDAdapterSettings>();
	if (!Settings)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[UCityBLDRoadGeometryProvider] Settings not available."));
		return;
	}

	const UTrafficRoadFamilySettings* FamSettings = GetDefault<UTrafficRoadFamilySettings>();
	const int32 FamilyCount = (FamSettings ? FamSettings->Families.Num() : 0);

	if (FamilyCount == 0)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[UCityBLDRoadGeometryProvider] No road families configured."));
	}

	int32 NextRoadId = 0;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}

		if (!IsRoadActor(Actor, Settings))
		{
			continue;
		}

		FTrafficRoad Road;
		Road.RoadId = NextRoadId++;
		Road.SourceActor = Actor;
		Road.FamilyId = (FamilyCount > 0)
			? FMath::Clamp(ResolveFamilyIdForActor(Actor), 0, FamilyCount - 1)
			: 0;

		if (!GetDisplayCenterlineForActor(Actor, Road.CenterlinePoints))
		{
			UE_LOG(LogTraffic, Warning,
				TEXT("[UCityBLDRoadGeometryProvider] Actor %s: failed to build centerline (mesh or spline)."),
				*Actor->GetName());
			continue;
		}

		OutRoads.Add(Road);

		int32 IncludedMeshes = 0;
		int32 ExcludedMeshes = 0;
		TArray<UStaticMeshComponent*> MeshComps;
		Actor->GetComponents(MeshComps);
		for (UStaticMeshComponent* MeshComp : MeshComps)
		{
			if (ShouldIncludeMeshForTraffic(MeshComp, Actor, Settings))
			{
				++IncludedMeshes;
			}
			else
			{
				++ExcludedMeshes;
			}
		}

		UE_LOG(LogTraffic, Log,
			TEXT("[UCityBLDRoadGeometryProvider] Actor %s: IncludedMeshes=%d ExcludedMeshes=%d."),
			*Actor->GetName(),
			IncludedMeshes,
			ExcludedMeshes);
	}

	UE_LOG(LogTraffic, Log,
		TEXT("[UCityBLDRoadGeometryProvider] Collected %d CityBLD roads."),
		OutRoads.Num());
}
