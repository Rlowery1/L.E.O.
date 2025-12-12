#include "TrafficRuntimeModule.h"

DEFINE_LOG_CATEGORY(LogTraffic);

#define LOCTEXT_NAMESPACE "FTrafficRuntimeModule"

void FTrafficRuntimeModule::StartupModule()
{
	UE_LOG(LogTraffic, Log, TEXT("[AAA Traffic] Plugin Version = %s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION));
	UE_LOG(LogTraffic, Log, TEXT("TrafficRuntime module started."));
}

void FTrafficRuntimeModule::ShutdownModule()
{
	UE_LOG(LogTraffic, Log, TEXT("TrafficRuntime module shutdown."));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTrafficRuntimeModule, TrafficRuntime)

