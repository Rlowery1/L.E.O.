// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrafficRoadTypes.h"

class UWorld;
class UTrafficRoadFamilySettings;

/**
 * Utility to generate ZoneGraph shapes (editor-only authoring actors) from an AAA Traffic network.
 *
 * This is primarily intended for Editor/PIE workflows: it spawns AZoneShape actors tagged with "TrafficZoneGraph"
 * and requests a ZoneGraph rebuild so the ZoneGraphSubsystem can produce updated ZoneGraphData.
 */
class TRAFFICRUNTIME_API FTrafficZoneGraphAdapter
{
public:
	static void BuildZoneGraphForNetwork(
		UWorld* World,
		const FTrafficNetwork& Network,
		const UTrafficRoadFamilySettings* FamilySettings);
};

