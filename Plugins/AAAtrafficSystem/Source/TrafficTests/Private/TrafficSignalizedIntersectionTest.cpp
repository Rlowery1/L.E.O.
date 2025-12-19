#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TrafficSystemController.h"
#include "TrafficVehicleAdapter.h"
#include "TrafficVehicleBase.h"
#include "TrafficNetworkAsset.h"
#include "TrafficRouting.h"
#include "TrafficKinematicFollower.h"
#include "HAL/PlatformTime.h"

#if WITH_EDITOR

namespace
{
	static const TCHAR* SignalizedMapPackage = TEXT("/AAAtrafficSystem/Maps/Traffic_BaselineCurve");
	static const float SignalizedSimSeconds = 20.0f;
	static const float MinEntrySpeedCmPerSec = 10.0f;
	static const float TimingToleranceSeconds = 0.75f;

	struct FFollowSnapshot
	{
		EPathFollowTargetType Type = EPathFollowTargetType::None;
		int32 Id = INDEX_NONE;
	};

	struct FTrafficSignalizedState
	{
		TWeakObjectPtr<UWorld> PIEWorld;
		TWeakObjectPtr<ATrafficSystemController> Controller;
		int32 IntersectionId = INDEX_NONE;

		bool bInitialized = false;
		bool bSawPhaseSwitch = false;
		bool bEntryOnGreen = false;
		bool bEntryOnSwitchedPhase = false;
		bool bChaosEntry = false;
		bool bViolation = false;
		bool bTimingComplete = false;

		int32 InitialActivePhaseIndex = INDEX_NONE;
		int32 InitialPhaseRaw = INDEX_NONE;
		int32 EntryPhaseIndex = INDEX_NONE;

		double StartTimeSeconds = 0.0;
		double LastPhaseChangeTimeSeconds = 0.0;
		int32 LastPhaseRaw = INDEX_NONE;
		TArray<float> PhaseDurations[3];

		float TargetGreenSeconds = 6.0f;
		float TargetYellowSeconds = 1.5f;
		float TargetAllRedSeconds = 1.0f;

		TSet<int32> PhaseIncomingLaneIds[2];
		TMap<TWeakObjectPtr<ATrafficVehicleBase>, FFollowSnapshot> PrevFollow;

		bool bSavedCVars = false;
		int32 PrevReservationEnabled = 1;
		int32 PrevRequireFullStop = 0;
		int32 PrevControlMode = 0;
		float PrevGreenSeconds = 10.0f;
		float PrevYellowSeconds = 2.0f;
		float PrevAllRedSeconds = 1.0f;
		int32 PrevCoordEnabled = 0;
		float PrevCoordSpeed = 1500.0f;
		int32 PrevCoordAxisMode = 0;
	};

	static bool FindControllerAndNetwork(UWorld* World, ATrafficSystemController*& OutController, const FTrafficNetwork*& OutNet)
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
		if (!NetAsset || NetAsset->Network.Intersections.Num() == 0 || NetAsset->Network.Movements.Num() == 0)
		{
			return false;
		}

