#pragma once

#include "CoreMinimal.h"
#include "TrafficAutomationLogger.generated.h"

struct TRAFFICRUNTIME_API FTrafficRunMetrics
{
	int32 VehiclesSpawned = 0;
	int32 VehiclesCompleted = 0;
	int32 VehiclesStuck = 0;
	int32 VehiclesCulled = 0;

	float MaxLateralErrorCm = 0.f;
	float SumLateralErrorCm = 0.f;
	int32 LateralSamples = 0;

	float MaxHeadingErrorDeg = 0.f;
	float SumHeadingErrorDeg = 0.f;
	int32 HeadingSamples = 0;

	float MinHeadwaySeconds = TNumericLimits<float>::Max();
	float SumHeadwaySeconds = 0.f;
	int32 HeadwaySamples = 0;

	float MaxAccelCmPerSec2 = 0.f;
	float MaxJerkCmPerSec3 = 0.f;

	int32 NumEmergencyBrakes = 0;
	int32 NumHardCollisions = 0;
	float GridlockSeconds = 0.f;

	float SimulatedSeconds = 0.f;

	void AccumulateLateralError(float ErrorCm);
	void AccumulateHeadingError(float ErrorDeg);
	void AccumulateHeadway(float HeadwaySeconds);
	void AccumulateAccel(float AccelCmPerSec2);
	void AccumulateJerk(float JerkCmPerSec3);
	void Finalize();
};

UCLASS()
class TRAFFICRUNTIME_API UTrafficAutomationLogger : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	static void BeginTestLog(const FString& TestName);

	UFUNCTION()
	static void LogLine(const FString& Line);

	UFUNCTION()
	static void LogMetric(const FString& Key, const FString& Value);

	UFUNCTION()
	static void LogMetricFloat(const FString& Key, float Value, int32 Precision = 3);

	UFUNCTION()
	static void LogMetricInt(const FString& Key, int32 Value);

	UFUNCTION()
	static void EndTestLog();

	static void LogRunMetrics(const FString& TestName, const FTrafficRunMetrics& Metrics);

private:
	static FString CurrentLogFilePath;
	static bool bLogOpen;

	static void EnsureDirectoryExists();
	static void AppendToFile(const FString& Text);
};
