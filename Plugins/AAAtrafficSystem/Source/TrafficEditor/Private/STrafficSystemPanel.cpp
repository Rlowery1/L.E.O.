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

#define LOCTEXT_NAMESPACE "STrafficSystemPanel"

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
					.IsEnabled_Lambda([this]() { return SelectedFamily.IsValid(); })
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
					.IsEnabled_Lambda([this]() { return SelectedFamily.IsValid(); })
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

				// Reset including tagged user roads
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(SButton)
					.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
					.HAlign(HAlign_Left)
					.ContentPadding(FMargin(16.0f, 8.0f))
					.OnClicked(this, &STrafficSystemPanel::OnResetLabIncludingTaggedClicked)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("RESET AAA + TAGGED USER ROADS")))
							.Font(TitleFont)
							.ColorAndOpacity(FLinearColor::White)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Also remove roads tagged for AAA conversion (destructive).")))
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
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("BUILD + CARS")))
							.Font(TitleFont)
							.ColorAndOpacity(FLinearColor::White)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Prepare, build network, and spawn vehicles in one click.")))
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
	if (UTrafficSystemEditorSubsystem* Subsys = GetSubsystem())
	{
		if (!HasDetectedFamilies())
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("No road families detected. Run Prepare Map first.")));
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
	if (UTrafficSystemEditorSubsystem* Subsys = GetSubsystem())
	{
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

FReply STrafficSystemPanel::OnBuildAndCarsClicked()
{
	if (UTrafficSystemEditorSubsystem* Subsys = GetSubsystem())
	{
		if (!HasDetectedFamilies())
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("No road families detected. Run Prepare Map first.")));
			return FReply::Handled();
		}
		if (!Subsys->HasAnyCalibratedFamilies())
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("No calibrated road families found. Calibrate and bake at least one family first.")));
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
	if (UTrafficSystemEditorSubsystem* Subsys = GetSubsystem())
	{
		if (!HasDetectedFamilies())
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("No road families detected. Run Prepare Map first.")));
			return FReply::Handled();
		}
		if (!Subsys->HasAnyCalibratedFamilies())
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("No calibrated road families found. Calibrate and bake at least one family first.")));
			return FReply::Handled();
		}
		Subsys->DoBuild();
	}
	return FReply::Handled();
}

FReply STrafficSystemPanel::OnCarsClicked()
{
	if (UTrafficSystemEditorSubsystem* Subsys = GetSubsystem())
	{
		if (!HasDetectedFamilies())
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("No road families detected. Run Prepare Map first.")));
			return FReply::Handled();
		}
		if (!Subsys->HasAnyCalibratedFamilies())
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("No calibrated road families found. Calibrate and bake at least one family first.")));
			return FReply::Handled();
		}
		Subsys->DoCars();
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
			Item->DisplayName = Info.DisplayName;
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

bool STrafficSystemPanel::HasDetectedFamilies() const
{
	const URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get();
	return Registry && Registry->GetAllFamilies().Num() > 0;
}

#undef LOCTEXT_NAMESPACE
