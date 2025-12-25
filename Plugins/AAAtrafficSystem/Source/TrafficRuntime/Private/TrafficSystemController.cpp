#include "TrafficSystemController.h"
#include "TrafficGeometryProvider.h"
#include "TrafficGeometryProviderFactory.h"
#include "TrafficNetworkAsset.h"
#include "TrafficNetworkBuilder.h"
#include "TrafficRoadFamilySettings.h"
#include "TrafficRuntimeModule.h"
#include "TrafficVehicleManager.h"
#include "TrafficZoneGraphAdapter.h"
#include "TrafficRuntimeSettings.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "TrafficVehicleBase.h"
#include "TrafficKinematicFollower.h"
#include "TrafficRouting.h"
#include "TrafficLaneGeometry.h"
#include "HAL/IConsoleManager.h"
#include "DrawDebugHelpers.h"

static TAutoConsoleVariable<int32> CVarTrafficIntersectionControlMode(
	TEXT("aaa.Traffic.Intersections.ControlMode"),
	0,
	TEXT("Intersection control mode: 0=Uncontrolled (yield only), 1=4-Way Stop, 2=Traffic Lights (fixed time)."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficLightGreenSeconds(
	TEXT("aaa.Traffic.Intersections.TrafficLight.GreenSeconds"),
	10.0f,
	TEXT("Fixed-time traffic light green duration (seconds)."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficLightYellowSeconds(
	TEXT("aaa.Traffic.Intersections.TrafficLight.YellowSeconds"),
	2.0f,
	TEXT("Fixed-time traffic light yellow duration (seconds)."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficLightAllRedSeconds(
	TEXT("aaa.Traffic.Intersections.TrafficLight.AllRedSeconds"),
	1.0f,
	TEXT("Fixed-time traffic light all-red clearance duration (seconds)."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficLightCoordinationEnabled(
	TEXT("aaa.Traffic.Intersections.TrafficLight.CoordinationEnabled"),
	0,
	TEXT("If non-zero, applies phase offsets for a simple green-wave coordination."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficLightCoordinationSpeedCmPerSec(
	TEXT("aaa.Traffic.Intersections.TrafficLight.CoordinationSpeedCmPerSec"),
	1500.0f,
	TEXT("Speed (cm/sec) used to compute signal offsets along the coordination axis."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficLightCoordinationAxisMode(
	TEXT("aaa.Traffic.Intersections.TrafficLight.CoordinationAxisMode"),
	0,
	TEXT("Coordination axis: 0=Phase0Axis, 1=WorldX, 2=WorldY."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficLightPermittedLeftYield(
	TEXT("aaa.Traffic.Intersections.TrafficLight.PermittedLeftYield"),
	1,
	TEXT("If non-zero, left turns on green yield to conflicting priority movements."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficLightPermittedLeftApproachDistanceCm(
	TEXT("aaa.Traffic.Intersections.TrafficLight.PermittedLeftApproachDistanceCm"),
	1200.0f,
	TEXT("Distance (cm) before the stop line to treat oncoming vehicles as priority for permitted left turns."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficLightDebug(
	TEXT("aaa.Traffic.Intersections.TrafficLight.Debug"),
	0,
	TEXT("If non-zero, logs traffic light phase transitions."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficLightDebugDraw(
	TEXT("aaa.Traffic.Intersections.TrafficLight.DebugDraw"),
	0,
	TEXT("If non-zero, draws traffic light phase labels and axes at intersections."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficIntersectionReservationConflictDistanceCm(
	TEXT("aaa.Traffic.Intersections.ReservationConflictDistanceCm"),
	250.0f,
	TEXT("Minimum separation (cm) between two movement paths to allow simultaneous intersection reservations (TrafficLights mode only)."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficIntersectionReservationMaxLifetimeSeconds(
	TEXT("aaa.Traffic.Intersections.ReservationMaxLifetimeSeconds"),
	12.0f,
	TEXT("Hard cap (seconds) on how long an intersection reservation can persist (safety deadlock breaker)."),
	ECVF_Default);

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#endif

ATrafficSystemController::ATrafficSystemController()
{
	PrimaryActorTick.bCanEverTick = true;
	BuiltNetworkAsset = nullptr;
}

static bool IsPriorityTurnType(ETrafficTurnType TurnType)
{
	return (TurnType == ETrafficTurnType::Through || TurnType == ETrafficTurnType::Right);
}

static float ReadFloatCVar_Controller(const TCHAR* Name, float DefaultValue)
{
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		return Var->GetFloat();
	}
	return DefaultValue;
}

static int32 ReadIntCVar(const TCHAR* Name, int32 DefaultValue)
{
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		return Var->GetInt();
	}
	return DefaultValue;
}

static float ComputeAutoStopLineOffsetCm_Controller(float LaneWidthCm)
{
	const float Width = (LaneWidthCm > KINDA_SMALL_NUMBER)
		? LaneWidthCm
		: FMath::Max(0.f, ReadFloatCVar_Controller(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoNominalLaneWidthCm"), 350.f));
	const float Scale = FMath::Max(0.f, ReadFloatCVar_Controller(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoWidthScale"), 0.25f));
	const float MinCm = FMath::Max(0.f, ReadFloatCVar_Controller(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoMinCm"), 40.f));
	const float MaxCm = FMath::Max(MinCm, ReadFloatCVar_Controller(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoMaxCm"), 120.f));
	const float BufferCm = FMath::Max(0.f, ReadFloatCVar_Controller(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoBufferCm"), 50.f));
	return FMath::Clamp(Width * Scale, MinCm, MaxCm) + BufferCm;
}

static float GetEffectiveStopLineOffsetCm_Controller(const FTrafficLane* Lane)
{
	const float Base = FMath::Max(0.f, ReadFloatCVar_Controller(TEXT("aaa.Traffic.Intersections.StopLineOffsetCm"), 0.f));
	if (ReadIntCVar(TEXT("aaa.Traffic.Intersections.StopLineOffsetAuto"), 0) == 0)
	{
		return Base;
	}

	const float LaneWidth = Lane ? Lane->Width : 0.f;
	return ComputeAutoStopLineOffsetCm_Controller(LaneWidth);
}

void ATrafficSystemController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UWorld* World = GetWorld();
	if (!World || !BuiltNetworkAsset)
	{
		return;
	}

	const EWorldType::Type WorldType = World->WorldType;
	if (WorldType == EWorldType::Editor)
	{
		return;
	}

	if (CVarTrafficIntersectionControlMode.GetValueOnGameThread() != 2)
	{
		return;
	}

	UpdateIntersectionSignals(BuiltNetworkAsset->Network, World->GetTimeSeconds());

	if (CVarTrafficLightDebugDraw.GetValueOnGameThread() != 0)
	{
		auto PhaseName = [](ETrafficSignalPhase P) -> const TCHAR*
		{
			switch (P)
			{
			case ETrafficSignalPhase::Green: return TEXT("Green");
			case ETrafficSignalPhase::Yellow: return TEXT("Yellow");
			case ETrafficSignalPhase::AllRed: return TEXT("AllRed");
			default: return TEXT("Unknown");
			}
		};

		for (const FTrafficIntersection& Intersection : BuiltNetworkAsset->Network.Intersections)
		{
			const FIntersectionSignalState* State = IntersectionSignals.Find(Intersection.IntersectionId);
			if (!State || !State->bHasTwoPhases)
			{
				continue;
			}

			const float TimeLeft = FMath::Max(0.f, State->PhaseEndTimeSeconds - World->GetTimeSeconds());
			const FString Label = FString::Printf(
				TEXT("Int %d: %s (phase %d) t=%.1fs"),
				Intersection.IntersectionId,
				PhaseName(State->Phase),
				State->ActivePhaseIndex,
				TimeLeft);

			DrawDebugString(World, Intersection.Center + FVector(0, 0, 120.f), Label, nullptr, FColor::White, 0.f, true);

			const float AxisLen = FMath::Max(300.f, Intersection.Radius + 300.f);
			for (int32 PhaseIdx = 0; PhaseIdx < 2; ++PhaseIdx)
			{
				FVector2D Axis2D = State->PhaseAxisDirs[PhaseIdx];
				if (Axis2D.IsNearlyZero())
				{
					continue;
				}

				Axis2D.Normalize();
				const FVector Axis3D(Axis2D.X, Axis2D.Y, 0.f);

				FColor Color = FColor::Red;
				if (State->Phase == ETrafficSignalPhase::Green && PhaseIdx == State->ActivePhaseIndex)
				{
					Color = FColor::Green;
				}
				else if (State->Phase == ETrafficSignalPhase::Yellow && PhaseIdx == State->ActivePhaseIndex)
				{
					Color = FColor::Yellow;
				}

				const FVector A = Intersection.Center - Axis3D * AxisLen;
				const FVector B = Intersection.Center + Axis3D * AxisLen;
				DrawDebugLine(World, A, B, Color, false, 0.f, 0, 10.f);
			}
		}
	}
}

bool ATrafficSystemController::BuildNetworkInternal(UWorld* World)
{
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficSystemController] BuildNetworkInternal: World is null."));
		return false;
	}

	const UTrafficRoadFamilySettings* FamSettings = GetDefault<UTrafficRoadFamilySettings>();
	if (!FamSettings || FamSettings->Families.Num() == 0)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficSystemController] No road families configured."));
		return false;
	}

	ITrafficRoadGeometryProvider* ProviderInterface = nullptr;
	TObjectPtr<UObject> ProviderObject = UTrafficGeometryProviderFactory::CreateProvider(World, ProviderInterface);
	if (!ProviderObject || !ProviderInterface)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficSystemController] Failed to create geometry provider."));
		return false;
	}

	FTrafficNetwork Network;
	FTrafficNetworkBuilder::BuildNetworkFromWorld(
		World,
		*ProviderInterface,
		FamSettings,
		Network);

	if (bGenerateZoneGraph)
	{
		FTrafficZoneGraphAdapter::BuildZoneGraphForNetwork(World, Network, FamSettings);
	}

	if (!BuiltNetworkAsset)
	{
		BuiltNetworkAsset = NewObject<UTrafficNetworkAsset>(this, TEXT("TrafficNetwork_Transient"));
	}
	BuiltNetworkAsset->Network = Network;
	IntersectionReservations.Reset();
	IntersectionSignals.Reset();

	InitializeIntersectionSignals(Network, World->GetTimeSeconds());

	UE_LOG(LogTraffic, Log,
		TEXT("[TrafficSystemController] Built transient network: Roads=%d Lanes=%d Intersections=%d Movements=%d"),
		Network.Roads.Num(),
		Network.Lanes.Num(),
		Network.Intersections.Num(),
		Network.Movements.Num());

	const int32 StopLineAuto = ReadIntCVar(TEXT("aaa.Traffic.Intersections.StopLineOffsetAuto"), 0);
	if (StopLineAuto != 0)
	{
		const float Scale = FMath::Max(0.f, ReadFloatCVar_Controller(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoWidthScale"), 0.15f));
		const float MinCm = FMath::Max(0.f, ReadFloatCVar_Controller(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoMinCm"), 40.f));
		const float MaxCm = FMath::Max(MinCm, ReadFloatCVar_Controller(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoMaxCm"), 90.f));
		const float NominalCm = FMath::Max(0.f, ReadFloatCVar_Controller(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoNominalLaneWidthCm"), 350.f));
		const float BufferCm = FMath::Max(0.f, ReadFloatCVar_Controller(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoBufferCm"), 20.f));

		const FTrafficLane* SampleLane = Network.Lanes.Num() > 0 ? &Network.Lanes[0] : nullptr;
		const float LaneWidth = SampleLane ? SampleLane->Width : 0.f;
		const float AutoOffset = ComputeAutoStopLineOffsetCm_Controller(LaneWidth);

		UE_LOG(LogTraffic, Log,
			TEXT("[TrafficSystemController] StopLineAuto enabled=1 laneWidth=%.1f autoOffset=%.1f scale=%.2f min=%.1f max=%.1f buffer=%.1f nominal=%.1f"),
			LaneWidth,
			AutoOffset,
			Scale,
			MinCm,
			MaxCm,
			BufferCm,
			NominalCm);
	}
	else
	{
		const float BaseOffset = FMath::Max(0.f, ReadFloatCVar_Controller(TEXT("aaa.Traffic.Intersections.StopLineOffsetCm"), 0.f));
		UE_LOG(LogTraffic, Log, TEXT("[TrafficSystemController] StopLineAuto enabled=0 baseOffset=%.1f (auto settings ignored)"),
			BaseOffset);
	}

	return Network.Roads.Num() > 0 && Network.Lanes.Num() > 0;
}

FVector2D ATrafficSystemController::ResolveCoordinationAxis2D(const FIntersectionSignalState& State)
{
	const int32 Mode = CVarTrafficLightCoordinationAxisMode.GetValueOnGameThread();
	if (Mode == 1)
	{
		return FVector2D(1.f, 0.f);
	}
	if (Mode == 2)
	{
		return FVector2D(0.f, 1.f);
	}

	FVector2D Axis = State.PhaseAxisDirs[0];
	if (Axis.IsNearlyZero())
	{
		Axis = FVector2D(1.f, 0.f);
	}
	Axis.Normalize();
	return Axis;
}

void ATrafficSystemController::InitSignalPhaseFromCycle(
	FIntersectionSignalState& State,
	float NowSeconds,
	float GreenSeconds,
	float YellowSeconds,
	float AllRedSeconds,
	float OffsetSeconds)
{
	const float PhaseSeconds = GreenSeconds + YellowSeconds + AllRedSeconds;
	const float CycleSeconds = PhaseSeconds * 2.0f;
	if (CycleSeconds <= KINDA_SMALL_NUMBER)
	{
		State.ActivePhaseIndex = 0;
		State.Phase = ETrafficSignalPhase::Green;
		State.PhaseEndTimeSeconds = NowSeconds + GreenSeconds;
		return;
	}

	float LocalTime = NowSeconds - OffsetSeconds;
	float CyclePos = FMath::Fmod(LocalTime, CycleSeconds);
	if (CyclePos < 0.f)
	{
		CyclePos += CycleSeconds;
	}

	const float Seg0 = GreenSeconds;
	const float Seg1 = Seg0 + YellowSeconds;
	const float Seg2 = Seg1 + AllRedSeconds;
	const float Seg3 = Seg2 + GreenSeconds;
	const float Seg4 = Seg3 + YellowSeconds;
	const float Seg5 = Seg4 + AllRedSeconds;

	if (CyclePos < Seg0)
	{
		State.ActivePhaseIndex = 0;
		State.Phase = ETrafficSignalPhase::Green;
		State.PhaseEndTimeSeconds = NowSeconds + (Seg0 - CyclePos);
	}
	else if (CyclePos < Seg1)
	{
		State.ActivePhaseIndex = 0;
		State.Phase = ETrafficSignalPhase::Yellow;
		State.PhaseEndTimeSeconds = NowSeconds + (Seg1 - CyclePos);
	}
	else if (CyclePos < Seg2)
	{
		State.ActivePhaseIndex = 0;
		State.Phase = ETrafficSignalPhase::AllRed;
		State.PhaseEndTimeSeconds = NowSeconds + (Seg2 - CyclePos);
	}
	else if (CyclePos < Seg3)
	{
		State.ActivePhaseIndex = 1;
		State.Phase = ETrafficSignalPhase::Green;
		State.PhaseEndTimeSeconds = NowSeconds + (Seg3 - CyclePos);
	}
	else if (CyclePos < Seg4)
	{
		State.ActivePhaseIndex = 1;
		State.Phase = ETrafficSignalPhase::Yellow;
		State.PhaseEndTimeSeconds = NowSeconds + (Seg4 - CyclePos);
	}
	else
	{
		State.ActivePhaseIndex = 1;
		State.Phase = ETrafficSignalPhase::AllRed;
		State.PhaseEndTimeSeconds = NowSeconds + (Seg5 - CyclePos);
	}
}

void ATrafficSystemController::InitializeIntersectionSignals(const FTrafficNetwork& Net, float NowSeconds)
{
	if (CVarTrafficIntersectionControlMode.GetValueOnGameThread() != 2)
	{
		return;
	}

	const float GreenSeconds = FMath::Max(0.1f, CVarTrafficLightGreenSeconds.GetValueOnGameThread());
	const float YellowSeconds = FMath::Max(0.f, CVarTrafficLightYellowSeconds.GetValueOnGameThread());
	const float AllRedSeconds = FMath::Max(0.f, CVarTrafficLightAllRedSeconds.GetValueOnGameThread());
	const bool bCoordination = (CVarTrafficLightCoordinationEnabled.GetValueOnGameThread() != 0);

	for (const FTrafficIntersection& Intersection : Net.Intersections)
	{
		FIntersectionSignalState State;
		State.IntersectionId = Intersection.IntersectionId;

		struct FAxis
		{
			FVector2D Dir = FVector2D::ZeroVector;
		};

		TArray<FAxis> Axes;
		Axes.Reserve(2);

		auto AddAxis = [&](FVector2D Dir)
		{
			Dir.Normalize();
			if (Dir.IsNearlyZero())
			{
				return;
			}

			// Treat opposite directions as the same axis.
			if (Dir.X < 0.f)
			{
				Dir *= -1.f;
			}

			for (const FAxis& Existing : Axes)
			{
				const float Dot = FMath::Abs(FVector2D::DotProduct(Dir, Existing.Dir));
				if (Dot > 0.85f)
				{
					return;
				}
			}

			if (Axes.Num() < 2)
			{
				FAxis A;
				A.Dir = Dir;
				Axes.Add(A);
			}
		};

		for (int32 IncomingLaneId : Intersection.IncomingLaneIds)
		{
			const FTrafficLane* Lane = TrafficRouting::FindLaneById(Net, IncomingLaneId);
			if (!Lane || Lane->CenterlinePoints.Num() < 2)
			{
				continue;
			}

			const FVector P0 = Lane->CenterlinePoints[Lane->CenterlinePoints.Num() - 2];
			const FVector P1 = Lane->CenterlinePoints[Lane->CenterlinePoints.Num() - 1];
			AddAxis(FVector2D(P1.X - P0.X, P1.Y - P0.Y));
		}

		if (Axes.Num() < 2)
		{
			continue;
		}

		State.PhaseAxisDirs[0] = Axes[0].Dir;
		State.PhaseAxisDirs[1] = Axes[1].Dir;

		for (int32 IncomingLaneId : Intersection.IncomingLaneIds)
		{
			const FTrafficLane* Lane = TrafficRouting::FindLaneById(Net, IncomingLaneId);
			if (!Lane || Lane->CenterlinePoints.Num() < 2)
			{
				continue;
			}

			const FVector P0 = Lane->CenterlinePoints[Lane->CenterlinePoints.Num() - 2];
			const FVector P1 = Lane->CenterlinePoints[Lane->CenterlinePoints.Num() - 1];
			FVector2D Dir(P1.X - P0.X, P1.Y - P0.Y);
			Dir.Normalize();
			if (Dir.X < 0.f)
			{
				Dir *= -1.f;
			}

			const float Dot0 = FMath::Abs(FVector2D::DotProduct(Dir, Axes[0].Dir));
			const float Dot1 = FMath::Abs(FVector2D::DotProduct(Dir, Axes[1].Dir));
			const int32 PhaseIdx = (Dot1 > Dot0) ? 1 : 0;
			State.PhaseIncomingLaneIds[PhaseIdx].Add(IncomingLaneId);
		}

		if (State.PhaseIncomingLaneIds[0].Num() == 0 || State.PhaseIncomingLaneIds[1].Num() == 0)
		{
			continue;
		}

		State.bHasTwoPhases = true;
		if (bCoordination)
		{
			const float SpeedCmPerSec = FMath::Max(1.f, CVarTrafficLightCoordinationSpeedCmPerSec.GetValueOnGameThread());
			const FVector2D Axis = ResolveCoordinationAxis2D(State);
			const FVector2D Center2D(Intersection.Center.X, Intersection.Center.Y);
			const float OffsetSeconds = FVector2D::DotProduct(Center2D, Axis) / SpeedCmPerSec;
			State.CycleOffsetSeconds = OffsetSeconds;
			InitSignalPhaseFromCycle(State, NowSeconds, GreenSeconds, YellowSeconds, AllRedSeconds, OffsetSeconds);
		}
		else
		{
			State.ActivePhaseIndex = 0;
			State.Phase = ETrafficSignalPhase::Green;
			State.PhaseEndTimeSeconds = NowSeconds + GreenSeconds;
		}

		IntersectionSignals.Add(State.IntersectionId, MoveTemp(State));
	}
}

void ATrafficSystemController::UpdateIntersectionSignals(const FTrafficNetwork& Net, float NowSeconds)
{
	if (IntersectionSignals.Num() == 0 && Net.Intersections.Num() > 0)
	{
		InitializeIntersectionSignals(Net, NowSeconds);
	}

	const float GreenSeconds = FMath::Max(0.1f, CVarTrafficLightGreenSeconds.GetValueOnGameThread());
	const float YellowSeconds = FMath::Max(0.f, CVarTrafficLightYellowSeconds.GetValueOnGameThread());
	const float AllRedSeconds = FMath::Max(0.f, CVarTrafficLightAllRedSeconds.GetValueOnGameThread());
	const bool bDebug = (CVarTrafficLightDebug.GetValueOnGameThread() != 0);

	for (TPair<int32, FIntersectionSignalState>& Pair : IntersectionSignals)
	{
		FIntersectionSignalState& State = Pair.Value;
		if (!State.bHasTwoPhases)
		{
			continue;
		}

		if (NowSeconds < State.PhaseEndTimeSeconds)
		{
			continue;
		}

		const ETrafficSignalPhase PrevPhase = State.Phase;
		const int32 PrevActive = State.ActivePhaseIndex;

		if (State.Phase == ETrafficSignalPhase::Green)
		{
			State.Phase = ETrafficSignalPhase::Yellow;
			State.PhaseEndTimeSeconds = NowSeconds + YellowSeconds;
		}
		else if (State.Phase == ETrafficSignalPhase::Yellow)
		{
			State.Phase = ETrafficSignalPhase::AllRed;
			State.PhaseEndTimeSeconds = NowSeconds + AllRedSeconds;
		}
		else
		{
			State.ActivePhaseIndex = (State.ActivePhaseIndex == 0) ? 1 : 0;
			State.Phase = ETrafficSignalPhase::Green;
			State.PhaseEndTimeSeconds = NowSeconds + GreenSeconds;
		}

		if (bDebug)
		{
			auto PhaseName = [](ETrafficSignalPhase P) -> const TCHAR*
			{
				switch (P)
				{
				case ETrafficSignalPhase::Green: return TEXT("Green");
				case ETrafficSignalPhase::Yellow: return TEXT("Yellow");
				case ETrafficSignalPhase::AllRed: return TEXT("AllRed");
				default: return TEXT("Unknown");
				}
			};

			UE_LOG(LogTraffic, Log, TEXT("[TrafficLights] Intersection %d %s->%s (phaseGroup %d->%d)"),
				State.IntersectionId,
				PhaseName(PrevPhase),
				PhaseName(State.Phase),
				PrevActive,
				State.ActivePhaseIndex);
		}
	}
}

bool ATrafficSystemController::IsMovementAllowedBySignals(const FTrafficNetwork& Net, int32 IntersectionId, int32 MovementId) const
{
	if (CVarTrafficIntersectionControlMode.GetValueOnGameThread() != 2)
	{
		return true;
	}

	const FIntersectionSignalState* State = IntersectionSignals.Find(IntersectionId);
	if (!State || !State->bHasTwoPhases)
	{
		return true;
	}

	// Only allow new entries on green for the active phase group.
	if (State->Phase != ETrafficSignalPhase::Green)
	{
		return false;
	}

	const FTrafficMovement* Movement = TrafficRouting::FindMovementById(Net, MovementId);
	if (!Movement)
	{
		return false;
	}

	return State->PhaseIncomingLaneIds[State->ActivePhaseIndex].Contains(Movement->IncomingLaneId);
}

bool ATrafficSystemController::DoMovementsConflict2D(const FTrafficNetwork& Net, int32 IntersectionId, int32 MovementAId, int32 MovementBId, float ConflictDistanceCm) const
{
	if (MovementAId == MovementBId)
	{
		return true;
	}

	const FTrafficMovement* A = TrafficRouting::FindMovementById(Net, MovementAId);
	const FTrafficMovement* B = TrafficRouting::FindMovementById(Net, MovementBId);
	if (!A || !B)
	{
		return true;
	}

	if (A->IncomingLaneId == B->IncomingLaneId)
	{
		return true;
	}

	if (A->PathPoints.Num() < 2 || B->PathPoints.Num() < 2)
	{
		return true;
	}

	const float DistSq = FMath::Square(FMath::Max(0.f, ConflictDistanceCm));

	for (int32 i = 0; i < A->PathPoints.Num() - 1; ++i)
	{
		const FVector A0(A->PathPoints[i].X, A->PathPoints[i].Y, 0.f);
		const FVector A1(A->PathPoints[i + 1].X, A->PathPoints[i + 1].Y, 0.f);

		for (int32 j = 0; j < B->PathPoints.Num() - 1; ++j)
		{
			const FVector B0(B->PathPoints[j].X, B->PathPoints[j].Y, 0.f);
			const FVector B1(B->PathPoints[j + 1].X, B->PathPoints[j + 1].Y, 0.f);

			FVector PA, PB;
			FMath::SegmentDistToSegment(A0, A1, B0, B1, PA, PB);
			if (FVector::DistSquared(PA, PB) <= DistSq)
			{
				return true;
			}
		}
	}

	return false;
}

void ATrafficSystemController::Editor_BuildTrafficNetwork()
{
#if WITH_EDITOR
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficSystemController] Editor_BuildTrafficNetwork: World is null."));
		return;
	}

	UE_LOG(LogTraffic, Log, TEXT("[TrafficSystemController] Editor_BuildTrafficNetwork: BEGIN"));
	BuildNetworkInternal(World);
#endif
}

void ATrafficSystemController::Editor_SpawnTestVehicles()
{
#if WITH_EDITOR
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficSystemController] Editor_SpawnTestVehicles: World is null."));
		return;
	}

	UE_LOG(LogTraffic, Log, TEXT("[TrafficSystemController] Editor_SpawnTestVehicles: Stub - vehicle spawning not yet implemented."));
#endif
}

void ATrafficSystemController::Runtime_BuildTrafficNetwork()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficSystemController] Runtime_BuildTrafficNetwork: World is null."));
		return;
	}

	BuildNetworkInternal(World);
}

void ATrafficSystemController::Runtime_SpawnTraffic()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficSystemController] Runtime_SpawnTraffic: World is null."));
		return;
	}

	// Find or spawn a TrafficVehicleManager in this world.
	ATrafficVehicleManager* Manager = nullptr;
	for (TActorIterator<ATrafficVehicleManager> It(World); It; ++It)
	{
		Manager = *It;
		break;
	}

	if (!Manager)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Manager = World->SpawnActor<ATrafficVehicleManager>(ATrafficVehicleManager::StaticClass(), FTransform::Identity, Params);
	}

	if (!Manager)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficSystemController] Runtime_SpawnTraffic: Failed to spawn TrafficVehicleManager."));
		return;
	}

	// Runtime uses Chaos visuals when configured; no logic-only forcing here.
	Manager->SetForceLogicOnlyForTests(false);
	if (bGenerateZoneGraph)
	{
		Manager->SpawnZoneGraphVehicles(VehiclesPerLaneRuntime, RuntimeSpeedCmPerSec, FName(TEXT("Vehicles")));

		// Fallback: if ZoneGraph produced nothing (e.g. no ZoneGraphData yet), keep existing spline-based spawning working.
		if (Manager->GetSpawnedVehicleCount() == 0)
		{
			UE_LOG(LogTraffic, Warning, TEXT("[TrafficSystemController] ZoneGraph spawn produced 0 vehicles; falling back to TrafficNetwork lane spawning."));
			Manager->SpawnTestVehicles(VehiclesPerLaneRuntime, RuntimeSpeedCmPerSec);
		}
	}
	else
	{
		Manager->SpawnTestVehicles(VehiclesPerLaneRuntime, RuntimeSpeedCmPerSec);
	}
}

void ATrafficSystemController::SetRuntimeConfigFromProjectSettings()
{
	const UTrafficRuntimeSettings* Settings = GetDefault<UTrafficRuntimeSettings>();
	if (!Settings)
	{
		return;
	}

	VehiclesPerLaneRuntime = FMath::Clamp(Settings->VehiclesPerLaneRuntime, 0, 50);
	RuntimeSpeedCmPerSec = FMath::Clamp(Settings->RuntimeSpeedCmPerSec, 0.f, 100000.f);
	bGenerateZoneGraph = Settings->bGenerateZoneGraph;
}

void ATrafficSystemController::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Only auto-run in PIE/Game worlds, not the pure Editor world.
	const EWorldType::Type WorldType = World->WorldType;
	if (WorldType == EWorldType::Editor)
	{
		return;
	}

	if (bAutoBuildOnBeginPlay)
	{
		SetRuntimeConfigFromProjectSettings();
		Runtime_BuildTrafficNetwork();
	}

	if (bAutoSpawnOnBeginPlay)
	{
		SetRuntimeConfigFromProjectSettings();
		Runtime_SpawnTraffic();
	}
}

int32 ATrafficSystemController::GetNumRoads() const
{
	return BuiltNetworkAsset ? BuiltNetworkAsset->Network.Roads.Num() : 0;
}

int32 ATrafficSystemController::GetNumLanes() const
{
	return BuiltNetworkAsset ? BuiltNetworkAsset->Network.Lanes.Num() : 0;
}

bool ATrafficSystemController::TryReserveIntersection(int32 IntersectionId, ATrafficVehicleBase* Vehicle, int32 MovementId, float HoldSeconds)
{
	if (IntersectionId < 0 || !Vehicle)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const float Now = World->GetTimeSeconds();
	const float ExpireAt = Now + FMath::Max(0.1f, HoldSeconds);
	const float MaxLifetimeSeconds = FMath::Max(0.f, CVarTrafficIntersectionReservationMaxLifetimeSeconds.GetValueOnGameThread());

	const int32 ControlMode = CVarTrafficIntersectionControlMode.GetValueOnGameThread();
	const bool bTrafficLightsMode = (ControlMode == 2);

	// Ensure signal phase is up-to-date even if vehicle ticks run before the controller tick this frame.
	if (bTrafficLightsMode && BuiltNetworkAsset)
	{
		UpdateIntersectionSignals(BuiltNetworkAsset->Network, Now);
	}

	if (BuiltNetworkAsset && !IsMovementAllowedBySignals(BuiltNetworkAsset->Network, IntersectionId, MovementId))
	{
		return false;
	}

	if (bTrafficLightsMode && BuiltNetworkAsset &&
		ShouldYieldPermittedLeft(BuiltNetworkAsset->Network, IntersectionId, MovementId, Vehicle))
	{
		return false;
	}

	const FTrafficIntersection* Intersection = nullptr;
	float KeepAliveRadiusCm = 0.f;
	if (BuiltNetworkAsset)
	{
		Intersection = BuiltNetworkAsset->Network.Intersections.FindByPredicate(
			[&](const FTrafficIntersection& I) { return I.IntersectionId == IntersectionId; });

		if (Intersection)
		{
			const float StopLineOffsetCm = GetEffectiveStopLineOffsetCm_Controller(nullptr);

			// Avoid dropping a reservation while the vehicle is still physically near/within the intersection,
			// even if the time-based hold expires (release should normally clear it).
			KeepAliveRadiusCm = FMath::Max(0.f, Intersection->Radius) + StopLineOffsetCm + 200.f;
		}
	}

	TArray<FIntersectionReservation>& Reservations = IntersectionReservations.FindOrAdd(IntersectionId);
	for (int32 Idx = Reservations.Num() - 1; Idx >= 0; --Idx)
	{
		const FIntersectionReservation& R = Reservations[Idx];
		const bool bExpired = (R.ExpireTimeSeconds > 0.f && Now > R.ExpireTimeSeconds);
		const bool bInvalid = !R.Vehicle.IsValid();
		if (bInvalid)
		{
			Reservations.RemoveAtSwap(Idx, 1, EAllowShrinking::No);
			continue;
		}

		if (bExpired)
		{
			if (MaxLifetimeSeconds > 0.f && R.CreatedTimeSeconds > 0.f && (Now - R.CreatedTimeSeconds) > MaxLifetimeSeconds)
			{
				Reservations.RemoveAtSwap(Idx, 1, EAllowShrinking::No);
				continue;
			}

			const ATrafficVehicleBase* ReservedVehicle = R.Vehicle.Get();
			if (Intersection && ReservedVehicle && KeepAliveRadiusCm > 0.f)
			{
				const float DistSq = FVector::DistSquared(ReservedVehicle->GetActorLocation(), Intersection->Center);
				if (DistSq <= FMath::Square(KeepAliveRadiusCm))
				{
					continue;
				}
			}

			Reservations.RemoveAtSwap(Idx, 1, EAllowShrinking::No);
		}
	}

	for (FIntersectionReservation& Existing : Reservations)
	{
		if (Existing.Vehicle.Get() == Vehicle)
		{
			Existing.MovementId = MovementId;
			Existing.ExpireTimeSeconds = ExpireAt;
			if (Existing.CreatedTimeSeconds <= 0.f)
			{
				Existing.CreatedTimeSeconds = Now;
			}
			return true;
		}
	}

	// In non-signal modes, keep the simple "one vehicle at a time" reservation to stay conservative.
	if (!bTrafficLightsMode)
	{
		if (Reservations.Num() > 0)
		{
			return false;
		}

		FIntersectionReservation NewRes;
		NewRes.Vehicle = Vehicle;
		NewRes.MovementId = MovementId;
		NewRes.ExpireTimeSeconds = ExpireAt;
		Reservations.Add(MoveTemp(NewRes));
		return true;
	}

	// In signal mode, allow multiple simultaneous reservations as long as their movement paths don't conflict.
	if (BuiltNetworkAsset)
	{
		const float ConflictDist = CVarTrafficIntersectionReservationConflictDistanceCm.GetValueOnGameThread();
		for (const FIntersectionReservation& Existing : Reservations)
		{
			if (Existing.MovementId == INDEX_NONE)
			{
				continue;
			}

			if (DoMovementsConflict2D(BuiltNetworkAsset->Network, IntersectionId, MovementId, Existing.MovementId, ConflictDist))
			{
				return false;
			}
		}
	}

	FIntersectionReservation NewRes;
	NewRes.Vehicle = Vehicle;
	NewRes.MovementId = MovementId;
	NewRes.ExpireTimeSeconds = ExpireAt;
	NewRes.CreatedTimeSeconds = Now;
	Reservations.Add(MoveTemp(NewRes));
	return true;
}

bool ATrafficSystemController::ShouldYieldPermittedLeft(const FTrafficNetwork& Net, int32 IntersectionId, int32 MovementId, const ATrafficVehicleBase* Vehicle) const
{
	if (!Vehicle)
	{
		return false;
	}

	if (CVarTrafficLightPermittedLeftYield.GetValueOnGameThread() == 0)
	{
		return false;
	}

	const FTrafficMovement* Movement = TrafficRouting::FindMovementById(Net, MovementId);
	if (!Movement || Movement->IntersectionId != IntersectionId || Movement->TurnType != ETrafficTurnType::Left)
	{
		return false;
	}

	const float ConflictDist = CVarTrafficIntersectionReservationConflictDistanceCm.GetValueOnGameThread();
	TSet<int32> PriorityMovements;
	TSet<int32> PriorityIncomingLaneIds;

	for (const FTrafficMovement& Other : Net.Movements)
	{
		if (Other.IntersectionId != IntersectionId || Other.MovementId == MovementId)
		{
			continue;
		}

		if (!IsPriorityTurnType(Other.TurnType))
		{
			continue;
		}

		if (DoMovementsConflict2D(Net, IntersectionId, MovementId, Other.MovementId, ConflictDist))
		{
			PriorityMovements.Add(Other.MovementId);
			PriorityIncomingLaneIds.Add(Other.IncomingLaneId);
		}
	}

	if (PriorityMovements.Num() == 0)
	{
		return false;
	}

	if (const TArray<FIntersectionReservation>* Reservations = IntersectionReservations.Find(IntersectionId))
	{
		for (const FIntersectionReservation& Existing : *Reservations)
		{
			if (PriorityMovements.Contains(Existing.MovementId))
			{
				return true;
			}
		}
	}

	const float ApproachDistance = FMath::Max(0.f, CVarTrafficLightPermittedLeftApproachDistanceCm.GetValueOnGameThread());
	if (ApproachDistance <= 0.f)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	for (TActorIterator<ATrafficVehicleBase> It(World); It; ++It)
	{
		ATrafficVehicleBase* OtherVehicle = *It;
		if (!OtherVehicle || OtherVehicle == Vehicle)
		{
			continue;
		}

		EPathFollowTargetType OtherType = EPathFollowTargetType::None;
		int32 OtherTargetId = INDEX_NONE;
		float OtherS = 0.f;
		if (!OtherVehicle->GetFollowTarget(OtherType, OtherTargetId, OtherS))
		{
			continue;
		}

		if (OtherType == EPathFollowTargetType::Movement)
		{
			if (PriorityMovements.Contains(OtherTargetId))
			{
				return true;
			}
			continue;
		}

		if (OtherType != EPathFollowTargetType::Lane || !PriorityIncomingLaneIds.Contains(OtherTargetId))
		{
			continue;
		}

		const FTrafficLane* Lane = TrafficRouting::FindLaneById(Net, OtherTargetId);
		if (!Lane)
		{
			continue;
		}

		const float LaneLen = TrafficLaneGeometry::ComputeLaneLengthCm(*Lane);
		if (LaneLen <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const float StopLineOffset = GetEffectiveStopLineOffsetCm_Controller(Lane);
		const float StopS = FMath::Max(0.f, LaneLen - StopLineOffset);
		const float DistToStop = StopS - OtherS;
		if (DistToStop <= ApproachDistance)
		{
			return true;
		}
	}

	return false;
}

void ATrafficSystemController::ReleaseIntersection(int32 IntersectionId, ATrafficVehicleBase* Vehicle)
{
	if (IntersectionId < 0 || !Vehicle)
	{
		return;
	}

	TArray<FIntersectionReservation>* Reservations = IntersectionReservations.Find(IntersectionId);
	if (!Reservations)
	{
		return;
	}

	for (int32 Idx = Reservations->Num() - 1; Idx >= 0; --Idx)
	{
		const FIntersectionReservation& R = (*Reservations)[Idx];
		if (!R.Vehicle.IsValid() || R.Vehicle.Get() == Vehicle)
		{
			Reservations->RemoveAtSwap(Idx, 1, EAllowShrinking::No);
		}
	}

	if (Reservations->Num() == 0)
	{
		IntersectionReservations.Remove(IntersectionId);
	}
}

bool ATrafficSystemController::GetIntersectionSignalSnapshot(
	int32 IntersectionId,
	int32& OutActivePhaseIndex,
	int32& OutPhaseRaw,
	float& OutPhaseEndTimeSeconds,
	TArray<int32>& OutPhase0IncomingLaneIds,
	TArray<int32>& OutPhase1IncomingLaneIds) const
{
	const FIntersectionSignalState* State = IntersectionSignals.Find(IntersectionId);
	if (!State || !State->bHasTwoPhases)
	{
		return false;
	}

	OutActivePhaseIndex = State->ActivePhaseIndex;
	OutPhaseRaw = static_cast<int32>(State->Phase);
	OutPhaseEndTimeSeconds = State->PhaseEndTimeSeconds;

	OutPhase0IncomingLaneIds = State->PhaseIncomingLaneIds[0].Array();
	OutPhase1IncomingLaneIds = State->PhaseIncomingLaneIds[1].Array();

	return true;
}
