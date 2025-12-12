#pragma once

#include "CoreMinimal.h"
#include "TrafficRoadTypes.h"
#include "TrafficGeometryProvider.generated.h"

class TRAFFICRUNTIME_API ITrafficRoadGeometryProvider
{
public:
	virtual ~ITrafficRoadGeometryProvider() = default;
	virtual void CollectRoads(UWorld* World, TArray<FTrafficRoad>& OutRoads) = 0;
	// Returns a display centerline polyline for the given road actor in world space. Default: unsupported.
	virtual bool GetDisplayCenterlineForActor(AActor* RoadActor, TArray<FVector>& OutPoints) const { return false; }
};

UCLASS(BlueprintType)
class TRAFFICRUNTIME_API UGenericSplineRoadGeometryProvider : public UObject, public ITrafficRoadGeometryProvider
{
	GENERATED_BODY()

public:
	virtual void CollectRoads(UWorld* World, TArray<FTrafficRoad>& OutRoads) override;
	virtual bool GetDisplayCenterlineForActor(AActor* RoadActor, TArray<FVector>& OutPoints) const override;
};
