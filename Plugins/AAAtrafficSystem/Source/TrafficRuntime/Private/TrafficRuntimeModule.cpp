#include "TrafficRuntimeModule.h"
#include "TrafficRoadFamilySettings.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Misc/CoreDelegates.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogTraffic);

#define LOCTEXT_NAMESPACE "FTrafficRuntimeModule"

namespace
{
	bool PreloadZoneLaneProfilesFromTrafficSettings()
	{
		const UTrafficRoadFamilySettings* FamilySettings = GetDefault<UTrafficRoadFamilySettings>();
		if (!FamilySettings)
		{
			return true;
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
			return true;
		}

		// UAssetManager can exist but not be initialized yet during early engine startup. Defer in that case.
		if (!UAssetManager::IsInitialized())
		{
			UE_LOG(LogTraffic, Verbose, TEXT("[AAA Traffic][ZoneGraph] AssetManager not initialized yet; deferring TrafficZoneLaneProfile preloading."));
			return false;
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
					TEXT("[AAA Traffic][ZoneGraph] Failed to preload TrafficZoneLaneProfile asset at '%s'. Create it under the AAAtrafficSystem plugin content if you intend to generate ZoneGraph."),
					*Path.ToString());
			}
		}

		UE_LOG(LogTraffic, Log, TEXT("[AAA Traffic][ZoneGraph] Preloaded %d TrafficZoneLaneProfile assets."), LoadedCount);
		return true;
	}
}

void FTrafficRuntimeModule::StartupModule()
{
	UE_LOG(LogTraffic, Log, TEXT("[AAA Traffic] Plugin Version = %s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION));
	UE_LOG(LogTraffic, Log, TEXT("[AAA Traffic] BuildStamp = %s %s"), TEXT(__DATE__), TEXT(__TIME__));
	if (const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("AAAtrafficSystem")))
	{
		UE_LOG(LogTraffic, Log, TEXT("[AAA Traffic] PluginDir = %s"), *Plugin->GetBaseDir());
	}
	UE_LOG(LogTraffic, Log, TEXT("[AAA Traffic] ModuleFile = %s"), *FModuleManager::Get().GetModuleFilename(TEXT("TrafficRuntime")));
	UE_LOG(LogTraffic, Log, TEXT("TrafficRuntime module started."));

	// Preload ZoneGraph lane profile assets so later ZoneGraph generation does not block on disk loads.
	if (!PreloadZoneLaneProfilesFromTrafficSettings())
	{
		PostEngineInitDelegateHandle = FCoreDelegates::OnPostEngineInit.AddRaw(this, &FTrafficRuntimeModule::HandlePostEngineInit);
	}
}

void FTrafficRuntimeModule::ShutdownModule()
{
	if (PostEngineInitDelegateHandle.IsValid())
	{
		FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitDelegateHandle);
		PostEngineInitDelegateHandle = FDelegateHandle();
	}

	UE_LOG(LogTraffic, Log, TEXT("TrafficRuntime module shutdown."));
}

void FTrafficRuntimeModule::HandlePostEngineInit()
{
	if (PostEngineInitDelegateHandle.IsValid())
	{
		FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitDelegateHandle);
		PostEngineInitDelegateHandle = FDelegateHandle();
	}

	PreloadZoneLaneProfilesFromTrafficSettings();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTrafficRuntimeModule, TrafficRuntime)
