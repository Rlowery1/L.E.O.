#include "TrafficRouting.h"

const FTrafficLane* TrafficRouting::FindLaneById(const FTrafficNetwork& Network, int32 LaneId)
{
	return Network.Lanes.FindByPredicate([&](const FTrafficLane& L)
	{
		return L.LaneId == LaneId;
	});
}

const FTrafficMovement* TrafficRouting::FindMovementById(const FTrafficNetwork& Network, int32 MovementId)
{
	return Network.Movements.FindByPredicate([&](const FTrafficMovement& M)
	{
		return M.MovementId == MovementId;
	});
}

void TrafficRouting::GetMovementsForIncomingLane(
	const FTrafficNetwork& Network,
	int32 IncomingLaneId,
	TArray<const FTrafficMovement*>& OutMovements)
{
	OutMovements.Reset();
	for (const FTrafficMovement& Movement : Network.Movements)
	{
		if (Movement.IncomingLaneId == IncomingLaneId)
		{
			OutMovements.Add(&Movement);
		}
	}
}

const FTrafficMovement* TrafficRouting::ChooseDefaultMovementForIncomingLane(
	const FTrafficNetwork& Network,
	int32 IncomingLaneId)
{
	TArray<const FTrafficMovement*> Options;
	GetMovementsForIncomingLane(Network, IncomingLaneId, Options);
	if (Options.Num() == 0)
	{
		return nullptr;
	}

	auto FindFirst = [&](ETrafficTurnType TurnType) -> const FTrafficMovement*
	{
		for (const FTrafficMovement* M : Options)
		{
			if (M && M->TurnType == TurnType)
			{
				return M;
			}
		}
		return nullptr;
	};

	if (const FTrafficMovement* Through = FindFirst(ETrafficTurnType::Through))
	{
		return Through;
	}
	if (const FTrafficMovement* Right = FindFirst(ETrafficTurnType::Right))
	{
		return Right;
	}
	if (const FTrafficMovement* Left = FindFirst(ETrafficTurnType::Left))
	{
		return Left;
	}

	return Options[0];
}

