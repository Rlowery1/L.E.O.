#include "TrafficTestsModule.h"

#define LOCTEXT_NAMESPACE "FTrafficTestsModule"

void FTrafficTestsModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("[TrafficTests] StartupModule - Traffic tests module loaded"));
}

void FTrafficTestsModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTrafficTestsModule, TrafficTests)

