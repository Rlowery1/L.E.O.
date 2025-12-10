#pragma once

#include "CoreMinimal.h"
#include "TrafficVehicleBase.h"
#include "TrafficVehicleAdapter.generated.h"

/**
 * Adapter vehicle that drives with the AAA kinematic follower but renders any user-supplied vehicle Blueprint/Class.
 * The spawned visual is attached and collision-disabled so it does not fight the kinematic pathing.
 */
UCLASS()
class TRAFFICRUNTIME_API ATrafficVehicleAdapter : public ATrafficVehicleBase
{
	GENERATED_BODY()

public:
	ATrafficVehicleAdapter();

	/** Optional external vehicle class to spawn and attach for visuals (can be any Actor, e.g. Chaos vehicle BP). */
	UPROPERTY(EditAnywhere, Category="Vehicle")
	TSoftClassPtr<AActor> ExternalVehicleClass;

	/** Explicitly set the visual class before spawning traffic (used by the manager). */
	void SetExternalVisualClass(const TSoftClassPtr<AActor>& InClass);

	/** True if a visual actor was spawned and attached. */
	bool HasVisualAttached() const { return SpawnedVisual != nullptr; }

	/** Access the spawned visual actor (may be null). */
	AActor* GetVisualActor() const { return SpawnedVisual; }
	/** Force-spawn the visual immediately (useful in editor/automation where BeginPlay may not run). */
	void EnsureVisualAttached();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	AActor* SpawnedVisual = nullptr;

	void EnsureVisualSpawned();
	void SyncVisualTransform();
};
