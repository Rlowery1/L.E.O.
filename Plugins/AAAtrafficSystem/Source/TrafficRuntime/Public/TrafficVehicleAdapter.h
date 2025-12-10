#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrafficVehicleAdapter.generated.h"

class ATrafficVehicleBase;

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
};
