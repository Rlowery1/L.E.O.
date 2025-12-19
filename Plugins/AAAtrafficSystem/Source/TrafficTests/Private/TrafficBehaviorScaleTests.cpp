#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TrafficSystemController.h"
#include "TrafficVehicleAdapter.h"
#include "TrafficVehicleBase.h"
#include "TrafficRouting.h"
#include "TrafficLaneGeometry.h"
#include "TrafficNetworkAsset.h"
#include "TrafficRuntimeSettings.h"
#include "TrafficKinematicFollower.h"
#include "HAL/PlatformTime.h"

#if WITH_EDITOR

namespace
{
	static const TCHAR* BehaviorMapPackage = TEXT("/AAAtrafficSystem/Maps/Traffic_BaselineCurve");
	static const float FollowingSimSeconds = 8.0f;
	static const float FollowingWarmupSeconds = 2.0f;
	static const float FollowingEndpointBufferCm = 500.0f;
	static const float ScaleSimSeconds = 6.0f;
	static const float MinMovingSpeedCmPerSec = 50.0f;

	struct FTrafficVehicleSample
	{
		float S = 0.f;
		float Speed = 0.f;
		float Length = 0.f;
	};

	struct FTrafficSettingsSnapshot
	{
		int32 VehiclesPerLaneRuntime = 1;
		float RuntimeSpeedCmPerSec = 800.f;
		bool bGenerateZoneGraph = false;
	};

	static void SaveRuntimeSettings(FTrafficSettingsSnapshot& OutSnapshot)
	{
		if (UTrafficRuntimeSettings* Settings = GetMutableDefault<UTrafficRuntimeSettings>())
		{
			OutSnapshot.VehiclesPerLaneRuntime = Settings->VehiclesPerLaneRuntime;
			OutSnapshot.RuntimeSpeedCmPerSec = Settings->RuntimeSpeedCmPerSec;
			OutSnapshot.bGenerateZoneGraph = Settings->bGenerateZoneGraph;
		}
	}

	static void RestoreRuntimeSettings(const FTrafficSettingsSnapshot& Snapshot)
	{
		if (UTrafficRuntimeSettings* Settings = GetMutableDefault<UTrafficRuntimeSettings>())
		{
			Settings->VehiclesPerLaneRuntime = Snapshot.VehiclesPerLaneRuntime;
			Settings->RuntimeSpeedCmPerSec = Snapshot.RuntimeSpeedCmPerSec;
			Settings->bGenerateZoneGraph = Snapshot.bGenerateZoneGraph;
		}
	}

	struct FTrafficFollowingState
	{
		TWeakObjectPtr<UWorld> PIEWorld;
		bool bSavedSettings = false;
		FTrafficSettingsSnapshot PrevSettings;
		int32 PrevVisualMode = 2;
		int32 PrevFollowingEnabled = 1;

		double StartTimeSeconds = 0.0;
		float MinGapCm = TNumericLimits<float>::Max();
		bool bObservedPair = false;
		int32 LaneCount = 0;
		int32 VehicleCount = 0;
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
		if (!NetAsset || NetAsset->Network.Lanes.Num() == 0)
		{
			return false;
		}

