#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TrafficSystemController.h"
#include "TrafficVehicleAdapter.h"
#include "TrafficVehicleBase.h"
#include "TrafficKinematicFollower.h"
#include "TrafficNetworkAsset.h"
#include "TrafficLaneGeometry.h"
#include "TrafficMovementGeometry.h"
#include "TrafficRoadTypes.h"
#include "TrafficRouting.h"
#include "TrafficRuntimeModule.h"
#include "HAL/PlatformTime.h"

#if WITH_EDITOR

namespace
{
	static const TCHAR* CoupledMapPackage = TEXT("/AAAtrafficSystem/Maps/Traffic_BaselineCurve");
	static const TCHAR* CoupledTestName = TEXT("Traffic.Calibration.LogicChaos.Coupled");
	static const float CoupledSimSeconds = 8.0f;
	static const float CoupledMaxPathErrorCm = 200.0f;
	static const float CoupledMaxSDeltaCm = 500.0f;
	static const float CoupledMaxLogicChaosSeparationCm = 300.0f;
	static const float CoupledMaxAdvanceDeltaCm = 500.0f;
	static const float CoupledMinSpeedRatio = 0.6f;
	static const float CoupledMinAdvanceCm = 200.0f;
	static const float CoupledMinSpeedCmPerSec = 50.0f;
	static const float CoupledMonotonicToleranceCm = 5.0f;

	struct FPerVehicleSnapshot
	{
		FVector LogicPos = FVector::ZeroVector;
		FVector ChaosPos = FVector::ZeroVector;
	};

	struct FTrafficLogicChaosCoupledState
	{
		bool bFailed = false;
		FString FailureMessage;
		TWeakObjectPtr<UWorld> PIEWorld;
		bool bSavedCVars = false;
		int32 PrevReservationEnabled = 1;
		int32 PrevRequireFullStop = 0;
		int32 PrevControlMode = 0;
		float PrevStopLineOffsetCm = 0.f;
		int32 PrevStopLineOffsetAuto = 0;

		int32 LogicCount = 0;
		int32 ChaosCount = 0;
		float MaxPathErrorCm = 0.f;
		float MaxAbsSDeltaCm = 0.f;
		float MaxLogicChaosSeparationCm = 0.f;
		float MaxLogicAdvanceCm = 0.f;
		float MaxChaosAdvanceCm = 0.f;
		float MaxAdvanceDeltaCm = 0.f;
		float MaxLogicSpeedCmPerSec = 0.f;
		float MaxChaosSpeedCmPerSec = 0.f;
		bool bChaosMonotonic = true;
		bool bLogicMonotonic = true;
		bool bChaosAdvanced = false;
		bool bLogicAdvanced = false;

		TMap<ATrafficVehicleAdapter*, FPerVehicleSnapshot> StartSnapshots;
	};

