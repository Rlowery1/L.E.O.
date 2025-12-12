#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "TrafficRuntimeSettings.generated.h"

/**
 * Runtime settings for the AAA Traffic System.
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="AAA Traffic Settings"))
class TRAFFICRUNTIME_API UTrafficRuntimeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Enables CityBLD-specific adapter behaviour (BP_MeshRoad detection). */
	UPROPERTY(EditAnywhere, Config, Category="Adapters", meta=(DisplayName="Enable CityBLD Adapter"))
	bool bEnableCityBLDAdapter = true;
};
