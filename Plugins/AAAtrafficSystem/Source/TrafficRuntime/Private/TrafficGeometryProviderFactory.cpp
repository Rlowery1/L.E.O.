#include "TrafficGeometryProviderFactory.h"
#include "TrafficGeometryProvider.h"
#include "CityBLDRoadGeometryProvider.h"
#include "StaticMeshRoadGeometryProvider.h"
#include "TrafficRuntimeModule.h"
#include "Interfaces/IPluginManager.h"

TObjectPtr<UObject> UTrafficGeometryProviderFactory::CreateProvider(
	UWorld* World,
	ITrafficRoadGeometryProvider*& OutInterface)
{
	OutInterface = nullptr;
	UObject* ProviderObject = nullptr;

	const TSharedPtr<IPlugin> CityBLDPlugin = IPluginManager::Get().FindPlugin(TEXT("CityBLD"));
	const bool bUseCityBLD = CityBLDPlugin.IsValid() && CityBLDPlugin->IsEnabled();

	if (bUseCityBLD)
	{
		UE_LOG(LogTraffic, Log, TEXT("[GeometryProviderFactory] Using CityBLD geometry provider."));
		UCityBLDRoadGeometryProvider* CityProvider =
			NewObject<UCityBLDRoadGeometryProvider>(GetTransientPackage());
		OutInterface = static_cast<ITrafficRoadGeometryProvider*>(CityProvider);
		ProviderObject = CityProvider;
	}
	else
	{
		UE_LOG(LogTraffic, Log, TEXT("[GeometryProviderFactory] Using static mesh provider."));
		UStaticMeshRoadGeometryProvider* MeshProvider =
			NewObject<UStaticMeshRoadGeometryProvider>(GetTransientPackage());
		OutInterface = static_cast<ITrafficRoadGeometryProvider*>(MeshProvider);
		ProviderObject = MeshProvider;
	}

	return ProviderObject;
}

void UTrafficGeometryProviderFactory::CreateProviderChainForEditorWorld(
	UWorld* World,
	TArray<TObjectPtr<UObject>>& OutProviders,
	TArray<ITrafficRoadGeometryProvider*>& OutInterfaces)
{
	OutProviders.Empty();
	OutInterfaces.Empty();

	const TSharedPtr<IPlugin> CityBLDPlugin = IPluginManager::Get().FindPlugin(TEXT("CityBLD"));
	const bool bUseCityBLD = CityBLDPlugin.IsValid() && CityBLDPlugin->IsEnabled();

	if (bUseCityBLD)
	{
		UCityBLDRoadGeometryProvider* CityProvider = NewObject<UCityBLDRoadGeometryProvider>(GetTransientPackage());
		OutInterfaces.Add(static_cast<ITrafficRoadGeometryProvider*>(CityProvider));
		OutProviders.Add(CityProvider);
	}

	UStaticMeshRoadGeometryProvider* MeshProvider = NewObject<UStaticMeshRoadGeometryProvider>(GetTransientPackage());
	OutInterfaces.Add(static_cast<ITrafficRoadGeometryProvider*>(MeshProvider));
	OutProviders.Add(MeshProvider);
}
