#include "TrafficVehicleAdapter.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/MovementComponent.h"
#include "Components/PrimitiveComponent.h"
#include "TrafficRuntimeModule.h"
#include "TrafficVehicleBase.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarTrafficKinematicChaosVisuals(
	TEXT("aaa.Traffic.Visual.KinematicChaosVisuals"),
	1,
	TEXT("If non-zero, AAA Traffic will treat spawned Chaos vehicle pawns as kinematic visuals:\n")
	TEXT("  - disables physics simulation on their primitive components\n")
	TEXT("  - disables movement components\n")
	TEXT("  - keeps collision enabled\n")
	TEXT("This prevents jitter/spinning caused by teleport-following active Chaos physics vehicles.\n")
	TEXT("Default: 1"),
	ECVF_Default);

ATrafficVehicleAdapter::ATrafficVehicleAdapter()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATrafficVehicleAdapter::Initialize(ATrafficVehicleBase* InLogic, APawn* InChaos)
{
	LogicVehicle = InLogic;
	ChaosVehicle = InChaos;

	if (ChaosVehicle.IsValid())
	{
		// By default, use kinematic-follow mode for Chaos pawns. Teleporting a fully simulated Chaos vehicle each tick
		// tends to cause spinning/jitter and looks broken in PIE.
		const bool bKinematic = (CVarTrafficKinematicChaosVisuals.GetValueOnGameThread() != 0);

		ChaosVehicle->SetActorEnableCollision(true);

		if (bKinematic)
		{
			TArray<UMovementComponent*> MoveComps;
			ChaosVehicle->GetComponents(MoveComps);
			for (UMovementComponent* Move : MoveComps)
			{
				if (Move)
				{
					Move->Deactivate();
					Move->SetComponentTickEnabled(false);
				}
			}

			TArray<UPrimitiveComponent*> PrimComps;
			ChaosVehicle->GetComponents(PrimComps);
			for (UPrimitiveComponent* Prim : PrimComps)
			{
				if (!Prim)
				{
					continue;
				}
				if (Prim->IsSimulatingPhysics())
				{
					Prim->SetSimulatePhysics(false);
				}
				Prim->SetEnableGravity(false);
				Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			}
		}
	}
}

void ATrafficVehicleAdapter::BeginPlay()
{
	Super::BeginPlay();
}

void ATrafficVehicleAdapter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!LogicVehicle.IsValid() || !ChaosVehicle.IsValid())
	{
		return;
	}

	// Simple transform-follow mode for deterministic tests.
	ChaosVehicle->SetActorLocationAndRotation(
		LogicVehicle->GetActorLocation(),
		LogicVehicle->GetActorRotation(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
}
