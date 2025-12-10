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

	static const UTrafficVehicleSettings* Get();

	virtual FName GetCategoryName() const override { return FName(TEXT("Plugins")); }
	virtual FName GetSectionName() const override { return FName(TEXT("AAA Traffic Vehicle Settings")); }

#if WITH_EDITOR
	virtual FText GetSectionText() const override { return FText::FromString(TEXT("AAA Traffic Vehicle Settings")); }
#endif
};
