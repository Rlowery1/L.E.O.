#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "TrafficVehicleSettings.generated.h"

class ATrafficVehicleBase;
class AActor;

/**
 * Vehicle defaults for AAA Traffic (test vehicles, Chaos class selection, etc.)
 */
UCLASS(Config=Game, DefaultConfig)
class TRAFFICRUNTIME_API UTrafficVehicleSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UTrafficVehicleSettings();

	/** Default vehicle class to spawn for test traffic (e.g., a Chaos vehicle BP from CitySample). */
	UPROPERTY(EditAnywhere, Config, Category="Vehicles")
	TSoftClassPtr<ATrafficVehicleBase> DefaultTestVehicleClass;

	/** Use an external visual Chaos vehicle (any Actor class) attached to an adapter that follows traffic lanes. */
	UPROPERTY(EditAnywhere, Config, Category="Vehicles")
	bool bUseExternalVehicleAdapter = false;

	/** The external vehicle Blueprint/Class to attach when using the adapter (can be City Sample or any Chaos vehicle). */
	UPROPERTY(EditAnywhere, Config, Category="Vehicles", meta=(EditCondition="bUseExternalVehicleAdapter"))
	TSoftClassPtr<AActor> ExternalVehicleClass;

	/** Tries to auto-resolve a City Sample Chaos sedan if the pack is installed. Returns null if not found. */
	static TSoftClassPtr<AActor> ResolveCitySampleDefaultVisual();

	virtual FName GetCategoryName() const override { return FName(TEXT("Plugins")); }
	virtual FName GetSectionName() const override { return FName(TEXT("AAA Traffic Vehicle Settings")); }

#if WITH_EDITOR
	virtual FText GetSectionText() const override { return FText::FromString(TEXT("AAA Traffic Vehicle Settings")); }
#endif
};
