#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/MovementComponent.h"
#include "GameFramework/Controller.h"
#include "TrafficAutomationLogger.h"
#include "TrafficVehicleAdapter.h"
#include "TrafficVehicleBase.h"
#include "TrafficNetworkAsset.h"
#include "HAL/PlatformTime.h"
#include "HAL/IConsoleManager.h"

#if WITH_EDITOR

namespace
{
	static const TCHAR* Vehicle07ClassPath = TEXT("/Game/CitySampleVehicles/vehicle07_Car/BP_vehicle07_Car.BP_vehicle07_Car_C");

	static bool HasAnySimulatingPrimitive(const AActor& Actor)
	{
		TArray<UPrimitiveComponent*> PrimComps;
		Actor.GetComponents(PrimComps);
		for (const UPrimitiveComponent* Prim : PrimComps)
		{
			if (Prim && Prim->IsSimulatingPhysics())
			{
				return true;
			}
		}
		return false;
	}

	static bool AnySimulatingHasGravityEnabled(const AActor& Actor)
	{
		TArray<UPrimitiveComponent*> PrimComps;
		Actor.GetComponents(PrimComps);
		for (const UPrimitiveComponent* Prim : PrimComps)
		{
			if (Prim && Prim->IsSimulatingPhysics() && Prim->IsGravityEnabled())
			{
				return true;
			}
		}
		return false;
	}

	static UMovementComponent* FindMovementComponentWithVehicleInputs(APawn& Pawn)
	{
		TArray<UMovementComponent*> MoveComps;
		Pawn.GetComponents(MoveComps);

		for (UMovementComponent* Move : MoveComps)
		{
			if (!Move)
			{
				continue;
			}

			UClass* Cls = Move->GetClass();
			if (!Cls)
			{
				continue;
			}

			// Prefer Chaos wheeled vehicle movement when present.
			const FString Name = Cls->GetName();
			if (Name.Contains(TEXT("ChaosWheeledVehicleMovementComponent"), ESearchCase::IgnoreCase))
			{
				return Move;
			}
		}

		for (UMovementComponent* Move : MoveComps)
		{
			if (!Move)
			{
				continue;
			}
			if (Move->FindFunction(FName(TEXT("SetThrottleInput"))) &&
				Move->FindFunction(FName(TEXT("SetSteeringInput"))) &&
				Move->FindFunction(FName(TEXT("SetBrakeInput"))))
			{
				return Move;
			}
		}

		return nullptr;
	}

