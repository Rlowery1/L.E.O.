#pragma once

#include "CoreMinimal.h"
#include "TrafficRoadTypes.h"
#include "TrafficKinematicFollower.h"

class APawn;
class ATrafficVehicleAdapter;
class ATrafficVehicleBase;
class UWorld;
struct FTrafficNetwork;

namespace TrafficChaosTestUtils
{
	bool ProjectChaosOntoFollowTarget(
		const FTrafficNetwork& Net,
		const APawn& ChaosPawn,
		EPathFollowTargetType FollowType,
		int32 FollowId,
		float& OutS,
		float& OutPathErrorCm);

	ATrafficVehicleAdapter* FindAdapterForLogicVehicle(UWorld& World, const ATrafficVehicleBase& Logic);

	bool EnsureChaosForLogicVehicle(
		UWorld& World,
		ATrafficVehicleBase& Logic,
		ATrafficVehicleAdapter*& OutAdapter,
		APawn*& OutChaos,
		FString& OutError);
}
