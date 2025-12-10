#pragma once

#include "CoreMinimal.h"
#include "TrafficRoadTypes.h"
#include "TrafficRoadFamilySettings.h"

class ITrafficRoadGeometryProvider;

struct FLaneEndpoint
{
	int32 LaneId = INDEX_NONE;
	FVector Position = FVector::ZeroVector;
	FVector Direction = FVector::ForwardVector;
	bool bIsStart = true;
};

struct FEndpointCluster
{
	FVector Center = FVector::ZeroVector;
	TArray<FLaneEndpoint> Endpoints;
};

class TRAFFICRUNTIME_API FTrafficNetworkBuilder
{
public:
	static void BuildNetworkFromRoads(
		const TArray<FTrafficRoad>& InputRoads,
		const UTrafficRoadFamilySettings* FamilySettings,
		FTrafficNetwork& OutNetwork);

	static void BuildNetworkFromWorld(
		UWorld* World,
		ITrafficRoadGeometryProvider& GeometryProvider,
		const UTrafficRoadFamilySettings* FamilySettings,
		FTrafficNetwork& OutNetwork);

private:
	static void GenerateLanesForRoad(
		const FTrafficRoad& SourceRoad,
		const FRoadFamilyDefinition& Family,
		int32& InOutNextLaneId,
		TArray<FTrafficLane>& OutLanes);

	static void BuildIntersectionsAndMovements(
		FTrafficNetwork& OutNetwork,
		float ClusterRadiusCm);
};

