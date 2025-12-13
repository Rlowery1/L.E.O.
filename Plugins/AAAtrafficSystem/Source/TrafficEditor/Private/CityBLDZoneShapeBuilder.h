#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"

class AActor;
class UWorld;

namespace CityBLDZoneShapeBuilder
{
	bool IsCityBLDRoadActor(const AActor* Actor);

#if WITH_EDITOR
	// Creates transient ZoneShape(s) for a CityBLD road using the provided (smoothed) centerline,
	// mirrors the provided UTrafficZoneLaneProfile into ZoneGraphSettings, and forces a ZoneGraph build.
	bool BuildVehicleZoneGraphForCityBLDRoad(
		UWorld* World,
		const AActor* RoadActor,
		const TArray<FVector>& RoadCenterlinePoints,
		const FSoftObjectPath& VehicleLaneProfileAssetPath);
#endif
}

