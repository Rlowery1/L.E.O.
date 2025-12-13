#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;

namespace ZoneGraphLaneOverlayUtils
{
#if WITH_EDITOR
	// Extracts lane polylines from ZoneGraph near the specified actor.
	// Each lane polyline is returned as an array of world-space points.
	bool GetZoneGraphLanePolylinesNearActor(
		UWorld* World,
		AActor* RoadActor,
		TArray<TArray<FVector>>& OutLanePolylines);
#endif
}

