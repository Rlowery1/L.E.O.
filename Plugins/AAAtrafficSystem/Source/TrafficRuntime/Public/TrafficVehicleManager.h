#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrafficAutomationLogger.h"
#include "TrafficVehicleManager.generated.h"

class UTrafficNetworkAsset;
class ATrafficVehicleBase;
class ATrafficVehicleAdapter;
class UTrafficVehicleProfile;
class APawn;

UCLASS()
class TRAFFICRUNTIME_API ATrafficVehicleManager : public AActor
{
	GENERATED_BODY()

public:
	ATrafficVehicleManager();

	bool LoadNetwork();
	void SpawnTestVehicles(int32 VehiclesPerLane = 3, float SpeedCmPerSec = 800.f);
	void SpawnZoneGraphVehicles(int32 VehiclesPerLane = 3, float SpeedCmPerSec = 800.f, FName RequiredLaneTag = FName(TEXT("Vehicles")));
	void ClearVehicles();
	void SetActiveRunMetrics(FTrafficRunMetrics* InMetrics);
	void SetForceLogicOnlyForTests(bool bInForce);
	int32 GetSpawnedVehicleCount() const { return Vehicles.Num(); }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	UTrafficNetworkAsset* NetworkAsset;

	UPROPERTY()
	TArray<ATrafficVehicleBase*> Vehicles;

	UPROPERTY()
	TArray<TWeakObjectPtr<APawn>> VisualVehicles;

	UPROPERTY()
	TArray<TWeakObjectPtr<ATrafficVehicleAdapter>> Adapters;

	FTrafficRunMetrics* ActiveMetrics = nullptr;
	TMap<int32, float> LastLaneSpawnTimes;
	bool bForceLogicOnlyForTests = false;

	// Runtime spawn deferral: some road kits build collision asynchronously at PIE start.
	FTimerHandle DeferredSpawnTimerHandle;
	bool bDeferredSpawnPending = false;
	bool bDeferredSpawnWasZoneGraph = false;
	int32 DeferredVehiclesPerLane = 0;
	float DeferredSpeedCmPerSec = 0.f;
	FName DeferredRequiredLaneTag = NAME_None;
	double DeferredSpawnStartWallSeconds = 0.0;
	int32 DeferredSpawnAttempts = 0;

	// Cache the wheel/suspension trace channel for the active Chaos vehicle class so we can defer spawn until that channel hits.
	ECollisionChannel CachedWheelTraceChannel = ECC_WorldDynamic;
	bool bHasCachedWheelTraceChannel = false;

	UTrafficNetworkAsset* FindNetworkAsset() const;
	const UTrafficVehicleProfile* ResolveDefaultVehicleProfile() const;
	void DestroyAdaptersAndVisuals();
	bool IsRoadCollisionReadyForChaosDrive(ECollisionChannel WheelTraceChannel) const;
	bool MaybeDeferChaosDriveSpawn(bool bZoneGraph, int32 VehiclesPerLane, float SpeedCmPerSec, FName RequiredLaneTag, ECollisionChannel WheelTraceChannel);
	void TryDeferredSpawn();
};
