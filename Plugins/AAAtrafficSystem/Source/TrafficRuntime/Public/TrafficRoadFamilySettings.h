#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPath.h"
#include "TrafficRoadFamilySettings.generated.h"

USTRUCT(BlueprintType)
struct TRAFFICRUNTIME_API FTrafficLaneLayoutSide
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Config, Category="Traffic")
	int32 NumLanes = 1;

	UPROPERTY(EditAnywhere, Config, Category="Traffic", meta=(ClampMin="50.0", ClampMax="1000.0"))
	float LaneWidthCm = 350.f;

	UPROPERTY(EditAnywhere, Config, Category="Traffic")
	float InnerLaneCenterOffsetCm = 175.f;
};

USTRUCT(BlueprintType)
struct TRAFFICRUNTIME_API FRoadFamilyDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Config, Category="Traffic")
	FName FamilyName;

	UPROPERTY(EditAnywhere, Config, Category="Traffic")
	FTrafficLaneLayoutSide Forward;

	UPROPERTY(EditAnywhere, Config, Category="Traffic")
	FTrafficLaneLayoutSide Backward;

	UPROPERTY(EditAnywhere, Config, Category="Traffic", meta=(ClampMin="0.0", ClampMax="300.0"))
	float DefaultSpeedLimitKmh = 50.f;

	/**
	 * ZoneGraph lane profile assets (AAA Traffic wrapper) used when generating ZoneGraph shapes for this family.
	 * Expected to reference assets under the AAAtrafficSystem plugin mount point, e.g.:
	 *   /AAAtrafficSystem/ZoneProfiles/CityBLDUrbanTwoLane.CityBLDUrbanTwoLane
	 */
	UPROPERTY(EditAnywhere, Config, Category="ZoneGraph")
	FSoftObjectPath VehicleLaneProfile;

	/** Optional lane profile for footpaths/sidewalks when generating ZoneGraph shapes. */
	UPROPERTY(EditAnywhere, Config, Category="ZoneGraph")
	FSoftObjectPath FootpathLaneProfile;
};

UCLASS(Config=Game, DefaultConfig)
class TRAFFICRUNTIME_API UTrafficRoadFamilySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UTrafficRoadFamilySettings();

	UPROPERTY(EditAnywhere, Config, Category="Families")
	TArray<FRoadFamilyDefinition> Families;

	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;

	const FRoadFamilyDefinition* FindFamilyByName(FName FamilyName) const;
};
