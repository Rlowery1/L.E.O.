#include "STrafficSystemPanel.h"
#include "TrafficSystemEditorSubsystem.h"
#include "TrafficRuntimeModule.h"
#include "RoadFamilyRegistry.h"
#include "Editor.h"
#include "Styling/AppStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Misc/MessageDialog.h"
#include "Internationalization/Text.h"
#include "HAL/IConsoleManager.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "HAL/PlatformApplicationMisc.h"
#include "TrafficRoadFamilySettings.h"

#define LOCTEXT_NAMESPACE "STrafficSystemPanel"

namespace
{
	bool IsDestructiveResetEnabled()
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.EnableDestructiveResets")))
		{
			return CVar->GetInt() != 0;
		}
		return false;
	}
}

void STrafficSystemPanel::Construct(const FArguments& InArgs)
{
	const FSlateFontInfo TitleFont = FAppStyle::GetFontStyle("BoldFont");
	FSlateFontInfo TitleFontLarge = TitleFont;
	TitleFontLarge.Size = 18;

	const FSlateFontInfo SubtitleFont = FAppStyle::GetFontStyle("RegularFont");

	RefreshFamilyList();

	ChildSlot
	[
		SNew(SBorder)
		.Padding(FMargin(16.0f))
		.BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 12.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("AAA Traffic System")))
					.Font(TitleFontLarge)
					.ColorAndOpacity(FLinearColor::White)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 4.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Spline-driven, user-road-first traffic.")))
					.Font(SubtitleFont)
					.ColorAndOpacity(FLinearColor(0.75f, 0.75f, 0.75f))
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f)
			[
				SNew(SBox)
				.HeightOverride(1.0f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("Brushes.Header"))
				]
			]

			// Road Setup group
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f)
			[
				SNew(SVerticalBox)

				// Prepare map for traffic
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(SButton)
					.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
					.HAlign(HAlign_Left)
					.ContentPadding(FMargin(16.0f, 8.0f))
					.OnClicked(this, &STrafficSystemPanel::OnPrepareMapForTrafficClicked)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("PREPARE MAP")))
							.Font(TitleFont)
							.ColorAndOpacity(FLinearColor::White)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Scan all spline roads, attach metadata, register families.")))
							.Font(SubtitleFont)
							.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
							.WrapTextAt(420.0f)
						]
					]
				]

				// Calibrate
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(SButton)
					.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
					.HAlign(HAlign_Left)
					.ContentPadding(FMargin(16.0f, 8.0f))
					.OnClicked(this, &STrafficSystemPanel::OnBeginCalibrationClicked)
					.IsEnabled_Lambda([this]()
					{
						if (UTrafficSystemEditorSubsystem* Subsys = GetSubsystem())
						{
							if (!SelectedFamily.IsValid())
							{
								return false;
							}
							if (!Subsys->HasAnyPreparedRoads())
							{
								return false;
							}
							return Subsys->GetNumActorsForFamily(SelectedFamily->FamilyId) > 0;
						}
						return false;
					})
					.ToolTipText_Lambda([this]()
					{
						if (UTrafficSystemEditorSubsystem* Subsys = GetSubsystem())
						{
							if (!Subsys->HasAnyPreparedRoads())
							{
								return LOCTEXT("Tooltip_NoPreparedRoads", "No prepared roads found. Run PREPARE MAP first.");
							}
							if (SelectedFamily.IsValid())
							{
								const int32 NumInstances = Subsys->GetNumActorsForFamily(SelectedFamily->FamilyId);
								if (NumInstances <= 0)
								{
									return LOCTEXT("Tooltip_NoActorsForFamily",
										"This road family has no actors in this level.\n"
										"Place or load at least one road actor for this family before calibrating.");
								}
							}
						}
						return LOCTEXT("Tooltip_BeginCalibration", "Spawn and adjust the calibration overlay for this road family.");
					})
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("BEGIN CALIBRATION")))
							.Font(TitleFont)
							.ColorAndOpacity(FLinearColor::White)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Spawn overlay for selected family; tweak lanes/widths.")))
							.Font(SubtitleFont)
							.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
							.WrapTextAt(420.0f)
						]
					]
				]

				// Bake calibration
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(SButton)
					.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
					.HAlign(HAlign_Left)
					.ContentPadding(FMargin(16.0f, 8.0f))
					.OnClicked(this, &STrafficSystemPanel::OnBakeCalibrationClicked)
					.IsEnabled_Lambda([this]()
					{
						if (UTrafficSystemEditorSubsystem* Subsys = GetSubsystem())
						{
							return SelectedFamily.IsValid() && Subsys->HasAnyPreparedRoads();
						}
						return false;
					})
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("BAKE CALIBRATION")))
							.Font(TitleFont)
							.ColorAndOpacity(FLinearColor::White)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Persist lane counts/widths to the selected family.")))
							.Font(SubtitleFont)
							.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
							.WrapTextAt(420.0f)
						]
					]
				]

				// Restore last calibration
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(SButton)
					.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SecondaryButton"))
					.HAlign(HAlign_Left)
					.ContentPadding(FMargin(16.0f, 8.0f))
					.OnClicked(this, &STrafficSystemPanel::OnRestoreCalibrationClicked)
					.IsEnabled_Lambda([this]()
					{
						return SelectedFamily.IsValid() && HasBackupForSelected();
					})
					.ToolTipText(LOCTEXT("RestoreCalib_Tooltip", "Restore the previous calibration snapshot for this family (if one exists)."))
					[
						SNew(STextBlock)
						.Text(LOCTEXT("RestoreCalib_Label", "Restore Last Calibration"))
						.Font(SubtitleFont)
						.ColorAndOpacity(FLinearColor::White)
					]
				]

				// Calibration families dropdown + rename
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 10.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Detected Road Families")))
						.Font(TitleFont)
						.ColorAndOpacity(FLinearColor::White)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f, 0.0f, 2.0f)
					[
						SNew(SComboBox<TSharedPtr<FFamilyListItem>>)
						.OptionsSource(&FamilyItems)
						.OnGenerateWidget(this, &STrafficSystemPanel::OnGenerateFamilyItem)
						.OnSelectionChanged(this, &STrafficSystemPanel::OnFamilySelectionChanged)
						.Content()
						[
							SNew(STextBlock)
							.Text(this, &STrafficSystemPanel::GetSelectedFamilyLabel)
							.Font(SubtitleFont)
							.ColorAndOpacity(FLinearColor(0.9f, 0.9f, 0.9f))
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(FamilyNameEdit, SEditableTextBox)
						.Text(this, &STrafficSystemPanel::GetSelectedFamilyEditableText)
						.OnTextCommitted(this, &STrafficSystemPanel::OnFamilyNameCommitted)
						.IsEnabled_Lambda([this]() { return SelectedFamily.IsValid(); })
						.MinDesiredWidth(260.f)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 6.0f, 0.0f, 0.0f)
					[
						SNew(SButton)
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SecondaryButton"))
						.HAlign(HAlign_Left)
						.ContentPadding(FMargin(16.0f, 8.0f))
						.OnClicked(this, &STrafficSystemPanel::OnDeleteFamilyClicked)
						.IsEnabled_Lambda([this]() { return SelectedFamily.IsValid(); })
						.ToolTipText(LOCTEXT("DeleteFamily_Tooltip",
							"Delete the selected road family from AAA Traffic.\n"
							"Optionally, you can also exclude any matching actors from traffic so the family won't be recreated on the next Prepare Map."))
						[
							SNew(STextBlock)
							.Text(LOCTEXT("DeleteFamily_Label", "Delete Selected Family"))
							.Font(SubtitleFont)
							.ColorAndOpacity(FLinearColor::White)
						]
					]
				]

				// Reset AAA only
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(SButton)
					.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
					.HAlign(HAlign_Left)
					.ContentPadding(FMargin(16.0f, 8.0f))
					.OnClicked(this, &STrafficSystemPanel::OnResetLabClicked)
					.ToolTipText(LOCTEXT("ResetAAA_Tooltip", "Remove AAA traffic overlay/controllers/vehicles. User roads and calibration remain untouched."))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("RESET AAA ACTORS")))
							.Font(TitleFont)
							.ColorAndOpacity(FLinearColor::White)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Remove AAA-tagged roads/overlays/vehicles (keep user roads).")))
							.Font(SubtitleFont)
							.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
							.WrapTextAt(420.0f)
						]
					]
				]

			]

			// Run group
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SVerticalBox)

				// Build + Cars
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(SButton)
					.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
					.HAlign(HAlign_Left)
					.ContentPadding(FMargin(16.0f, 8.0f))
					.OnClicked(this, &STrafficSystemPanel::OnBuildAndCarsClicked)
					.IsEnabled_Lambda([this]()
					{
						if (UTrafficSystemEditorSubsystem* Subsys = GetSubsystem())
						{
							return Subsys->HasAnyPreparedRoads() && Subsys->HasAnyCalibratedFamilies();
						}
						return false;
					})
					.ToolTipText(LOCTEXT("Tooltip_BuildCars", "Prepare roads, build the transient traffic network, and spawn test vehicles in the EDITOR world.\nThis is an editor-only test harness for calibration and behavior tuning. Runtime/PIE integration will spawn traffic according to your game setup.\nRequires at least one prepared and calibrated road family."))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("BUILD + CARS (Editor Test)")))
							.Font(TitleFont)
							.ColorAndOpacity(FLinearColor::White)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Prepare, build, and spawn vehicles in the editor world for quick testing.")))
							.Font(SubtitleFont)
							.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
							.WrapTextAt(420.0f)
						]
					]
				]
			]

			// Advanced / dev-only tools
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 12.0f, 0.0f, 0.0f)
			[
				SNew(SExpandableArea)
				.InitiallyCollapsed(true)
				.AreaTitle(LOCTEXT("AAATrafficAdvancedTools", "Advanced / Dev Tools"))
				.BodyContent()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						SNew(SButton)
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
						.HAlign(HAlign_Left)
						.ContentPadding(FMargin(16.0f, 8.0f))
						.OnClicked(this, &STrafficSystemPanel::OnDrawIntersectionDebugClicked)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("DEBUG: Draw intersection movement paths")))
								.Font(TitleFont)
								.ColorAndOpacity(FLinearColor::White)
								.WrapTextAt(420.0f)
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 2.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Builds the traffic network (if needed) and draws turn connectors at intersections.")))
								.Font(SubtitleFont)
								.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
								.WrapTextAt(420.0f)
							]
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						SNew(SButton)
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SecondaryButton"))
						.HAlign(HAlign_Left)
						.ContentPadding(FMargin(16.0f, 8.0f))
						.OnClicked(this, &STrafficSystemPanel::OnToggleReverseDirectionForSelectionClicked)
						.ToolTipText(LOCTEXT("ToggleReverseDir_Tooltip",
							"Flips the travel direction for the selected prepared road actor(s).\n"
							"Use this for one-way ramps where on-ramps and off-ramps share the same 'Ramp - 1 Lane' family."))
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ToggleReverseDir_Label", "Flip Selected Road Direction"))
							.Font(SubtitleFont)
							.ColorAndOpacity(FLinearColor::White)
							.WrapTextAt(420.0f)
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						SNew(SButton)
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SecondaryButton"))
						.HAlign(HAlign_Left)
						.ContentPadding(FMargin(16.0f, 8.0f))
						.OnClicked(this, &STrafficSystemPanel::OnCopyRoadFamilyDefaultsIniClicked)
						.ToolTipText(LOCTEXT("CopyDefaultsIni_Tooltip",
							"Copies an INI snippet for TrafficRoadFamilySettings (lane counts/widths/offsets) to your clipboard.\n"
							"Use this to ship calibrated defaults in a plugin/project DefaultGame.ini."))
						[
							SNew(STextBlock)
							.Text(LOCTEXT("CopyDefaultsIni_Label", "DEV: Copy Road Family Defaults (INI)"))
							.Font(SubtitleFont)
							.ColorAndOpacity(FLinearColor::White)
							.WrapTextAt(420.0f)
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						SNew(SButton)
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
						.HAlign(HAlign_Left)
						.ContentPadding(FMargin(16.0f, 8.0f))
						.OnClicked(this, &STrafficSystemPanel::OnConvertSelectedClicked)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("ADVANCED: Copy selected spline actors into AAA debug roads")))
								.Font(TitleFont)
								.ColorAndOpacity(FLinearColor::White)
								.WrapTextAt(420.0f)
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 2.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Internal testing only. Not required for CityBLD or other road kits.")))
								.Font(SubtitleFont)
								.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
								.WrapTextAt(420.0f)
							]
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						SNew(SButton)
						.Visibility_Lambda([]() { return IsDestructiveResetEnabled() ? EVisibility::Visible : EVisibility::Collapsed; })
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
						.HAlign(HAlign_Left)
						.ContentPadding(FMargin(16.0f, 8.0f))
						.OnClicked(this, &STrafficSystemPanel::OnResetLabIncludingTaggedClicked)
						.ToolTipText(LOCTEXT("ResetTagged_Tooltip", "Deletes AAA actors AND user road actors tagged for conversion. Destructive and hidden unless explicitly enabled."))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("DEV-ONLY: DELETE AAA + Tagged Road Actors (DESTRUCTIVE)")))
							.Font(TitleFont)
							.ColorAndOpacity(FLinearColor::White)
							.WrapTextAt(420.0f)
						]
					]
				]
			]
		]
	];
}

