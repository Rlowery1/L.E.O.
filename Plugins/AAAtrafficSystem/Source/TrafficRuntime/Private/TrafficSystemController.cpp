#include "TrafficSystemController.h"
#include "TrafficRuntimeModule.h"
#include "TrafficNetworkBuilder.h"
#include "TrafficRoadFamilySettings.h"
#include "TrafficGeometryProviderFactory.h"
#include "TrafficGeometryProvider.h"
#include "TrafficNetworkAsset.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#endif

ATrafficSystemController::ATrafficSystemController()
{
	PrimaryActorTick.bCanEverTick = false;
	BuiltNetworkAsset = nullptr;
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

	const UTrafficRoadFamilySettings* FamSettings = GetDefault<UTrafficRoadFamilySettings>();
	if (!FamSettings || FamSettings->Families.Num() == 0)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficSystemController] No road families configured."));
		return;
	}

	ITrafficRoadGeometryProvider* ProviderInterface = nullptr;
	TObjectPtr<UObject> ProviderObject = UTrafficGeometryProviderFactory::CreateProvider(World, ProviderInterface);
	if (!ProviderObject || !ProviderInterface)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficSystemController] Failed to create geometry provider."));
		return;
	}

	FTrafficNetwork Network;
	FTrafficNetworkBuilder::BuildNetworkFromWorld(
		World,
		*ProviderInterface,
		FamSettings,
		Network);

	if (!BuiltNetworkAsset)
	{
		BuiltNetworkAsset = NewObject<UTrafficNetworkAsset>(this, TEXT("TrafficNetwork_Transient"));
	}
	BuiltNetworkAsset->Network = Network;

	UE_LOG(LogTraffic, Log,
		TEXT("[TrafficSystemController] Built transient network: Roads=%d Lanes=%d Intersections=%d Movements=%d"),
		Network.Roads.Num(),
		Network.Lanes.Num(),
		Network.Intersections.Num(),
		Network.Movements.Num());
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
