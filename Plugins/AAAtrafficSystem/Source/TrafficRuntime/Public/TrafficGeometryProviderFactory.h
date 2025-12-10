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
	// Creates an editor-provider chain (CityBLD first, then generic spline) and returns provider objects to keep them alive.
	static void CreateProviderChainForEditorWorld(UWorld* World, TArray<TObjectPtr<UObject>>& OutProviders, TArray<ITrafficRoadGeometryProvider*>& OutInterfaces);
};
