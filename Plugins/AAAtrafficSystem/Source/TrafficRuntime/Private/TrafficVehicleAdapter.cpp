#include "TrafficVehicleAdapter.h"
#include "TrafficRuntimeModule.h"
#include "TrafficVehicleBase.h"

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
		ChaosVehicle->SetActorEnableCollision(false);
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
