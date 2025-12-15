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

	/** Automatically spawns a TrafficSystemController and starts traffic when entering PIE/Game (one-button runtime setup). */
	UPROPERTY(EditAnywhere, Config, Category="Runtime", meta=(DisplayName="Auto Spawn Traffic On Begin Play"))
	bool bAutoSpawnTrafficOnBeginPlay = false;

	/** If enabled, only auto-spawn in PIE (not in packaged game). */
	UPROPERTY(EditAnywhere, Config, Category="Runtime", meta=(DisplayName="Auto Spawn Only In PIE"))
	bool bAutoSpawnOnlyInPIE = true;

	/** When auto-spawning, build the transient traffic network first. */
	UPROPERTY(EditAnywhere, Config, Category="Runtime", meta=(EditCondition="bAutoSpawnTrafficOnBeginPlay"))
	bool bAutoBuildNetwork = true;

	/** When auto-spawning, also spawn traffic vehicles. */
	UPROPERTY(EditAnywhere, Config, Category="Runtime", meta=(EditCondition="bAutoSpawnTrafficOnBeginPlay"))
	bool bAutoSpawnVehicles = true;

	/** Vehicles per lane to spawn at runtime (PIE/Game). */
	UPROPERTY(EditAnywhere, Config, Category="Runtime", meta=(EditCondition="bAutoSpawnTrafficOnBeginPlay", ClampMin="0"))
	int32 VehiclesPerLaneRuntime = 8;

	/** Target cruising speed (cm/sec) for kinematic follower vehicles at runtime. */
	UPROPERTY(EditAnywhere, Config, Category="Runtime", meta=(EditCondition="bAutoSpawnTrafficOnBeginPlay", ClampMin="0.0"))
	float RuntimeSpeedCmPerSec = 800.f;

	/** When auto-spawning, generate ZoneGraph and prefer ZoneGraph-based spawning. */
	UPROPERTY(EditAnywhere, Config, Category="Runtime", meta=(EditCondition="bAutoSpawnTrafficOnBeginPlay"))
	bool bGenerateZoneGraph = true;
};
