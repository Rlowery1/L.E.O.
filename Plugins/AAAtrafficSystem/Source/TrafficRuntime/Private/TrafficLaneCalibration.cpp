#include "TrafficLaneCalibration.h"
#include "TrafficLaneGeometry.h"
#include "TrafficRuntimeModule.h"
#include "Engine/World.h"

bool TrafficLaneCalibration::EvaluateLaneSurfaceCoverage(
	UWorld* World,
	const FTrafficLane& Lane,
	int32 NumSamplesAlongLane,
	float TraceHeightAboveCm,
	float TraceDepthBelowCm,
	FLaneSurfaceCoverageMetrics& OutMetrics)
{
	OutMetrics = FLaneSurfaceCoverageMetrics();
	OutMetrics.LaneId = Lane.LaneId;
	OutMetrics.RoadId = Lane.RoadId;

	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficLaneCalibration] World is null."));
		return false;
	}

	const float LaneLength = TrafficLaneGeometry::ComputeLaneLengthCm(Lane);
	if (LaneLength <= KINDA_SMALL_NUMBER || Lane.CenterlinePoints.Num() < 2)
	{
		UE_LOG(LogTraffic, Warning,
			TEXT("[TrafficLaneCalibration] LaneId=%d has insufficient geometry."),
			Lane.LaneId);
		return false;
	}

	NumSamplesAlongLane = FMath::Clamp(NumSamplesAlongLane, 4, 256);

	OutMetrics.NumSamples = NumSamplesAlongLane;
	OutMetrics.NumSamplesOnRoad = 0;
	OutMetrics.CoveragePercent = 0.0f;
	OutMetrics.MaxVerticalGapCm = 0.0f;

	const float StepS = LaneLength / static_cast<float>(NumSamplesAlongLane - 1);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TrafficLaneCalibration), false);
	QueryParams.bReturnPhysicalMaterial = false;

	const ECollisionChannel TraceChannel = ECC_WorldStatic;

	for (int32 i = 0; i < NumSamplesAlongLane; ++i)
	{
		const float S = StepS * i;

		FVector LanePos, LaneTangent;
		if (!TrafficLaneGeometry::SamplePoseAtS(Lane, S, LanePos, LaneTangent))
		{
			continue;
		}

		const FVector Start = LanePos + FVector::UpVector * TraceHeightAboveCm;
		const FVector End = LanePos - FVector::UpVector * TraceDepthBelowCm;

		FHitResult Hit;
		const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, QueryParams);

		if (bHit)
		{
			++OutMetrics.NumSamplesOnRoad;

			const float VerticalGap = FMath::Abs(LanePos.Z - Hit.ImpactPoint.Z);
			OutMetrics.MaxVerticalGapCm = FMath::Max(OutMetrics.MaxVerticalGapCm, VerticalGap);
		}
	}

	if (OutMetrics.NumSamples > 0)
	{
		OutMetrics.CoveragePercent =
			100.0f * static_cast<float>(OutMetrics.NumSamplesOnRoad) / static_cast<float>(OutMetrics.NumSamples);
	}

	return true;
}

bool TrafficLaneCalibration::EvaluateNetworkSurfaceCoverage(
	UWorld* World,
	const FTrafficNetwork& Network,
	int32 NumSamplesPerLane,
	float TraceHeightAboveCm,
	float TraceDepthBelowCm,
	TArray<FLaneSurfaceCoverageMetrics>& OutMetrics)
{
	OutMetrics.Reset();

	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficLaneCalibration] EvaluateNetworkSurfaceCoverage: World is null."));
		return false;
	}

	bool bAnyEvaluated = false;

	for (const FTrafficLane& Lane : Network.Lanes)
	{
		FLaneSurfaceCoverageMetrics Metrics;
		if (EvaluateLaneSurfaceCoverage(
				World,
				Lane,
				NumSamplesPerLane,
				TraceHeightAboveCm,
				TraceDepthBelowCm,
				Metrics))
		{
			OutMetrics.Add(Metrics);
			bAnyEvaluated = true;
		}
	}

	UE_LOG(LogTraffic, Log,
		TEXT("[TrafficLaneCalibration] Evaluated coverage for %d lanes."),
		OutMetrics.Num());

	return bAnyEvaluated;
}

