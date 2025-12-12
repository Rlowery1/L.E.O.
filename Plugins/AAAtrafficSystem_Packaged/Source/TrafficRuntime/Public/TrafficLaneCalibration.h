#pragma once

#include "CoreMinimal.h"
#include "TrafficRoadTypes.h"
#include "TrafficLaneCalibration.generated.h"

USTRUCT(BlueprintType)
struct TRAFFICRUNTIME_API FLaneSurfaceCoverageMetrics
{
	GENERATED_BODY()

	UPROPERTY()
	int32 LaneId = INDEX_NONE;

	UPROPERTY()
	int32 RoadId = INDEX_NONE;

	UPROPERTY()
	int32 NumSamples = 0;

	UPROPERTY()
	int32 NumSamplesOnRoad = 0;

	UPROPERTY()
	float CoveragePercent = 0.0f;

	UPROPERTY()
	float MaxVerticalGapCm = 0.0f;
};

USTRUCT(BlueprintType)
struct TRAFFICRUNTIME_API FTrafficLaneFamilyCalibration
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Config, Category="Traffic")
	int32 NumLanesPerSideForward = 1;

	UPROPERTY(EditAnywhere, Config, Category="Traffic")
	int32 NumLanesPerSideBackward = 1;

	UPROPERTY(EditAnywhere, Config, Category="Traffic", meta=(ClampMin="50.0", ClampMax="1000.0"))
	float LaneWidthCm = 350.f;

	/** Lateral offset from center for the first lane on each side. */
	UPROPERTY(EditAnywhere, Config, Category="Traffic")
	float CenterlineOffsetCm = 175.f;
};

namespace TrafficLaneCalibration
{
	TRAFFICRUNTIME_API bool EvaluateLaneSurfaceCoverage(
		UWorld* World,
		const FTrafficLane& Lane,
		int32 NumSamplesAlongLane,
		float TraceHeightAboveCm,
		float TraceDepthBelowCm,
		FLaneSurfaceCoverageMetrics& OutMetrics);

	TRAFFICRUNTIME_API bool EvaluateNetworkSurfaceCoverage(
		UWorld* World,
		const FTrafficNetwork& Network,
		int32 NumSamplesPerLane,
		float TraceHeightAboveCm,
		float TraceDepthBelowCm,
		TArray<FLaneSurfaceCoverageMetrics>& OutMetrics);
}