	static bool ProjectChaosOntoFollowTarget(
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

	DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(FTrafficCoupledWaitForPIEWorldCommand, TSharedRef<FTrafficLogicChaosCoupledState>, State, FAutomationTestBase*, Test, double, TimeoutSeconds, double, StartTime);
	bool FTrafficCoupledWaitForPIEWorldCommand::Update()
	{
		if (GEditor && GEditor->PlayWorld)
		{
			State->PIEWorld = GEditor->PlayWorld;
			return true;
		}

		const double Elapsed = FPlatformTime::Seconds() - StartTime;
		if (Elapsed > TimeoutSeconds)
		{
			State->bFailed = true;
			if (Test)
			{
				Test->AddError(TEXT("PIE world did not start for logic/chaos coupled test."));
			}
			return true;
		}

		return false;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FTrafficCoupledCaptureStartCommand, TSharedRef<FTrafficLogicChaosCoupledState>, State, FAutomationTestBase*, Test);
	bool FTrafficCoupledCaptureStartCommand::Update()
	{
		if (State->bFailed)
		{
			return false;
		}

		if (!State->PIEWorld.IsValid() && GEditor && GEditor->PlayWorld)
		{
			State->PIEWorld = GEditor->PlayWorld;
		}

		if (!State->PIEWorld.IsValid())
		{
			return false;
		}

		UWorld* World = State->PIEWorld.Get();
		if (!World)
		{
			return false;
		}

		ATrafficSystemController* Controller = nullptr;
		for (TActorIterator<ATrafficSystemController> It(World); It; ++It)
		{
			Controller = *It;
			break;
		}
		if (!Controller)
		{
			return false;
		}

		State->StartSnapshots.Reset();
		int32 CurrentLogicCount = 0;
		int32 CurrentChaosCount = 0;

		for (TActorIterator<ATrafficVehicleAdapter> It(World); It; ++It)
		{
			ATrafficVehicleAdapter* Adapter = *It;
			if (!Adapter || !Adapter->LogicVehicle.IsValid() || !Adapter->ChaosVehicle.IsValid())
			{
				continue;
			}

			ATrafficVehicleBase* Logic = Adapter->LogicVehicle.Get();
			APawn* Chaos = Adapter->ChaosVehicle.Get();
			if (!Logic || !Chaos)
			{
				continue;
			}

			++CurrentLogicCount;
			++CurrentChaosCount;

			FPerVehicleSnapshot Snapshot;
			Snapshot.LogicPos = Logic->GetActorLocation();
			Snapshot.ChaosPos = Chaos->GetActorLocation();
			State->StartSnapshots.Add(Adapter, Snapshot);
		}

		if (CurrentLogicCount == 0 || CurrentChaosCount == 0)
		{
			State->StartSnapshots.Reset();
			return false;
		}

		State->LogicCount = CurrentLogicCount;
		State->ChaosCount = CurrentChaosCount;
		return true;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_THREE_PARAMETER(
		FTrafficCoupledSampleMotionCommand,
		TSharedRef<FTrafficLogicChaosCoupledState>, State,
		double, DurationSeconds,
		double, StartTime);
	bool FTrafficCoupledSampleMotionCommand::Update()
	{
		if (!State->PIEWorld.IsValid() && GEditor && GEditor->PlayWorld)
		{
			State->PIEWorld = GEditor->PlayWorld;
		}

		if (!State->PIEWorld.IsValid())
		{
			return false;
		}

		UWorld* World = State->PIEWorld.Get();
		if (!World)
		{
			return false;
		}

		const double Now = static_cast<double>(World->GetTimeSeconds());

		auto RebuildSnapshots = [&]() -> bool
		{
			State->StartSnapshots.Reset();
			int32 CurrentLogicCount = 0;
			int32 CurrentChaosCount = 0;

			for (TActorIterator<ATrafficVehicleAdapter> It(World); It; ++It)
			{
				ATrafficVehicleAdapter* Adapter = *It;
				if (!Adapter || !Adapter->LogicVehicle.IsValid() || !Adapter->ChaosVehicle.IsValid())
				{
					continue;
				}

				ATrafficVehicleBase* Logic = Adapter->LogicVehicle.Get();
				APawn* Chaos = Adapter->ChaosVehicle.Get();
				if (!Logic || !Chaos)
				{
					continue;
				}

				++CurrentLogicCount;
				++CurrentChaosCount;

				FPerVehicleSnapshot Snapshot;
				Snapshot.LogicPos = Logic->GetActorLocation();
				Snapshot.ChaosPos = Chaos->GetActorLocation();
				State->StartSnapshots.Add(Adapter, Snapshot);
			}

			if (CurrentLogicCount == 0 || CurrentChaosCount == 0)
			{
				State->StartSnapshots.Reset();
				return false;
			}

			State->LogicCount = FMath::Max(State->LogicCount, CurrentLogicCount);
			State->ChaosCount = FMath::Max(State->ChaosCount, CurrentChaosCount);
			return true;
		};

		if (State->StartSnapshots.Num() == 0)
		{
			if (!RebuildSnapshots())
			{
				return false;
			}
			StartTime = Now;
		}
		else
		{
			bool bAnyValid = false;
			for (const TPair<ATrafficVehicleAdapter*, FPerVehicleSnapshot>& Pair : State->StartSnapshots)
			{
				if (Pair.Key && Pair.Key->LogicVehicle.IsValid() && Pair.Key->ChaosVehicle.IsValid())
				{
					bAnyValid = true;
					break;
				}
			}
			if (!bAnyValid)
			{
				if (!RebuildSnapshots())
				{
					return false;
				}
				StartTime = Now;
			}
		}
		if (StartTime <= 0.0 || Now < StartTime)
		{
			StartTime = Now;
		}

		const double Elapsed = Now - StartTime;
		if (Elapsed >= DurationSeconds)
		{
			return true;
		}

		for (const TPair<ATrafficVehicleAdapter*, FPerVehicleSnapshot>& Pair : State->StartSnapshots)
		{
			ATrafficVehicleAdapter* Adapter = Pair.Key;
			const FPerVehicleSnapshot& Start = Pair.Value;
			if (!Adapter || !Adapter->LogicVehicle.IsValid() || !Adapter->ChaosVehicle.IsValid())
			{
				continue;
			}

			ATrafficVehicleBase* Logic = Adapter->LogicVehicle.Get();
			APawn* Chaos = Adapter->ChaosVehicle.Get();
			if (!Logic || !Chaos)
			{
				continue;
			}

			const float LogicAdvance = FVector::Dist2D(Start.LogicPos, Logic->GetActorLocation());
			const float ChaosAdvance = FVector::Dist2D(Start.ChaosPos, Chaos->GetActorLocation());
			const float AdvanceDelta = FMath::Abs(LogicAdvance - ChaosAdvance);
			const float LogicChaosSeparation = FVector::Dist2D(Logic->GetActorLocation(), Chaos->GetActorLocation());
			const float LogicSpeed = Logic->GetPlannedSpeedCmPerSec();
			const float ChaosSpeed = Chaos->GetVelocity().Size2D();

			State->MaxLogicAdvanceCm = FMath::Max(State->MaxLogicAdvanceCm, LogicAdvance);
			State->MaxChaosAdvanceCm = FMath::Max(State->MaxChaosAdvanceCm, ChaosAdvance);
			State->MaxAdvanceDeltaCm = FMath::Max(State->MaxAdvanceDeltaCm, AdvanceDelta);
			State->MaxLogicSpeedCmPerSec = FMath::Max(State->MaxLogicSpeedCmPerSec, LogicSpeed);
			State->MaxChaosSpeedCmPerSec = FMath::Max(State->MaxChaosSpeedCmPerSec, ChaosSpeed);

			if (LogicAdvance >= CoupledMinAdvanceCm || LogicSpeed >= CoupledMinSpeedCmPerSec)
			{
				State->bLogicAdvanced = true;
			}
			if (ChaosAdvance >= CoupledMinAdvanceCm || ChaosSpeed >= CoupledMinSpeedCmPerSec)
			{
				State->bChaosAdvanced = true;
			}
		}

		return false;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FTrafficCoupledCaptureEndCommand, TSharedRef<FTrafficLogicChaosCoupledState>, State, FAutomationTestBase*, Test);
	bool FTrafficCoupledCaptureEndCommand::Update()
	{
		if (State->bFailed)
		{
			return true;
		}

		if (!State->PIEWorld.IsValid() && GEditor && GEditor->PlayWorld)
		{
			State->PIEWorld = GEditor->PlayWorld;
		}

		if (!State->PIEWorld.IsValid())
		{
			return true;
		}

		UWorld* World = State->PIEWorld.Get();
		if (!World)
		{
			return true;
		}

		ATrafficSystemController* Controller = nullptr;
		for (TActorIterator<ATrafficSystemController> It(World); It; ++It)
		{
			Controller = *It;
			break;
		}
		if (!Controller)
		{
			return true;
		}

		UTrafficNetworkAsset* NetAsset = Controller->GetBuiltNetworkAsset();
		if (!NetAsset || NetAsset->Network.Lanes.Num() == 0)
		{
			return true;
		}

		const FTrafficNetwork& Net = NetAsset->Network;

		int32 CurrentLogicCount = 0;
		int32 CurrentChaosCount = 0;

		for (const TPair<ATrafficVehicleAdapter*, FPerVehicleSnapshot>& Pair : State->StartSnapshots)
		{
			ATrafficVehicleAdapter* Adapter = Pair.Key;
			const FPerVehicleSnapshot& Start = Pair.Value;
			if (!Adapter || !Adapter->LogicVehicle.IsValid() || !Adapter->ChaosVehicle.IsValid())
			{
				continue;
			}

			ATrafficVehicleBase* Logic = Adapter->LogicVehicle.Get();
			APawn* Chaos = Adapter->ChaosVehicle.Get();
			if (!Logic || !Chaos)
			{
				continue;
			}

			++CurrentLogicCount;
			++CurrentChaosCount;

			const float LogicAdvance = FVector::Dist2D(Start.LogicPos, Logic->GetActorLocation());
			const float ChaosAdvance = FVector::Dist2D(Start.ChaosPos, Chaos->GetActorLocation());
			const float LogicChaosSeparation = FVector::Dist2D(Logic->GetActorLocation(), Chaos->GetActorLocation());
			const float LogicSpeed = Logic->GetPlannedSpeedCmPerSec();
			const float ChaosSpeed = Chaos->GetVelocity().Size2D();

			if (LogicAdvance >= CoupledMinAdvanceCm || LogicSpeed >= CoupledMinSpeedCmPerSec)
			{
				State->bLogicAdvanced = true;
			}
			if (ChaosAdvance >= CoupledMinAdvanceCm || ChaosSpeed >= CoupledMinSpeedCmPerSec)
			{
				State->bChaosAdvanced = true;
			}

			EPathFollowTargetType FollowType = EPathFollowTargetType::None;
			int32 FollowId = INDEX_NONE;
			float LogicS = 0.f;
			if (!Logic->GetFollowTarget(FollowType, FollowId, LogicS))
			{
				continue;
			}

			float ChaosS = 0.f;
			float PathErrorCm = 0.f;
			if (!ProjectChaosOntoFollowTarget(Net, *Chaos, FollowType, FollowId, ChaosS, PathErrorCm))
			{
				State->bFailed = true;
				State->FailureMessage = TEXT("ProjectionFailed");
				if (Test)
				{
					Test->AddError(TEXT("Failed to project Chaos pawn onto its follow target (lane/movement)."));
				}
				return true;
			}

			State->MaxPathErrorCm = FMath::Max(State->MaxPathErrorCm, PathErrorCm);
			State->MaxLogicChaosSeparationCm = FMath::Max(State->MaxLogicChaosSeparationCm, LogicChaosSeparation);
			if (FollowType == EPathFollowTargetType::Lane)
			{
				float SDeltaCm = FMath::Abs(ChaosS - LogicS);
				if (const FTrafficLane* Lane = TrafficRouting::FindLaneById(Net, FollowId))
				{
					const float LaneLengthCm = TrafficLaneGeometry::ComputeLaneLengthCm(*Lane);
					if (LaneLengthCm > KINDA_SMALL_NUMBER)
					{
						SDeltaCm = FMath::Min(SDeltaCm, FMath::Abs(LaneLengthCm - SDeltaCm));
					}
				}
				State->MaxAbsSDeltaCm = FMath::Max(State->MaxAbsSDeltaCm, SDeltaCm);
			}
		}

		State->bLogicAdvanced = State->bLogicAdvanced ||
			(State->MaxLogicAdvanceCm >= CoupledMinAdvanceCm) ||
			(State->MaxLogicSpeedCmPerSec >= CoupledMinSpeedCmPerSec);
		State->bChaosAdvanced = State->bChaosAdvanced ||
			(State->MaxChaosAdvanceCm >= CoupledMinAdvanceCm) ||
			(State->MaxChaosSpeedCmPerSec >= CoupledMinSpeedCmPerSec);

		State->LogicCount = FMath::Max(State->LogicCount, CurrentLogicCount);
		State->ChaosCount = FMath::Max(State->ChaosCount, CurrentChaosCount);
		return true;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FTrafficCoupledEndPIECommand, TSharedRef<FTrafficLogicChaosCoupledState>, State, FAutomationTestBase*, Test);
	bool FTrafficCoupledEndPIECommand::Update()
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
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.StopLineOffsetCm")))
			{
				Var->Set(State->PrevStopLineOffsetCm, ECVF_SetByCode);
			}
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.StopLineOffsetAuto")))
			{
				Var->Set(State->PrevStopLineOffsetAuto, ECVF_SetByCode);
			}
			State->bSavedCVars = false;
		}

		if (GEditor)
		{
			GEditor->EndPlayMap();
		}

		const bool bHasVehicle = (State->LogicCount > 0);
		const bool bHasChaos = (State->ChaosCount > 0) && (State->ChaosCount == State->LogicCount);
		const bool bPathOk = (State->MaxPathErrorCm <= CoupledMaxPathErrorCm);
		const float SpeedRatio = (State->MaxLogicSpeedCmPerSec > 1.0f)
			? (State->MaxChaosSpeedCmPerSec / State->MaxLogicSpeedCmPerSec)
			: 0.0f;
		const bool bSAligned = (SpeedRatio >= CoupledMinSpeedRatio);
		const bool bMonotonic = State->bChaosMonotonic && State->bLogicMonotonic;
		const bool bMoved = State->bChaosAdvanced && State->bLogicAdvanced;

		UE_LOG(LogTraffic, Display, TEXT("[TrafficLogicChaosCoupled] VehiclesSpawned=%d"), State->LogicCount);
		UE_LOG(LogTraffic, Display, TEXT("[TrafficLogicChaosCoupled] ChaosVehicles=%d"), State->ChaosCount);
		UE_LOG(LogTraffic, Display, TEXT("[TrafficLogicChaosCoupled] MaxPathErrorCm=%.2f"), State->MaxPathErrorCm);
		UE_LOG(LogTraffic, Display, TEXT("[TrafficLogicChaosCoupled] MaxLogicChaosSeparationCm=%.2f"), State->MaxLogicChaosSeparationCm);
		UE_LOG(LogTraffic, Display, TEXT("[TrafficLogicChaosCoupled] MaxAbsSDeltaCm=%.2f"), State->MaxAbsSDeltaCm);
		UE_LOG(LogTraffic, Display, TEXT("[TrafficLogicChaosCoupled] MaxAdvanceDeltaCm=%.2f"), State->MaxAdvanceDeltaCm);
		UE_LOG(LogTraffic, Display, TEXT("[TrafficLogicChaosCoupled] MaxLogicAdvanceCm=%.1f MaxChaosAdvanceCm=%.1f"), State->MaxLogicAdvanceCm, State->MaxChaosAdvanceCm);
		UE_LOG(LogTraffic, Display, TEXT("[TrafficLogicChaosCoupled] MaxLogicSpeedCmPerSec=%.1f MaxChaosSpeedCmPerSec=%.1f"), State->MaxLogicSpeedCmPerSec, State->MaxChaosSpeedCmPerSec);
		UE_LOG(LogTraffic, Display, TEXT("[TrafficLogicChaosCoupled] SpeedRatio=%.2f"), SpeedRatio);
		UE_LOG(LogTraffic, Display, TEXT("[TrafficLogicChaosCoupled] LogicAdvanced=%d ChaosAdvanced=%d"), State->bLogicAdvanced ? 1 : 0, State->bChaosAdvanced ? 1 : 0);

		if (!bHasVehicle && Test)
		{
			Test->AddError(TEXT("Expected at least one TrafficVehicleAdapter in coupled test."));
		}
		if (!bHasChaos && Test)
		{
			Test->AddError(TEXT("Expected Chaos visual pawns for all coupled test vehicles."));
		}
		if (!bPathOk && Test)
		{
			Test->AddError(TEXT("Chaos pawn deviated too far from its follow path (logic/chaos coupling)."));
		}
		if (!bSAligned && Test)
		{
			Test->AddError(TEXT("Chaos speed fell below minimum ratio of logic speed."));
		}
		if (!bMonotonic && Test)
		{
			Test->AddError(TEXT("Logic or Chaos distance along target decreased unexpectedly."));
		}
		if (!bMoved && Test)
		{
			Test->AddError(TEXT("Logic/Chaos did not advance along the target during the coupled test window."));
		}

		const bool bPass =
			!State->bFailed &&
			bHasVehicle &&
			bHasChaos &&
			bPathOk &&
			bSAligned &&
			bMonotonic &&
			bMoved;

		UE_LOG(LogTraffic, Display, TEXT("[TrafficLogicChaosCoupled] Result=%s"), bPass ? TEXT("PASS") : TEXT("FAIL"));
		return true;
	}
}

