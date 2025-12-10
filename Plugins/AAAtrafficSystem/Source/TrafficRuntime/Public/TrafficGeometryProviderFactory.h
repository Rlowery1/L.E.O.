#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TrafficGeometryProviderFactory.generated.h"

class ITrafficRoadGeometryProvider;

UCLASS()
class TRAFFICRUNTIME_API UTrafficGeometryProviderFactory : public UObject
{
	GENERATED_BODY()

public:
	static TObjectPtr<UObject> CreateProvider(UWorld* World, ITrafficRoadGeometryProvider*& OutInterface);
};
