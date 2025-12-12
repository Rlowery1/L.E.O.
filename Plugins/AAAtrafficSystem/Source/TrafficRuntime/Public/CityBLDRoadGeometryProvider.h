#pragma once

#include "CoreMinimal.h"
#include "TrafficGeometryProvider.h"
#include "CityBLDRoadGeometryProvider.generated.h"

class UDynamicMeshComponent;
class USplineComponent;

/**
 * CityBLD-specific geometry provider. Extracts centrelines from BP_MeshRoad actors by sampling
 * their control spline and using the dynamic mesh to find cross-section midpoints when available.
 */
UCLASS()
class TRAFFICRUNTIME_API UCityBLDRoadGeometryProvider : public UObject, public ITrafficRoadGeometryProvider
{
	GENERATED_BODY()

public:
	virtual void CollectRoads(UWorld* World, TArray<FTrafficRoad>& OutRoads) override;

protected:
	bool BuildCenterlineFromCityBLDRoad(const AActor* Actor, TArray<FVector>& OutPoints) const;
};
