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
#include "TrafficRouting.h"
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

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#endif

ATrafficSystemController::ATrafficSystemController()
{
	PrimaryActorTick.bCanEverTick = true;
	BuiltNetworkAsset = nullptr;
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

	return Network.Roads.Num() > 0 && Network.Lanes.Num() > 0;
}

void ATrafficSystemController::InitializeIntersectionSignals(const FTrafficNetwork& Net, float NowSeconds)
{
	if (CVarTrafficIntersectionControlMode.GetValueOnGameThread() != 2)
	{
		return;
	}

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
		State.ActivePhaseIndex = 0;
		State.Phase = ETrafficSignalPhase::Green;
		State.PhaseEndTimeSeconds = NowSeconds + FMath::Max(0.1f, CVarTrafficLightGreenSeconds.GetValueOnGameThread());

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

	if (BuiltNetworkAsset && !IsMovementAllowedBySignals(BuiltNetworkAsset->Network, IntersectionId, MovementId))
	{
		return false;
	}

	const float Now = World->GetTimeSeconds();
	const float ExpireAt = Now + FMath::Max(0.1f, HoldSeconds);

	TArray<FIntersectionReservation>& Reservations = IntersectionReservations.FindOrAdd(IntersectionId);
	for (int32 Idx = Reservations.Num() - 1; Idx >= 0; --Idx)
	{
		const FIntersectionReservation& R = Reservations[Idx];
		const bool bExpired = (R.ExpireTimeSeconds > 0.f && Now > R.ExpireTimeSeconds);
		const bool bInvalid = !R.Vehicle.IsValid();
		if (bExpired || bInvalid)
		{
			Reservations.RemoveAtSwap(Idx, 1, EAllowShrinking::No);
		}
	}

	for (FIntersectionReservation& Existing : Reservations)
	{
		if (Existing.Vehicle.Get() == Vehicle)
		{
			Existing.MovementId = MovementId;
			Existing.ExpireTimeSeconds = ExpireAt;
			return true;
		}
	}

	const int32 ControlMode = CVarTrafficIntersectionControlMode.GetValueOnGameThread();
	const bool bTrafficLightsMode = (ControlMode == 2);

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
	Reservations.Add(MoveTemp(NewRes));
	return true;
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
