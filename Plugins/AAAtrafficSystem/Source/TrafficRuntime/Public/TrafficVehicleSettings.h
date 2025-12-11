#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "TrafficVehicleSettings.generated.h"

class UTrafficVehicleProfile;

/**
 * Vehicle defaults for AAA Traffic (test vehicles, Chaos class selection, etc.)
 */
UCLASS(Config=Game, DefaultConfig)
class TRAFFICRUNTIME_API UTrafficVehicleSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UTrafficVehicleSettings();

	/** Default vehicle profile data asset (can point to a Chaos vehicle profile in project content). */
	UPROPERTY(EditAnywhere, Config, Category="TrafficVehicle")
	FSoftObjectPath DefaultVehicleProfile;

	/** Optional additional profiles for variety (not yet sampled automatically). */
	UPROPERTY(EditAnywhere, Config, Category="TrafficVehicle")
	TArray<FSoftObjectPath> AdditionalVehicleProfiles;

	/** Minimum spacing multiplier between spawned vehicles: min spacing = VehicleLengthCm * MinSpawnSpacingMultiplier. */
	UPROPERTY(EditAnywhere, Config, Category="TrafficVehicle|Spawning", meta=(ClampMin="1.0"))
	float MinSpawnSpacingMultiplier = 2.0f;

	/** Minimum usable lane length to consider spawning vehicles (in cm). */
	UPROPERTY(EditAnywhere, Config, Category="TrafficVehicle|Spawning", meta=(ClampMin="0.0"))
	float MinUsableLaneLengthCm = 2000.f; // 20m default threshold

	static const UTrafficVehicleSettings* Get();

	/** Resolve the configured default vehicle profile (may load the asset). */
	const class UTrafficVehicleProfile* GetDefaultVehicleProfile() const;

	virtual FName GetCategoryName() const override { return FName(TEXT("Plugins")); }
	virtual FName GetSectionName() const override { return FName(TEXT("AAA Traffic Vehicle Settings")); }

#if WITH_EDITOR
	virtual FText GetSectionText() const override { return FText::FromString(TEXT("AAA Traffic Vehicle Settings")); }
#endif
};
