#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/MovementComponent.h"
#include "TrafficVehicleAdapter.h"
#include "TrafficRuntimeModule.h"
#include "HAL/IConsoleManager.h"
#include "UObject/UnrealType.h"
#include "Components/PrimitiveComponent.h"

#if WITH_EDITOR

namespace
{
	static const TCHAR* BaselineCurveMapPackage_Vehicle07Ground = TEXT("/AAAtrafficSystem/Maps/Traffic_BaselineCurve");

	static ECollisionChannel ResolveWheelTraceChannel(UMovementComponent* Move)
	{
		if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Visual.ChaosDrive.WheelTraceChannelOverride")))
		{
			const int32 Override = Var->GetInt();
			if (Override >= 0 && Override <= 31)
			{
				return static_cast<ECollisionChannel>(Override);
			}
		}

		if (!Move)
		{
			return ECC_WorldDynamic;
		}

		static const FName WheelTraceChannelPropName(TEXT("WheelTraceCollisionChannel"));
		if (FProperty* Prop = Move->GetClass()->FindPropertyByName(WheelTraceChannelPropName))
		{
			void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Move);
			if (ValuePtr)
			{
				if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
				{
					if (const FNumericProperty* Underlying = EnumProp->GetUnderlyingProperty())
					{
						const int64 V = Underlying->GetSignedIntPropertyValue(ValuePtr);
						if (V >= 0 && V <= 31)
						{
							return static_cast<ECollisionChannel>(V);
						}
					}
				}
				else if (const FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
				{
					const uint8 V = ByteProp->GetPropertyValue(ValuePtr);
					if (V <= 31)
					{
						return static_cast<ECollisionChannel>(V);
					}
				}
				else if (const FIntProperty* IntProp = CastField<FIntProperty>(Prop))
				{
					const int32 V = IntProp->GetPropertyValue(ValuePtr);
					if (V >= 0 && V <= 31)
					{
						return static_cast<ECollisionChannel>(V);
					}
				}
			}
		}

		return ECC_WorldDynamic;
	}

	static bool TraceDownFromActorBounds(
		UWorld& World,
		const AActor& Actor,
		ECollisionChannel Channel,
		const float TraceDepthCm,
		FHitResult& OutHit)
	{
		const FBox Bounds = Actor.GetComponentsBoundingBox(true);
		const FVector FallbackCenter = Actor.GetActorLocation();
		const FVector Center = Bounds.IsValid ? Bounds.GetCenter() : FallbackCenter;
		const float TopZ = Bounds.IsValid ? Bounds.Max.Z : FallbackCenter.Z;
		const float BottomZ = Bounds.IsValid ? Bounds.Min.Z : FallbackCenter.Z;

		const FVector Start(Center.X, Center.Y, TopZ + 50.f);
		const FVector End(Center.X, Center.Y, BottomZ - FMath::Max(0.f, TraceDepthCm));

		FCollisionQueryParams Params(SCENE_QUERY_STAT(AAA_Traffic_GroundedTraceTest), /*bTraceComplex=*/false);
		Params.bReturnPhysicalMaterial = false;
		Params.AddIgnoredActor(&Actor);

		return World.LineTraceSingleByChannel(OutHit, Start, End, Channel, Params);
	}

	static bool IsGroundedByChannel(
		UWorld& World,
		const AActor& Actor,
		ECollisionChannel Channel,
		const float MaxGapCm,
		const float TraceDepthCm,
		float& OutGapCm,
		FHitResult& OutHit)
	{
		if (!TraceDownFromActorBounds(World, Actor, Channel, TraceDepthCm, OutHit))
		{
			OutGapCm = TNumericLimits<float>::Max();
			return false;
		}

		const FBox Bounds = Actor.GetComponentsBoundingBox(true);
		const float BottomZ = Bounds.IsValid ? Bounds.Min.Z : Actor.GetActorLocation().Z;
		const float Gap = BottomZ - OutHit.ImpactPoint.Z;
		OutGapCm = Gap;
		return Gap <= FMath::Max(0.f, MaxGapCm);
	}

	struct FVehicle07GroundState
	{
		bool bFailed = false;
		bool bSucceeded = false;
		FString Failure;

		TWeakObjectPtr<ATrafficVehicleAdapter> Adapter;
		TWeakObjectPtr<APawn> Pawn;