UTrafficSystemEditorSubsystem* STrafficSystemPanel::GetSubsystem() const
{
#if WITH_EDITOR
	if (GEditor)
	{
		return GEditor->GetEditorSubsystem<UTrafficSystemEditorSubsystem>();
	}
#endif
	return nullptr;
}

bool STrafficSystemPanel::CanCalibrate() const
{
	return SelectedFamily.IsValid();
}

bool STrafficSystemPanel::CanBake() const
{
	return SelectedFamily.IsValid();
}

FReply STrafficSystemPanel::OnBeginCalibrationClicked()
{
	const bool bAutomationOrCmdlet = IsRunningCommandlet() || GIsAutomationTesting;
	if (UTrafficSystemEditorSubsystem* Subsys = GetSubsystem())
	{
		if (!HasDetectedFamilies())
		{
			if (!bAutomationOrCmdlet)
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("No road families detected. Run Prepare Map first.")));
			}
			else
			{
				UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor][Automation] No road families detected; dialog suppressed."));
			}
			return FReply::Handled();
		}
		if (SelectedFamily.IsValid())
		{
			Subsys->Editor_BeginCalibrationForFamily(SelectedFamily->FamilyId);
		}
	}
	return FReply::Handled();
}

FReply STrafficSystemPanel::OnBakeCalibrationClicked()
{
	if (UTrafficSystemEditorSubsystem* Subsys = GetSubsystem())
	{
		if (SelectedFamily.IsValid())
		{
			Subsys->Editor_BakeCalibrationForActiveFamily();
			RefreshFamilyList();
		}
	}
	return FReply::Handled();
}

