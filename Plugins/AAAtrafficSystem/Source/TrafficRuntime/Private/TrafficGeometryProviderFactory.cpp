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

	// Check for CityBLD adapter.
	const bool bEnableAdapter = GetDefault<UTrafficRuntimeSettings>()->bEnableCityBLDAdapter;
	bool bFoundCityBLDRoad = false;
	if (bEnableAdapter)
	{
		for (TObjectIterator<UClass> It; It; ++It)
		{
			if (It->GetName().Contains(TEXT("BP_MeshRoad")))
			{
				bFoundCityBLDRoad = true;
				break;
			}
		}
	}

	UE_LOG(LogTraffic, Warning,
		TEXT("[Factory] bEnableCityBLDAdapter=%s, bFoundCityBLDRoad=%s"),
		bEnableAdapter ? TEXT("true") : TEXT("false"),
		bFoundCityBLDRoad ? TEXT("true") : TEXT("false"));

	if (bEnableAdapter && bFoundCityBLDRoad)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[Factory] Instantiating CityBLD provider."));
		UCityBLDRoadGeometryProvider* CityProvider =
			NewObject<UCityBLDRoadGeometryProvider>(GetTransientPackage());
		OutInterface = static_cast<ITrafficRoadGeometryProvider*>(CityProvider);
		ProviderObject = CityProvider;
		return ProviderObject;
	}

	UE_LOG(LogTraffic, Warning, TEXT("[Factory] Instantiating StaticMeshRoadGeometryProvider."));
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
