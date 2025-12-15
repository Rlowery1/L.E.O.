#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrafficSystemController.generated.h"

class UTrafficNetworkAsset;
class UWorld;
class ATrafficVehicleBase;

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

	/** Apply runtime config (speed/vehicles-per-lane/ZoneGraph) from Project Settings -> AAA Traffic Settings. */
	UFUNCTION(BlueprintCallable, Category="Traffic|Runtime")
	void SetRuntimeConfigFromProjectSettings();

	UTrafficNetworkAsset* GetBuiltNetworkAsset() const { return BuiltNetworkAsset; }
	int32 GetNumRoads() const;
	int32 GetNumLanes() const;

	// Intersection right-of-way (simple reservation system).
	bool TryReserveIntersection(int32 IntersectionId, ATrafficVehicleBase* Vehicle, int32 MovementId, float HoldSeconds);
	void ReleaseIntersection(int32 IntersectionId, ATrafficVehicleBase* Vehicle);

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

	// ZoneGraph integration (Editor/PIE workflows; requires ZoneGraph plugin).
	UPROPERTY(EditAnywhere, Category="Traffic|ZoneGraph")
	bool bGenerateZoneGraph = true;

private:
	bool BuildNetworkInternal(UWorld* World);

	struct FIntersectionReservation
	{
		TWeakObjectPtr<ATrafficVehicleBase> Vehicle;
		int32 MovementId = INDEX_NONE;
		float ExpireTimeSeconds = 0.f;
	};

	TMap<int32, FIntersectionReservation> IntersectionReservations;
};
