#pragma once

#include "CoreMinimal.h"
#include "TrafficRoadFamilySettings.h"
#include "TrafficLaneCalibration.h"
#include "UObject/SoftObjectPath.h"
#include "RoadFamilyRegistry.generated.h"

USTRUCT(BlueprintType)
struct TRAFFICRUNTIME_API FRoadFamilyInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Config, Category="Traffic")
	FGuid FamilyId;

	UPROPERTY(EditAnywhere, Config, Category="Traffic")
	FString DisplayName;

	UPROPERTY(EditAnywhere, Config, Category="Traffic")
	FSoftClassPath RoadClassPath;

	UPROPERTY(EditAnywhere, Config, Category="Traffic")
	FRoadFamilyDefinition FamilyDefinition;

	UPROPERTY(EditAnywhere, Config, Category="Traffic")
	FTrafficLaneFamilyCalibration CalibrationData;

	UPROPERTY(EditAnywhere, Config, Category="Traffic")
	bool bIsCalibrated = false;

	// Last calibration snapshot (backup) for quick restore.
	UPROPERTY()
	FTrafficLaneFamilyCalibration BackupCalibration;

	UPROPERTY()
	bool bHasBackupCalibration = false;
};

/**
 * Maintains a per-road-class registry of discovered road families for editor automation.
 * Families are persisted in editor user settings and keyed by the road actor class.
 */
UCLASS(Config=EditorPerProjectUserSettings)
class TRAFFICRUNTIME_API URoadFamilyRegistry : public UObject
{
	GENERATED_BODY()

public:
	static URoadFamilyRegistry* Get();

	/** Returns existing family or creates a new one for the provided road class. */
	FRoadFamilyInfo* FindOrCreateFamilyForClass(UClass* RoadClass, bool* bOutCreated = nullptr);

	/** Finds family by id. */
	FRoadFamilyInfo* FindFamilyById(const FGuid& FamilyId);
	const FRoadFamilyInfo* FindFamilyById(const FGuid& FamilyId) const;

	/** Finds family by class if already registered. */
	const FRoadFamilyInfo* FindFamilyByClass(UClass* RoadClass) const;

	const TArray<FRoadFamilyInfo>& GetAllFamilies() const { return Families; }
	TArray<FRoadFamilyInfo> GetAllFamiliesCopy() const { return Families; }

	/** Renames the display name (persisted) and keeps the calibration FamilyName in sync. */
	bool RenameFamily(const FGuid& FamilyId, const FString& NewDisplayName);
	bool SetFamilyDisplayName(const FGuid& FamilyId, const FString& NewDisplayName) { return RenameFamily(FamilyId, NewDisplayName); }

	/** Apply calibration data and mark calibrated. */
	void ApplyCalibration(const FGuid& FamilyId, const FTrafficLaneFamilyCalibration& NewCalibration);

	/** Restore last backed-up calibration if present. */
	bool RestoreLastCalibration(const FGuid& FamilyId);

	/** Find a family by id (const). */
	const FRoadFamilyInfo* GetFamilyById(const FGuid& FamilyId) const { return FindFamilyById(FamilyId); }

	/** Rebuilds internal caches if classes changed on disk. */
	void RefreshCache() const;

private:
	UPROPERTY(Config)
	TArray<FRoadFamilyInfo> Families;

	mutable TMap<UClass*, int32> ClassToIndex;

	void RebuildClassCache() const;
	FString SanitizeDisplayName(const FString& InClassName) const;
};