		OutNet = &NetAsset->Network;
		return true;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(FTrafficSignalizedWaitForPIEWorldCommand, TSharedRef<FTrafficSignalizedState>, State, FAutomationTestBase*, Test, double, TimeoutSeconds, double, StartTime);
	bool FTrafficSignalizedWaitForPIEWorldCommand::Update()
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
				Test->AddError(TEXT("PIE world did not start for signalized intersection test."));
			}
			return true;
		}

		return false;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FTrafficSignalizedInitCommand, TSharedRef<FTrafficSignalizedState>, State, FAutomationTestBase*, Test);
	bool FTrafficSignalizedInitCommand::Update()
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
		if (!FindControllerAndNetwork(World, Controller, Net))
		{
			return false;
		}

		State->Controller = Controller;

		for (const FTrafficIntersection& Intersection : Net->Intersections)
		{
			int32 ActivePhaseIndex = INDEX_NONE;
			int32 PhaseRaw = INDEX_NONE;
			float PhaseEnd = 0.f;
			TArray<int32> Phase0;
			TArray<int32> Phase1;
			if (!Controller->GetIntersectionSignalSnapshot(Intersection.IntersectionId, ActivePhaseIndex, PhaseRaw, PhaseEnd, Phase0, Phase1))
			{
				continue;
			}

			if (Phase0.Num() == 0 || Phase1.Num() == 0)
			{
				continue;
			}

			State->IntersectionId = Intersection.IntersectionId;
			State->InitialActivePhaseIndex = ActivePhaseIndex;
			State->InitialPhaseRaw = PhaseRaw;
			State->PhaseIncomingLaneIds[0] = TSet<int32>(Phase0);
			State->PhaseIncomingLaneIds[1] = TSet<int32>(Phase1);
			State->StartTimeSeconds = FPlatformTime::Seconds();
			State->bInitialized = true;
			return true;
		}

		if (Test)
		{
			Test->AddError(TEXT("No signalized intersection found (missing two-phase traffic lights)."));
		}
		return true;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FTrafficSignalizedSampleCommand, TSharedRef<FTrafficSignalizedState>, State, FAutomationTestBase*, Test);
	bool FTrafficSignalizedSampleCommand::Update()
	{
		if (!State->PIEWorld.IsValid() && GEditor && GEditor->PlayWorld)
		{
			State->PIEWorld = GEditor->PlayWorld;
		}

		UWorld* World = State->PIEWorld.Get();
		if (!World || !State->bInitialized)
		{
			return false;
		}

		ATrafficSystemController* Controller = State->Controller.Get();
		if (!Controller)
		{
			for (TActorIterator<ATrafficSystemController> It(World); It; ++It)
			{
				Controller = *It;
				State->Controller = Controller;
				break;
			}
		}
		if (!Controller)
		{
			return false;
		}

		const FTrafficNetwork* Net = nullptr;
		{
			ATrafficSystemController* Tmp = nullptr;
			if (!FindControllerAndNetwork(World, Tmp, Net) || !Net)
			{
				return false;
			}
		}

		int32 ActivePhaseIndex = INDEX_NONE;
		int32 PhaseRaw = INDEX_NONE;
		float PhaseEnd = 0.f;
		TArray<int32> Phase0;
		TArray<int32> Phase1;
		if (!Controller->GetIntersectionSignalSnapshot(State->IntersectionId, ActivePhaseIndex, PhaseRaw, PhaseEnd, Phase0, Phase1))
		{
			return false;
		}

		if (State->StartTimeSeconds <= 0.0)
		{
			State->StartTimeSeconds = FPlatformTime::Seconds();
		}

		if (ActivePhaseIndex != State->InitialActivePhaseIndex)
		{
			State->bSawPhaseSwitch = true;
		}

		const double NowSeconds = FPlatformTime::Seconds();
		if (State->LastPhaseRaw == INDEX_NONE)
		{
			State->LastPhaseRaw = PhaseRaw;
			State->LastPhaseChangeTimeSeconds = NowSeconds;
		}
		else if (PhaseRaw != State->LastPhaseRaw)
		{
			const double Duration = NowSeconds - State->LastPhaseChangeTimeSeconds;
			if (State->LastPhaseRaw >= 0 && State->LastPhaseRaw < 3)
			{
				State->PhaseDurations[State->LastPhaseRaw].Add(static_cast<float>(Duration));
			}
			State->LastPhaseRaw = PhaseRaw;
			State->LastPhaseChangeTimeSeconds = NowSeconds;
		}

		if (!State->bTimingComplete)
		{
			if (State->PhaseDurations[0].Num() > 0 &&
				State->PhaseDurations[1].Num() > 0 &&
				State->PhaseDurations[2].Num() > 0)
			{
				State->bTimingComplete = true;
			}
		}

		const int32 InactivePhaseIndex = (ActivePhaseIndex == 0) ? 1 : 0;
		const TSet<int32>& ActiveLanes = State->PhaseIncomingLaneIds[ActivePhaseIndex];
		const TSet<int32>& InactiveLanes = State->PhaseIncomingLaneIds[InactivePhaseIndex];
		const bool bGreen = (PhaseRaw == 0);

		for (TActorIterator<ATrafficVehicleAdapter> It(World); It; ++It)
		{
			ATrafficVehicleAdapter* Adapter = *It;
			if (!Adapter || !Adapter->LogicVehicle.IsValid())
			{
				continue;
			}

			ATrafficVehicleBase* Logic = Adapter->LogicVehicle.Get();
			APawn* Chaos = Adapter->ChaosVehicle.Get();
			if (!Logic)
			{
				continue;
			}

			EPathFollowTargetType FollowType = EPathFollowTargetType::None;
			int32 FollowId = INDEX_NONE;
			float FollowS = 0.f;
			if (!Logic->GetFollowTarget(FollowType, FollowId, FollowS))
			{
				continue;
			}

			FFollowSnapshot Prev = State->PrevFollow.FindRef(Logic);

			if (FollowType == EPathFollowTargetType::Movement &&
				(Prev.Type != EPathFollowTargetType::Movement || Prev.Id != FollowId))
			{
				const FTrafficMovement* Movement = TrafficRouting::FindMovementById(*Net, FollowId);
				if (Movement && Movement->IntersectionId == State->IntersectionId)
				{
					const bool bInActiveLane = ActiveLanes.Contains(Movement->IncomingLaneId);
					const bool bInInactiveLane = InactiveLanes.Contains(Movement->IncomingLaneId);
					const bool bAllowed = bGreen && bInActiveLane;

					if (!bAllowed)
					{
						State->bViolation = true;
						if (Test)
						{
							Test->AddError(TEXT("Signal violation: movement entry occurred on red/inactive phase."));
						}
						return true;
					}

					if (State->EntryPhaseIndex == INDEX_NONE)
					{
						State->EntryPhaseIndex = ActivePhaseIndex;
						State->bEntryOnGreen = true;
					}
					else if (ActivePhaseIndex != State->EntryPhaseIndex)
					{
						State->bEntryOnSwitchedPhase = true;
					}

					if (Chaos && Chaos->GetVelocity().Size2D() >= MinEntrySpeedCmPerSec)
					{
						State->bChaosEntry = true;
					}
				}
			}

			FFollowSnapshot Snapshot;
			Snapshot.Type = FollowType;
			Snapshot.Id = FollowId;
			State->PrevFollow.Add(Logic, Snapshot);
		}

		if (NowSeconds - State->StartTimeSeconds >= SignalizedSimSeconds)
		{
			return true;
		}

		if (State->bTimingComplete && State->bSawPhaseSwitch && State->bEntryOnGreen && State->bChaosEntry)
		{
			return true;
		}

		return false;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FTrafficSignalizedEndPIECommand, TSharedRef<FTrafficSignalizedState>, State, FAutomationTestBase*, Test);
	bool FTrafficSignalizedEndPIECommand::Update()
	{
		if (State->bSavedCVars)
		{
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.ReservationEnabled")))
			{
				Var->Set(State->PrevReservationEnabled, ECVF_SetByCode);
			}
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.RequireFullStop")))
			{
				Var->Set(State->PrevRequireFullStop, ECVF_SetByCode);
			}
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.ControlMode")))
			{
				Var->Set(State->PrevControlMode, ECVF_SetByCode);
			}
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.GreenSeconds")))
			{
				Var->Set(State->PrevGreenSeconds, ECVF_SetByCode);
			}
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.YellowSeconds")))
			{
				Var->Set(State->PrevYellowSeconds, ECVF_SetByCode);
			}
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.AllRedSeconds")))
			{
				Var->Set(State->PrevAllRedSeconds, ECVF_SetByCode);
			}
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.CoordinationEnabled")))
			{
				Var->Set(State->PrevCoordEnabled, ECVF_SetByCode);
			}
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.CoordinationSpeedCmPerSec")))
			{
				Var->Set(State->PrevCoordSpeed, ECVF_SetByCode);
			}
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.CoordinationAxisMode")))
			{
				Var->Set(State->PrevCoordAxisMode, ECVF_SetByCode);
			}
			State->bSavedCVars = false;
		}

		if (GEditor)
		{
			GEditor->EndPlayMap();
		}

		if (State->IntersectionId == INDEX_NONE)
		{
			Test->AddError(TEXT("Signalized test did not identify a valid intersection."));
		}

		if (!State->bSawPhaseSwitch)
		{
			Test->AddError(TEXT("Signalized test did not observe a phase switch within the test window."));
		}

		if (!State->bEntryOnGreen)
		{
			Test->AddError(TEXT("No movement entered during a green phase."));
		}

		if (!State->bChaosEntry)
		{
			Test->AddError(TEXT("Chaos vehicles did not show motion when entering the signalized intersection."));
		}

		for (int32 PhaseIdx = 0; PhaseIdx < 3; ++PhaseIdx)
		{
			const TArray<float>& Samples = State->PhaseDurations[PhaseIdx];
			if (Samples.Num() == 0)
			{
				Test->AddError(TEXT("Signal timing test did not capture all phase durations."));
				break;
			}

			float Avg = 0.f;
			for (float V : Samples)
			{
				Avg += V;
			}
			Avg /= Samples.Num();

			const float Target = (PhaseIdx == 0) ? State->TargetGreenSeconds
				: (PhaseIdx == 1) ? State->TargetYellowSeconds
				: State->TargetAllRedSeconds;

			if (FMath::Abs(Avg - Target) > TimingToleranceSeconds)
			{
				Test->AddError(FString::Printf(TEXT("Signal phase duration out of tolerance (phase=%d avg=%.2fs target=%.2fs)."),
					PhaseIdx, Avg, Target));
			}
		}

		return true;
	}
}

