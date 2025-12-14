#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "TrafficSystemEditorSubsystem.generated.h"

class ATrafficSystemController;
class UTrafficCalibrationSubsystem;
class ALaneCalibrationOverlayActor;
class USplineComponent;
class UProceduralMeshComponent;
struct FRoadFamilyDefinition;

USTRUCT()
struct FCalibrationSnippet
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FVector> SnippetPoints;

	UPROPERTY()
	float SnippetLength = 0.f;

	UPROPERTY()
	int32 StartPointIndex = 0;

	UPROPERTY()
	int32 EndPointIndex = 0;
};

UCLASS()
class TRAFFICEDITOR_API UTrafficSystemEditorSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void DoPrepare();
	void DoBuild();
	void DoCars();
	void DoDrawIntersectionDebug();

	/** Scans the current level for spline-based roads and prepares families/metadata. */
	void Editor_PrepareMapForTraffic();

	void ResetRoadLab();
	void Editor_ResetRoadLabHard(bool bIncludeTaggedUserRoads = false);
	void ConvertSelectedSplineActors();

	void CalibrateSelectedRoad();
	bool CanCalibrateSelectedRoad() const;

	bool ExtractCalibrationSnippet(AActor* RoadActor, FCalibrationSnippet& OutSnippet);

	void Editor_CreateRoadLabMap(bool bOverwriteExisting = false);

private:
	ATrafficSystemController* GetOrSpawnController();
	UWorld* GetEditorWorld() const;

public:
	// Calibration session management
	UFUNCTION(CallInEditor)
	void Editor_BeginCalibrationForFamily(const FGuid& FamilyId);

	UFUNCTION(CallInEditor)
	void Editor_BakeCalibrationForActiveFamily();

	UFUNCTION(CallInEditor)
	void Editor_RestoreCalibrationForFamily(const FGuid& FamilyId);

	// True if any actor in the editor world has traffic road metadata (prepared).
	bool HasAnyPreparedRoads() const;

	bool HasAnyCalibratedFamilies() const;

	// Returns how many actors for this road family exist in the current editor world.
	int32 GetNumActorsForFamily(const FGuid& FamilyId) const;
	void GetActorsForFamily(const FGuid& FamilyId, TArray<AActor*>& OutActors) const;

private:
	void TagAsRoadLab(AActor* Actor);
	void FocusCameraOnActor(AActor* Actor);
	void BuildRoadRibbonForActor(AActor* RoadActor, const FRoadFamilyDefinition& Family);
	void Editor_DrawCenterlineDebug(UWorld* World, const TArray<FVector>& CenterlinePoints, bool bColorByCurvature = true) const;
	float TraceGroundHeight(UWorld* World, float X, float Y, float FallbackZ);
	bool EnsureDetectedFamilies(const TCHAR* Context) const;

	UPROPERTY()
	TWeakObjectPtr<ALaneCalibrationOverlayActor> CalibrationOverlayActor;

	UPROPERTY()
	TWeakObjectPtr<AActor> ActiveCalibrationRoadActor;

	UPROPERTY()
	FGuid ActiveFamilyId;

	UPROPERTY()
	TArray<UProceduralMeshComponent*> RoadLabRibbonMeshes;

	static const FName RoadLabTag;
	static constexpr float SnippetMinLength = 4000.f;
	static constexpr float SnippetMaxLength = 6000.f;
};
