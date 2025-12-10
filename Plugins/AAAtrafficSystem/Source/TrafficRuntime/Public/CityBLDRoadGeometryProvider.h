#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TrafficGeometryProvider.h"
#include "CityBLDRoadGeometryProvider.generated.h"

class USplineComponent;
class UTrafficCityBLDAdapterSettings;

UCLASS()
class TRAFFICRUNTIME_API UCityBLDRoadGeometryProvider : public UObject, public ITrafficRoadGeometryProvider
{
	GENERATED_BODY()

public:
	virtual void CollectRoads(UWorld* World, TArray<FTrafficRoad>& OutRoads) override;

private:
	bool IsRoadActor(AActor* Actor, const UTrafficCityBLDAdapterSettings* Settings) const;
	USplineComponent* FindRoadSpline(AActor* Actor, const UTrafficCityBLDAdapterSettings* Settings) const;
	int32 ResolveFamilyIdForActor(AActor* Actor) const;
};
