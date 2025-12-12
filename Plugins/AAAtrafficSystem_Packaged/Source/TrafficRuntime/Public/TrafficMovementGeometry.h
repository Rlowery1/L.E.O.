#pragma once

#include "CoreMinimal.h"
#include "TrafficRoadTypes.h"
#include "TrafficMovementGeometry.generated.h"

USTRUCT(BlueprintType)
struct TRAFFICRUNTIME_API FMovementSample
{
	GENERATED_BODY()

	UPROPERTY()
	float S = 0.0f;

	UPROPERTY()
	FVector Position = FVector::ZeroVector;

	UPROPERTY()
	FVector Tangent = FVector::ForwardVector;

	UPROPERTY()
	float Curvature = 0.0f;
};

namespace TrafficMovementGeometry
{
	TRAFFICRUNTIME_API void BuildSmoothMovementPath(
		const FVector& InStartPos,
		const FVector& InStartDir,
		const FVector& InEndPos,
		const FVector& InEndDir,
		int32 NumSamples,
		FTrafficMovement& InOutMovement);

	TRAFFICRUNTIME_API void AnalyzeMovementPath(
		const FTrafficMovement& Movement,
		TArray<FMovementSample>& OutSamples);

	TRAFFICRUNTIME_API bool SamplePoseAtS(
		const FTrafficMovement& Movement,
		float S,
		FVector& OutPosition,
		FVector& OutTangent);
}

