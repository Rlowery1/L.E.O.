#include "CityBLDRoadGeometryProvider.h"
#include "TrafficCityBLDAdapterSettings.h"
#include "TrafficRoadFamilySettings.h"
#include "TrafficRoadMetadataComponent.h"
#include "TrafficRuntimeModule.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

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

	// Use the same sampling as CollectRoads for now. If CityBLD exposes richer data, it can be swapped here without touching callers.
	if (USplineComponent* RoadSpline = FindRoadSpline(RoadActor, Settings))
	{
		TArray<FVector> RawSamples;
		TArray<FVector> SmoothedSamples;

		static const float SampleStepCm = 100.f; // 1m resolution for smoother driving path.
		static const int32 SmoothRadius = 3;     // Moving-average radius.

		SampleSplineUniformDistance(RoadSpline, SampleStepCm, RawSamples);
		SmoothPolyline(RawSamples, SmoothRadius, SmoothedSamples);

		// Preserve endpoints from raw data to avoid shortening the road.
		if (SmoothedSamples.Num() >= 2 && RawSamples.Num() >= 2)
		{
			SmoothedSamples[0] = RawSamples[0];
			SmoothedSamples.Last() = RawSamples.Last();
		}

		OutPoints = SmoothedSamples;
		if (OutPoints.Num() < 2)
		{
			return false;
		}

		UE_LOG(LogTraffic, Verbose,
			TEXT("[CityBLD] Smoothed centerline with %d samples for road %s"),
			OutPoints.Num(), *GetNameSafe(RoadActor));
		return true;
	}

	return false;
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

		USplineComponent* RoadSpline = FindRoadSpline(Actor, Settings);
		if (!RoadSpline)
		{
			UE_LOG(LogTraffic, Verbose,
				TEXT("[UCityBLDRoadGeometryProvider] Actor %s selected as road but has no spline."),
				*Actor->GetName());
			continue;
		}

		const int32 NumPoints = RoadSpline->GetNumberOfSplinePoints();
		if (NumPoints < 2)
		{
			continue;
		}

		FTrafficRoad Road;
		Road.RoadId = NextRoadId++;
		Road.SourceActor = Actor;
		Road.FamilyId = (FamilyCount > 0)
			? FMath::Clamp(ResolveFamilyIdForActor(Actor), 0, FamilyCount - 1)
			: 0;

		static const float SampleStepCm = 100.f; // Match display sampling.
		static const int32 SmoothRadius = 3;

		TArray<FVector> RawSamples;
		TArray<FVector> SmoothedSamples;
		SampleSplineUniformDistance(RoadSpline, SampleStepCm, RawSamples);
		SmoothPolyline(RawSamples, SmoothRadius, SmoothedSamples);

		if (SmoothedSamples.Num() >= 2 && RawSamples.Num() >= 2)
		{
			SmoothedSamples[0] = RawSamples[0];
			SmoothedSamples.Last() = RawSamples.Last();
		}

		if (SmoothedSamples.Num() < 2)
		{
			continue;
		}

		Road.CenterlinePoints = SmoothedSamples;

		OutRoads.Add(Road);
	}

	UE_LOG(LogTraffic, Log,
		TEXT("[UCityBLDRoadGeometryProvider] Collected %d CityBLD roads."),
		OutRoads.Num());
}
