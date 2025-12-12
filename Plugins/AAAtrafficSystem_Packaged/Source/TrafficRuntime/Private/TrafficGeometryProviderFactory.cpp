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

	// FORCE CityBLD provider for now to verify it runs. Always return it.
	UCityBLDRoadGeometryProvider* CityProvider = NewObject<UCityBLDRoadGeometryProvider>();
	OutInterface = static_cast<ITrafficRoadGeometryProvider*>(CityProvider);
	ProviderObject = CityProvider;
	UE_LOG(LogTraffic, Warning, TEXT("[Factory] Forcing CityBLD provider at runtime."));
	return ProviderObject;
}

void UTrafficGeometryProviderFactory::CreateProviderChainForEditorWorld(
	UWorld* World,
	TArray<TObjectPtr<UObject>>& OutProviders,
	TArray<ITrafficRoadGeometryProvider*>& OutInterfaces)
{
	OutProviders.Empty();
	OutInterfaces.Empty();

	// FORCE CityBLD provider first for the editor.  Always add it to the chain.
	UCityBLDRoadGeometryProvider* CityProvider = NewObject<UCityBLDRoadGeometryProvider>();
	OutInterfaces.Add(static_cast<ITrafficRoadGeometryProvider*>(CityProvider));
	OutProviders.Add(CityProvider);
	UE_LOG(LogTraffic, Warning, TEXT("[FactoryEditor] Forcing CityBLD provider in editor chain."));

	// Add static mesh provider as fallback in case CityBLD provider fails
	UStaticMeshRoadGeometryProvider* MeshProvider = NewObject<UStaticMeshRoadGeometryProvider>();
	OutInterfaces.Add(static_cast<ITrafficRoadGeometryProvider*>(MeshProvider));
	OutProviders.Add(MeshProvider);
}
