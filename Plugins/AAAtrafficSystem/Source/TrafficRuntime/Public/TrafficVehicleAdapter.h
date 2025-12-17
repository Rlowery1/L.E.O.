#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrafficVehicleAdapter.generated.h"

class ATrafficVehicleBase;
class UPrimitiveComponent;

/**
 * Adapter that binds a logic vehicle (ATrafficVehicleBase) to a visual/Chaos pawn.
 * For now, the adapter simply teleports the visual to the logic transform each tick.
 */
UCLASS()
class TRAFFICRUNTIME_API ATrafficVehicleAdapter : public AActor
{
	GENERATED_BODY()

public:
	ATrafficVehicleAdapter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TrafficVehicle")
	TWeakObjectPtr<ATrafficVehicleBase> LogicVehicle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TrafficVehicle")
	TWeakObjectPtr<APawn> ChaosVehicle;

	/** Initialize bindings after spawn. */
	void Initialize(ATrafficVehicleBase* InLogic, APawn* InChaos);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	struct FChaosDrivePrimWarmupState
	{
		TWeakObjectPtr<UPrimitiveComponent> Prim;
		bool bWasSimulatingPhysics = false;
		bool bWasGravityEnabled = true;
	};

	// Debug: track follow target transitions (lane <-> movement) for sampled vehicles.
	uint8 PrevFollowTargetTypeRaw = 0;
	int32 PrevFollowTargetId = INDEX_NONE;
	float PrevFollowTargetS = 0.f;
	bool bHasPrevFollowTarget = false;

	bool bLoggedChaosDriveInit = false;
	bool bLoggedChaosDriveMissingMovement = false;
	bool bChaosDriveMovementSuspended = false;
	float ChaosDriveMovementResumeAgeSeconds = 0.f;
	bool bChaosDriveUprightFixed = false;
	bool bChaosDrivePhysicsSuspended = false;
	float ChaosDrivePhysicsResumeAgeSeconds = 0.f;
	TArray<FChaosDrivePrimWarmupState> ChaosDrivePhysicsComps;

	// When road collision isn't ready yet, temporarily disable physics and (optionally) hide the pawn.
	TArray<FChaosDrivePrimWarmupState> ChaosDriveRoadHoldPhysicsComps;

	// Temporary spawn stabilization (damping) for ChaosDrive vehicles.
	TWeakObjectPtr<UPrimitiveComponent> ChaosDriveSpawnDampedPrim;
	float ChaosDriveSavedLinearDamping = 0.f;
	float ChaosDriveSavedAngularDamping = 0.f;
	bool bChaosDriveHasSavedDamping = false;

	// Grounding diagnostics for ChaosDrive stability.
	bool bChaosDriveEverGrounded = false;
	float ChaosDriveLastGroundDiagAgeSeconds = -1000.f;
	float ChaosDriveLastHoldDiagAgeSeconds = -1000.f;
	bool bChaosDriveLoggedGroundMismatch = false;
	bool bChaosDriveAwaitingRoadCollision = false;
	bool bChaosDriveWasHiddenForRoadCollision = false;

	float PrevSteer = 0.f;
	float PrevThrottle = 0.f;
	float PrevBrake = 0.f;
};
