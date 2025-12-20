#include "TrafficChaosTestUtils.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "TrafficLaneGeometry.h"
#include "TrafficMovementGeometry.h"
#include "TrafficRouting.h"
#include "TrafficKinematicFollower.h"
#include "TrafficVehicleAdapter.h"
#include "TrafficVehicleBase.h"

namespace TrafficChaosTestUtils
{
	bool ProjectChaosOntoFollowTarget(
		const FTrafficNetwork& Net,
		const APawn& ChaosPawn,
		EPathFollowTargetType FollowType,
		int32 FollowId,
		float& OutS,
		float& OutPathErrorCm)
	{
		const FVector ChaosPos = ChaosPawn.GetActorLocation();
		const FVector ChaosFwd = ChaosPawn.GetActorForwardVector();

		if (FollowType == EPathFollowTargetType::Lane)
		{
			if (const FTrafficLane* Lane = TrafficRouting::FindLaneById(Net, FollowId))
			{
				FLaneProjectionResult Proj;
				if (TrafficLaneGeometry::ProjectPointOntoLane(*Lane, ChaosPos, ChaosFwd, Proj))
				{
					OutS = Proj.S;
					OutPathErrorCm = FMath::Abs(Proj.LateralOffsetCm);
					return true;
				}
			}
			return false;
		}

		if (FollowType == EPathFollowTargetType::Movement)
		{
			if (const FTrafficMovement* Move = TrafficRouting::FindMovementById(Net, FollowId))
			{
				float ProjS = 0.f;
				if (!TrafficMovementGeometry::ProjectPointOntoMovement(*Move, ChaosPos, ProjS))
				{
					return false;
				}

				FVector SamplePos;
				FVector SampleTangent;
				if (!TrafficMovementGeometry::SamplePoseAtS(*Move, ProjS, SamplePos, SampleTangent))
				{
					SamplePos = Move->PathPoints.Num() > 0 ? Move->PathPoints.Last() : ChaosPos;
				}

				OutS = ProjS;
				OutPathErrorCm = FVector::Dist2D(ChaosPos, SamplePos);
				return true;
			}
			return false;
		}

		return false;
	}

	ATrafficVehicleAdapter* FindAdapterForLogicVehicle(UWorld& World, const ATrafficVehicleBase& Logic)
	{
		for (TActorIterator<ATrafficVehicleAdapter> It(&World); It; ++It)
		{
			ATrafficVehicleAdapter* Adapter = *It;
			if (Adapter && Adapter->LogicVehicle.Get() == &Logic)
			{
				return Adapter;
			}
		}
		return nullptr;
	}

	bool EnsureChaosForLogicVehicle(
		UWorld& World,
		ATrafficVehicleBase& Logic,
		ATrafficVehicleAdapter*& OutAdapter,
		APawn*& OutChaos,
		FString& OutError)
	{
		OutAdapter = FindAdapterForLogicVehicle(World, Logic);
		if (!OutAdapter)
		{
			OutError = FString::Printf(
				TEXT("Logic vehicle '%s' has no TrafficVehicleAdapter; Chaos coupling is required for this test."),
				*Logic.GetName());
			return false;
		}

		if (!OutAdapter->ChaosVehicle.IsValid())
		{
			OutError = FString::Printf(
				TEXT("Logic vehicle '%s' is missing a valid Chaos vehicle; ensure VisualMode=2 and adapter/Chaos spawn succeeded."),
				*Logic.GetName());
			return false;
		}

		OutChaos = OutAdapter->ChaosVehicle.Get();
		return true;
	}
}
