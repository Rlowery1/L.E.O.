#include "TrafficGeometryProvider.h"
#include "TrafficRuntimeModule.h"
#include "TrafficRoadMetadataComponent.h"
#include "TrafficRoadFamilySettings.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

bool UGenericSplineRoadGeometryProvider::GetDisplayCenterlineForActor(AActor* RoadActor, TArray<FVector>& OutPoints) const
{
	OutPoints.Reset();

	if (!RoadActor)
	{
		return false;
	}

	if (USplineComponent* Spline = RoadActor->FindComponentByClass<USplineComponent>())
	{
		const float Length = Spline->GetSplineLength();
		const float Step = 100.f;

		for (float Dist = 0.f; Dist <= Length; Dist += Step)
		{
			OutPoints.Add(Spline->GetLocationAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::World));
		}

		// Ensure endpoint included
		if (Length > 0.f && (OutPoints.Num() == 0 || !OutPoints.Last().Equals(Spline->GetLocationAtDistanceAlongSpline(Length, ESplineCoordinateSpace::World))))
		{
			OutPoints.Add(Spline->GetLocationAtDistanceAlongSpline(Length, ESplineCoordinateSpace::World));
		}

		return OutPoints.Num() >= 2;
	}

	return false;
}

void UGenericSplineRoadGeometryProvider::CollectRoads(UWorld* World, TArray<FTrafficRoad>& OutRoads)
{
	OutRoads.Reset();

	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[UGenericSplineRoadGeometryProvider] CollectRoads: World is null."));
		return;
	}

	const UTrafficRoadFamilySettings* FamSettings = GetDefault<UTrafficRoadFamilySettings>();
	const TArray<FRoadFamilyDefinition>* FamiliesPtr = FamSettings ? &FamSettings->Families : nullptr;
	const int32 FamilyCount = FamiliesPtr ? FamiliesPtr->Num() : 0;

	int32 NextRoadId = 0;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}

		USplineComponent* Spline = Actor->FindComponentByClass<USplineComponent>();
		if (!Spline)
		{
			continue;
		}

		UTrafficRoadMetadataComponent* Meta = Actor->FindComponentByClass<UTrafficRoadMetadataComponent>();
		if (Meta && !Meta->bIncludeInTraffic)
		{
			continue;
		}

		int32 FamilyId = 0;
		if (FamSettings && FamilyCount > 0 && Meta && !Meta->FamilyName.IsNone())
		{
			const FRoadFamilyDefinition* Found = FamSettings->FindFamilyByName(Meta->FamilyName);
			if (Found)
			{
				const FRoadFamilyDefinition* Base = FamiliesPtr->GetData();
				const ptrdiff_t Offset = Found - Base;
				FamilyId = (Offset >= 0 && Offset < FamilyCount) ? (int32)Offset : 0;
			}
			else
			{
				UE_LOG(LogTraffic, Warning,
					TEXT("[UGenericSplineRoadGeometryProvider] Actor %s has unknown FamilyName '%s'. Using index 0."),
					*Actor->GetName(),
					*Meta->FamilyName.ToString());
			}
		}

		const int32 NumPoints = Spline->GetNumberOfSplinePoints();
		if (NumPoints < 2)
		{
			continue;
		}

		FTrafficRoad Road;
		Road.RoadId = NextRoadId++;
		Road.FamilyId = (FamilyCount > 0) ? FMath::Clamp(FamilyId, 0, FamilyCount - 1) : 0;
		Road.SourceActor = Actor;
		Road.CenterlinePoints.Reserve(NumPoints);

		for (int32 i = 0; i < NumPoints; ++i)
		{
			const FVector Pos = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
			Road.CenterlinePoints.Add(Pos);
		}

		OutRoads.Add(Road);
	}

	UE_LOG(LogTraffic, Log,
		TEXT("[UGenericSplineRoadGeometryProvider] CollectRoads: Found %d spline roads."),
		OutRoads.Num());
}