FReply STrafficSystemPanel::OnResetLabClicked()
{
	if (UTrafficSystemEditorSubsystem* Subsys = GetSubsystem())
	{
		Subsys->Editor_ResetRoadLabHard();
	}
	return FReply::Handled();
}

FReply STrafficSystemPanel::OnResetLabIncludingTaggedClicked()
{
	const bool bAutomationOrCmdlet = IsRunningCommandlet() || GIsAutomationTesting;
	if (UTrafficSystemEditorSubsystem* Subsys = GetSubsystem())
	{
		if (!bAutomationOrCmdlet)
		{
			const EAppReturnType::Type Resp = FMessageDialog::Open(
				EAppMsgType::OkCancel,
				LOCTEXT("Traffic_ConfirmDestructiveReset",
					"WARNING: This will DELETE road actors tagged for AAA conversion from this level.\n"
					"This is DESTRUCTIVE and cannot be undone.\n\n"
					"Are you sure you want to continue?"));
			if (Resp != EAppReturnType::Ok)
			{
				return FReply::Handled();
			}
		}
		else
		{
			UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor][Automation] Destructive reset dialog suppressed; proceeding."));
		}

		Subsys->Editor_ResetRoadLabHard(true);
	}
	return FReply::Handled();
}

FReply STrafficSystemPanel::OnConvertSelectedClicked()
{
	if (UTrafficSystemEditorSubsystem* Subsys = GetSubsystem())
	{
		Subsys->ConvertSelectedSplineActors();
	}
	return FReply::Handled();
}

