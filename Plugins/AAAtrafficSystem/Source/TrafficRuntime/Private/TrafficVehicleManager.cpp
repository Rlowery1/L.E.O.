#include "TrafficVehicleManager.h"
#include "TrafficVehicleBase.h"
#include "TrafficNetworkAsset.h"
#include "TrafficRoadTypes.h"
#include "TrafficSystemController.h"
#include "TrafficRuntimeModule.h"
#include "TrafficVehicleSettings.h"
#include "TrafficVehicleAdapter.h"
#include "TrafficVehicleProfile.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

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
	DestroyAdaptersAndVisuals();

	for (ATrafficVehicleBase* Vehicle : Vehicles)
	{
		if (Vehicle)
		{
			Vehicle->Destroy();
		}
	}
	Vehicles.Empty();
	LastLaneSpawnTimes.Empty();
}

void ATrafficVehicleManager::DestroyAdaptersAndVisuals()
{
	for (TWeakObjectPtr<ATrafficVehicleAdapter>& Adapter : Adapters)
	{
		if (Adapter.IsValid())
		{
			Adapter->Destroy();
		}
	}
	Adapters.Empty();

	for (TWeakObjectPtr<APawn>& Visual : VisualVehicles)
	{
		if (Visual.IsValid())
		{
			Visual->Destroy();
		}
	}
	VisualVehicles.Empty();
}

void ATrafficVehicleManager::SetActiveRunMetrics(FTrafficRunMetrics* InMetrics)
{
	ActiveMetrics = InMetrics;
}

void ATrafficVehicleManager::SetForceLogicOnlyForTests(bool bInForce)
{
	bForceLogicOnlyForTests = bInForce;
}

const UTrafficVehicleProfile* ATrafficVehicleManager::ResolveDefaultVehicleProfile() const
{
	const UTrafficVehicleSettings* Settings = UTrafficVehicleSettings::Get();
	if (!Settings)
	{
		return nullptr;
	}

	if (Settings->DefaultVehicleProfile.IsNull())
	{
		return nullptr;
	}

	UObject* Loaded = Settings->DefaultVehicleProfile.TryLoad();
	const UTrafficVehicleProfile* Profile = Cast<UTrafficVehicleProfile>(Loaded);
	if (!Profile)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[VehicleManager] DefaultVehicleProfile could not be loaded: %s"), *Settings->DefaultVehicleProfile.ToString());
	}
	return Profile;
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

	const UTrafficVehicleProfile* Profile = ResolveDefaultVehicleProfile();
	if (Profile && !bForceLogicOnlyForTests)
	{
		UE_LOG(LogTraffic, Log, TEXT("[VehicleManager] Using vehicle profile '%s' (%s)."),
			*Profile->GetName(),
			Profile->VehicleClass.IsNull() ? TEXT("<no class>") : *Profile->VehicleClass.ToString());
	}

	TSubclassOf<ATrafficVehicleBase> LogicClass = ATrafficVehicleBase::StaticClass();
	TSubclassOf<APawn> VisualClass = nullptr;
	if (!bForceLogicOnlyForTests && Profile && Profile->VehicleClass.IsValid())
	{
		VisualClass = Profile->VehicleClass.LoadSynchronous();
	}

	const TArray<FTrafficLane>& Lanes = NetworkAsset->Network.Lanes;
	LastLaneSpawnTimes.Empty();

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

			ATrafficVehicleBase* Vehicle = World->SpawnActor<ATrafficVehicleBase>(LogicClass);
			if (!Vehicle)
			{
				UE_LOG(LogTraffic, Warning, TEXT("[VehicleManager] Failed to spawn vehicle for lane %d (class %s)."), LaneIndex, *LogicClass->GetName());
				continue;
			}

			const int32 LaneKey = (Lane.LaneId >= 0) ? Lane.LaneId : LaneIndex;
			const float Now = World->GetTimeSeconds();
			if (ActiveMetrics)
			{
				if (const float* LastTime = LastLaneSpawnTimes.Find(LaneKey))
				{
					ActiveMetrics->AccumulateHeadway(Now - *LastTime);
				}
			}
			LastLaneSpawnTimes.Add(LaneKey, Now);

			Vehicle->InitializeOnLane(&Lane, InitialS, SpeedCmPerSec);
			Vehicles.Add(Vehicle);
			if (ActiveMetrics)
			{
				ActiveMetrics->VehiclesSpawned++;
			}

			if (VisualClass)
			{
				FActorSpawnParameters Params;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				Params.Owner = this;
				APawn* VisualPawn = World->SpawnActor<APawn>(VisualClass, Vehicle->GetActorTransform(), Params);
				if (VisualPawn)
				{
					VisualPawn->SetActorEnableCollision(false);
					ATrafficVehicleAdapter* Adapter = World->SpawnActor<ATrafficVehicleAdapter>(Params);
					if (Adapter)
					{
						Adapter->Initialize(Vehicle, VisualPawn);
						Adapters.Add(Adapter);
					}
					VisualVehicles.Add(VisualPawn);
				}
			}
		}
	}

	if (!bForceLogicOnlyForTests && (!Profile || !VisualClass))
	{
		UE_LOG(LogTraffic, Warning,
			TEXT("[VehicleManager] No valid Chaos vehicle class configured. "
				 "Spawning logic-only TrafficVehicleBase pawns (debug cubes) without visual Chaos vehicles.\n"
				 "Configure a DefaultVehicleProfile and VehicleClass in Project Settings -> AAA Traffic Vehicle Settings for full Chaos visuals."));
	}

	UE_LOG(LogTraffic, Log, TEXT("[VehicleManager] Spawned %d vehicles."), Vehicles.Num());
}
