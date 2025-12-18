#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TrafficVehicleAdapter.h"
#include "TrafficRuntimeModule.h"
#include "Components/PrimitiveComponent.h"
#include "Containers/Map.h"

#if WITH_EDITOR

namespace
{
	static const TCHAR* BaselineCurveMapPackage_SpinStability = TEXT("/AAAtrafficSystem/Maps/Traffic_BaselineCurve");

	struct FSpinStabilityState
	{
		bool bFailed = false;
		float StartTimeSeconds = 0.f;
		float EndTimeSeconds = 0.f;
		float MaxAngularDegPerSec = 0.f;
		int32 NumSampled = 0;
		bool bHasAnyChaos = false;

		struct FYawSample
		{
			float PrevYawDeg = 0.f;
			float PrevTimeSeconds = 0.f;
			bool bHasPrev = false;
		};
		TMap<TWeakObjectPtr<APawn>, FYawSample> YawSamples;
	};

	DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(FWaitForPIEWorldCommand, TSharedRef<FSpinStabilityState>, State, FAutomationTestBase*, Test, double, TimeoutSeconds, double, StartTime);
	bool FWaitForPIEWorldCommand::Update()
	{
		if (GEditor && GEditor->PlayWorld)
		{
			State->StartTimeSeconds = GEditor->PlayWorld->GetTimeSeconds();
			return true;
		}

		const double Elapsed = FPlatformTime::Seconds() - StartTime;
		if (Elapsed > TimeoutSeconds)
		{
			State->bFailed = true;
			if (Test)
			{
				Test->AddError(TEXT("PIE world did not start for baseline curve stability test."));
			}
			return true;
		}

		return false;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_THREE_PARAMETER(FSpinStabilitySampleCommand, TSharedRef<FSpinStabilityState>, State, FAutomationTestBase*, Test, float, DurationSeconds);
	bool FSpinStabilitySampleCommand::Update()
	{
		if (State->bFailed || !GEditor || !GEditor->PlayWorld)
		{
			if (!State->bFailed && Test)
			{
				Test->AddError(TEXT("PIE world ended unexpectedly during baseline curve stability sampling."));
			}
			return true;
		}

		UWorld* PIEWorld = GEditor->PlayWorld;
		const float Now = PIEWorld->GetTimeSeconds();
		State->EndTimeSeconds = Now;

		int32 SampledThisFrame = 0;
		for (TActorIterator<ATrafficVehicleAdapter> It(PIEWorld); It; ++It)
		{
			if (!It->ChaosVehicle.IsValid())
			{
				continue;
			}
			APawn* Pawn = It->ChaosVehicle.Get();
			if (!Pawn)
			{
				continue;
			}

			State->bHasAnyChaos = true;

			// Measure actor yaw rate rather than physics angular velocity to avoid counting wheel spin.
			const float YawDeg = Pawn->GetActorRotation().Yaw;
			FSpinStabilityState::FYawSample& Sample = State->YawSamples.FindOrAdd(Pawn);
			if (Sample.bHasPrev)
			{
				const float Dt = Now - Sample.PrevTimeSeconds;
				if (Dt > KINDA_SMALL_NUMBER)
				{
					const float DeltaYaw = FMath::Abs(FMath::FindDeltaAngleDegrees(Sample.PrevYawDeg, YawDeg));
					const float YawRate = DeltaYaw / Dt;
					State->MaxAngularDegPerSec = FMath::Max(State->MaxAngularDegPerSec, YawRate);
				}
			}
			Sample.PrevYawDeg = YawDeg;
			Sample.PrevTimeSeconds = Now;
			Sample.bHasPrev = true;

			++SampledThisFrame;
			if (SampledThisFrame >= 3)
			{
				break;
			}
		}

		State->NumSampled = FMath::Max(State->NumSampled, SampledThisFrame);

		if (Now - State->StartTimeSeconds >= DurationSeconds)
		{
			// Conservative: actor spinning "like crazy" typically exceeds 720+ deg/sec. Flag anything above 540 deg/sec.
			if (!State->bHasAnyChaos && Test)
			{
				Test->AddError(TEXT("No Chaos traffic vehicles were found to sample on baseline curve map."));
			}
			if (State->bHasAnyChaos && State->MaxAngularDegPerSec > 540.f && Test)
			{
				Test->AddError(FString::Printf(TEXT("Observed excessive yaw spin rate during spawn: %.1f deg/sec (expected <= 540)."), State->MaxAngularDegPerSec));
			}
			return true;
		}

		return false;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FEndPIECommand, TSharedRef<FSpinStabilityState>, State, FAutomationTestBase*, Test);
	bool FEndPIECommand::Update()
	{
		if (GEditor)
		{
			GEditor->EndPlayMap();
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficBaselineCurveSpinStabilityTest,
	"Traffic.Visual.BaselineCurve.SpawnSpinStability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficBaselineCurveSpinStabilityTest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(BaselineCurveMapPackage_SpinStability))
	{
		AddError(TEXT("Failed to load /AAAtrafficSystem/Maps/Traffic_BaselineCurve."));
		return false;
	}

	TSharedRef<FSpinStabilityState> State = MakeShared<FSpinStabilityState>();
	// Some editor systems attempt to run editor-only utilities during PIE, which logs this error.
	// It appears twice (engine log + automation controller echo), so we allow 6 occurrences here.
	AddExpectedError(TEXT("The Editor is currently in a play mode."), EAutomationExpectedErrorFlags::Contains, 6);

	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForPIEWorldCommand(State, this, 15.0, FPlatformTime::Seconds()));
	ADD_LATENT_AUTOMATION_COMMAND(FSpinStabilitySampleCommand(State, this, 3.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPIECommand(State, this));

	return true;
}

#endif // WITH_EDITOR
