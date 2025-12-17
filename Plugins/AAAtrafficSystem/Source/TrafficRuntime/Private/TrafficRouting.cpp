#include "TrafficRouting.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarTrafficRoutingTurnPolicy(
	TEXT("aaa.Traffic.Routing.TurnPolicy"),
	0,
	TEXT("Default turn selection policy when multiple movements exist for an incoming lane:\n")
	TEXT("  0 = ThroughFirst (default)\n")
	TEXT("  1 = LeftFirst\n")
	TEXT("  2 = RightFirst\n")
	TEXT("  3 = PreferNonThrough (Left/Right before Through; chooses Left/Right deterministically when both exist)\n"),
	ECVF_Default);

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

	const int32 Policy = CVarTrafficRoutingTurnPolicy.GetValueOnGameThread();

	// Cache common picks.
	const FTrafficMovement* Through = FindFirst(ETrafficTurnType::Through);
	const FTrafficMovement* Left = FindFirst(ETrafficTurnType::Left);
	const FTrafficMovement* Right = FindFirst(ETrafficTurnType::Right);
	const FTrafficMovement* UTurn = FindFirst(ETrafficTurnType::UTurn);

	auto PreferTurnDeterministic = [&]() -> const FTrafficMovement*
	{
		if (Left && Right)
		{
			// Deterministic split across lanes (no randomness, stable in tests).
			return ((IncomingLaneId & 1) == 0) ? Left : Right;
		}
		return Left ? Left : Right;
	};

	switch (Policy)
	{
	case 1: // LeftFirst
		if (Left) return Left;
		if (Through) return Through;
		if (Right) return Right;
		if (UTurn) return UTurn;
		break;
	case 2: // RightFirst
		if (Right) return Right;
		if (Through) return Through;
		if (Left) return Left;
		if (UTurn) return UTurn;
		break;
	case 3: // PreferNonThrough
		if (const FTrafficMovement* Turn = PreferTurnDeterministic()) return Turn;
		if (Through) return Through;
		if (UTurn) return UTurn;
		break;
	default: // ThroughFirst
		if (Through) return Through;
		if (Right) return Right;
		if (Left) return Left;
		if (UTurn) return UTurn;
		break;
	}

	return Options[0];
}