		OutNet = &NetAsset->Network;
		return true;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(FTrafficWaitForPIEWorldCommand, TSharedRef<FTrafficFollowingState>, State, FAutomationTestBase*, Test, double, TimeoutSeconds, double, StartTime);
	bool FTrafficWaitForPIEWorldCommand::Update()
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
				Test->AddError(TEXT("PIE world did not start for traffic behavior tests."));
			}
			return true;
		}

		return false;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FTrafficFollowingSampleCommand, TSharedRef<FTrafficFollowingState>, State, FAutomationTestBase*, Test);
	bool FTrafficFollowingSampleCommand::Update()
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

		if (State->StartTimeSeconds <= 0.0)
		{
			State->StartTimeSeconds = FPlatformTime::Seconds();
		}

		TMap<int32, TArray<FTrafficVehicleSample>> LaneVehicles; // LaneId -> [(S, Speed, Length)]
		int32 VehicleCount = 0;

		for (TActorIterator<ATrafficVehicleAdapter> It(World); It; ++It)
		{
			ATrafficVehicleAdapter* Adapter = *It;
			if (!Adapter || !Adapter->LogicVehicle.IsValid())
			{
				continue;
			}

			ATrafficVehicleBase* Logic = Adapter->LogicVehicle.Get();
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

			if (FollowType != EPathFollowTargetType::Lane)
			{
				continue;
			}

			const float Speed = Logic->GetPlannedSpeedCmPerSec();
			const float Length = Logic->GetApproxVehicleLengthCm();
			FTrafficVehicleSample Sample;
			Sample.S = FollowS;
			Sample.Speed = Speed;
			Sample.Length = Length;
			LaneVehicles.FindOrAdd(FollowId).Add(Sample);
			++VehicleCount;
		}

		State->VehicleCount = FMath::Max(State->VehicleCount, VehicleCount);
		State->LaneCount = Net ? Net->Lanes.Num() : 0;

		float LocalMinGap = State->MinGapCm;
		const double Now = FPlatformTime::Seconds();
		const bool bWarmupComplete = (Now - State->StartTimeSeconds) >= FollowingWarmupSeconds;

		for (TPair<int32, TArray<FTrafficVehicleSample>>& Pair : LaneVehicles)
		{
			TArray<FTrafficVehicleSample>& Vehicles = Pair.Value;
			if (Vehicles.Num() < 2)
			{
				continue;
			}

			const FTrafficLane* Lane = Net ? TrafficRouting::FindLaneById(*Net, Pair.Key) : nullptr;
			const float LaneLengthCm = Lane ? TrafficLaneGeometry::ComputeLaneLengthCm(*Lane) : 0.f;
			const bool bCheckEndpoints = LaneLengthCm > (FollowingEndpointBufferCm * 2.0f);

			Vehicles.Sort([](const FTrafficVehicleSample& A, const FTrafficVehicleSample& B)
			{
				return A.S < B.S;
			});

			for (int32 i = 1; i < Vehicles.Num(); ++i)
			{
				const float LeadS = Vehicles[i].S;
				const float FollowS = Vehicles[i - 1].S;
				const float FollowSpeed = Vehicles[i - 1].Speed;
				const float LeadLength = Vehicles[i].Length;

				if (FollowSpeed < MinMovingSpeedCmPerSec)
				{
					continue;
				}

				if (!bWarmupComplete)
				{
					continue;
				}

				if (bCheckEndpoints)
				{
					if (FollowS < FollowingEndpointBufferCm || LeadS < FollowingEndpointBufferCm)
					{
						continue;
					}
					if ((LaneLengthCm - FollowS) < FollowingEndpointBufferCm || (LaneLengthCm - LeadS) < FollowingEndpointBufferCm)
					{
						continue;
					}
				}

				const float AvailableGap = (LeadS - FollowS) - LeadLength;
				LocalMinGap = FMath::Min(LocalMinGap, AvailableGap);
				State->bObservedPair = true;
			}
		}

		State->MinGapCm = LocalMinGap;

		return (Now - State->StartTimeSeconds) >= FollowingSimSeconds;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FTrafficFollowingEndCommand, TSharedRef<FTrafficFollowingState>, State, FAutomationTestBase*, Test);
	bool FTrafficFollowingEndCommand::Update()
	{
		if (State->bSavedSettings)
		{
			RestoreRuntimeSettings(State->PrevSettings);

			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Visual.Mode")))
			{
				Var->Set(State->PrevVisualMode, ECVF_SetByCode);
			}
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Following.Enabled")))
			{
				Var->Set(State->PrevFollowingEnabled, ECVF_SetByCode);
			}

			State->bSavedSettings = false;
		}

		if (GEditor)
		{
			GEditor->EndPlayMap();
		}

		if (!State->bObservedPair)
		{
			Test->AddError(TEXT("Following test did not observe any lane with multiple moving vehicles."));
		}

		if (State->MinGapCm < 0.f)
		{
			Test->AddError(FString::Printf(TEXT("Following gap overlap detected (min gap %.1fcm)."), State->MinGapCm));
		}

		const float MinGapThresholdCm = 200.0f;
		if (State->MinGapCm < MinGapThresholdCm)
		{
			Test->AddError(FString::Printf(TEXT("Following gap below threshold (min gap %.1fcm)."), State->MinGapCm));
		}

		return true;
	}

	struct FTrafficScaleState
	{
		TWeakObjectPtr<UWorld> PIEWorld;
		bool bSavedSettings = false;
		FTrafficSettingsSnapshot PrevSettings;
		int32 PrevVisualMode = 2;
		int32 ExpectedVehiclesPerLane = 2;
		int32 LaneCount = 0;
		int32 VehicleCount = 0;
		double StartTimeSeconds = 0.0;
	};

	DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(FTrafficScaleWaitForPIEWorldCommand, TSharedRef<FTrafficScaleState>, State, FAutomationTestBase*, Test, double, TimeoutSeconds, double, StartTime);
	bool FTrafficScaleWaitForPIEWorldCommand::Update()
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
				Test->AddError(TEXT("PIE world did not start for traffic scale test."));
			}
			return true;
		}

		return false;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FTrafficScaleSampleCommand, TSharedRef<FTrafficScaleState>, State, FAutomationTestBase*, Test);
	bool FTrafficScaleSampleCommand::Update()
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

		if (State->StartTimeSeconds <= 0.0)
		{
			State->StartTimeSeconds = FPlatformTime::Seconds();
		}

		int32 VehicleCount = 0;
		for (TActorIterator<ATrafficVehicleAdapter> It(World); It; ++It)
		{
			ATrafficVehicleAdapter* Adapter = *It;
			if (Adapter && Adapter->LogicVehicle.IsValid())
			{
				++VehicleCount;
			}
		}

		State->LaneCount = Net ? Net->Lanes.Num() : 0;
		State->VehicleCount = VehicleCount;

		const double Now = FPlatformTime::Seconds();
		return (Now - State->StartTimeSeconds) >= ScaleSimSeconds;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FTrafficScaleEndCommand, TSharedRef<FTrafficScaleState>, State, FAutomationTestBase*, Test);
	bool FTrafficScaleEndCommand::Update()
	{
		if (State->bSavedSettings)
		{
			RestoreRuntimeSettings(State->PrevSettings);

			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Visual.Mode")))
			{
				Var->Set(State->PrevVisualMode, ECVF_SetByCode);
			}

			State->bSavedSettings = false;
		}

		if (GEditor)
		{
			GEditor->EndPlayMap();
		}

		if (State->LaneCount <= 0)
		{
			Test->AddError(TEXT("Scale test did not detect any lanes."));
			return true;
		}

		const int32 ExpectedMinVehicles = FMath::Max(1, State->LaneCount * State->ExpectedVehiclesPerLane);
		const float CoverageRatio = (ExpectedMinVehicles > 0) ? (static_cast<float>(State->VehicleCount) / ExpectedMinVehicles) : 0.f;

		if (CoverageRatio < 0.8f)
		{
			Test->AddError(FString::Printf(TEXT("Scale test vehicle count too low (%d / expected %d)."),
				State->VehicleCount, ExpectedMinVehicles));
		}

		return true;
	}
}

