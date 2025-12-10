#include "TrafficVehicleManager.h"
#include "TrafficVehicleBase.h"
#include "TrafficNetworkAsset.h"
#include "TrafficRoadTypes.h"
#include "TrafficSystemController.h"
#include "TrafficRuntimeModule.h"
#include "TrafficVehicleSettings.h"
#include "TrafficVehicleAdapter.h"
#include "EngineUtils.h"
#include "Engine/World.h"

ATrafficVehicleManager::ATrafficVehicleManager()
{
	PrimaryActorTick.bCanEverTick = false;
	NetworkAsset = nullptr;
}

void ATrafficVehicleManager::BeginPlay()
{
	Super::BeginPlay();
}

UTrafficNetworkAsset* ATrafficVehicleManager::FindNetworkAsset() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ATrafficSystemController> It(World); It; ++It)
	{
		if (ATrafficSystemController* Controller = *It)
		{
			return Controller->GetBuiltNetworkAsset();
		}
	}

	return nullptr;
}

bool ATrafficVehicleManager::LoadNetwork()
{
	NetworkAsset = FindNetworkAsset();
	if (!NetworkAsset)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[VehicleManager] No network asset available."));
		return false;
	}
	return true;
}

void ATrafficVehicleManager::ClearVehicles()
{
	for (ATrafficVehicleBase* Vehicle : Vehicles)
	{
		if (Vehicle)
		{
			Vehicle->Destroy();
		}
	}
	Vehicles.Empty();
}

void ATrafficVehicleManager::SpawnTestVehicles(int32 VehiclesPerLane, float SpeedCmPerSec)
{
	if (!LoadNetwork())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ClearVehicles();

	const UTrafficVehicleSettings* VehicleSettings = GetDefault<UTrafficVehicleSettings>();
	const bool bUseAdapter = VehicleSettings && VehicleSettings->bUseExternalVehicleAdapter;

	TSoftClassPtr<AActor> ResolvedVisual;
	if (bUseAdapter)
	{
		if (VehicleSettings->ExternalVehicleClass.IsValid())
		{
			ResolvedVisual = VehicleSettings->ExternalVehicleClass;
		}
		else
		{
			ResolvedVisual = UTrafficVehicleSettings::ResolveCitySampleDefaultVisual();
			if (!ResolvedVisual.IsNull())
			{
				UE_LOG(LogTraffic, Log, TEXT("[VehicleManager] Auto-resolved City Sample Chaos visual: %s"), *ResolvedVisual.ToString());
			}
			else
			{
				ResolvedVisual = TSoftClassPtr<AActor>(AActor::StaticClass());
				UE_LOG(LogTraffic, Warning, TEXT("[VehicleManager] Adapter enabled but no ExternalVehicleClass set and City Sample not found; using placeholder Actor."));
			}
		}
	}

	TSubclassOf<ATrafficVehicleBase> VehicleClass = bUseAdapter
		? ATrafficVehicleAdapter::StaticClass()
		: ATrafficVehicleBase::StaticClass();

	if (!bUseAdapter && VehicleSettings && VehicleSettings->DefaultTestVehicleClass.IsValid())
	{
		if (TSubclassOf<ATrafficVehicleBase> UserClass = VehicleSettings->DefaultTestVehicleClass.LoadSynchronous())
		{
			VehicleClass = UserClass;
		}
	}

	if (!VehicleClass)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[VehicleManager] No vehicle class resolved; using ATrafficVehicleBase placeholder."));
		VehicleClass = ATrafficVehicleBase::StaticClass();
	}
	else if (bUseAdapter)
	{
		UE_LOG(LogTraffic, Log, TEXT("[VehicleManager] Using adapter vehicle; visual class=%s"),
			ResolvedVisual.IsNull() ? TEXT("<none>") : *ResolvedVisual.ToString());
	}
	else
	{
		UE_LOG(LogTraffic, Log, TEXT("[VehicleManager] Using vehicle class: %s"), *VehicleClass->GetName());
	}

	const TArray<FTrafficLane>& Lanes = NetworkAsset->Network.Lanes;

	for (int32 LaneIndex = 0; LaneIndex < Lanes.Num(); ++LaneIndex)
	{
		const FTrafficLane& Lane = Lanes[LaneIndex];

		if (Lane.CenterlinePoints.Num() < 2)
		{
			continue;
		}

		for (int32 i = 0; i < VehiclesPerLane; ++i)
		{
			const float SpacingCm = 5000.f;
			const float InitialS = i * SpacingCm;

			ATrafficVehicleBase* Vehicle = World->SpawnActor<ATrafficVehicleBase>(VehicleClass);
			if (!Vehicle)
			{
				UE_LOG(LogTraffic, Warning, TEXT("[VehicleManager] Failed to spawn vehicle for lane %d (class %s)."), LaneIndex, *VehicleClass->GetName());
				continue;
			}

			if (bUseAdapter)
			{
				if (ATrafficVehicleAdapter* Adapter = Cast<ATrafficVehicleAdapter>(Vehicle))
				{
					Adapter->SetExternalVisualClass(ResolvedVisual);
					Adapter->EnsureVisualAttached();
				}
			}

			Vehicle->InitializeOnLane(&Lane, InitialS, SpeedCmPerSec);
			Vehicles.Add(Vehicle);
		}
	}

	UE_LOG(LogTraffic, Log, TEXT("[VehicleManager] Spawned %d vehicles."), Vehicles.Num());
}

