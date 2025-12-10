#include "CityBLDRoadGeometryProvider.h"
#include "TrafficCityBLDAdapterSettings.h"
#include "TrafficRoadFamilySettings.h"
#include "TrafficRoadMetadataComponent.h"
#include "TrafficRuntimeModule.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

bool UCityBLDRoadGeometryProvider::IsRoadActor(
	AActor* Actor,
	const UTrafficCityBLDAdapterSettings* Settings) const
{
	if (!Actor || !Settings)
	{
		return false;
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
		const int32 NumPoints = RoadSpline->GetNumberOfSplinePoints();
		if (NumPoints < 2)
		{
			return false;
		}

		const float Length = RoadSpline->GetSplineLength();
		const float Step = 100.f;
		for (float Dist = 0.f; Dist <= Length; Dist += Step)
		{
			OutPoints.Add(RoadSpline->GetLocationAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::World));
		}
		OutPoints.Add(RoadSpline->GetLocationAtDistanceAlongSpline(Length, ESplineCoordinateSpace::World));

		return OutPoints.Num() >= 2;
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

		Road.CenterlinePoints.Reserve(NumPoints);
		for (int32 i = 0; i < NumPoints; ++i)
		{
			const FVector Pos = RoadSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
			Road.CenterlinePoints.Add(Pos);
		}

		OutRoads.Add(Road);
	}

	UE_LOG(LogTraffic, Log,
		TEXT("[UCityBLDRoadGeometryProvider] Collected %d CityBLD roads."),
		OutRoads.Num());
}

