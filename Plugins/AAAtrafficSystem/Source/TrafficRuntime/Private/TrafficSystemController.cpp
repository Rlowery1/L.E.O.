#include "TrafficSystemController.h"
#include "TrafficGeometryProvider.h"
#include "TrafficGeometryProviderFactory.h"
#include "TrafficNetworkAsset.h"
#include "TrafficNetworkBuilder.h"
#include "TrafficRoadFamilySettings.h"
#include "TrafficRuntimeModule.h"
#include "TrafficVehicleManager.h"
#include "TrafficZoneGraphAdapter.h"
#include "TrafficRuntimeSettings.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "TrafficVehicleBase.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#endif

ATrafficSystemController::ATrafficSystemController()
{
	PrimaryActorTick.bCanEverTick = false;
	BuiltNetworkAsset = nullptr;
}

bool ATrafficSystemController::BuildNetworkInternal(UWorld* World)
{
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficSystemController] BuildNetworkInternal: World is null."));
		return false;
	}

	const UTrafficRoadFamilySettings* FamSettings = GetDefault<UTrafficRoadFamilySettings>();
	if (!FamSettings || FamSettings->Families.Num() == 0)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficSystemController] No road families configured."));
		return false;
	}

	ITrafficRoadGeometryProvider* ProviderInterface = nullptr;
	TObjectPtr<UObject> ProviderObject = UTrafficGeometryProviderFactory::CreateProvider(World, ProviderInterface);
	if (!ProviderObject || !ProviderInterface)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficSystemController] Failed to create geometry provider."));
		return false;
	}

	FTrafficNetwork Network;
	FTrafficNetworkBuilder::BuildNetworkFromWorld(
		World,
		*ProviderInterface,
		FamSettings,
		Network);

	if (bGenerateZoneGraph)
	{
		FTrafficZoneGraphAdapter::BuildZoneGraphForNetwork(World, Network, FamSettings);
	}

	if (!BuiltNetworkAsset)
	{
		BuiltNetworkAsset = NewObject<UTrafficNetworkAsset>(this, TEXT("TrafficNetwork_Transient"));
	}
	BuiltNetworkAsset->Network = Network;
	IntersectionReservations.Reset();

	UE_LOG(LogTraffic, Log,
		TEXT("[TrafficSystemController] Built transient network: Roads=%d Lanes=%d Intersections=%d Movements=%d"),
		Network.Roads.Num(),
		Network.Lanes.Num(),
		Network.Intersections.Num(),
		Network.Movements.Num());

	return Network.Roads.Num() > 0 && Network.Lanes.Num() > 0;
}

void ATrafficSystemController::Editor_BuildTrafficNetwork()
{
#if WITH_EDITOR
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficSystemController] Editor_BuildTrafficNetwork: World is null."));
		return;
	}

	UE_LOG(LogTraffic, Log, TEXT("[TrafficSystemController] Editor_BuildTrafficNetwork: BEGIN"));
	BuildNetworkInternal(World);
#endif
}

void ATrafficSystemController::Editor_SpawnTestVehicles()
{
#if WITH_EDITOR
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficSystemController] Editor_SpawnTestVehicles: World is null."));
		return;
	}

	UE_LOG(LogTraffic, Log, TEXT("[TrafficSystemController] Editor_SpawnTestVehicles: Stub - vehicle spawning not yet implemented."));
#endif
}

void ATrafficSystemController::Runtime_BuildTrafficNetwork()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficSystemController] Runtime_BuildTrafficNetwork: World is null."));
		return;
	}

	BuildNetworkInternal(World);
}

