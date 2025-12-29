#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TrafficChaosTestUtils.h"
#include "TrafficSystemController.h"
#include "TrafficVehicleAdapter.h"
#include "TrafficVehicleBase.h"
#include "TrafficNetworkAsset.h"
#include "TrafficRouting.h"
#include "TrafficLaneGeometry.h"
#include "TrafficRuntimeSettings.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"

#if WITH_EDITOR

namespace
{
	static const TCHAR* RightOfWayMapPackage = TEXT("/AAAtrafficSystem/Maps/Traffic_BaselineCurve");
	static const float WaitForGreenTimeoutSeconds = 10.0f;
	static const float ReservationHoldSeconds = 2.0f;
	static const float PermittedLeftApproachDistanceCm = 1500.0f;

	struct FTrafficRightOfWaySettingsSnapshot
	{
		int32 VehiclesPerLaneRuntime = 1;
		float RuntimeSpeedCmPerSec = 800.f;
		bool bGenerateZoneGraph = false;
	};

	static void SaveRuntimeSettings_RightOfWay(FTrafficRightOfWaySettingsSnapshot& OutSnapshot)
	{
		if (UTrafficRuntimeSettings* Settings = GetMutableDefault<UTrafficRuntimeSettings>())
		{
			OutSnapshot.VehiclesPerLaneRuntime = Settings->VehiclesPerLaneRuntime;
			OutSnapshot.RuntimeSpeedCmPerSec = Settings->RuntimeSpeedCmPerSec;
			OutSnapshot.bGenerateZoneGraph = Settings->bGenerateZoneGraph;
		}
	}

	static void RestoreRuntimeSettings_RightOfWay(const FTrafficRightOfWaySettingsSnapshot& Snapshot)
	{
		if (UTrafficRuntimeSettings* Settings = GetMutableDefault<UTrafficRuntimeSettings>())
		{
			Settings->VehiclesPerLaneRuntime = Snapshot.VehiclesPerLaneRuntime;
			Settings->RuntimeSpeedCmPerSec = Snapshot.RuntimeSpeedCmPerSec;
			Settings->bGenerateZoneGraph = Snapshot.bGenerateZoneGraph;
		}
	}

