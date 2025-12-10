#include "TrafficKinematicFollower.h"
#include "TrafficLaneGeometry.h"
#include "TrafficMovementGeometry.h"
#include "TrafficRuntimeModule.h"

void UTrafficKinematicFollower::InitForLane(
	const FTrafficLane* InLane,
	float InStartS,
	float InSpeedCmPerSec)
{
	LanePtr = InLane;
	MovementPtr = nullptr;
	State.TargetType = EPathFollowTargetType::Lane;
	State.TargetId = InLane ? InLane->LaneId : INDEX_NONE;
	State.S = FMath::Max(0.0f, InStartS);
	State.SpeedCmPerSec = InSpeedCmPerSec;
}

void UTrafficKinematicFollower::InitForMovement(
	const FTrafficMovement* InMovement,
	float InStartS,
	float InSpeedCmPerSec)
{
	LanePtr = nullptr;
	MovementPtr = InMovement;
	State.TargetType = EPathFollowTargetType::Movement;
	State.TargetId = InMovement ? InMovement->MovementId : INDEX_NONE;
	State.S = FMath::Max(0.0f, InStartS);
	State.SpeedCmPerSec = InSpeedCmPerSec;
}

void UTrafficKinematicFollower::Step(float DeltaTime)
{
	if (DeltaTime <= 0.0f)
	{
		return;
	}

	if (State.TargetType == EPathFollowTargetType::Lane && LanePtr)
	{
		const float DeltaS = State.SpeedCmPerSec * DeltaTime;
		State.S += DeltaS;

		const float LaneLength = TrafficLaneGeometry::ComputeLaneLengthCm(*LanePtr);
		State.S = FMath::Clamp(State.S, 0.0f, LaneLength);
	}
	else if (State.TargetType == EPathFollowTargetType::Movement && MovementPtr)
	{
		TArray<FMovementSample> Samples;
		TrafficMovementGeometry::AnalyzeMovementPath(*MovementPtr, Samples);
		float MovementLength = 0.0f;
		if (Samples.Num() > 0)
		{
			MovementLength = Samples.Last().S;
		}

		const float DeltaS = State.SpeedCmPerSec * DeltaTime;
		State.S += DeltaS;
		State.S = FMath::Clamp(State.S, 0.0f, MovementLength);
	}
}

bool UTrafficKinematicFollower::GetCurrentPose(FVector& OutPosition, FVector& OutTangent) const
{
	if (State.TargetType == EPathFollowTargetType::Lane && LanePtr)
	{
		return TrafficLaneGeometry::SamplePoseAtS(*LanePtr, State.S, OutPosition, OutTangent);
	}
	else if (State.TargetType == EPathFollowTargetType::Movement && MovementPtr)
	{
		return TrafficMovementGeometry::SamplePoseAtS(*MovementPtr, State.S, OutPosition, OutTangent);
	}
	return false;
}

