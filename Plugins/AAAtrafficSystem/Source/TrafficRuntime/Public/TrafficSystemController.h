#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrafficRoadTypes.h"
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

	virtual void Tick(float DeltaSeconds) override;

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

	// Signal state snapshot (for tests/debug).
	bool GetIntersectionSignalSnapshot(
		int32 IntersectionId,
		int32& OutActivePhaseIndex,
		int32& OutPhaseRaw,
		float& OutPhaseEndTimeSeconds,
		TArray<int32>& OutPhase0IncomingLaneIds,
		TArray<int32>& OutPhase1IncomingLaneIds) const;

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
		float CreatedTimeSeconds = 0.f;
	};

	TMap<int32, TArray<FIntersectionReservation>> IntersectionReservations;

	bool DoMovementsConflict2D(const FTrafficNetwork& Net, int32 IntersectionId, int32 MovementAId, int32 MovementBId, float ConflictDistanceCm) const;

	enum class ETrafficSignalPhase : uint8
	{
		Green,
		Yellow,
		AllRed,
	};

	struct FIntersectionSignalState
	{
		int32 IntersectionId = INDEX_NONE;
		TSet<int32> PhaseIncomingLaneIds[2];
		FVector2D PhaseAxisDirs[2] = { FVector2D::ZeroVector, FVector2D::ZeroVector };
		bool bHasTwoPhases = false;
		int32 ActivePhaseIndex = 0;
		ETrafficSignalPhase Phase = ETrafficSignalPhase::Green;
		float PhaseEndTimeSeconds = 0.f;
	};

	TMap<int32, FIntersectionSignalState> IntersectionSignals;

	void InitializeIntersectionSignals(const FTrafficNetwork& Net, float NowSeconds);
	void UpdateIntersectionSignals(const FTrafficNetwork& Net, float NowSeconds);
	bool IsMovementAllowedBySignals(const FTrafficNetwork& Net, int32 IntersectionId, int32 MovementId) const;
};