FReply STrafficSystemPanel::OnRestoreCalibrationClicked()
{
	if (UTrafficSystemEditorSubsystem* Subsys = GetSubsystem())
	{
		if (SelectedFamily.IsValid())
		{
			Subsys->Editor_RestoreCalibrationForFamily(SelectedFamily->FamilyId);
			RefreshFamilyList();
		}
	}
	return FReply::Handled();
}

FReply STrafficSystemPanel::OnBuildAndCarsClicked()
{
	const bool bAutomationOrCmdlet = IsRunningCommandlet() || GIsAutomationTesting;
	if (UTrafficSystemEditorSubsystem* Subsys = GetSubsystem())
	{
		if (!HasDetectedFamilies())
		{
			if (!bAutomationOrCmdlet)
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("No road families detected. Run Prepare Map first.")));
			}
			else
			{
				UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor][Automation] No road families detected; dialog suppressed."));
			}
			return FReply::Handled();
		}
		if (!Subsys->HasAnyCalibratedFamilies())
		{
			if (!bAutomationOrCmdlet)
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("No calibrated road families found. Calibrate and bake at least one family first.")));
			}
			else
			{
				UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor][Automation] No calibrated families; dialog suppressed."));
			}
			return FReply::Handled();
		}
		Subsys->DoPrepare();
		Subsys->DoBuild();
		Subsys->DoCars();
	}
	return FReply::Handled();
}

