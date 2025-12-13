#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrafficAutomationLogger.h"
#include "ZoneGraphTypes.h"
#include "TrafficVehicleBase.generated.h"

class UStaticMeshComponent;
class UTrafficKinematicFollower;
class UZoneGraphSubsystem;
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

protected:
	UPROPERTY(VisibleAnywhere, Category="Traffic|Vehicle")
	UStaticMeshComponent* Body;

	UPROPERTY()
	UTrafficKinematicFollower* Follower;

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
