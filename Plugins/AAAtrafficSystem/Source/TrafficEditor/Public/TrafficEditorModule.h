#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class SDockTab;
class FSpawnTabArgs;

class FTrafficEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	TSharedRef<SDockTab> SpawnTrafficSystemPanelTab(const FSpawnTabArgs& SpawnTabArgs);

	static const FName TrafficSystemPanelTabName;

private:
	void RegisterMenus();
	void OnPreBeginPIE(const bool bIsSimulating);
	bool AreTrafficBinariesStale(FString& OutDetails) const;

	FDelegateHandle PreBeginPIEHandle;
};
