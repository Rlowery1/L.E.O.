#include "TrafficEditorModule.h"
#include "STrafficSystemPanel.h"
#include "TrafficSystemEditorSubsystem.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Styling/AppStyle.h"
#include "Editor.h"
#include "HAL/IConsoleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

static TAutoConsoleVariable<int32> CVarTrafficEnableDestructiveResets(
	TEXT("aaa.Traffic.EnableDestructiveResets"),
	0,
	TEXT("Enables destructive reset buttons in the AAA Traffic editor panel."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficEditorBlockPIEWhenBinariesStale(
	TEXT("aaa.Traffic.Editor.BlockPIEWhenBinariesStale"),
	1,
	TEXT("If non-zero, cancels PIE start when AAA Traffic plugin source files are newer than the loaded TrafficRuntime binary.\n")
	TEXT("This prevents the common workflow trap where the Editor keeps an old plugin DLL loaded and your rebuild doesn't affect visuals.\n")
	TEXT("Default: 1"),
	ECVF_Default);

const FName FTrafficEditorModule::TrafficSystemPanelTabName(TEXT("AAA_TrafficSystemPanel"));

void FTrafficEditorModule::StartupModule()
{
	if (!IsRunningCommandlet())
	{
		PreBeginPIEHandle = FEditorDelegates::PreBeginPIE.AddRaw(this, &FTrafficEditorModule::OnPreBeginPIE);

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
		if (PreBeginPIEHandle.IsValid())
		{
			FEditorDelegates::PreBeginPIE.Remove(PreBeginPIEHandle);
			PreBeginPIEHandle.Reset();
		}

		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TrafficSystemPanelTabName);
	}
}

void FTrafficEditorModule::OnPreBeginPIE(const bool bIsSimulating)
{
	(void)bIsSimulating;

	if (!GEditor || IsRunningCommandlet())
	{
		return;
	}

	if (CVarTrafficEditorBlockPIEWhenBinariesStale.GetValueOnGameThread() == 0)
	{
		return;
	}

	FString Details;
	if (!AreTrafficBinariesStale(Details))
	{
		return;
	}

	// Cancel PIE before it starts so users don't chase "no visual change" ghost bugs.
	GEditor->CancelRequestPlaySession();

	const FString Msg = FString::Printf(
		TEXT("AAA Traffic: Your plugin binaries are stale vs source, so PIE would run old code and visuals will not change.\n\n%s\n\nFix: Close the Editor completely and run RebuildAndLaunch_TrafficBaselineCurve.bat (or build L_E_OEditor with the Editor closed)."),
		*Details);

	UE_LOG(LogTemp, Error, TEXT("%s"), *Msg);

	// Avoid modal dialogs in unattended runs (automation/CI).
	if (!FApp::IsUnattended())
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Msg));
	}
}

bool FTrafficEditorModule::AreTrafficBinariesStale(FString& OutDetails) const
{
	OutDetails.Reset();

	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("AAAtrafficSystem"));
	if (!Plugin.IsValid())
	{
		return false;
	}

	const FString PluginDir = Plugin->GetBaseDir();
	const FString SourceDir = FPaths::Combine(PluginDir, TEXT("Source"));

	const FString RuntimeModulePath = FModuleManager::Get().GetModuleFilename(TEXT("TrafficRuntime"));
	if (RuntimeModulePath.IsEmpty())
	{
		return false;
	}

	const FDateTime RuntimeBinaryTime = IFileManager::Get().GetTimeStamp(*RuntimeModulePath);

	TArray<FString> SourceFiles;
	IFileManager::Get().FindFilesRecursive(SourceFiles, *SourceDir, TEXT("*.cpp"), /*Files=*/true, /*Directories=*/false);
	IFileManager::Get().FindFilesRecursive(SourceFiles, *SourceDir, TEXT("*.h"), /*Files=*/true, /*Directories=*/false);
	IFileManager::Get().FindFilesRecursive(SourceFiles, *SourceDir, TEXT("*.inl"), /*Files=*/true, /*Directories=*/false);

	FDateTime LatestSource = FDateTime::MinValue();
	FString LatestSourcePath;
	for (const FString& File : SourceFiles)
	{
		const FDateTime T = IFileManager::Get().GetTimeStamp(*File);
		if (T > LatestSource)
		{
			LatestSource = T;
			LatestSourcePath = File;
		}
	}

	if (LatestSource == FDateTime::MinValue())
	{
		return false;
	}

	// Allow a small clock skew/FS rounding.
	const bool bStale = LatestSource > (RuntimeBinaryTime + FTimespan::FromSeconds(1.0));
	if (!bStale)
	{
		return false;
	}

	OutDetails = FString::Printf(
		TEXT("Loaded runtime module:\n  %s\n  Timestamp: %s\n\nNewest AAAtrafficSystem source file:\n  %s\n  Timestamp: %s"),
		*RuntimeModulePath,
		*RuntimeBinaryTime.ToString(),
		*LatestSourcePath,
		*LatestSource.ToString());
	return true;
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