FReply STrafficSystemPanel::OnPrepareClicked()
{
	if (UTrafficSystemEditorSubsystem* Subsys = GetSubsystem())
	{
		Subsys->DoPrepare();
	}
	return FReply::Handled();
}

FReply STrafficSystemPanel::OnBuildClicked()
{
	const bool bAutomationOrCmdlet = IsRunningCommandlet() || GIsAutomationTesting;
	if (UTrafficSystemEditorSubsystem* Subsys = GetSubsystem())
	{
		if (!HasDetectedFamilies())
		{
			if (!bAutomationOrCmdlet)
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("No road families detected. Run Prepare Map first.")));
			}
			else
			{
				UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor][Automation] No road families detected; dialog suppressed."));
			}
			return FReply::Handled();
		}
		if (!Subsys->HasAnyCalibratedFamilies())
		{
			if (!bAutomationOrCmdlet)
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("No calibrated road families found. Calibrate and bake at least one family first.")));
			}
			else
			{
				UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor][Automation] No calibrated families; dialog suppressed."));
			}
			return FReply::Handled();
		}
		Subsys->DoBuild();
	}
	return FReply::Handled();
}

FReply STrafficSystemPanel::OnCarsClicked()
{
	const bool bAutomationOrCmdlet = IsRunningCommandlet() || GIsAutomationTesting;
	if (UTrafficSystemEditorSubsystem* Subsys = GetSubsystem())
	{
		if (!HasDetectedFamilies())
		{
			if (!bAutomationOrCmdlet)
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("No road families detected. Run Prepare Map first.")));
			}
			else
			{
				UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor][Automation] No road families detected; dialog suppressed."));
			}
			return FReply::Handled();
		}
		if (!Subsys->HasAnyCalibratedFamilies())
		{
			if (!bAutomationOrCmdlet)
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("No calibrated road families found. Calibrate and bake at least one family first.")));
			}
			else
			{
				UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor][Automation] No calibrated families; dialog suppressed."));
			}
			return FReply::Handled();
		}
		Subsys->DoCars();
	}
	return FReply::Handled();
}

