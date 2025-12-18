#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TrafficSystemController.h"
#include "TrafficVehicleAdapter.h"
#include "TrafficNetworkAsset.h"
#include "TrafficLaneGeometry.h"
#include "TrafficRoadTypes.h"
#include "Components/PrimitiveComponent.h"

#if WITH_EDITOR

namespace
{
	static const TCHAR* BaselineCurveMapPackage_ChaosDriveVisual = TEXT("/AAAtrafficSystem/Maps/Traffic_BaselineCurve");

	static bool FindBestLaneProjection_ChaosDriveVisual(
		const TArray<FTrafficLane>& Lanes,
		const FVector& Location,
		const FVector& Forward,
		FLaneProjectionResult& OutProj)
	{
		bool bFound = false;
		float BestDistSq = TNumericLimits<float>::Max();
		for (const FTrafficLane& Lane : Lanes)
		{
			FLaneProjectionResult Proj;
			if (TrafficLaneGeometry::ProjectPointOntoLane(Lane, Location, Forward, Proj))
			{
				const float DistSq = FVector::DistSquared(Location, Proj.ClosestPoint);
				if (DistSq < BestDistSq)
				{
					BestDistSq = DistSq;
					OutProj = Proj;
					bFound = true;
				}
			}
		}
		return bFound;
	}

	static bool TraceGroundZ(UWorld& World, const FVector& From, float& OutGroundZ)
	{
		const FVector Start = From + FVector(0.f, 0.f, 3000.f);
		const FVector End = From - FVector(0.f, 0.f, 8000.f);

		FCollisionQueryParams Params(SCENE_QUERY_STAT(AAA_Traffic_GroundTrace), /*bTraceComplex=*/false);
		Params.bReturnPhysicalMaterial = false;

		FHitResult Hit;
		bool bHit =
			World.LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params) ||
			World.LineTraceSingleByChannel(Hit, Start, End, ECC_WorldDynamic, Params) ||
			World.LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

