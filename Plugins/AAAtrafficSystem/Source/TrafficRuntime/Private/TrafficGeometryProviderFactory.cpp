#include "TrafficGeometryProviderFactory.h"
#include "TrafficGeometryProvider.h"
#include "StaticMeshRoadGeometryProvider.h"
#include "TrafficRuntimeModule.h"

TObjectPtr<UObject> UTrafficGeometryProviderFactory::CreateProvider(
	UWorld* World,
	ITrafficRoadGeometryProvider*& OutInterface)
{
	OutInterface = nullptr;
	UObject* ProviderObject = nullptr;

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

	// Single provider chain: static mesh provider handles all roads. Optionally add spline fallback here if needed.
	UStaticMeshRoadGeometryProvider* MeshProvider = NewObject<UStaticMeshRoadGeometryProvider>(GetTransientPackage());
	OutInterfaces.Add(static_cast<ITrafficRoadGeometryProvider*>(MeshProvider));
	OutProviders.Add(MeshProvider);
}
