#include "TrafficGeometryProviderFactory.h"
#include "TrafficGeometryProvider.h"
#include "CityBLDRoadGeometryProvider.h"
#include "TrafficRuntimeModule.h"
#include "Interfaces/IPluginManager.h"

TObjectPtr<UObject> UTrafficGeometryProviderFactory::CreateProvider(
	UWorld* World,
	ITrafficRoadGeometryProvider*& OutInterface)
{
	OutInterface = nullptr;
	UObject* ProviderObject = nullptr;

	const TSharedPtr<IPlugin> CityBLDPlugin = IPluginManager::Get().FindPlugin(TEXT("CityBLD"));
	if (CityBLDPlugin.IsValid() && CityBLDPlugin->IsEnabled())
	{
		UE_LOG(LogTraffic, Log, TEXT("[GeometryProviderFactory] Using CityBLD provider."));
		UCityBLDRoadGeometryProvider* CityProvider =
			NewObject<UCityBLDRoadGeometryProvider>(GetTransientPackage());
		OutInterface = static_cast<ITrafficRoadGeometryProvider*>(CityProvider);
		ProviderObject = CityProvider;
	}
	else
	{
		UE_LOG(LogTraffic, Log, TEXT("[GeometryProviderFactory] Using generic spline provider."));
		UGenericSplineRoadGeometryProvider* GenericProvider =
			NewObject<UGenericSplineRoadGeometryProvider>(GetTransientPackage());
		OutInterface = static_cast<ITrafficRoadGeometryProvider*>(GenericProvider);
		ProviderObject = GenericProvider;
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
	if (CityBLDPlugin.IsValid() && CityBLDPlugin->IsEnabled())
	{
		UCityBLDRoadGeometryProvider* CityProvider = NewObject<UCityBLDRoadGeometryProvider>(GetTransientPackage());
		OutInterfaces.Add(static_cast<ITrafficRoadGeometryProvider*>(CityProvider));
		OutProviders.Add(CityProvider);
	}

	// Always add generic fallback.
	UGenericSplineRoadGeometryProvider* GenericProvider = NewObject<UGenericSplineRoadGeometryProvider>(GetTransientPackage());
	OutInterfaces.Add(static_cast<ITrafficRoadGeometryProvider*>(GenericProvider));
	OutProviders.Add(GenericProvider);
}