		if (!bHit)
		{
			FCollisionObjectQueryParams ObjParams;
			ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
			ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);
			ObjParams.AddObjectTypesToQuery(ECC_PhysicsBody);
			ObjParams.AddObjectTypesToQuery(ECC_Vehicle);
			bHit = World.LineTraceSingleByObjectType(Hit, Start, End, ObjParams, Params);
		}

		if (!bHit)
		{
			return false;
		}

		OutGroundZ = Hit.ImpactPoint.Z;
		return true;
	}

	static bool TraceGroundZByChannel(UWorld& World, const AActor& IgnoreActor, const FVector& From, ECollisionChannel Channel, float& OutGroundZ)
	{
		const FVector Start = From + FVector(0.f, 0.f, 3000.f);
		const FVector End = From - FVector(0.f, 0.f, 8000.f);

		FCollisionQueryParams Params(SCENE_QUERY_STAT(AAA_Traffic_GroundTrace), /*bTraceComplex=*/false);
		Params.bReturnPhysicalMaterial = false;
		Params.AddIgnoredActor(&IgnoreActor);

		FHitResult Hit;
		if (!World.LineTraceSingleByChannel(Hit, Start, End, Channel, Params))
		{
			return false;
		}

		OutGroundZ = Hit.ImpactPoint.Z;
		return true;
	}

	struct FChaosDriveVisualState
	{
		bool bFailed = false;
		TArray<TWeakObjectPtr<APawn>> SamplePawns;
		TArray<float> StartS;
		float StartTime = 0.f;
		float MaxAltitudeCm = 0.f;
		float MinUpZ = 1.f;
	};

	DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(FWaitForPIEWorld_ChaosDriveVisual, TSharedRef<FChaosDriveVisualState>, State, FAutomationTestBase*, Test, double, TimeoutSeconds, double, StartTime);
	bool FWaitForPIEWorld_ChaosDriveVisual::Update()
	{
		if (GEditor && GEditor->PlayWorld)
		{
			State->StartTime = GEditor->PlayWorld->GetTimeSeconds();
			return true;
		}

		const double Elapsed = FPlatformTime::Seconds() - StartTime;
		if (Elapsed > TimeoutSeconds)
		{
			State->bFailed = true;
			if (Test)
			{
				Test->AddError(TEXT("PIE world did not start for ChaosDrive baseline visual test."));
			}
			return true;
		}

		return false;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_THREE_PARAMETER(FSampleChaosDriveVisuals, TSharedRef<FChaosDriveVisualState>, State, FAutomationTestBase*, Test, float, DurationSeconds);
	bool FSampleChaosDriveVisuals::Update()
	{
		if (State->bFailed || !GEditor || !GEditor->PlayWorld)
		{
			return true;
		}

		UWorld* World = GEditor->PlayWorld;

		ATrafficSystemController* Controller = nullptr;
		for (TActorIterator<ATrafficSystemController> It(World); It; ++It)
		{
			Controller = *It;
			break;
		}
		if (!Controller)
		{
			// Wait a bit for the subsystem/controller to spawn traffic.
			return false;
		}

		UTrafficNetworkAsset* Net = Controller->GetBuiltNetworkAsset();
		if (!Net || Net->Network.Lanes.Num() == 0)
		{
			return false;
		}

		const TArray<FTrafficLane>& Lanes = Net->Network.Lanes;

		// Pick a few sample pawns once.
		if (State->SamplePawns.Num() == 0)
		{
			for (TActorIterator<ATrafficVehicleAdapter> It(World); It; ++It)
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
				State->SamplePawns.Add(Pawn);
				if (State->SamplePawns.Num() >= 3)
				{
					break;
				}
			}

			for (const TWeakObjectPtr<APawn>& PawnPtr : State->SamplePawns)
			{
				float S = 0.f;
				if (APawn* Pawn = PawnPtr.Get())
				{
					FLaneProjectionResult Proj;
					if (FindBestLaneProjection_ChaosDriveVisual(Lanes, Pawn->GetActorLocation(), Pawn->GetActorForwardVector(), Proj))
					{
						S = Proj.S;
					}
				}
				State->StartS.Add(S);
			}
		}

		for (int32 i = 0; i < State->SamplePawns.Num(); ++i)
		{
			APawn* Pawn = State->SamplePawns[i].Get();
			if (!Pawn)
			{
				continue;
			}

			// Altitude check against ground collision.
			float GroundZ = 0.f;
			if (TraceGroundZ(*World, Pawn->GetActorLocation(), GroundZ))
			{
				const float Alt = Pawn->GetActorLocation().Z - GroundZ;
				State->MaxAltitudeCm = FMath::Max(State->MaxAltitudeCm, Alt);
			}

			State->MinUpZ = FMath::Min(State->MinUpZ, Pawn->GetActorUpVector().Z);
		}

		const float Now = World->GetTimeSeconds();
		if (Now - State->StartTime < DurationSeconds)
		{
			return false;
		}

		if (State->SamplePawns.Num() == 0 && Test)
		{
			Test->AddError(TEXT("No Chaos traffic pawns found to sample on baseline curve map."));
			return true;
		}

		// Validate movement along lane.
		for (int32 i = 0; i < State->SamplePawns.Num(); ++i)
		{
			APawn* Pawn = State->SamplePawns[i].Get();
			if (!Pawn)
			{
				continue;
			}

			FLaneProjectionResult Proj;
			if (!FindBestLaneProjection_ChaosDriveVisual(Lanes, Pawn->GetActorLocation(), Pawn->GetActorForwardVector(), Proj))
			{
				if (Test) Test->AddError(TEXT("Failed to project Chaos pawn onto baseline curve lane network."));
				continue;
			}

			const float DeltaS = Proj.S - State->StartS[i];
			if (DeltaS < 200.f && Test)
			{
				Test->AddError(TEXT("ChaosDrive pawn did not advance along the lane on baseline curve (appears stuck)."));
			}
		}

		// Wheel/suspension trace channel sanity: Chaos vehicles commonly use ECC_WorldDynamic for suspension traces.
		// If the road doesn't block this channel, vehicles can hover/flip and never get traction.
		for (int32 i = 0; i < State->SamplePawns.Num(); ++i)
		{
			APawn* Pawn = State->SamplePawns[i].Get();
			if (!Pawn)
			{
				continue;
			}

			float DynZ = 0.f;
			if (!TraceGroundZByChannel(*World, *Pawn, Pawn->GetActorLocation(), ECC_WorldDynamic, DynZ) && Test)
			{
				Test->AddError(TEXT("ECC_WorldDynamic ground trace under ChaosDrive pawn failed (road may not block the wheel/suspension trace channel)."));
			}
		}

		// Grounded/upright sanity checks.
		if (State->MaxAltitudeCm > 400.f && Test)
		{
			Test->AddError(FString::Printf(TEXT("ChaosDrive pawn got too far above ground (max altitude %.1fcm)."), State->MaxAltitudeCm));
		}
		if (State->MinUpZ < 0.2f && Test)
		{
			Test->AddError(FString::Printf(TEXT("ChaosDrive pawn became too tilted/upside-down (min Up.Z %.2f)."), State->MinUpZ));
		}

		return true;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FEndPIE_ChaosDriveVisual, TSharedRef<FChaosDriveVisualState>, State, FAutomationTestBase*, Test);
	bool FEndPIE_ChaosDriveVisual::Update()
	{
		if (GEditor)
		{
			GEditor->EndPlayMap();
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficBaselineCurveChaosDriveVisualTest,
	"Traffic.Visual.BaselineCurve.ChaosDriveStaysGrounded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficBaselineCurveChaosDriveVisualTest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(BaselineCurveMapPackage_ChaosDriveVisual))
	{
		AddError(TEXT("Failed to load /AAAtrafficSystem/Maps/Traffic_BaselineCurve."));
		return false;
	}

	TSharedRef<FChaosDriveVisualState> State = MakeShared<FChaosDriveVisualState>();
	// Some editor systems attempt to run editor-only utilities during PIE, which logs this error.
	// It can appear multiple times (engine log + automation controller echo), so allow a small buffer.
	AddExpectedError(TEXT("The Editor is currently in a play mode."), EAutomationExpectedErrorFlags::Contains, 6);

	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForPIEWorld_ChaosDriveVisual(State, this, 15.0, FPlatformTime::Seconds()));
	ADD_LATENT_AUTOMATION_COMMAND(FSampleChaosDriveVisuals(State, this, 6.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPIE_ChaosDriveVisual(State, this));

	return true;
}

#endif // WITH_EDITOR