#endif // WITH_EDITOR

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficFollowingBaselineTest,
	"Traffic.Behavior.Following.BaselineCurve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficFollowingBaselineTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	if (!AutomationOpenMap(BehaviorMapPackage))
	{
		AddError(TEXT("Failed to load /AAAtrafficSystem/Maps/Traffic_BaselineCurve."));
		return false;
	}

	TSharedRef<FTrafficFollowingState> State = MakeShared<FTrafficFollowingState>();
	State->bSavedSettings = true;
	SaveRuntimeSettings(State->PrevSettings);

	if (UTrafficRuntimeSettings* Settings = GetMutableDefault<UTrafficRuntimeSettings>())
	{
		Settings->VehiclesPerLaneRuntime = 2;
		Settings->RuntimeSpeedCmPerSec = 800.f;
		Settings->bGenerateZoneGraph = false;
	}

	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Visual.Mode")))
	{
		State->PrevVisualMode = Var->GetInt();
		Var->Set(2, ECVF_SetByCode);
	}
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Following.Enabled")))
	{
		State->PrevFollowingEnabled = Var->GetInt();
		Var->Set(1, ECVF_SetByCode);
	}

	AddExpectedError(TEXT("The Editor is currently in a play mode."), EAutomationExpectedErrorFlags::Contains, 6);

	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficWaitForPIEWorldCommand(State, this, 15.0, FPlatformTime::Seconds()));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficFollowingSampleCommand(State, this));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficFollowingEndCommand(State, this));

	return true;
#else
	return false;
#endif
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficScaleBaselineTest,
	"Traffic.Scale.Spawn.BaselineCurve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficScaleBaselineTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	if (!AutomationOpenMap(BehaviorMapPackage))
	{
		AddError(TEXT("Failed to load /AAAtrafficSystem/Maps/Traffic_BaselineCurve."));
		return false;
	}

	TSharedRef<FTrafficScaleState> State = MakeShared<FTrafficScaleState>();
	State->bSavedSettings = true;
	SaveRuntimeSettings(State->PrevSettings);

	if (UTrafficRuntimeSettings* Settings = GetMutableDefault<UTrafficRuntimeSettings>())
	{
		Settings->VehiclesPerLaneRuntime = State->ExpectedVehiclesPerLane;
		Settings->RuntimeSpeedCmPerSec = 800.f;
		Settings->bGenerateZoneGraph = false;
	}

	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Visual.Mode")))
	{
		State->PrevVisualMode = Var->GetInt();
		Var->Set(2, ECVF_SetByCode);
	}

	AddExpectedError(TEXT("The Editor is currently in a play mode."), EAutomationExpectedErrorFlags::Contains, 6);

	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficScaleWaitForPIEWorldCommand(State, this, 15.0, FPlatformTime::Seconds()));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficScaleSampleCommand(State, this));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficScaleEndCommand(State, this));

	return true;
#else
	return false;
#endif
}
