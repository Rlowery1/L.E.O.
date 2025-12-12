#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UTrafficSystemEditorSubsystem;
struct FRoadFamilyInfo;
class SEditableTextBox;

class STrafficSystemPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STrafficSystemPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	struct FFamilyListItem
	{
		FGuid FamilyId;
		FString DisplayName;
		FString ClassDisplay;
		bool bIsCalibrated = false;
	};

	FReply OnBeginCalibrationClicked();
	FReply OnBakeCalibrationClicked();
	FReply OnRestoreCalibrationClicked();
	FReply OnResetLabClicked();
	FReply OnResetLabIncludingTaggedClicked();
	FReply OnConvertSelectedClicked();
	FReply OnBuildAndCarsClicked();
	FReply OnPrepareClicked();
	FReply OnBuildClicked();
	FReply OnCarsClicked();
	FReply OnPrepareMapForTrafficClicked();

	bool CanCalibrate() const;
	bool CanBake() const;
	UTrafficSystemEditorSubsystem* GetSubsystem() const;

	void RefreshFamilyList();
	TSharedRef<SWidget> OnGenerateFamilyItem(TSharedPtr<FFamilyListItem> Item) const;
	void OnFamilySelectionChanged(TSharedPtr<FFamilyListItem> Item, ESelectInfo::Type SelectInfo);
	FText GetSelectedFamilyLabel() const;
	FText GetSelectedFamilyEditableText() const;
	void OnFamilyNameCommitted(const FText& NewText, ETextCommit::Type CommitType);
	bool HasDetectedFamilies() const;
	bool HasBackupForSelected() const;

	TArray<TSharedPtr<FFamilyListItem>> FamilyItems;
	TSharedPtr<FFamilyListItem> SelectedFamily;
	TSharedPtr<SEditableTextBox> FamilyNameEdit;
};
