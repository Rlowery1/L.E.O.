#include "TrafficGeometryProviderFactory.h"
#include "TrafficGeometryProvider.h"
#include "CityBLDRoadGeometryProvider.h"
#include "StaticMeshRoadGeometryProvider.h"
#include "RoadFamilyRegistry.h"
#include "TrafficRuntimeModule.h"
#include "TrafficRuntimeSettings.h"
#include "TrafficCityBLDAdapterSettings.h"
#include "UObject/UObjectIterator.h"
#include "Interfaces/IPluginManager.h"
#include "Components/SplineComponent.h"
#include "EngineUtils.h"

namespace
{
	static bool IsCityBLDRoadCandidate(const AActor* Actor, const UTrafficCityBLDAdapterSettings* AdapterSettings)
	{
		if (!Actor)
		{
			return false;
		}

		if (AdapterSettings && !AdapterSettings->RoadActorTag.IsNone() && Actor->ActorHasTag(AdapterSettings->RoadActorTag))
		{
			return true;
		}

		const FString ClassName = Actor->GetClass()->GetName();
		if (ClassName.Contains(TEXT("BP_MeshRoad"), ESearchCase::IgnoreCase))
		{
			return true;
		}

		if (AdapterSettings)
		{
			for (const FString& Contains : AdapterSettings->RoadClassNameContains)
			{
				if (!Contains.IsEmpty() && ClassName.Contains(Contains, ESearchCase::IgnoreCase))
				{
					return true;
				}
			}
		}

		return false;
	}
}

TObjectPtr<UObject> UTrafficGeometryProviderFactory::CreateProvider(
	UWorld* World,
	ITrafficRoadGeometryProvider*& OutInterface)
{
	OutInterface = nullptr;
	UObject* ProviderObject = nullptr;

	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[GeometryProviderFactory] CreateProvider: World is null; using static mesh provider."));
		UStaticMeshRoadGeometryProvider* MeshProvider = NewObject<UStaticMeshRoadGeometryProvider>();
		OutInterface = static_cast<ITrafficRoadGeometryProvider*>(MeshProvider);
		return MeshProvider;
	}

	const UTrafficRuntimeSettings* Settings = GetDefault<UTrafficRuntimeSettings>();
	const bool bCityBLDEnabled = Settings ? Settings->bEnableCityBLDAdapter : true;
	const UTrafficCityBLDAdapterSettings* AdapterSettings = GetDefault<UTrafficCityBLDAdapterSettings>();

	// Detect whether CityBLD roads exist in the world before selecting the CityBLD provider.
	bool bHasCityBLDRoads = false;
	if (bCityBLDEnabled)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (IsCityBLDRoadCandidate(*It, AdapterSettings) && It->FindComponentByClass<USplineComponent>())
			{
				bHasCityBLDRoads = true;

#if WITH_EDITOR
				// When we detect the first CityBLD road, assign a default lane profile for its family if missing.
				// Keep this behavior scoped to BP_MeshRoad to avoid creating unexpected families for other spline actors.
				if (It->GetClass()->GetName().Contains(TEXT("BP_MeshRoad"), ESearchCase::IgnoreCase))
				{
					URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get();
					if (Registry)
					{
						FRoadFamilyInfo* Info = Registry->FindOrCreateFamilyForClass(It->GetClass(), nullptr);
						if (Info && !Info->FamilyDefinition.VehicleLaneProfile.IsValid())
						{
							// Set a default vehicle lane profile for CityBLD roads (used for ZoneGraph + calibration overlay).
							// Prefer adapter settings so projects can override via config.
							Info->FamilyDefinition.VehicleLaneProfile = (AdapterSettings && AdapterSettings->DefaultCityBLDVehicleLaneProfile.IsValid())
								? AdapterSettings->DefaultCityBLDVehicleLaneProfile
								: FSoftObjectPath(TEXT("/AAAtrafficSystem/ZoneProfiles/CityBLDUrbanTwoLane.CityBLDUrbanTwoLane"));

							// Also set reasonable default lane counts (1 lane forward, 1 lane backward).
							Info->FamilyDefinition.Forward.NumLanes = 1;
							Info->FamilyDefinition.Backward.NumLanes = 1;

							Registry->RefreshCache();
							Registry->SaveConfig();

							UE_LOG(LogTraffic, Log, TEXT("[GeometryProviderFactory] Assigned CityBLD default lane profile to family %s"), *Info->DisplayName);
						}
					}
				}
#endif

				break;
			}
		}
	}

	if (bHasCityBLDRoads)
	{
		UE_LOG(LogTraffic, Log, TEXT("[GeometryProviderFactory] Detected CityBLD roads; using CityBLD provider."));
		UCityBLDRoadGeometryProvider* CityProvider = NewObject<UCityBLDRoadGeometryProvider>();
		OutInterface = static_cast<ITrafficRoadGeometryProvider*>(CityProvider);
		ProviderObject = CityProvider;
	}
	else
	{
		if (!bCityBLDEnabled)
		{
			UE_LOG(LogTraffic, Log, TEXT("[GeometryProviderFactory] CityBLD adapter disabled; using static mesh provider."));
		}
		else
		{
			UE_LOG(LogTraffic, Log, TEXT("[GeometryProviderFactory] No CityBLD roads detected; using static mesh provider."));
		}

		UStaticMeshRoadGeometryProvider* MeshProvider = NewObject<UStaticMeshRoadGeometryProvider>();
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

	if (!World)
	{
		UStaticMeshRoadGeometryProvider* MeshProvider = NewObject<UStaticMeshRoadGeometryProvider>();
		OutInterfaces.Add(static_cast<ITrafficRoadGeometryProvider*>(MeshProvider));
		OutProviders.Add(MeshProvider);
		return;
	}

	const UTrafficRuntimeSettings* Settings = GetDefault<UTrafficRuntimeSettings>();
	const bool bCityBLDEnabled = Settings ? Settings->bEnableCityBLDAdapter : true;
	const UTrafficCityBLDAdapterSettings* AdapterSettings = GetDefault<UTrafficCityBLDAdapterSettings>();

	bool bHasCityBLDRoads = false;
	if (bCityBLDEnabled)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (IsCityBLDRoadCandidate(*It, AdapterSettings) && It->FindComponentByClass<USplineComponent>())
			{
				bHasCityBLDRoads = true;
				break;
			}
		}
	}

	if (bHasCityBLDRoads)
	{
		UCityBLDRoadGeometryProvider* CityProvider = NewObject<UCityBLDRoadGeometryProvider>();
		OutInterfaces.Add(static_cast<ITrafficRoadGeometryProvider*>(CityProvider));
		OutProviders.Add(CityProvider);
	}

	// Always add static mesh provider as fallback.
	UStaticMeshRoadGeometryProvider* MeshProvider = NewObject<UStaticMeshRoadGeometryProvider>();
	OutInterfaces.Add(static_cast<ITrafficRoadGeometryProvider*>(MeshProvider));
	OutProviders.Add(MeshProvider);
}