FReply STrafficSystemPanel::OnDrawIntersectionDebugClicked()
{
	const bool bAutomationOrCmdlet = IsRunningCommandlet() || GIsAutomationTesting;
	if (UTrafficSystemEditorSubsystem* Subsys = GetSubsystem())
	{
		if (!HasDetectedFamilies())
		{
			if (!bAutomationOrCmdlet)
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("No road families detected. Run Prepare Map first.")));
			}
			else
			{
				UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor][Automation] No road families detected; dialog suppressed."));
			}
			return FReply::Handled();
		}

		Subsys->DoDrawIntersectionDebug();
	}
	return FReply::Handled();
}

FReply STrafficSystemPanel::OnPrepareMapForTrafficClicked()
{
	if (UTrafficSystemEditorSubsystem* Subsys = GetSubsystem())
	{
		Subsys->Editor_PrepareMapForTraffic();
		RefreshFamilyList();
	}
	return FReply::Handled();
}

FReply STrafficSystemPanel::OnToggleReverseDirectionForSelectionClicked()
{
	if (UTrafficSystemEditorSubsystem* Subsys = GetSubsystem())
	{
		Subsys->Editor_ToggleReverseDirectionForSelectedRoads();
	}
	return FReply::Handled();
}

FString STrafficSystemPanel::BuildRoadFamilySettingsIniSnippet() const
{
	const UTrafficRoadFamilySettings* RoadSettings = GetDefault<UTrafficRoadFamilySettings>();
	const URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get();
	if (!RoadSettings || !Registry)
	{
		return FString();
	}

	TSet<FName> RelevantFamilyNames;
	for (const FRoadFamilyInfo& Info : Registry->GetAllFamilies())
	{
		if (!Info.FamilyDefinition.FamilyName.IsNone())
		{
			RelevantFamilyNames.Add(Info.FamilyDefinition.FamilyName);
		}
		else
		{
			RelevantFamilyNames.Add(FName(*Info.DisplayName));
		}
	}

	auto FormatFloat = [](float Value) -> FString
	{
		return FString::SanitizeFloat(Value);
	};

	auto FormatFloatArray = [&](const TArray<float>& Values) -> FString
	{
		if (Values.Num() == 0)
		{
			return TEXT("()");
		}

		TArray<FString> Parts;
		Parts.Reserve(Values.Num());
		for (float V : Values)
		{
			Parts.Add(FormatFloat(V));
		}
		return FString::Printf(TEXT("(%s)"), *FString::Join(Parts, TEXT(",")));
	};

	auto FormatSide = [&](const FTrafficLaneLayoutSide& Side) -> FString
	{
		return FString::Printf(
			TEXT("(NumLanes=%d,LaneWidthCm=%s,InnerLaneCenterOffsetCm=%s,LaneCenterOffsetAdjustmentsCm=%s)"),
			Side.NumLanes,
			*FormatFloat(Side.LaneWidthCm),
			*FormatFloat(Side.InnerLaneCenterOffsetCm),
			*FormatFloatArray(Side.LaneCenterOffsetAdjustmentsCm));
	};

	const FString Section = UTrafficRoadFamilySettings::StaticClass()->GetPathName();
	FString Out;
	Out += FString::Printf(TEXT("[%s]\n"), *Section);
	Out += TEXT("!Families=ClearArray\n");

	int32 NumWritten = 0;
	for (const FRoadFamilyDefinition& Family : RoadSettings->Families)
	{
		if (Family.FamilyName.IsNone() || !RelevantFamilyNames.Contains(Family.FamilyName))
		{
			continue;
		}

		FString Line = FString::Printf(
			TEXT("+Families=(FamilyName=\"%s\",Forward=%s,Backward=%s,DefaultSpeedLimitKmh=%s"),
			*Family.FamilyName.ToString(),
			*FormatSide(Family.Forward),
			*FormatSide(Family.Backward),
			*FormatFloat(Family.DefaultSpeedLimitKmh));

		if (!Family.VehicleLaneProfile.IsNull())
		{
			Line += FString::Printf(TEXT(",VehicleLaneProfile=\"%s\""), *Family.VehicleLaneProfile.ToString());
		}
		if (!Family.FootpathLaneProfile.IsNull())
		{
			Line += FString::Printf(TEXT(",FootpathLaneProfile=\"%s\""), *Family.FootpathLaneProfile.ToString());
		}

		Line += TEXT(")\n");
		Out += Line;
		++NumWritten;
	}

	if (NumWritten == 0)
	{
		return FString();
	}

	return Out;
}

