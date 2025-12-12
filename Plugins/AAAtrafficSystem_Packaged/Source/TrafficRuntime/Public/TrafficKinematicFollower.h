#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TrafficRoadTypes.h"
#include "TrafficKinematicFollower.generated.h"

UENUM(BlueprintType)
enum class EPathFollowTargetType : uint8
{
	None,
	Lane,
	Movement
};

USTRUCT(BlueprintType)
struct TRAFFICRUNTIME_API FPathFollowState
{
	GENERATED_BODY()

	UPROPERTY()
	EPathFollowTargetType TargetType = EPathFollowTargetType::None;

	UPROPERTY()
	int32 TargetId = INDEX_NONE;

	UPROPERTY()
	float S = 0.0f;

	UPROPERTY()
	float SpeedCmPerSec = 0.0f;
};

UCLASS()
class TRAFFICRUNTIME_API UTrafficKinematicFollower : public UObject
{
	GENERATED_BODY()

public:
	void InitForLane(const FTrafficLane* InLane, float InStartS, float InSpeedCmPerSec);

	void InitForMovement(const FTrafficMovement* InMovement, float InStartS, float InSpeedCmPerSec);

	void Step(float DeltaTime);

	bool GetCurrentPose(FVector& OutPosition, FVector& OutTangent) const;

	const FPathFollowState& GetState() const { return State; }
	float GetCurrentSpeedCmPerSec() const { return State.SpeedCmPerSec; }
	float GetDistanceAlongLane() const { return State.S; }
	const FTrafficLane* GetCurrentLane() const { return LanePtr; }

private:
	FPathFollowState State;

	const FTrafficLane* LanePtr = nullptr;
	const FTrafficMovement* MovementPtr = nullptr;
};

