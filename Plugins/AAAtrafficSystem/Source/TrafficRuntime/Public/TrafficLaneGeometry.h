#pragma once

#include "CoreMinimal.h"
#include "TrafficRoadTypes.h"
#include "TrafficLaneGeometry.generated.h"

USTRUCT(BlueprintType)
struct TRAFFICRUNTIME_API FLaneProjectionResult
{
	GENERATED_BODY()

	UPROPERTY()
	int32 LaneId = INDEX_NONE;

	UPROPERTY()
	float S = 0.0f;

	UPROPERTY()
	FVector ClosestPoint = FVector::ZeroVector;

	UPROPERTY()
	float LateralOffsetCm = 0.0f;

	UPROPERTY()
	float HeadingErrorDeg = 0.0f;

	UPROPERTY()
	int32 SegmentIndex = INDEX_NONE;
};

namespace TrafficLaneGeometry
{
	TRAFFICRUNTIME_API float ComputeLaneLengthCm(const FTrafficLane& Lane);

	TRAFFICRUNTIME_API void BuildCumulativeDistances(
		const FTrafficLane& Lane,
		TArray<float>& OutCumulative);

	TRAFFICRUNTIME_API bool ProjectPointOntoLane(
		const FTrafficLane& Lane,
		const FVector& WorldLocation,
		const FVector& VehicleForward,
		FLaneProjectionResult& OutResult,
		int32 SearchStartIndex = 0);

	TRAFFICRUNTIME_API bool SamplePoseAtS(
		const FTrafficLane& Lane,
		float S,
		FVector& OutPosition,
		FVector& OutTangent);
}

