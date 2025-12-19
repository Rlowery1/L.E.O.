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

UENUM(BlueprintType)
enum class ETrafficTurnPolicy : uint8
{
	ThroughFirst UMETA(DisplayName="Through First"),
	LeftFirst UMETA(DisplayName="Left First"),
	RightFirst UMETA(DisplayName="Right First"),
	PreferNonThrough UMETA(DisplayName="Prefer Non-Through (Turns)")
};

UENUM(BlueprintType)
enum class ETrafficSignalCoordinationAxis : uint8
{
	Phase0Axis UMETA(DisplayName="Phase 0 Axis"),
	WorldX UMETA(DisplayName="World X"),
	WorldY UMETA(DisplayName="World Y"),
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

	/** Enables fixed-time signal coordination (green wave) along a chosen axis. */
	UPROPERTY(EditAnywhere, Config, Category="Intersections|Traffic Lights")
	bool bTrafficLightCoordinationEnabled = false;

	/** Coordination speed (cm/sec) used to compute phase offsets along the chosen axis. */
	UPROPERTY(EditAnywhere, Config, Category="Intersections|Traffic Lights", meta=(ClampMin="1.0"))
	float TrafficLightCoordinationSpeedCmPerSec = 1500.0f;

	/** Axis used to compute signal offsets for coordination. */
	UPROPERTY(EditAnywhere, Config, Category="Intersections|Traffic Lights")
	ETrafficSignalCoordinationAxis TrafficLightCoordinationAxis = ETrafficSignalCoordinationAxis::Phase0Axis;

	/** Permitted-left behavior in signalized intersections. */
	UPROPERTY(EditAnywhere, Config, Category="Intersections|Traffic Lights", meta=(DisplayName="Permitted Left Turns Yield"))
	bool bTrafficLightPermittedLeftYield = true;

	/** Distance (cm) before the stop line to treat oncoming vehicles as priority when permitting left turns. */
	UPROPERTY(EditAnywhere, Config, Category="Intersections|Traffic Lights", meta=(ClampMin="0.0"))
	float TrafficLightPermittedLeftApproachDistanceCm = 1200.0f;

	/** Default routing turn policy used at runtime (PIE/Game). */
	UPROPERTY(EditAnywhere, Config, Category="Routing")
	ETrafficTurnPolicy TurnPolicy = ETrafficTurnPolicy::ThroughFirst;
};