#endif // WITH_EDITOR

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficLogicChaosCoupledTest,
	"Traffic.Calibration.LogicChaos.Coupled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficLogicChaosCoupledTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	if (!AutomationOpenMap(CoupledMapPackage))
	{
		AddError(TEXT("Failed to load /AAAtrafficSystem/Maps/Traffic_BaselineCurve."));
		return false;
	}

	TSharedRef<FTrafficLogicChaosCoupledState> State = MakeShared<FTrafficLogicChaosCoupledState>();
	State->bSavedCVars = true;
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.ReservationEnabled")))
	{
		State->PrevReservationEnabled = Var->GetInt();
		Var->Set(0, ECVF_SetByCode);
	}
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.RequireFullStop")))
	{
		State->PrevRequireFullStop = Var->GetInt();
		Var->Set(0, ECVF_SetByCode);
	}
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.ControlMode")))
	{
		State->PrevControlMode = Var->GetInt();
		Var->Set(0, ECVF_SetByCode);
	}
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.StopLineOffsetCm")))
	{
		State->PrevStopLineOffsetCm = Var->GetFloat();
		Var->Set(0.f, ECVF_SetByCode);
	}
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.StopLineOffsetAuto")))
	{
		State->PrevStopLineOffsetAuto = Var->GetInt();
		Var->Set(0, ECVF_SetByCode);
	}
	AddExpectedError(TEXT("The Editor is currently in a play mode."), EAutomationExpectedErrorFlags::Contains, 6);

	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficCoupledWaitForPIEWorldCommand(State, this, 15.0, FPlatformTime::Seconds()));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficCoupledCaptureStartCommand(State, this));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficCoupledSampleMotionCommand(State, CoupledSimSeconds, 0.0));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficCoupledCaptureEndCommand(State, this));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficCoupledEndPIECommand(State, this));

	return true;
#else
	return false;
#endif
}