		float LastSampleTime = -1000.f;
		float StartWorldTime = -1.f;
		float OverallStartWorldTime = -1.f;
		int32 ReacquireCount = 0;
		int32 ConsecutiveGroundedSamples = 0;
		float WorstGapCm = 0.f;
		ECollisionChannel WheelChannel = ECC_WorldDynamic;
	};

	DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(FWaitForPIEWorld_Vehicle07Ground, TSharedRef<FVehicle07GroundState>, State, FAutomationTestBase*, Test, double, TimeoutSeconds, double, StartTime);
	bool FWaitForPIEWorld_Vehicle07Ground::Update()
	{
		if (GEditor && GEditor->PlayWorld)
		{
			return true;
		}

		const double Elapsed = FPlatformTime::Seconds() - StartTime;
		if (Elapsed > TimeoutSeconds)
		{
			if (Test) Test->AddError(TEXT("PIE world did not start for Vehicle07 ground contact test."));
			return true;
		}

		return false;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FAcquireFirstChaosPawn, TSharedRef<FVehicle07GroundState>, State, FAutomationTestBase*, Test);
	bool FAcquireFirstChaosPawn::Update()
	{
		if (!GEditor || !GEditor->PlayWorld)
		{
			if (Test) Test->AddError(TEXT("PIE world missing while acquiring Chaos pawn."));
			return true;
		}

		UWorld* World = GEditor->PlayWorld;

		// Reset if the pawn got destroyed and we need to reacquire.
		if (!State->Pawn.IsValid())
		{
			State->Adapter.Reset();
		}

		for (TActorIterator<ATrafficVehicleAdapter> It(World); It; ++It)
		{
			if (!It->ChaosVehicle.IsValid())
			{
				continue;
			}

			State->Adapter = *It;
			State->Pawn = It->ChaosVehicle.Get();
			State->ConsecutiveGroundedSamples = 0;
			State->WorstGapCm = 0.f;
			State->StartWorldTime = (State->StartWorldTime < 0.f) ? World->GetTimeSeconds() : State->StartWorldTime;
			State->OverallStartWorldTime = (State->OverallStartWorldTime < 0.f) ? World->GetTimeSeconds() : State->OverallStartWorldTime;
			return true;
		}

		// Keep waiting; traffic may spawn after deferred collision readiness.
		return false;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_THREE_PARAMETER(FWaitWorldSeconds, TSharedRef<FVehicle07GroundState>, State, FAutomationTestBase*, Test, float, Seconds);
	bool FWaitWorldSeconds::Update()
	{
		if (!GEditor || !GEditor->PlayWorld)
		{
			if (Test) Test->AddError(TEXT("PIE world missing while waiting."));
			return true;
		}

		UWorld* World = GEditor->PlayWorld;
		const float Now = World->GetTimeSeconds();
		if (State->OverallStartWorldTime < 0.f)
		{
			State->OverallStartWorldTime = Now;
		}

		return (Now - State->OverallStartWorldTime) >= Seconds;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_THREE_PARAMETER(FRequireStableGroundContact, TSharedRef<FVehicle07GroundState>, State, FAutomationTestBase*, Test, float, TimeoutSeconds);
	bool FRequireStableGroundContact::Update()
	{
		if (State->bFailed || State->bSucceeded)
		{
			return true;
		}

		if (!GEditor || !GEditor->PlayWorld)
		{
			State->bFailed = true;
			State->Failure = TEXT("PIE world ended unexpectedly during grounding check.");
			return true;
		}

		UWorld* World = GEditor->PlayWorld;
		const float Now = World->GetTimeSeconds();
		if (State->OverallStartWorldTime < 0.f)
		{
			State->OverallStartWorldTime = Now;
		}

		APawn* Pawn = State->Pawn.Get();
		if (!Pawn)
		{
			// Traffic can legitimately clear & respawn visuals during early PIE. Reacquire a new pawn in-place.
			for (TActorIterator<ATrafficVehicleAdapter> It(World); It; ++It)
			{
				if (!It->ChaosVehicle.IsValid())
				{
					continue;
				}

				State->Adapter = *It;
				State->Pawn = It->ChaosVehicle.Get();
				State->ConsecutiveGroundedSamples = 0;
				State->WorstGapCm = 0.f;
				State->LastSampleTime = -1000.f;
				State->ReacquireCount++;
				Pawn = State->Pawn.Get();
				break;
			}

			if (!Pawn)
			{
				// Keep waiting for the next spawn.
				if ((Now - State->OverallStartWorldTime) < TimeoutSeconds)
				{
					return false;
				}

				State->bFailed = true;
				State->Failure = TEXT("No Chaos pawn found to validate ground contact (timed out).");
				return true;
			}
		}

		if (State->StartWorldTime < 0.f)
		{
			State->StartWorldTime = Now;
		}
		static const float SampleInterval = 0.1f;
		if ((Now - State->LastSampleTime) < SampleInterval)
		{
			return false;
		}
		State->LastSampleTime = Now;

		UMovementComponent* Move = nullptr;
		{
			TArray<UMovementComponent*> MoveComps;
			Pawn->GetComponents(MoveComps);
			for (UMovementComponent* M : MoveComps)
			{
				if (M)
				{
					Move = M;
					break;
				}
			}
		}

		const ECollisionChannel WheelCh = ResolveWheelTraceChannel(Move);
		State->WheelChannel = WheelCh;

		float GapCm = 0.f;
		FHitResult Hit;
		const bool bGrounded = IsGroundedByChannel(*World, *Pawn, WheelCh, /*MaxGapCm=*/300.f, /*TraceDepthCm=*/20000.f, GapCm, Hit);
		State->WorstGapCm = FMath::Max(State->WorstGapCm, GapCm);

		if (bGrounded)
		{
			State->ConsecutiveGroundedSamples++;
		}
		else
		{
			State->ConsecutiveGroundedSamples = 0;
		}

		// Require 1 second of continuous "grounded" samples (10 * 0.1s) to prove it can sit on the ground stably.
		if (State->ConsecutiveGroundedSamples >= 10)
		{
			State->bSucceeded = true;
			return true;
		}

		if ((Now - State->OverallStartWorldTime) >= TimeoutSeconds)
		{
			State->bFailed = true;
			State->Failure = FString::Printf(
				TEXT("Did not achieve stable ground contact within timeout. wheelCh=%d consecutive=%d worstGap=%.1fcm upZ=%.2f speed=%.1fcm/s reacquire=%d"),
				static_cast<int32>(WheelCh),
				State->ConsecutiveGroundedSamples,
				State->WorstGapCm,
				Pawn->GetActorUpVector().Z,
				Pawn->GetVelocity().Size(),
				State->ReacquireCount);
			return true;
		}

		return false;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FEndPIE_Vehicle07Ground, TSharedRef<FVehicle07GroundState>, State, FAutomationTestBase*, Test);
	bool FEndPIE_Vehicle07Ground::Update()
	{
		if (GEditor)
		{
			GEditor->RequestEndPlayMap();
		}

		if (State->bFailed && Test)
		{
			Test->AddError(State->Failure);
		}

		if (State->bSucceeded && Test)
		{
			Test->AddInfo(FString::Printf(
				TEXT("PASS: Vehicle grounded stably. wheelCh=%d"),
				static_cast<int32>(State->WheelChannel)));
		}

		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficVehicle07GroundContactTest,
	"Traffic.Visual.ChaosDrive.Vehicle07GroundContact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficVehicle07GroundContactTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	// Make the test deterministic and avoid spawning many vehicles.
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Spawn.MaxVehicles")))
	{
		Var->Set(1, ECVF_SetByCode);
	}
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Visual.Mode")))
	{
		Var->Set(2, ECVF_SetByCode);
	}

	if (!AutomationOpenMap(BaselineCurveMapPackage_Vehicle07Ground))
	{
		AddError(TEXT("Failed to load /AAAtrafficSystem/Maps/Traffic_BaselineCurve."));
		return false;
	}

	// Some editor systems attempt to run editor-only utilities during PIE, which logs this error.
	AddExpectedError(TEXT("The Editor is currently in a play mode."), EAutomationExpectedErrorFlags::Contains, 6);

	TSharedRef<FVehicle07GroundState> State = MakeShared<FVehicle07GroundState>();

	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForPIEWorld_Vehicle07Ground(State, this, 20.0, FPlatformTime::Seconds()));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitWorldSeconds(State, this, 3.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FAcquireFirstChaosPawn(State, this));
	ADD_LATENT_AUTOMATION_COMMAND(FRequireStableGroundContact(State, this, /*TimeoutSeconds=*/20.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPIE_Vehicle07Ground(State, this));

	return true;
}

#endif // WITH_EDITOR
