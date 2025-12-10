#include "TrafficVehicleAdapter.h"
#include "TrafficRuntimeModule.h"

ATrafficVehicleAdapter::ATrafficVehicleAdapter()
{
	// Hide the base cube by default; visuals come from the external actor.
	if (Body)
	{
		Body->SetHiddenInGame(true);
		Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ATrafficVehicleAdapter::SetExternalVisualClass(const TSoftClassPtr<AActor>& InClass)
{
	ExternalVehicleClass = InClass;
}

void ATrafficVehicleAdapter::BeginPlay()
{
	Super::BeginPlay();
	EnsureVisualSpawned();
}

void ATrafficVehicleAdapter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	SyncVisualTransform();
}

void ATrafficVehicleAdapter::EnsureVisualSpawned()
{
	if (SpawnedVisual || !GetWorld())
	{
		return;
	}

	UClass* VisualClass = nullptr;
	if (!ExternalVehicleClass.IsNull())
	{
		VisualClass = ExternalVehicleClass.LoadSynchronous();
	}
	if (!VisualClass)
	{
		VisualClass = AActor::StaticClass();
	}

	if (!VisualClass)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[VehicleAdapter] No ExternalVehicleClass set; using base cube visual."));
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	SpawnedVisual = GetWorld()->SpawnActor<AActor>(VisualClass, GetActorTransform(), Params);
	if (!SpawnedVisual)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[VehicleAdapter] Failed to spawn visual actor of class %s"), *VisualClass->GetName());
		return;
	}

	SpawnedVisual->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
	SpawnedVisual->SetActorEnableCollision(false);

	// Also hide the base cube mesh to avoid double visuals.
	if (Body)
	{
		Body->SetHiddenInGame(true);
		Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	UE_LOG(LogTraffic, Log, TEXT("[VehicleAdapter] Spawned visual actor %s for %s"), *SpawnedVisual->GetName(), *GetName());
}

void ATrafficVehicleAdapter::SyncVisualTransform()
{
	if (!SpawnedVisual)
	{
		return;
	}

	SpawnedVisual->SetActorLocationAndRotation(GetActorLocation(), GetActorRotation(), false, nullptr, ETeleportType::TeleportPhysics);
}

void ATrafficVehicleAdapter::EnsureVisualAttached()
{
	EnsureVisualSpawned();
	SyncVisualTransform();
}
