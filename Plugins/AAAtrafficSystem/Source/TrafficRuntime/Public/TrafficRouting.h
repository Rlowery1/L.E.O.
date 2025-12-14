#pragma once

#include "CoreMinimal.h"
#include "TrafficRoadTypes.h"

namespace TrafficRouting
{
	// Returns the first lane with the given id, or nullptr.
	TRAFFICRUNTIME_API const FTrafficLane* FindLaneById(const FTrafficNetwork& Network, int32 LaneId);

	// Returns the first movement with the given id, or nullptr.
	TRAFFICRUNTIME_API const FTrafficMovement* FindMovementById(const FTrafficNetwork& Network, int32 MovementId);

	// Returns all movements that start from the given incoming lane id.
	TRAFFICRUNTIME_API void GetMovementsForIncomingLane(
		const FTrafficNetwork& Network,
		int32 IncomingLaneId,
		TArray<const FTrafficMovement*>& OutMovements);

	// Simple driving policy: Through > Right > Left > any.
	TRAFFICRUNTIME_API const FTrafficMovement* ChooseDefaultMovementForIncomingLane(
		const FTrafficNetwork& Network,
		int32 IncomingLaneId);
}

