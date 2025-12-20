#include "CoreMinimal.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TrafficVehicleAdapter.h"
#include "TrafficVehicleBase.h"

#if WITH_EDITOR

namespace
{
	static const TCHAR* BaselineCurveMapPackage_NoFlyaway = TEXT("/AAAtrafficSystem/Maps/Traffic_BaselineCurve");

	struct FNoFlyawayPIEState
	{
		bool bFailed = false;
		FString Failure;
		UWorld* PIEWorld = nullptr;
		float LastSampleWorldTime = -1000.f;
		double LastSampleRealTime = -1000.0;
		float EndWorldTime = -1.f;
		double EndRealTime = -1.0;
		int32 Samples = 0;
		float StartWorldTime = -1.f;
		double StartRealTime = -1.0;
		int32 ConsecutiveBadSamples = 0;
	};

	DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(
		FWaitForPIEWorld_NoFlyaway,
		TSharedRef<FNoFlyawayPIEState>,
		State,
		FAutomationTestBase*,
		Test,
		double,
		TimeoutSeconds,
		double,
		StartTime);

	bool FWaitForPIEWorld_NoFlyaway::Update()
	{
		if (State->PIEWorld)
		{
			return true;
		}

		if (GEditor && GEditor->PlayWorld)
		{
			State->PIEWorld = GEditor->PlayWorld;
			return true;
		}

		const double Elapsed = FPlatformTime::Seconds() - StartTime;
		if (Elapsed > TimeoutSeconds)
		{
			State->bFailed = true;
			State->Failure = TEXT("PIE world did not start.");
			if (Test)
			{
				Test->AddError(State->Failure);
			}
			return true;
		}

		return false;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_THREE_PARAMETER(
		FMonitorNoFlyaways,
		TSharedRef<FNoFlyawayPIEState>,
		State,
		FAutomationTestBase*,
		Test,
		float,
		DurationSeconds);

	bool FMonitorNoFlyaways::Update()
	{
		if (State->bFailed)
		{
			return true;
		}

		UWorld* World = State->PIEWorld;
		if (!World)
		{
			State->bFailed = true;
			State->Failure = TEXT("PIE world missing.");
			if (Test)
			{
				Test->AddError(State->Failure);
			}
			return true;
		}

		const float Now = World->GetTimeSeconds();
		const double NowReal = FPlatformTime::Seconds();
		if (State->StartWorldTime < 0.f)
		{
			State->StartWorldTime = Now;
		}
		if (State->StartRealTime < 0.0)
		{
			State->StartRealTime = NowReal;
		}
		if (State->EndWorldTime < 0.f)
		{
			State->EndWorldTime = Now + FMath::Max(0.f, DurationSeconds);
		}
		if (State->EndRealTime < 0.0)
		{
			State->EndRealTime = NowReal + FMath::Max(0.f, DurationSeconds);
		}

		static const float SampleInterval = 0.2f;
		if ((NowReal - State->LastSampleRealTime) < SampleInterval)
		{
			return false;
		}
		State->LastSampleWorldTime = Now;
		State->LastSampleRealTime = NowReal;
		State->Samples++;

		int32 Checked = 0;
		for (TActorIterator<ATrafficVehicleAdapter> It(World); It; ++It)
		{
			ATrafficVehicleAdapter* Adapter = *It;
			if (!Adapter || !Adapter->LogicVehicle.IsValid() || !Adapter->ChaosVehicle.IsValid())
			{
				continue;
			}

			const FVector LogicPos = Adapter->LogicVehicle.Get()->GetActorLocation();
			const FVector ChaosPos = Adapter->ChaosVehicle->GetActorLocation();
			const float Dz = ChaosPos.Z - LogicPos.Z;
			Checked++;

			// Allow a brief initial window where visual + logic actors may not yet be aligned (streaming / first tick ordering).
			const bool bPastWarmup = (NowReal - State->StartRealTime) >= 2.0;

			// This catches the exact "snapped into the air / violently flying" failure: Chaos pawn Z diverges from
			// the lane-following logic vehicle by a large margin for multiple consecutive samples.
			if (bPastWarmup && FMath::Abs(Dz) > 300.f)
			{
				State->ConsecutiveBadSamples++;
				if (State->ConsecutiveBadSamples >= 5)
				{
					State->bFailed = true;
					State->Failure = FString::Printf(
						TEXT("Flyaway detected: %s dz=%.1fcm chaosZ=%.1f logicZ=%.1f upZ=%.2f speed=%.1fcm/s"),
						*Adapter->ChaosVehicle->GetName(),
						Dz,
						ChaosPos.Z,
						LogicPos.Z,
						Adapter->ChaosVehicle->GetActorUpVector().Z,
						Adapter->ChaosVehicle->GetVelocity().Size());
					if (Test)
					{
						Test->AddError(State->Failure);
					}
					return true;
				}
			}
			else
			{
				State->ConsecutiveBadSamples = 0;
			}
		}

		// If we couldn't find any adapters at all, fail (map/config regression).
		if (Checked == 0 && (NowReal - State->StartRealTime) > 2.0)
		{
			State->bFailed = true;
			State->Failure = TEXT("No TrafficVehicleAdapter actors found to validate.");
			if (Test)
			{
				Test->AddError(State->Failure);
			}
			return true;
		}

		if (NowReal >= State->EndRealTime)
		{
			if (Test && !State->bFailed)
			{
				Test->AddInfo(FString::Printf(TEXT("PASS: No flyaways detected across %d samples."), State->Samples));
			}
			return true;
		}

		return false;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FEndPIE_NoFlyaway, TSharedRef<FNoFlyawayPIEState>, State, FAutomationTestBase*, Test);
	bool FEndPIE_NoFlyaway::Update()
	{
		if (GEditor)
		{
			GEditor->RequestEndPlayMap();
		}

		if (State->bFailed && Test && !State->Failure.IsEmpty())
		{
			// Error already recorded; keep a trailing info line for readability in logs.
			Test->AddInfo(TEXT("FAIL: BaselineCurveNoFlyaways"));
		}

		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficBaselineCurveNoFlyawayPIETest,
	"Traffic.Visual.ChaosDrive.BaselineCurveNoFlyaways",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficBaselineCurveNoFlyawayPIETest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	if (!AutomationOpenMap(BaselineCurveMapPackage_NoFlyaway))
	{
		AddError(TEXT("Failed to load /AAAtrafficSystem/Maps/Traffic_BaselineCurve."));
		return false;
	}

	// Some editor systems attempt to run editor-only utilities during PIE, which logs this error.
	AddExpectedError(TEXT("The Editor is currently in a play mode."), EAutomationExpectedErrorFlags::Contains, 6);

	TSharedRef<FNoFlyawayPIEState> State = MakeShared<FNoFlyawayPIEState>();

	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForPIEWorld_NoFlyaway(State, this, 20.0, FPlatformTime::Seconds()));
	ADD_LATENT_AUTOMATION_COMMAND(FMonitorNoFlyaways(State, this, /*DurationSeconds=*/10.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPIE_NoFlyaway(State, this));

	return true;
}

#endif // WITH_EDITOR