#endif // WITH_EDITOR

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficSignalizedIntersectionTest,
	"Traffic.Intersections.Signalized.BaselineCurve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficSignalizedIntersectionTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	if (!AutomationOpenMap(SignalizedMapPackage))
	{
		AddError(TEXT("Failed to load /AAAtrafficSystem/Maps/Traffic_BaselineCurve."));
		return false;
	}

	TSharedRef<FTrafficSignalizedState> State = MakeShared<FTrafficSignalizedState>();
	State->bSavedCVars = true;
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
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.ControlMode")))
	{
		State->PrevControlMode = Var->GetInt();
		Var->Set(2, ECVF_SetByCode);
	}
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.GreenSeconds")))
	{
		State->PrevGreenSeconds = Var->GetFloat();
		Var->Set(State->TargetGreenSeconds, ECVF_SetByCode);
	}
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.YellowSeconds")))
	{
		State->PrevYellowSeconds = Var->GetFloat();
		Var->Set(State->TargetYellowSeconds, ECVF_SetByCode);
	}
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.AllRedSeconds")))
	{
		State->PrevAllRedSeconds = Var->GetFloat();
		Var->Set(State->TargetAllRedSeconds, ECVF_SetByCode);
	}
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.CoordinationEnabled")))
	{
		State->PrevCoordEnabled = Var->GetInt();
		Var->Set(0, ECVF_SetByCode);
	}
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.CoordinationSpeedCmPerSec")))
	{
		State->PrevCoordSpeed = Var->GetFloat();
	}
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.CoordinationAxisMode")))
	{
		State->PrevCoordAxisMode = Var->GetInt();
	}

	AddExpectedError(TEXT("The Editor is currently in a play mode."), EAutomationExpectedErrorFlags::Contains, 6);

	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficSignalizedWaitForPIEWorldCommand(State, this, 15.0, FPlatformTime::Seconds()));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficSignalizedInitCommand(State, this));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficSignalizedSampleCommand(State, this));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficSignalizedEndPIECommand(State, this));

	return true;
#else
	return false;
#endif
}
