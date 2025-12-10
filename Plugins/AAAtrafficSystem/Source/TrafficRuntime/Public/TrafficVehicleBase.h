#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrafficVehicleBase.generated.h"

class UStaticMeshComponent;
class UTrafficKinematicFollower;
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

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Body;

	UPROPERTY()
	UTrafficKinematicFollower* Follower;

	UTrafficKinematicFollower* EnsureFollower();
};

