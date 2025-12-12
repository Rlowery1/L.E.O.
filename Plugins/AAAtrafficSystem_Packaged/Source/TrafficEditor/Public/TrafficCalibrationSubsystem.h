#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "TrafficCalibrationSubsystem.generated.h"

class ARoadLanePreviewActor;
class UTrafficRoadMetadataComponent;
class UTrafficNetworkAsset;

UCLASS()
class TRAFFICEDITOR_API UTrafficCalibrationSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void SpawnPreviewForSelected();

	void PrepareAllRoads();

	void PreviewSelectedRoadFamily();

	UFUNCTION(CallInEditor, Category="Traffic")
	void Editor_BuildTrafficNetwork();

	UFUNCTION(CallInEditor, Category="Traffic")
	void Editor_BuildTrafficNetworkToAsset(const FString& AssetPath);

private:
	UTrafficRoadMetadataComponent* GetOrAddRoadMetadata(AActor* Actor) const;
};
