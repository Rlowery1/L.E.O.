#include "CityBLDRoadGeometryProvider.h"
#include "TrafficCityBLDAdapterSettings.h"
#include "TrafficRoadFamilySettings.h"
#include "TrafficRoadMetadataComponent.h"
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
#include "DrawDebugHelpers.h"

namespace
{
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

			// Heuristic: skip invisible or non-colliding components to limit noise.
			if (!Comp->IsVisible())
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

		float SMin = TNumericLimits<float>::Max();
		float SMax = TNumericLimits<float>::Lowest();
		TArray<float> SValues;
		SValues.Reserve(Vertices.Num());
		for (const FVector& V : Vertices)
		{
			const FVector Delta = V - Origin;
			const float S = FVector::DotProduct(Delta, Dir);
			SValues.Add(S);
			SMin = FMath::Min(SMin, S);
			SMax = FMath::Max(SMax, S);
		}

		const float BinStep = 50.f; // 0.5m bins along the road
		const int32 NumBins = FMath::Max(2, FMath::CeilToInt((SMax - SMin) / BinStep));
		struct FBinInfo
		{
			bool bHasData = false;
			float MinT = TNumericLimits<float>::Max();
			float MaxT = TNumericLimits<float>::Lowest();
			float SCenter = 0.f;
		};
		TArray<FBinInfo> Bins;
		Bins.SetNum(NumBins);

		for (int32 BinIdx = 0; BinIdx < NumBins; ++BinIdx)
		{
			Bins[BinIdx].SCenter = SMin + (BinIdx + 0.5f) * BinStep;
		}

		for (int32 Idx = 0; Idx < Vertices.Num(); ++Idx)
		{
			const float S = SValues[Idx];
			const float T = FVector::DotProduct(Vertices[Idx] - Origin, Perp);
			const int32 BinIdx = FMath::Clamp(FMath::FloorToInt((S - SMin) / BinStep), 0, NumBins - 1);
			FBinInfo& Bin = Bins[BinIdx];
			Bin.bHasData = true;
			Bin.MinT = FMath::Min(Bin.MinT, T);
			Bin.MaxT = FMath::Max(Bin.MaxT, T);
		}

		TArray<FVector> CenterlineSamples;
		for (const FBinInfo& Bin : Bins)
		{
			if (!Bin.bHasData)
			{
				continue;
			}

			const FVector LeftPoint = Origin + Dir * Bin.SCenter + Perp * Bin.MinT;
			const FVector RightPoint = Origin + Dir * Bin.SCenter + Perp * Bin.MaxT;
			CenterlineSamples.Add((LeftPoint + RightPoint) * 0.5f);
		}

		if (CenterlineSamples.Num() < 2)
		{
			return false;
		}

		TArray<FVector> SmoothedSamples;
		SmoothPolyline(CenterlineSamples, 5, SmoothedSamples);

		if (SmoothedSamples.Num() >= 2 && CenterlineSamples.Num() >= 2)
		{
			SmoothedSamples[0] = CenterlineSamples[0];
			SmoothedSamples.Last() = CenterlineSamples.Last();
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

	// Metadata is authoritative when present.
	if (UTrafficRoadMetadataComponent* Meta = Actor->FindComponentByClass<UTrafficRoadMetadataComponent>())
	{
		if (!Meta->bIncludeInTraffic)
		{
			return false;
		}
		return true;
	}

	if (!Settings->RoadActorTag.IsNone())
	{
		if (Actor->ActorHasTag(Settings->RoadActorTag))
		{
			return true;
		}
	}

	if (Settings->RoadClassNameContains.Num() > 0)
	{
		const FString ClassPath = Actor->GetClass()->GetPathName();
		for (const FString& Substring : Settings->RoadClassNameContains)
		{
			if (!Substring.IsEmpty() && ClassPath.Contains(Substring))
			{
				return true;
			}
		}
		return false;
	}

	return Actor->FindComponentByClass<USplineComponent>() != nullptr;
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

	return BuildMeshDrivenCenterline(RoadActor, OutPoints);
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

		if (!BuildMeshDrivenCenterline(Actor, Road.CenterlinePoints))
		{
			UE_LOG(LogTraffic, Warning,
				TEXT("[UCityBLDRoadGeometryProvider] Actor %s: failed to build mesh-driven centerline."),
				*Actor->GetName());
			continue;
		}

		OutRoads.Add(Road);
	}

	UE_LOG(LogTraffic, Log,
		TEXT("[UCityBLDRoadGeometryProvider] Collected %d CityBLD roads."),
		OutRoads.Num());
}
