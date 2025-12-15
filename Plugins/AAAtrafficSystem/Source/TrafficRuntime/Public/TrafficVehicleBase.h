#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrafficAutomationLogger.h"
#include "ZoneGraphTypes.h"
#include "TrafficVehicleBase.generated.h"

class UStaticMeshComponent;
class UTrafficKinematicFollower;
class UZoneGraphSubsystem;
class UTrafficNetworkAsset;
class ATrafficSystemController;
struct FTrafficLane;
struct FTrafficMovement;

UCLASS()
class TRAFFICRUNTIME_API ATrafficVehicleBase : public AActor
{
	GENERATED_BODY()

public:
	ATrafficVehicleBase();

	virtual void Tick(float DeltaSeconds) override;

	void InitializeOnLane(const FTrafficLane* Lane, float InitialS, float SpeedCmPerSec);
	void InitializeOnMovement(const FTrafficMovement* Movement, float InitialS, float SpeedCmPerSec);
	void SetNetworkAsset(UTrafficNetworkAsset* InNetworkAsset);
	void SetTrafficSystemController(ATrafficSystemController* InController) { TrafficController = InController; }

	/**
	 * Initialize this logic vehicle to follow a ZoneGraph lane (instead of an AAA Traffic lane polyline).
	 * This is used by ATrafficVehicleManager when spawning vehicles directly from ZoneGraph lanes.
	 */
	void InitializeOnZoneGraphLane(UZoneGraphSubsystem* ZoneGraphSubsystem, const FZoneGraphLaneHandle& LaneHandle, float InitialDistanceCm, float SpeedCmPerSec);

	void SampleLaneTrackingError(FTrafficRunMetrics& Metrics) const;
	void SampleDynamics(FTrafficRunMetrics& Metrics, float DeltaSeconds);

	/** Show/hide the debug body mesh (the cube). Used when Chaos visuals are active. */
	void SetDebugBodyVisible(bool bVisible);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UPROPERTY(VisibleAnywhere, Category="Traffic|Vehicle")
	UStaticMeshComponent* Body;

	UPROPERTY()
	UTrafficKinematicFollower* Follower;

	UPROPERTY()
	TObjectPtr<UTrafficNetworkAsset> NetworkAsset = nullptr;

	UPROPERTY()
	TWeakObjectPtr<ATrafficSystemController> TrafficController;

	// When following a movement, remember where we're headed so we can re-enter lane following.
	int32 PendingOutgoingLaneId = INDEX_NONE;

	// Intersection reservation / yielding.
	bool bWaitingForIntersection = false;
	int32 WaitingIntersectionId = INDEX_NONE;
	int32 WaitingMovementId = INDEX_NONE;
	int32 WaitingOutgoingLaneId = INDEX_NONE;
	bool bHasIntersectionReservation = false;
	int32 ReservedIntersectionId = INDEX_NONE;
	int32 ReservedMovementId = INDEX_NONE;
	int32 ReservedOutgoingLaneId = INDEX_NONE;

	int32 ActiveReservedIntersectionId = INDEX_NONE;
	float CruiseSpeedCmPerSec = 800.f;
	float StopUntilTimeSeconds = -1.f;
	float LastStopLineDebugTimeSeconds = -1.f;

	// ZoneGraph-following mode (alternative to UTrafficKinematicFollower).
	bool bUseZoneGraphLane = false;

	UPROPERTY()
	TObjectPtr<UZoneGraphSubsystem> ZoneGraph = nullptr;

	FZoneGraphLaneLocation ZoneLaneLocation;
	float ZoneSpeedCmPerSec = 0.f;

	mutable FVector LastPos = FVector::ZeroVector;
	mutable float LastSpeed = 0.f;
	mutable float LastAccel = 0.f;
	mutable float LastJerk = 0.f;

	UTrafficKinematicFollower* EnsureFollower();
};