	static float ReadFloatCVar_RightOfWay(const TCHAR* Name, float DefaultValue)
	{
		if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			return Var->GetFloat();
		}
		return DefaultValue;
	}

	static int32 ReadIntCVar_RightOfWay(const TCHAR* Name, int32 DefaultValue)
	{
		if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			return Var->GetInt();
		}
		return DefaultValue;
	}

	struct FTrafficRightOfWayState
	{
		TWeakObjectPtr<UWorld> PIEWorld;
		TWeakObjectPtr<ATrafficSystemController> Controller;
		TWeakObjectPtr<ATrafficVehicleBase> LeftVehicle;
		TWeakObjectPtr<ATrafficVehicleBase> ThroughVehicle;

		bool bInitialized = false;
		bool bLeftYielded = false;
		bool bLeftReservedAfterClear = false;

		int32 IntersectionId = INDEX_NONE;
		int32 LeftMovementId = INDEX_NONE;
		int32 ThroughMovementId = INDEX_NONE;
		int32 LeftIncomingLaneId = INDEX_NONE;
		int32 ThroughIncomingLaneId = INDEX_NONE;
		int32 LeftPhaseIndex = INDEX_NONE;

		double PhaseWaitStartSeconds = 0.0;

		bool bSavedSettings = false;
		FTrafficRightOfWaySettingsSnapshot PrevSettings;

		int32 PrevControlMode = 0;
		int32 PrevReservationEnabled = 1;
		int32 PrevRequireFullStop = 0;
		int32 PrevStopLineOffsetAuto = 0;
		int32 PrevPermittedLeftYield = 0;
		float PrevPermittedLeftDistance = 0.f;
		int32 PrevCoordinationEnabled = 0;
	};

	static bool FindControllerAndNetwork_RightOfWay(UWorld* World, ATrafficSystemController*& OutController, const FTrafficNetwork*& OutNet)
	{
		OutController = nullptr;
		OutNet = nullptr;

		for (TActorIterator<ATrafficSystemController> It(World); It; ++It)
		{
			OutController = *It;
			break;
		}

		if (!OutController)
		{
			return false;
		}

		UTrafficNetworkAsset* NetAsset = OutController->GetBuiltNetworkAsset();
		if (!NetAsset || NetAsset->Network.Intersections.Num() == 0)
		{
			return false;
		}

		OutNet = &NetAsset->Network;
		return true;
	}

	static bool DoMovementsConflict2D(const FTrafficNetwork& Net, int32 MovementAId, int32 MovementBId, float ConflictDistanceCm)
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

	static bool FindPermittedLeftScenario(
		ATrafficSystemController& Controller,
		const FTrafficNetwork& Net,
		int32& OutIntersectionId,
		int32& OutLeftMovementId,
		int32& OutThroughMovementId,
		int32& OutLeftIncomingLaneId,
		int32& OutThroughIncomingLaneId,
		int32& OutLeftPhaseIndex)
	{
		const float ConflictDist = ReadFloatCVar_RightOfWay(TEXT("aaa.Traffic.Intersections.ReservationConflictDistanceCm"), 300.f);

		for (const FTrafficIntersection& Intersection : Net.Intersections)
		{
			int32 ActivePhaseIndex = INDEX_NONE;
			int32 PhaseRaw = INDEX_NONE;
			float PhaseEnd = 0.f;
			TArray<int32> Phase0;
			TArray<int32> Phase1;

			if (!Controller.GetIntersectionSignalSnapshot(Intersection.IntersectionId, ActivePhaseIndex, PhaseRaw, PhaseEnd, Phase0, Phase1))
			{
				continue;
			}

			const TSet<int32> PhaseLanes[2] = { TSet<int32>(Phase0), TSet<int32>(Phase1) };

			for (const FTrafficMovement& LeftMove : Net.Movements)
			{
				if (LeftMove.IntersectionId != Intersection.IntersectionId ||
					LeftMove.TurnType != ETrafficTurnType::Left)
				{
					continue;
				}

				int32 PhaseIndex = INDEX_NONE;
				if (PhaseLanes[0].Contains(LeftMove.IncomingLaneId))
				{
					PhaseIndex = 0;
				}
				else if (PhaseLanes[1].Contains(LeftMove.IncomingLaneId))
				{
					PhaseIndex = 1;
				}

				if (PhaseIndex == INDEX_NONE)
				{
					continue;
				}

				for (const FTrafficMovement& ThroughMove : Net.Movements)
				{
					if (ThroughMove.IntersectionId != Intersection.IntersectionId ||
						ThroughMove.TurnType != ETrafficTurnType::Through)
					{
						continue;
					}

					if (!PhaseLanes[PhaseIndex].Contains(ThroughMove.IncomingLaneId))
					{
						continue;
					}

					if (!DoMovementsConflict2D(Net, LeftMove.MovementId, ThroughMove.MovementId, ConflictDist))
					{
						continue;
					}

					OutIntersectionId = Intersection.IntersectionId;
					OutLeftMovementId = LeftMove.MovementId;
					OutThroughMovementId = ThroughMove.MovementId;
					OutLeftIncomingLaneId = LeftMove.IncomingLaneId;
					OutThroughIncomingLaneId = ThroughMove.IncomingLaneId;
					OutLeftPhaseIndex = PhaseIndex;
					return true;
				}
			}
		}

		return false;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(FTrafficRightOfWayWaitForPIEWorldCommand, TSharedRef<FTrafficRightOfWayState>, State, FAutomationTestBase*, Test, double, TimeoutSeconds, double, StartTime);
	bool FTrafficRightOfWayWaitForPIEWorldCommand::Update()
	{
		if (GEditor && GEditor->PlayWorld)
		{
			State->PIEWorld = GEditor->PlayWorld;
			return true;
		}

		const double Elapsed = FPlatformTime::Seconds() - StartTime;
		if (Elapsed > TimeoutSeconds)
		{
			if (Test)
			{
				Test->AddError(TEXT("PIE world did not start for right-of-way test."));
			}
			return true;
		}

		return false;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FTrafficRightOfWayInitCommand, TSharedRef<FTrafficRightOfWayState>, State, FAutomationTestBase*, Test);
	bool FTrafficRightOfWayInitCommand::Update()
	{
		if (!State->PIEWorld.IsValid() && GEditor && GEditor->PlayWorld)
		{
			State->PIEWorld = GEditor->PlayWorld;
		}

		UWorld* World = State->PIEWorld.Get();
		if (!World)
		{
			return false;
		}

		ATrafficSystemController* Controller = nullptr;
		const FTrafficNetwork* Net = nullptr;
		if (!FindControllerAndNetwork_RightOfWay(World, Controller, Net))
		{
			return false;
		}

		State->Controller = Controller;

		if (!FindPermittedLeftScenario(*Controller, *Net,
			State->IntersectionId,
			State->LeftMovementId,
			State->ThroughMovementId,
			State->LeftIncomingLaneId,
			State->ThroughIncomingLaneId,
			State->LeftPhaseIndex))
		{
			if (Test)
			{
				Test->AddError(TEXT("Right-of-way test could not find a conflicting left/through movement pair in the map."));
			}
			return true;
		}

		const FTrafficLane* LeftLane = TrafficRouting::FindLaneById(*Net, State->LeftIncomingLaneId);
		const FTrafficLane* ThroughLane = TrafficRouting::FindLaneById(*Net, State->ThroughIncomingLaneId);
		if (!LeftLane || !ThroughLane)
		{
			if (Test)
			{
				Test->AddError(TEXT("Right-of-way test could not resolve incoming lanes."));
			}
			return true;
		}

		const float StopLineOffset = ReadFloatCVar_RightOfWay(TEXT("aaa.Traffic.Intersections.StopLineOffsetCm"), 0.f);
		const float LeftLaneLen = TrafficLaneGeometry::ComputeLaneLengthCm(*LeftLane);
		const float ThroughLaneLen = TrafficLaneGeometry::ComputeLaneLengthCm(*ThroughLane);
		const float LeftStopS = FMath::Max(0.f, LeftLaneLen - StopLineOffset);
		const float ThroughStopS = FMath::Max(0.f, ThroughLaneLen - StopLineOffset);

		const float LeftStartS = FMath::Max(0.f, LeftStopS - 200.f);
		const float ThroughStartS = FMath::Max(0.f, ThroughStopS - (PermittedLeftApproachDistanceCm * 0.5f));

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		State->LeftVehicle = World->SpawnActor<ATrafficVehicleBase>(ATrafficVehicleBase::StaticClass(), FTransform::Identity, Params);
		State->ThroughVehicle = World->SpawnActor<ATrafficVehicleBase>(ATrafficVehicleBase::StaticClass(), FTransform::Identity, Params);

		if (!State->LeftVehicle.IsValid() || !State->ThroughVehicle.IsValid())
		{
			if (Test)
			{
				Test->AddError(TEXT("Right-of-way test failed to spawn logic vehicles."));
			}
			return true;
		}

		UTrafficNetworkAsset* NetAsset = Controller->GetBuiltNetworkAsset();
		State->LeftVehicle->SetNetworkAsset(NetAsset);
		State->ThroughVehicle->SetNetworkAsset(NetAsset);
		State->LeftVehicle->SetTrafficSystemController(Controller);
		State->ThroughVehicle->SetTrafficSystemController(Controller);
		State->LeftVehicle->InitializeOnLane(LeftLane, LeftStartS, 0.f);
		State->ThroughVehicle->InitializeOnLane(ThroughLane, ThroughStartS, 0.f);

		ATrafficVehicleAdapter* LeftAdapter = nullptr;
		ATrafficVehicleAdapter* ThroughAdapter = nullptr;
		APawn* LeftChaos = nullptr;
		APawn* ThroughChaos = nullptr;
		FString ChaosError;
		if (!TrafficChaosTestUtils::EnsureChaosForLogicVehicle(*World, *State->LeftVehicle, LeftAdapter, LeftChaos, ChaosError) ||
			!TrafficChaosTestUtils::EnsureChaosForLogicVehicle(*World, *State->ThroughVehicle, ThroughAdapter, ThroughChaos, ChaosError))
		{
			if (Test)
			{
				Test->AddError(ChaosError.IsEmpty() ? TEXT("Right-of-way test failed to spawn Chaos vehicles.") : *ChaosError);
			}
			return true;
		}

		State->LeftVehicle->SetActorTickEnabled(false);
		State->ThroughVehicle->SetActorTickEnabled(false);

		State->PhaseWaitStartSeconds = FPlatformTime::Seconds();
		State->bInitialized = true;
		return true;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FTrafficRightOfWayWaitForGreenCommand, TSharedRef<FTrafficRightOfWayState>, State, FAutomationTestBase*, Test);
	bool FTrafficRightOfWayWaitForGreenCommand::Update()
	{
		if (!State->bInitialized)
		{
			return false;
		}

		UWorld* World = State->PIEWorld.Get();
		if (!World)
		{
			return false;
		}

		ATrafficSystemController* Controller = State->Controller.Get();
		if (!Controller)
		{
			return false;
		}

		int32 ActivePhaseIndex = INDEX_NONE;
		int32 PhaseRaw = INDEX_NONE;
		float PhaseEnd = 0.f;
		TArray<int32> Phase0;
		TArray<int32> Phase1;

		if (Controller->GetIntersectionSignalSnapshot(State->IntersectionId, ActivePhaseIndex, PhaseRaw, PhaseEnd, Phase0, Phase1))
		{
			if (ActivePhaseIndex == State->LeftPhaseIndex && PhaseRaw == 0)
			{
				return true;
			}
		}

		const double Elapsed = FPlatformTime::Seconds() - State->PhaseWaitStartSeconds;
		if (Elapsed > WaitForGreenTimeoutSeconds)
		{
			if (Test)
			{
				Test->AddError(TEXT("Right-of-way test timed out waiting for the left-turn phase to be green."));
			}
			return true;
		}

		return false;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FTrafficRightOfWayRunCommand, TSharedRef<FTrafficRightOfWayState>, State, FAutomationTestBase*, Test);
	bool FTrafficRightOfWayRunCommand::Update()
	{
		if (!State->bInitialized)
		{
			return true;
		}

		ATrafficSystemController* Controller = State->Controller.Get();
		if (!Controller)
		{
			if (Test)
			{
				Test->AddError(TEXT("Right-of-way test missing controller during run."));
			}
			return true;
		}

		if (!State->LeftVehicle.IsValid() || !State->ThroughVehicle.IsValid())
		{
			if (Test)
			{
				Test->AddError(TEXT("Right-of-way test vehicles missing during run."));
			}
			return true;
		}

		State->bLeftYielded = !Controller->TryReserveIntersection(State->IntersectionId, State->LeftVehicle.Get(), State->LeftMovementId, ReservationHoldSeconds);

		const FTrafficNetwork* Net = nullptr;
		{
			ATrafficSystemController* Tmp = nullptr;
			if (!FindControllerAndNetwork_RightOfWay(State->PIEWorld.Get(), Tmp, Net) || !Net)
			{
				if (Test)
				{
					Test->AddError(TEXT("Right-of-way test could not reacquire network."));
				}
				return true;
			}
		}

		const FTrafficLane* ThroughLane = TrafficRouting::FindLaneById(*Net, State->ThroughIncomingLaneId);
		if (ThroughLane)
		{
			const float StopLineOffset = ReadFloatCVar_RightOfWay(TEXT("aaa.Traffic.Intersections.StopLineOffsetCm"), 0.f);
			const float LaneLen = TrafficLaneGeometry::ComputeLaneLengthCm(*ThroughLane);
			const float StopS = FMath::Max(0.f, LaneLen - StopLineOffset);
			const float FarS = FMath::Max(0.f, StopS - (PermittedLeftApproachDistanceCm + 2000.f));
			State->ThroughVehicle->InitializeOnLane(ThroughLane, FarS, 0.f);
		}

		State->bLeftReservedAfterClear = Controller->TryReserveIntersection(State->IntersectionId, State->LeftVehicle.Get(), State->LeftMovementId, ReservationHoldSeconds);

		return true;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FTrafficRightOfWayEndCommand, TSharedRef<FTrafficRightOfWayState>, State, FAutomationTestBase*, Test);
	bool FTrafficRightOfWayEndCommand::Update()
	{
		if (State->bSavedSettings)
		{
			RestoreRuntimeSettings_RightOfWay(State->PrevSettings);

			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.ControlMode")))
			{
				Var->Set(State->PrevControlMode, ECVF_SetByCode);
			}
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.ReservationEnabled")))
			{
				Var->Set(State->PrevReservationEnabled, ECVF_SetByCode);
			}
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.RequireFullStop")))
			{
				Var->Set(State->PrevRequireFullStop, ECVF_SetByCode);
			}
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.PermittedLeftYield")))
			{
				Var->Set(State->PrevPermittedLeftYield, ECVF_SetByCode);
			}
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.StopLineOffsetAuto")))
			{
				Var->Set(State->PrevStopLineOffsetAuto, ECVF_SetByCode);
			}
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.PermittedLeftApproachDistanceCm")))
			{
				Var->Set(State->PrevPermittedLeftDistance, ECVF_SetByCode);
			}
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.CoordinationEnabled")))
			{
				Var->Set(State->PrevCoordinationEnabled, ECVF_SetByCode);
			}

			State->bSavedSettings = false;
		}

		if (GEditor)
		{
			GEditor->EndPlayMap();
		}

		if (!State->bLeftYielded)
		{
			Test->AddError(TEXT("Permitted-left vehicle did not yield to opposing through traffic."));
		}

		if (!State->bLeftReservedAfterClear)
		{
			Test->AddError(TEXT("Permitted-left vehicle did not reserve after opposing traffic cleared."));
		}

		return true;
	}
}

