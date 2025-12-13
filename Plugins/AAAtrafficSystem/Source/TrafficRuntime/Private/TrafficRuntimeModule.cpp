#include "TrafficRuntimeModule.h"
#include "TrafficRoadFamilySettings.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

DEFINE_LOG_CATEGORY(LogTraffic);

#define LOCTEXT_NAMESPACE "FTrafficRuntimeModule"

namespace
{
	void PreloadZoneLaneProfilesFromTrafficSettings()
	{
		const UTrafficRoadFamilySettings* FamilySettings = GetDefault<UTrafficRoadFamilySettings>();
		if (!FamilySettings)
		{
			return;
		}

		TSet<FSoftObjectPath> UniquePaths;
		for (const FRoadFamilyDefinition& Family : FamilySettings->Families)
		{
			if (Family.VehicleLaneProfile.IsValid())
			{
				UniquePaths.Add(Family.VehicleLaneProfile);
			}
			if (Family.FootpathLaneProfile.IsValid())
			{
				UniquePaths.Add(Family.FootpathLaneProfile);
			}
		}

		if (UniquePaths.Num() == 0)
		{
			return;
		}

		FStreamableManager& Streamable = UAssetManager::GetStreamableManager();

		int32 LoadedCount = 0;
		for (const FSoftObjectPath& Path : UniquePaths)
		{
			UObject* LoadedObj = Streamable.LoadSynchronous(Path, /*bManageActiveHandle*/ false);
			if (LoadedObj)
			{
				++LoadedCount;
			}
			else
			{
				UE_LOG(LogTraffic, Warning,
					TEXT("[AAA Traffic][ZoneGraph] Failed to preload ZoneLaneProfile asset at '%s'. Create it under the AAAtrafficSystem plugin content if you intend to generate ZoneGraph."),
					*Path.ToString());
			}
		}

		UE_LOG(LogTraffic, Log, TEXT("[AAA Traffic][ZoneGraph] Preloaded %d ZoneLaneProfile assets."), LoadedCount);
	}
}

void FTrafficRuntimeModule::StartupModule()
{
	UE_LOG(LogTraffic, Log, TEXT("[AAA Traffic] Plugin Version = %s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION));
	UE_LOG(LogTraffic, Log, TEXT("TrafficRuntime module started."));

	// Preload ZoneGraph lane profile assets so later ZoneGraph generation does not block on disk loads.
	PreloadZoneLaneProfilesFromTrafficSettings();
}

void FTrafficRuntimeModule::ShutdownModule()
{
	UE_LOG(LogTraffic, Log, TEXT("TrafficRuntime module shutdown."));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTrafficRuntimeModule, TrafficRuntime)
