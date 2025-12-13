#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrafficRoadFamilySettings.h"
#include "TrafficLaneCalibration.h"
#include "LaneCalibrationOverlayActor.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;
class UProceduralMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UTrafficVisualSettings;

UCLASS()
class TRAFFICEDITOR_API ALaneCalibrationOverlayActor : public AActor
{
	GENERATED_BODY()

public:
	ALaneCalibrationOverlayActor();

	virtual void BeginPlay() override;

	void BuildForRoad(
		const TArray<FVector>& CenterlinePoints,
		const FRoadFamilyDefinition& Family,
		const FTransform& RoadTransform,
		bool bHasUnderlyingMesh);

	// Build overlay lanes from a provided centerline and calibration data.
	void BuildFromCenterline(const TArray<FVector>& CenterlinePoints, const FTrafficLaneFamilyCalibration& Calibration, const FTransform& RoadTransform = FTransform::Identity);

	// Build overlay arrows directly from lane polylines (e.g. extracted from ZoneGraph).
	void BuildFromLanePolylines(const TArray<TArray<FVector>>& LanePolylines, const FTransform& RoadTransform = FTransform::Identity);

	void ClearOverlay();

	// Editable preview settings exposed for calibration UI.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Calibration")
	int32 NumLanesPerSideForward = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Calibration")
	int32 NumLanesPerSideBackward = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Calibration", meta=(ClampMin="50.0", ClampMax="1000.0"))
	float LaneWidthCm = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Calibration")
	float CenterlineOffsetCm = 175.f;

	void ApplyCalibrationSettings(int32 InNumForward, int32 InNumBackward, float InLaneWidthCm, float InCenterOffsetCm);

	int32 GetArrowInstanceCount() const;

	// Rebuild overlay from cached data (used by editor property edits).
	UFUNCTION(CallInEditor, Category="Traffic|Calibration")
	void Editor_RebuildFromCachedCenterline();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY()
	TArray<UProceduralMeshComponent*> LaneRibbonMeshes;

	UPROPERTY()
	TArray<UInstancedStaticMeshComponent*> ChevronArrowComponents;

	UPROPERTY()
	UStaticMesh* FallbackArrowMesh;

	UPROPERTY()
	UMaterialInterface* DefaultArrowMaterial;

	TArray<FVector> ComputeLaneCenterline(
		const TArray<FVector>& RoadCenterline,
		float LateralOffset);

	bool IsCalibrationValid(const FTrafficLaneFamilyCalibration& Calib) const;

	void BuildLaneRibbon(
		const TArray<FVector>& LaneCenterPoints,
		float LaneWidth,
		bool bForwardDirection,
		int32 LaneIndex,
		const UTrafficVisualSettings* VisualSettings);

	void BuildChevronArrows(
		UInstancedStaticMeshComponent* ISM,
		const TArray<FVector>& LaneCenterPoints,
		bool bForwardDirection,
		const UTrafficVisualSettings* VisualSettings);

	UStaticMesh* GetArrowMesh(bool bForwardDirection, const UTrafficVisualSettings* VisualSettings) const;

	UMaterialInstanceDynamic* CreateArrowMaterialInstance(
		UInstancedStaticMeshComponent* ISM,
		const FLinearColor& Color);

	UPROPERTY()
	TArray<FVector> CachedCenterlinePoints;

	UPROPERTY()
	FTrafficLaneFamilyCalibration CachedCalibration;

	UPROPERTY()
	FTransform CachedRoadTransform;
};
