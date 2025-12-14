#include "TrafficEditorModule.h"
#include "STrafficSystemPanel.h"
#include "TrafficSystemEditorSubsystem.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Styling/AppStyle.h"
#include "Editor.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarTrafficEnableDestructiveResets(
	TEXT("aaa.Traffic.EnableDestructiveResets"),
	0,
	TEXT("Enables destructive reset buttons in the AAA Traffic editor panel."),
	ECVF_Default);

const FName FTrafficEditorModule::TrafficSystemPanelTabName(TEXT("AAA_TrafficSystemPanel"));

void FTrafficEditorModule::StartupModule()
{
	if (!IsRunningCommandlet())
	{
		// Register the AAA Traffic System panel tab
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			TrafficSystemPanelTabName,
			FOnSpawnTab::CreateRaw(this, &FTrafficEditorModule::SpawnTrafficSystemPanelTab))
			.SetDisplayName(FText::FromString(TEXT("AAA Traffic System")))
			.SetTooltipText(FText::FromString(TEXT("AAA geometry-driven traffic tools.")))
			.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"))
			.SetMenuType(ETabSpawnerMenuType::Enabled);

		// Register menus
		UToolMenus::RegisterStartupCallback(
			FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FTrafficEditorModule::RegisterMenus));

		// Auto-open panel
		FGlobalTabmanager::Get()->TryInvokeTab(TrafficSystemPanelTabName);
	}
}

void FTrafficEditorModule::ShutdownModule()
{
	if (!IsRunningCommandlet())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TrafficSystemPanelTabName);
	}
}

void FTrafficEditorModule::RegisterMenus()
{
	// Window → AAA Traffic System (open panel)
	UToolMenu* WindowMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
	FToolMenuSection& WindowSection = WindowMenu->AddSection(
		"AAAtrafficSystemWindowSection",
		FText::FromString(TEXT("AAA Traffic System")));
	WindowSection.AddMenuEntry(
		"OpenAAATrafficSystemPanel",
		FText::FromString(TEXT("AAA Traffic System")),
		FText::FromString(TEXT("Open the AAA Traffic System editor panel.")),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"),
		FUIAction(FExecuteAction::CreateLambda([]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(FTrafficEditorModule::TrafficSystemPanelTabName);
		}))
	);
}

TSharedRef<SDockTab> FTrafficEditorModule::SpawnTrafficSystemPanelTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(STrafficSystemPanel)
		];
}

IMPLEMENT_MODULE(FTrafficEditorModule, TrafficEditor)
