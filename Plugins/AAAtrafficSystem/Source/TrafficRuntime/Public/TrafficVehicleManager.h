#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrafficVehicleManager.generated.h"

class UTrafficNetworkAsset;
class ATrafficVehicleBase;

UCLASS()
class TRAFFICRUNTIME_API ATrafficVehicleManager : public AActor
{
	GENERATED_BODY()

public:
	ATrafficVehicleManager();

	bool LoadNetwork();
	void SpawnTestVehicles(int32 VehiclesPerLane = 3, float SpeedCmPerSec = 800.f);
	void ClearVehicles();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	UTrafficNetworkAsset* NetworkAsset;

	UPROPERTY()
	TArray<ATrafficVehicleBase*> Vehicles;

	UTrafficNetworkAsset* FindNetworkAsset() const;
};

