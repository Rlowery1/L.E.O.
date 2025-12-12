#include "TrafficGeometryProviderFactory.h"
#include "TrafficGeometryProvider.h"
#include "CityBLDRoadGeometryProvider.h"
#include "StaticMeshRoadGeometryProvider.h"
#include "TrafficRuntimeModule.h"
#include "TrafficRuntimeSettings.h"
#include "UObject/UObjectIterator.h"
#include "Interfaces/IPluginManager.h"

TObjectPtr<UObject> UTrafficGeometryProviderFactory::CreateProvider(
	UWorld* World,
	ITrafficRoadGeometryProvider*& OutInterface)
{
	OutInterface = nullptr;
	UObject* ProviderObject = nullptr;

	if (GetDefault<UTrafficRuntimeSettings>()->bEnableCityBLDAdapter)
	{
		for (TObjectIterator<UClass> It; It; ++It)
		{
			if (It->GetName().Contains(TEXT("BP_MeshRoad")))
			{
				UE_LOG(LogTraffic, Log, TEXT("[GeometryProviderFactory] Found BP_MeshRoad class; using CityBLD provider."));
				UCityBLDRoadGeometryProvider* CityProvider =
					NewObject<UCityBLDRoadGeometryProvider>(GetTransientPackage());
				OutInterface = static_cast<ITrafficRoadGeometryProvider*>(CityProvider);
				ProviderObject = CityProvider;
				return ProviderObject;
			}
		}
	}

	UE_LOG(LogTraffic, Log, TEXT("[GeometryProviderFactory] Using static mesh provider."));
	UStaticMeshRoadGeometryProvider* MeshProvider =
		NewObject<UStaticMeshRoadGeometryProvider>(GetTransientPackage());
	OutInterface = static_cast<ITrafficRoadGeometryProvider*>(MeshProvider);
	ProviderObject = MeshProvider;

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