FReply STrafficSystemPanel::OnCopyRoadFamilyDefaultsIniClicked()
{
	const bool bAutomationOrCmdlet = IsRunningCommandlet() || GIsAutomationTesting;
	const FString Snippet = BuildRoadFamilySettingsIniSnippet();
	if (Snippet.IsEmpty())
	{
		if (!bAutomationOrCmdlet)
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("CopyDefaultsIni_None", "No road family defaults were available to export.\nRun PREPARE MAP first."));
		}
		return FReply::Handled();
	}

	FPlatformApplicationMisc::ClipboardCopy(*Snippet);

	if (!bAutomationOrCmdlet)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("CopyDefaultsIni_Done", "Copied road family defaults INI snippet to clipboard."));
	}

	return FReply::Handled();
}

void STrafficSystemPanel::RefreshFamilyList()
{
	const FGuid PreviousSelection = SelectedFamily.IsValid() ? SelectedFamily->FamilyId : FGuid();

	FamilyItems.Empty();
	if (const URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get())
	{
		TArray<FRoadFamilyInfo> Families = Registry->GetAllFamiliesCopy();
		Families.Sort([](const FRoadFamilyInfo& A, const FRoadFamilyInfo& B)
		{
			return A.DisplayName < B.DisplayName;
		});

		for (const FRoadFamilyInfo& Info : Families)
		{
			TSharedPtr<FFamilyListItem> Item = MakeShared<FFamilyListItem>();
			Item->FamilyId = Info.FamilyId;
			int32 NumInstances = 0;
			if (UTrafficSystemEditorSubsystem* Subsys = GetSubsystem())
			{
				NumInstances = Subsys->GetNumActorsForFamily(Info.FamilyId);
			}
			Item->DisplayName = NumInstances > 0 ? Info.DisplayName : FString::Printf(TEXT("%s (0 actors)"), *Info.DisplayName);
			Item->ClassDisplay = Info.RoadClassPath.GetAssetName();
			Item->bIsCalibrated = Info.bIsCalibrated;
			FamilyItems.Add(Item);
		}
	}

	SelectedFamily.Reset();
	for (const TSharedPtr<FFamilyListItem>& Item : FamilyItems)
	{
		if (PreviousSelection.IsValid() && Item->FamilyId == PreviousSelection)
		{
			SelectedFamily = Item;
			break;
		}
	}
	if (!SelectedFamily.IsValid() && FamilyItems.Num() > 0)
	{
		SelectedFamily = FamilyItems[0];
	}
	if (FamilyNameEdit.IsValid())
	{
		FamilyNameEdit->SetText(GetSelectedFamilyEditableText());
	}
}

