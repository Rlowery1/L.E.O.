#pragma once

#include "CoreMinimal.h"
#include "TrafficRoadTypes.h"
#include "TrafficGeometryProvider.generated.h"

class TRAFFICRUNTIME_API ITrafficRoadGeometryProvider
{
public:
	virtual ~ITrafficRoadGeometryProvider() = default;
	virtual void CollectRoads(UWorld* World, TArray<FTrafficRoad>& OutRoads) = 0;
};

UCLASS(BlueprintType)
class TRAFFICRUNTIME_API UGenericSplineRoadGeometryProvider : public UObject, public ITrafficRoadGeometryProvider
{
	GENERATED_BODY()

public:
	virtual void CollectRoads(UWorld* World, TArray<FTrafficRoad>& OutRoads) override;
};

