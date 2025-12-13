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
	// IMPORTANT: Editor calibration overlay calls GetDisplayCenterlineForActor() through the provider chain.
	// If CityBLD provider does not implement this, the chain falls back to other providers (often raw spline),
	// producing 90-degree snapping and arrows flying off-road.
	virtual bool GetDisplayCenterlineForActor(AActor* RoadActor, TArray<FVector>& OutPoints) const override;

protected:
	bool BuildCenterlineFromCityBLDRoad(const AActor* Actor, TArray<FVector>& OutPoints) const;
};
