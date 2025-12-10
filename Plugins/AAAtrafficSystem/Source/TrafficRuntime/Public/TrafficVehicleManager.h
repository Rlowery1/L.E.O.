#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrafficAutomationLogger.h"
#include "TrafficVehicleManager.generated.h"

class UTrafficNetworkAsset;
class ATrafficVehicleBase;
class ATrafficVehicleAdapter;
class UTrafficVehicleProfile;

UCLASS()
class TRAFFICRUNTIME_API ATrafficVehicleManager : public AActor
{
	GENERATED_BODY()

public:
	ATrafficVehicleManager();

	bool LoadNetwork();
	void SpawnTestVehicles(int32 VehiclesPerLane = 3, float SpeedCmPerSec = 800.f);
	void ClearVehicles();
	void SetActiveRunMetrics(FTrafficRunMetrics* InMetrics);

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

	UTrafficNetworkAsset* FindNetworkAsset() const;
	const UTrafficVehicleProfile* ResolveDefaultVehicleProfile() const;
	void DestroyAdaptersAndVisuals();
};