#endif // WITH_EDITOR

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficRightOfWayTest,
	"Traffic.Intersections.RightOfWay.BaselineCurve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficRightOfWayTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	if (!AutomationOpenMap(RightOfWayMapPackage))
	{
		AddError(TEXT("Failed to load /AAAtrafficSystem/Maps/Traffic_BaselineCurve."));
		return false;
	}

	TSharedRef<FTrafficRightOfWayState> State = MakeShared<FTrafficRightOfWayState>();
	State->bSavedSettings = true;
	SaveRuntimeSettings_RightOfWay(State->PrevSettings);

	if (UTrafficRuntimeSettings* Settings = GetMutableDefault<UTrafficRuntimeSettings>())
	{
		Settings->VehiclesPerLaneRuntime = 0;
		Settings->RuntimeSpeedCmPerSec = 800.f;
		Settings->bGenerateZoneGraph = false;
	}

	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.ControlMode")))
	{
		State->PrevControlMode = Var->GetInt();
		Var->Set(2, ECVF_SetByCode);
	}
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.ReservationEnabled")))
	{
		State->PrevReservationEnabled = Var->GetInt();
		Var->Set(1, ECVF_SetByCode);
	}
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.RequireFullStop")))
	{
		State->PrevRequireFullStop = Var->GetInt();
		Var->Set(0, ECVF_SetByCode);
	}
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.StopLineOffsetAuto")))
	{
		State->PrevStopLineOffsetAuto = Var->GetInt();
		Var->Set(0, ECVF_SetByCode);
	}
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.PermittedLeftYield")))
	{
		State->PrevPermittedLeftYield = Var->GetInt();
		Var->Set(1, ECVF_SetByCode);
	}
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.PermittedLeftApproachDistanceCm")))
	{
		State->PrevPermittedLeftDistance = Var->GetFloat();
		Var->Set(PermittedLeftApproachDistanceCm, ECVF_SetByCode);
	}
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.CoordinationEnabled")))
	{
		State->PrevCoordinationEnabled = Var->GetInt();
		Var->Set(0, ECVF_SetByCode);
	}

	AddExpectedError(TEXT("The Editor is currently in a play mode."), EAutomationExpectedErrorFlags::Contains, 3);

	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficRightOfWayWaitForPIEWorldCommand(State, this, 15.0, FPlatformTime::Seconds()));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficRightOfWayInitCommand(State, this));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficRightOfWayWaitForGreenCommand(State, this));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficRightOfWayRunCommand(State, this));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficRightOfWayEndCommand(State, this));

	return true;
#else
	return false;
#endif
}
