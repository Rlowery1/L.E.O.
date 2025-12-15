#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "TrafficRuntimeSettings.generated.h"

/**
 * Runtime settings for the AAA Traffic System.
 */
UENUM(BlueprintType)
enum class ETrafficIntersectionControlMode : uint8
{
	Uncontrolled UMETA(DisplayName="Uncontrolled (Yield Only)"),
	FourWayStop UMETA(DisplayName="4-Way Stop"),
	TrafficLightsFixedTime UMETA(DisplayName="Traffic Lights (Fixed Time)")
};

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
	int32 VehiclesPerLaneRuntime = 1;

	/** Target cruising speed (cm/sec) for kinematic follower vehicles at runtime. */
	UPROPERTY(EditAnywhere, Config, Category="Runtime", meta=(EditCondition="bAutoSpawnTrafficOnBeginPlay", ClampMin="0.0"))
	float RuntimeSpeedCmPerSec = 800.f;

	/** When auto-spawning, generate ZoneGraph and prefer ZoneGraph-based spawning. */
	UPROPERTY(EditAnywhere, Config, Category="Runtime", meta=(EditCondition="bAutoSpawnTrafficOnBeginPlay"))
	bool bGenerateZoneGraph = false;

	/**
	 * If enabled, overrides the default intersection stop-line offset (cm).
	 * This is a global setting and applies to all road kits/providers.
	 */
	UPROPERTY(EditAnywhere, Config, Category="Intersections", meta=(DisplayName="Override Stop Line Offset"))
	bool bOverrideIntersectionStopLineOffset = false;

	/** Distance (cm) before an intersection boundary where vehicles stop while yielding (approximate stop line). */
	UPROPERTY(EditAnywhere, Config, Category="Intersections", meta=(EditCondition="bOverrideIntersectionStopLineOffset", ClampMin="0.0"))
	float IntersectionStopLineOffsetCm = 300.f;

	/** Default intersection control mode used at runtime (PIE/Game). */
	UPROPERTY(EditAnywhere, Config, Category="Intersections")
	ETrafficIntersectionControlMode IntersectionControlMode = ETrafficIntersectionControlMode::Uncontrolled;

	/** Fixed-time traffic light: green duration (seconds) per phase. */
	UPROPERTY(EditAnywhere, Config, Category="Intersections|Traffic Lights", meta=(ClampMin="0.1"))
	float TrafficLightGreenSeconds = 10.0f;

	/** Fixed-time traffic light: yellow duration (seconds) per phase. */
	UPROPERTY(EditAnywhere, Config, Category="Intersections|Traffic Lights", meta=(ClampMin="0.0"))
	float TrafficLightYellowSeconds = 2.0f;

	/** Fixed-time traffic light: all-red clearance duration (seconds) between phases. */
	UPROPERTY(EditAnywhere, Config, Category="Intersections|Traffic Lights", meta=(ClampMin="0.0"))
	float TrafficLightAllRedSeconds = 1.0f;
};