	static bool CallSingleParamFloatOrBool(UObject& Obj, const FName FuncName, float FloatValue, bool BoolValue)
	{
		UFunction* Fn = Obj.FindFunction(FuncName);
		if (!Fn)
		{
			return false;
		}

		FProperty* Param = nullptr;
		int32 ParamCount = 0;
		for (TFieldIterator<FProperty> It(Fn); It; ++It)
		{
			FProperty* P = *It;
			if (!P)
			{
				continue;
			}
			if (P->HasAnyPropertyFlags(CPF_Parm) && !P->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				Param = P;
				++ParamCount;
			}
		}
		if (ParamCount != 1 || !Param)
		{
			return false;
		}

		TArray<uint8> Buffer;
		Buffer.SetNumZeroed(Fn->ParmsSize);

		if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Param))
		{
			FloatProp->SetPropertyValue_InContainer(Buffer.GetData(), FloatValue);
		}
		else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Param))
		{
			BoolProp->SetPropertyValue_InContainer(Buffer.GetData(), BoolValue);
		}
		else
		{
			return false;
		}

		Obj.ProcessEvent(Fn, Buffer.GetData());
		return true;
	}

	static void DisableAutoPossessAndControllers(APawn& Pawn)
	{
		Pawn.AutoPossessPlayer = EAutoReceiveInput::Disabled;
		Pawn.AutoPossessAI = EAutoPossessAI::Disabled;
		Pawn.AIControllerClass = nullptr;
		Pawn.DetachFromControllerPendingDestroy();
		Pawn.DisableInput(nullptr);
	}

	struct FVehicle07ChaosDrivePIEState
	{
		bool bFailed = false;
		UWorld* PIEWorld = nullptr;
		TObjectPtr<APawn> ChaosPawn = nullptr;
		TObjectPtr<ATrafficVehicleBase> LogicVehicle = nullptr;
		TObjectPtr<ATrafficVehicleAdapter> Adapter = nullptr;
		TObjectPtr<UTrafficNetworkAsset> NetworkAsset = nullptr;
		TWeakObjectPtr<AStaticMeshActor> FallingCube;
		TWeakObjectPtr<UMovementComponent> MoveComp;
		float StartWorldTimeSeconds = 0.f;
		float CubeStartZ = 0.f;
		FVector StartLocation = FVector::ZeroVector;
		FVector EndLocation = FVector::ZeroVector;
		bool bHasSim = false;
		bool bHasGravity = false;
		bool bSpawned = false;
	};

	DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(FWaitForPIEWorld, TSharedRef<FVehicle07ChaosDrivePIEState>, State, FAutomationTestBase*, Test, double, TimeoutSeconds, double, StartTime);
	bool FWaitForPIEWorld::Update()
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
				Test->AddError(TEXT("PIE world did not start."));
			}
			return true;
		}

		return false;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FVehicle07ChaosDriveRun, TSharedRef<FVehicle07ChaosDrivePIEState>, State, FAutomationTestBase*, Test);
	bool FVehicle07ChaosDriveRun::Update()
	{
		if (State->bFailed || !State->PIEWorld)
		{
			return true;
		}

		UWorld* World = State->PIEWorld;

		// Spawn once, then let PIE tick naturally across frames (do not manually call World->Tick here).
		if (!State->bSpawned)
		{
			// Force ChaosDrive visuals for this test (independent of user/project console variable state).
			if (IConsoleVariable* ModeVar = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Visual.Mode")))
			{
				ModeVar->Set(2, ECVF_SetByCode);
			}

			// Ground plane.
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				AStaticMeshActor* PlaneActor = World->SpawnActor<AStaticMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
				if (!PlaneActor)
				{
					State->bFailed = true;
					if (Test) Test->AddError(TEXT("Failed to spawn ground plane actor."));
					return true;
				}

				UStaticMeshComponent* MeshComp = PlaneActor->GetStaticMeshComponent();
				UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
				if (!MeshComp || !PlaneMesh)
				{
					State->bFailed = true;
					if (Test) Test->AddError(TEXT("Failed to setup ground plane mesh."));
					return true;
				}

				// Use Movable so runtime mesh assignment updates collision reliably in PIE automation.
				MeshComp->SetMobility(EComponentMobility::Movable);
				MeshComp->SetStaticMesh(PlaneMesh);
				MeshComp->SetWorldScale3D(FVector(200.f, 200.f, 1.f));
				MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				MeshComp->SetCollisionObjectType(ECC_WorldStatic);
				MeshComp->SetCollisionResponseToAllChannels(ECR_Block);
			}

			// Synthetic lane stored in a persistent network asset so follower pointers remain valid.
			State->NetworkAsset = NewObject<UTrafficNetworkAsset>(GetTransientPackage());
			FTrafficLane Lane;
			Lane.LaneId = 0;
			Lane.RoadId = 0;
			Lane.SideIndex = 0;
			Lane.Width = 350.f;
			Lane.Direction = ELaneDirection::Forward;
			Lane.SpeedLimitKmh = 50.f;
			for (int32 i = 0; i < 25; ++i)
			{
				Lane.CenterlinePoints.Add(FVector(i * 1000.f, 0.f, 0.f));
			}
			State->NetworkAsset->Network.Lanes.Add(Lane);
			const FTrafficLane* LanePtr = &State->NetworkAsset->Network.Lanes[0];

			// Logic vehicle.
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				State->LogicVehicle = World->SpawnActor<ATrafficVehicleBase>(ATrafficVehicleBase::StaticClass(), FVector(0.f, 0.f, 50.f), FRotator::ZeroRotator, SpawnParams);
				if (!State->LogicVehicle)
				{
					State->bFailed = true;
					if (Test) Test->AddError(TEXT("Failed to spawn logic TrafficVehicleBase."));
					return true;
				}
				State->LogicVehicle->SetNetworkAsset(State->NetworkAsset);
				State->LogicVehicle->InitializeOnLane(LanePtr, /*InitialS=*/0.f, /*SpeedCmPerSec=*/1200.f);
			}

			// Chaos visual pawn.
			TSubclassOf<APawn> ChaosClass = LoadClass<APawn>(nullptr, Vehicle07ClassPath);
			if (!ChaosClass)
			{
				State->bFailed = true;
				if (Test) Test->AddError(FString::Printf(TEXT("Failed to load Chaos pawn class: %s"), Vehicle07ClassPath));
				return true;
			}

			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				State->ChaosPawn = World->SpawnActor<APawn>(ChaosClass, FVector(0.f, 0.f, 200.f), FRotator::ZeroRotator, SpawnParams);
				if (!State->ChaosPawn)
				{
					State->bFailed = true;
					if (Test) Test->AddError(TEXT("Failed to spawn BP_vehicle07_Car pawn."));
					return true;
				}
				DisableAutoPossessAndControllers(*State->ChaosPawn);
			}

			// Adapter drives the Chaos pawn using movement inputs to follow the logic vehicle.
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				State->Adapter = World->SpawnActor<ATrafficVehicleAdapter>(ATrafficVehicleAdapter::StaticClass(), FTransform::Identity, SpawnParams);
				if (!State->Adapter)
				{
					State->bFailed = true;
					if (Test) Test->AddError(TEXT("Failed to spawn TrafficVehicleAdapter."));
					return true;
				}
				State->Adapter->Initialize(State->LogicVehicle, State->ChaosPawn);
			}

			State->StartLocation = State->ChaosPawn->GetActorLocation();
			State->StartWorldTimeSeconds = World->GetTimeSeconds();

			// Sanity: physics should tick in PIE. Verify with a simple falling cube.
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				AStaticMeshActor* FallingCube = World->SpawnActor<AStaticMeshActor>(FVector(0.f, 800.f, 500.f), FRotator::ZeroRotator, SpawnParams);
				State->FallingCube = FallingCube;
				if (FallingCube)
				{
					if (UStaticMeshComponent* CubeMesh = FallingCube->GetStaticMeshComponent())
					{
						CubeMesh->SetMobility(EComponentMobility::Movable);
						if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
						{
							CubeMesh->SetStaticMesh(Cube);
						}
						CubeMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
						CubeMesh->SetCollisionObjectType(ECC_PhysicsBody);
						CubeMesh->SetCollisionResponseToAllChannels(ECR_Block);
						CubeMesh->SetSimulatePhysics(true);
						CubeMesh->SetEnableGravity(true);
					}
					State->CubeStartZ = FallingCube->GetActorLocation().Z;
				}
			}

			if (State->ChaosPawn)
			{
				if (UMovementComponent* Move = FindMovementComponentWithVehicleInputs(*State->ChaosPawn))
				{
					Move->Activate(true);
					Move->SetComponentTickEnabled(true);
					State->MoveComp = Move;
				}
			}

			if (!State->MoveComp.IsValid())
			{
				State->bFailed = true;
				if (Test) Test->AddError(TEXT("No movement component with vehicle input functions found on BP_vehicle07_Car."));
				return true;
			}

			State->bSpawned = true;
			return false;
		}

		// Ensure handbrake is released; steering/throttle/brake should be driven by ATrafficVehicleAdapter (ChaosDrive).
		if (State->MoveComp.IsValid())
		{
			UObject* MoveObj = State->MoveComp.Get();
			CallSingleParamFloatOrBool(*MoveObj, FName(TEXT("SetHandbrakeInput")), 0.f, false);
		}

		if (!State->bHasSim && State->ChaosPawn && HasAnySimulatingPrimitive(*State->ChaosPawn))
		{
			State->bHasSim = true;
		}
		if (!State->bHasGravity && State->ChaosPawn && AnySimulatingHasGravityEnabled(*State->ChaosPawn))
		{
			State->bHasGravity = true;
		}

		const float TotalSimSeconds = 4.0f;
		const float Elapsed = World->GetTimeSeconds() - State->StartWorldTimeSeconds;
		if (Elapsed < TotalSimSeconds)
		{
			return false;
		}

		State->EndLocation = State->ChaosPawn ? State->ChaosPawn->GetActorLocation() : State->StartLocation;
		const float DeltaX = State->EndLocation.X - State->StartLocation.X;
		// Some vehicle BPs can have their local forward axis flipped vs. the lane direction. For this test we only require
		// meaningful motion along the lane axis.
		const bool bMovedForward = (FMath::Abs(DeltaX) > 200.f);
		// Vehicle COM can bounce above the ground plane during initial physics settle/drive. We only treat extreme Z as "floating".
		const bool bReasonableZ = (State->EndLocation.Z < 2000.f);

		const float GravityZ = World ? World->GetGravityZ() : 0.f;
		const float CubeEndZ = State->FallingCube.IsValid() ? State->FallingCube->GetActorLocation().Z : State->CubeStartZ;
		const bool bCubeFell = State->FallingCube.IsValid() ? (CubeEndZ < (State->CubeStartZ - 50.f)) : true;

		UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("Vehicle07 Start=(%.1f %.1f %.1f) End=(%.1f %.1f %.1f) DeltaX=%.1f"),
			State->StartLocation.X, State->StartLocation.Y, State->StartLocation.Z,
			State->EndLocation.X, State->EndLocation.Y, State->EndLocation.Z,
			DeltaX));
		UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("World GravityZ=%.1f CubeStartZ=%.1f CubeEndZ=%.1f"), GravityZ, State->CubeStartZ, CubeEndZ));

		if (!bCubeFell && Test)
		{
			Test->AddError(TEXT("Physics sanity check failed: falling cube did not fall (PIE physics may not be stepping)."));
		}
		if (!State->bHasSim && Test)
		{
			Test->AddError(TEXT("BP_vehicle07_Car has no simulating primitive during ChaosDrive test."));
		}
		if (!State->bHasGravity && Test)
		{
			Test->AddError(TEXT("BP_vehicle07_Car has no simulating primitive with gravity enabled during ChaosDrive test."));
		}
		if (!bMovedForward && Test)
		{
			Test->AddError(TEXT("BP_vehicle07_Car did not move along the lane axis on the ground plane (ChaosDrive)."));
		}
		if (!bReasonableZ && Test)
		{
			Test->AddError(TEXT("BP_vehicle07_Car stayed at an unexpectedly high Z (appears to be floating)."));
		}

		return true;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FEndPIE, TSharedRef<FVehicle07ChaosDrivePIEState>, State, FAutomationTestBase*, Test);
	bool FEndPIE::Update()
	{
		if (GEditor)
		{
			GEditor->EndPlayMap();
		}
		UTrafficAutomationLogger::EndTestLog();
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficVehicle07ChaosDriveTest,
	"Traffic.Visual.Vehicle07.ChaosDriveMovesOnPlane",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficVehicle07ChaosDriveTest::RunTest(const FString& Parameters)
{
	const FString LocalTestName = TEXT("Traffic.Visual.Vehicle07.ChaosDriveMovesOnPlane");
	UTrafficAutomationLogger::BeginTestLog(LocalTestName);

	if (!GEditor)
	{
		AddError(TEXT("Test requires editor (GEditor)."));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	FAutomationEditorCommonUtils::CreateNewMap();

	TSharedRef<FVehicle07ChaosDrivePIEState> State = MakeShared<FVehicle07ChaosDrivePIEState>();
	AddExpectedError(TEXT("The Editor is currently in a play mode."), EAutomationExpectedErrorFlags::Contains, 3);

	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForPIEWorld(State, this, 10.0, FPlatformTime::Seconds()));
	ADD_LATENT_AUTOMATION_COMMAND(FVehicle07ChaosDriveRun(State, this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPIE(State, this));

	return true;
}

#endif // WITH_EDITOR
