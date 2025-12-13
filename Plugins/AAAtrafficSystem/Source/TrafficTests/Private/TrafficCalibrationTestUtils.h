#pragma once

#include "CoreMinimal.h"

class FAutomationTestBase;
class UWorld;
class UTrafficSystemEditorSubsystem;
class UTrafficNetworkAsset;

struct FGuid;
struct FTrafficNetwork;
struct FTrafficLaneFamilyCalibration;

namespace TrafficCalibrationTestUtils
{
	struct FAlignmentEvalParams
	{
		int32 SamplesPerLane = 96;
		float LateralToleranceCm = 20.0f;

		// Fallback sweep when collision closest-point queries are unavailable.
		float TraceHeightAboveCm = 200.0f;
		float TraceDepthBelowCm = 500.0f;
		float SweepRadiusCm = 500.0f;

		// When dynamic mesh vertices are available (CityBLD), estimate road lateral bounds from a local slice.
		float CrossSectionHalfLengthCm = 500.0f;
		int32 MinVertsPerCrossSection = 8;
	};

	struct FAlignmentThresholds
	{
		float MaxMeanDeviationCm = 10.0f;
		float MaxDeviationCm = 20.0f;
		float MaxHeadingErrorDeg = 10.0f;
		float MinCoveragePercent = 95.0f;
	};

	struct FAlignmentMetrics
	{
		int32 TotalSamples = 0;
		int32 SamplesWithSurface = 0;
		int32 SamplesWithinTolerance = 0;

		float MeanLateralDeviationCm = 0.0f;
		float MaxLateralDeviationCm = 0.0f;
		float MaxHeadingErrorDeg = 0.0f;
		float CoveragePercent = 0.0f;

		// Positive means lanes are biased outward (off the road edge), negative inward.
		float MeanOutwardSignedErrorCm = 0.0f;
	};

	UTrafficNetworkAsset* FindBuiltNetworkAsset(UWorld* World);

	bool EvaluateNetworkLaneAlignment(
		UWorld* World,
		const FTrafficNetwork& Network,
		const FAlignmentEvalParams& Params,
		FAlignmentMetrics& OutMetrics);

	bool AlignmentMeetsThresholds(
		const FAlignmentMetrics& Metrics,
		const FAlignmentThresholds& Thresholds,
		FString* OutFailureReason = nullptr);

	bool ApplyCalibrationToRoadFamilySettings(const FTrafficLaneFamilyCalibration& Calib, int32 FamilyIndex = 0);

	FTrafficLaneFamilyCalibration ComputeNextCalibration(
		const FTrafficLaneFamilyCalibration& Current,
		const FAlignmentMetrics& Metrics,
		float MaxStepOffsetCm = 20.0f,
		float MaxStepWidthCm = 8.0f);

	bool RunEditorCalibrationLoop(
		FAutomationTestBase* Test,
		UWorld* World,
        UTrafficSystemEditorSubsystem* Subsys,
        const FString& MetricPrefix,
        const FGuid& FamilyId,
        int32 MaxIterations,
        const FAlignmentEvalParams& EvalParams,
        const FAlignmentThresholds& Thresholds,
        FAlignmentMetrics& OutFinalMetrics,
        FTrafficLaneFamilyCalibration& OutFinalCalibration,
        double MaxWallSeconds = 600.0);
}
