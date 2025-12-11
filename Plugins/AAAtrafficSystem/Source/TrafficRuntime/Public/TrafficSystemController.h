#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrafficSystemController.generated.h"

class UTrafficNetworkAsset;
class UWorld;

UCLASS()
class TRAFFICRUNTIME_API ATrafficSystemController : public AActor
{
	GENERATED_BODY()

public:
	ATrafficSystemController();

	// Editor test tools
	UFUNCTION(CallInEditor, Category="Traffic")
	void Editor_BuildTrafficNetwork();

	UFUNCTION(CallInEditor, Category="Traffic")
	void Editor_SpawnTestVehicles();

	// Runtime/PIE helpers
	UFUNCTION(BlueprintCallable, Category="Traffic|Runtime")
	void Runtime_BuildTrafficNetwork();

	UFUNCTION(BlueprintCallable, Category="Traffic|Runtime")
	void Runtime_SpawnTraffic();

	UTrafficNetworkAsset* GetBuiltNetworkAsset() const { return BuiltNetworkAsset; }
	int32 GetNumRoads() const;
	int32 GetNumLanes() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	UTrafficNetworkAsset* BuiltNetworkAsset;

	// Runtime config
	UPROPERTY(EditAnywhere, Category="Traffic|Runtime")
	bool bAutoBuildOnBeginPlay = false;

	UPROPERTY(EditAnywhere, Category="Traffic|Runtime")
	bool bAutoSpawnOnBeginPlay = false;

	UPROPERTY(EditAnywhere, Category="Traffic|Runtime", meta=(ClampMin="0"))
	int32 VehiclesPerLaneRuntime = 3;

	UPROPERTY(EditAnywhere, Category="Traffic|Runtime")
	float RuntimeSpeedCmPerSec = 800.f;

private:
	bool BuildNetworkInternal(UWorld* World);
};