TSharedRef<SWidget> STrafficSystemPanel::OnGenerateFamilyItem(TSharedPtr<FFamilyListItem> Item) const
{
FString Label = Item.IsValid() ? Item->DisplayName : TEXT("No families detected");
if (Item.IsValid() && !Item->ClassDisplay.IsEmpty())
{
	Label += FString::Printf(TEXT("  (%s)"), *Item->ClassDisplay);
}
	if (Item.IsValid() && Item->bIsCalibrated)
	{
		Label += TEXT("  [Calibrated]");
	}

	return SNew(STextBlock)
		.Text(FText::FromString(Label));
}

void STrafficSystemPanel::OnFamilySelectionChanged(TSharedPtr<FFamilyListItem> Item, ESelectInfo::Type SelectInfo)
{
	SelectedFamily = Item;
	if (FamilyNameEdit.IsValid())
	{
		FamilyNameEdit->SetText(GetSelectedFamilyEditableText());
	}
}

FText STrafficSystemPanel::GetSelectedFamilyLabel() const
{
	return SelectedFamily.IsValid()
		? FText::FromString(SelectedFamily->DisplayName)
		: FText::FromString(TEXT("No road families detected"));
}

FText STrafficSystemPanel::GetSelectedFamilyEditableText() const
{
	return SelectedFamily.IsValid()
		? FText::FromString(SelectedFamily->DisplayName)
		: FText::GetEmpty();
}

void STrafficSystemPanel::OnFamilyNameCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
	if (!SelectedFamily.IsValid())
	{
		return;
	}

	if (URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get())
	{
		if (Registry->SetFamilyDisplayName(SelectedFamily->FamilyId, NewText.ToString()))
		{
			Registry->SaveConfig();
			RefreshFamilyList();
		}
	}
}

FReply STrafficSystemPanel::OnDeleteFamilyClicked()
{
	const bool bAutomationOrCmdlet = IsRunningCommandlet() || GIsAutomationTesting;
	if (!SelectedFamily.IsValid())
	{
		return FReply::Handled();
	}

	if (UTrafficSystemEditorSubsystem* Subsys = GetSubsystem())
	{
		bool bExcludeActors = false;
		if (!bAutomationOrCmdlet)
		{
			const EAppReturnType::Type Choice = FMessageDialog::Open(
				EAppMsgType::YesNoCancel,
				FText::FromString(FString::Printf(
					TEXT("Delete road family '%s'?\n\n")
					TEXT("Yes: delete and EXCLUDE matching actors from traffic (recommended for accidental families)\n")
					TEXT("No: delete only (family may be recreated by Prepare Map if actors still match)\n")
					TEXT("Cancel: do nothing"),
					*SelectedFamily->DisplayName)));

			if (Choice == EAppReturnType::Cancel)
			{
				return FReply::Handled();
			}
			bExcludeActors = (Choice == EAppReturnType::Yes);
		}

		Subsys->Editor_DeleteFamily(SelectedFamily->FamilyId, bExcludeActors);
		RefreshFamilyList();
	}

	return FReply::Handled();
}

bool STrafficSystemPanel::HasDetectedFamilies() const
{
	const URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get();
	return Registry && Registry->GetAllFamilies().Num() > 0;
}

bool STrafficSystemPanel::HasBackupForSelected() const
{
	if (!SelectedFamily.IsValid())
	{
		return false;
	}

	if (const URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get())
	{
		for (const FRoadFamilyInfo& Info : Registry->GetAllFamilies())
		{
			if (Info.FamilyId == SelectedFamily->FamilyId)
			{
				return Info.bHasBackupCalibration;
			}
		}
	}
	return false;
}

#undef LOCTEXT_NAMESPACE
