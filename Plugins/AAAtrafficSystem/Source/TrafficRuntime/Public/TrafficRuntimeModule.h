#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"
#include "Delegates/Delegate.h"

#define AAA_TRAFFIC_PLUGIN_VERSION "Phase21"

TRAFFICRUNTIME_API DECLARE_LOG_CATEGORY_EXTERN(LogTraffic, Log, All);

class TRAFFICRUNTIME_API FTrafficRuntimeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void HandlePostEngineInit();

	FDelegateHandle PostEngineInitDelegateHandle;
};
