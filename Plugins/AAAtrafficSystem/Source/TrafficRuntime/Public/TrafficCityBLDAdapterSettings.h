#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPath.h"
#include "TrafficCityBLDAdapterSettings.generated.h"

UCLASS(Config=Game, DefaultConfig)
class TRAFFICRUNTIME_API UTrafficCityBLDAdapterSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UTrafficCityBLDAdapterSettings();

	virtual FName GetCategoryName() const override
	{
		return TEXT("Plugins");
	}

	virtual FName GetSectionName() const override
	{
		return TEXT("AAA Traffic System - CityBLD Adapter");
	}

	UPROPERTY(EditAnywhere, Config, Category="Detection")
	FName RoadActorTag;

	UPROPERTY(EditAnywhere, Config, Category="Detection")
	TArray<FString> RoadClassNameContains;

	UPROPERTY(EditAnywhere, Config, Category="Detection")
	FName RoadSplineTag;

	UPROPERTY(EditAnywhere, Config, Category="Families")
	FName DefaultFamilyName;

	/** List of substrings to match drivable materials (e.g. "Asphalt", "Road"). If empty, material names are ignored. */
	UPROPERTY(EditAnywhere, Config, Category="Filtering")
	TArray<FString> DrivableMaterialKeywords;

	/** Maximum lateral offset (cm) from actor forward/right for a mesh to be considered part of the road. Set to 0 to disable. */
	UPROPERTY(EditAnywhere, Config, Category="Filtering")
	float MaxMeshLateralOffsetCm = 0.f;

	/** Maximum mesh height (in cm) to consider it part of the drivable surface. Set to 0 to disable. */
	UPROPERTY(EditAnywhere, Config, Category="Filtering")
	float MaxMeshHeightCm = 200.f;

	/** Negative keywords: exclude meshes if the name (component or mesh) contains any of these substrings. */
	UPROPERTY(EditAnywhere, Config, Category="Filtering")
	TArray<FString> ExcludedMeshNameKeywords;

	/** Draws provider-derived display centerlines during calibration (debug only). */
	UPROPERTY(EditAnywhere, Config, Category="Debug")
	bool bDrawCalibrationCenterlineDebug = false;

	/** Uses the BP_MeshRoad control spline directly for the display centerline (recommended for calibration overlay). */
	UPROPERTY(EditAnywhere, Config, Category="Centerline")
	bool bUseControlSplineForDisplayCenterline = true;

	/** Uses ZoneGraph lane polylines (when available) for CityBLD calibration overlay arrow placement. */
	UPROPERTY(EditAnywhere, Config, Category="ZoneGraph")
	bool bUseZoneGraphLanePolylinesForCalibrationOverlay = true;

	/** Default VehicleLaneProfile asset used when auto-building ZoneGraph for CityBLD calibration. */
	UPROPERTY(EditAnywhere, Config, Category="ZoneGraph")
	FSoftObjectPath DefaultCityBLDVehicleLaneProfile;

	// NOTE: MinMeshAspectRatio filter has been removed to avoid excluding valid road meshes.
};
