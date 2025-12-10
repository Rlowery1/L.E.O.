#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrafficSystemController.generated.h"

class UTrafficNetworkAsset;

UCLASS()
class TRAFFICRUNTIME_API ATrafficSystemController : public AActor
{
	GENERATED_BODY()

public:
	ATrafficSystemController();

	UFUNCTION(CallInEditor, Category="Traffic")
	void Editor_BuildTrafficNetwork();

	UFUNCTION(CallInEditor, Category="Traffic")
	void Editor_SpawnTestVehicles();

	UTrafficNetworkAsset* GetBuiltNetworkAsset() const { return BuiltNetworkAsset; }
	int32 GetNumRoads() const;
	int32 GetNumLanes() const;

protected:
	UPROPERTY()
	UTrafficNetworkAsset* BuiltNetworkAsset;
};