void ATrafficSystemController::Runtime_SpawnTraffic()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficSystemController] Runtime_SpawnTraffic: World is null."));
		return;
	}

	// Find or spawn a TrafficVehicleManager in this world.
	ATrafficVehicleManager* Manager = nullptr;
	for (TActorIterator<ATrafficVehicleManager> It(World); It; ++It)
	{
		Manager = *It;
		break;
	}

	if (!Manager)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Manager = World->SpawnActor<ATrafficVehicleManager>(ATrafficVehicleManager::StaticClass(), FTransform::Identity, Params);
	}

	if (!Manager)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficSystemController] Runtime_SpawnTraffic: Failed to spawn TrafficVehicleManager."));
		return;
	}

	// Runtime uses Chaos visuals when configured; no logic-only forcing here.
	Manager->SetForceLogicOnlyForTests(false);
	if (bGenerateZoneGraph)
	{
		Manager->SpawnZoneGraphVehicles(VehiclesPerLaneRuntime, RuntimeSpeedCmPerSec, FName(TEXT("Vehicles")));

		// Fallback: if ZoneGraph produced nothing (e.g. no ZoneGraphData yet), keep existing spline-based spawning working.
		if (Manager->GetSpawnedVehicleCount() == 0)
		{
			UE_LOG(LogTraffic, Warning, TEXT("[TrafficSystemController] ZoneGraph spawn produced 0 vehicles; falling back to TrafficNetwork lane spawning."));
			Manager->SpawnTestVehicles(VehiclesPerLaneRuntime, RuntimeSpeedCmPerSec);
		}
	}
	else
	{
		Manager->SpawnTestVehicles(VehiclesPerLaneRuntime, RuntimeSpeedCmPerSec);
	}
}

void ATrafficSystemController::SetRuntimeConfigFromProjectSettings()
{
	const UTrafficRuntimeSettings* Settings = GetDefault<UTrafficRuntimeSettings>();
	if (!Settings)
	{
		return;
	}

	VehiclesPerLaneRuntime = FMath::Clamp(Settings->VehiclesPerLaneRuntime, 0, 50);
	RuntimeSpeedCmPerSec = FMath::Clamp(Settings->RuntimeSpeedCmPerSec, 0.f, 100000.f);
	bGenerateZoneGraph = Settings->bGenerateZoneGraph;
}

void ATrafficSystemController::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Only auto-run in PIE/Game worlds, not the pure Editor world.
	const EWorldType::Type WorldType = World->WorldType;
	if (WorldType == EWorldType::Editor)
	{
		return;
	}

	if (bAutoBuildOnBeginPlay)
	{
		SetRuntimeConfigFromProjectSettings();
		Runtime_BuildTrafficNetwork();
	}

	if (bAutoSpawnOnBeginPlay)
	{
		SetRuntimeConfigFromProjectSettings();
		Runtime_SpawnTraffic();
	}
}

int32 ATrafficSystemController::GetNumRoads() const
{
	return BuiltNetworkAsset ? BuiltNetworkAsset->Network.Roads.Num() : 0;
}

int32 ATrafficSystemController::GetNumLanes() const
{
	return BuiltNetworkAsset ? BuiltNetworkAsset->Network.Lanes.Num() : 0;
}

bool ATrafficSystemController::TryReserveIntersection(int32 IntersectionId, ATrafficVehicleBase* Vehicle, int32 MovementId, float HoldSeconds)
{
	if (IntersectionId < 0 || !Vehicle)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const float Now = World->GetTimeSeconds();
	const float ExpireAt = Now + FMath::Max(0.1f, HoldSeconds);

	FIntersectionReservation& Res = IntersectionReservations.FindOrAdd(IntersectionId);

	// If current holder is gone/expired, allow takeover.
	const bool bExpired = (Res.ExpireTimeSeconds > 0.f && Now > Res.ExpireTimeSeconds);
	const bool bInvalid = !Res.Vehicle.IsValid();

	if (bExpired || bInvalid || Res.Vehicle.Get() == Vehicle)
	{
		Res.Vehicle = Vehicle;
		Res.MovementId = MovementId;
		Res.ExpireTimeSeconds = ExpireAt;
		return true;
	}

	return false;
}

void ATrafficSystemController::ReleaseIntersection(int32 IntersectionId, ATrafficVehicleBase* Vehicle)
{
	if (IntersectionId < 0 || !Vehicle)
	{
		return;
	}

	FIntersectionReservation* Res = IntersectionReservations.Find(IntersectionId);
	if (!Res)
	{
		return;
	}

	if (!Res->Vehicle.IsValid() || Res->Vehicle.Get() == Vehicle)
	{
		IntersectionReservations.Remove(IntersectionId);
	}
}
