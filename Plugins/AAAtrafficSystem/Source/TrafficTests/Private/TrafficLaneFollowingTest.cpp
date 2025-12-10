#include "Misc/AutomationTest.h"
#include "TrafficRoadTypes.h"
#include "TrafficLaneGeometry.h"
#include "TrafficAutomationLogger.h"
#include "Modules/ModuleManager.h"
#include "TrafficRuntimeModule.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficLaneFollowingStraightSyntheticTest,
	"Traffic.Motion.LaneFollowing.StraightSynthetic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FTrafficLaneFollowingStraightSyntheticTest::RunTest(const FString& Parameters)
{
	const FString LocalTestName = TEXT("Traffic.Motion.LaneFollowing.StraightSynthetic");
	UTrafficAutomationLogger::BeginTestLog(LocalTestName);
	UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION)));

	if (!FModuleManager::Get().IsModuleLoaded("TrafficRuntime"))
	{
		AddError(TEXT("TrafficRuntime module is not loaded."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=TrafficRuntime module not loaded"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	FTrafficLane Lane;
	Lane.LaneId = 0;
	Lane.RoadId = 0;
	Lane.SideIndex = 0;
	Lane.Width = 350.f;
	Lane.Direction = ELaneDirection::Forward;
	Lane.SpeedLimitKmh = 50.f;

	const int32 NumPoints = 11;
	const float SegmentLength = 1000.f;
	for (int32 i = 0; i < NumPoints; ++i)
	{
		const float X = i * SegmentLength;
		Lane.CenterlinePoints.Add(FVector(X, 0.f, 0.f));
	}

	const float LaneLength = TrafficLaneGeometry::ComputeLaneLengthCm(Lane);
	UTrafficAutomationLogger::LogMetricFloat(TEXT("LaneLengthCm"), LaneLength, 1);

	const int32 NumSamples = 100;
	UTrafficAutomationLogger::LogMetric(TEXT("NumSamples"), FString::FromInt(NumSamples));

	float MaxLateralErrorCm = 0.0f;
	float SumLateralErrorSq = 0.0f;
	float MaxHeadingErrorDeg = 0.0f;

	int32 LastSegmentIndex = 0;

	for (int32 i = 0; i < NumSamples; ++i)
	{
		const float Alpha = (NumSamples > 1) ? static_cast<float>(i) / static_cast<float>(NumSamples - 1) : 0.0f;
		const float TargetS = LaneLength * Alpha;

		TArray<float> Cumulative;
		TrafficLaneGeometry::BuildCumulativeDistances(Lane, Cumulative);

		int32 SegmentIdx = 0;
		for (int32 s = 0; s < Cumulative.Num() - 1; ++s)
		{
			if (TargetS >= Cumulative[s] && TargetS <= Cumulative[s + 1])
			{
				SegmentIdx = s;
				break;
			}
		}

		const FVector A = Lane.CenterlinePoints[SegmentIdx];
		const FVector B = Lane.CenterlinePoints[SegmentIdx + 1];
		const float SegStartS = Cumulative[SegmentIdx];
		const float SegLen = FVector::Dist(A, B);
		const float LocalT = (SegLen > KINDA_SMALL_NUMBER)
			? (TargetS - SegStartS) / SegLen
			: 0.0f;

		const FVector IdealPos = FMath::Lerp(A, B, LocalT);
		const FVector LaneDir = (B - A).GetSafeNormal();

		FVector VehiclePos = IdealPos;
		FVector VehicleForward = LaneDir;

		FLaneProjectionResult Proj;
		const bool bOk = TrafficLaneGeometry::ProjectPointOntoLane(
			Lane,
			VehiclePos,
			VehicleForward,
			Proj,
			LastSegmentIndex);

		if (!bOk)
		{
			AddError(TEXT("ProjectPointOntoLane failed for synthetic straight lane."));
			UTrafficAutomationLogger::LogLine(TEXT("Error=ProjectPointOntoLane failed"));
			UTrafficAutomationLogger::EndTestLog();
			return false;
		}

		LastSegmentIndex = Proj.SegmentIndex;

		const float LateralError = Proj.LateralOffsetCm;
		const float HeadingError = Proj.HeadingErrorDeg;

		MaxLateralErrorCm = FMath::Max(MaxLateralErrorCm, FMath::Abs(LateralError));
		MaxHeadingErrorDeg = FMath::Max(MaxHeadingErrorDeg, FMath::Abs(HeadingError));
		SumLateralErrorSq += LateralError * LateralError;
	}

	const float RmsLateralErrorCm = (NumSamples > 0)
		? FMath::Sqrt(SumLateralErrorSq / static_cast<float>(NumSamples))
		: 0.0f;

	UTrafficAutomationLogger::LogMetricFloat(TEXT("MaxLateralErrorCm"), MaxLateralErrorCm, 4);
	UTrafficAutomationLogger::LogMetricFloat(TEXT("RmsLateralErrorCm"), RmsLateralErrorCm, 4);
	UTrafficAutomationLogger::LogMetricFloat(TEXT("MaxHeadingErrorDeg"), MaxHeadingErrorDeg, 4);

	const float MaxLateralToleranceCm = 0.5f;
	const float RmsLateralToleranceCm = 0.2f;
	const float MaxHeadingToleranceDeg = 0.1f;

	const bool bWithinMaxLat = (MaxLateralErrorCm <= MaxLateralToleranceCm);
	const bool bWithinRmsLat = (RmsLateralErrorCm <= RmsLateralToleranceCm);
	const bool bWithinHeading = (MaxHeadingErrorDeg <= MaxHeadingToleranceDeg);

	if (!bWithinMaxLat)
	{
		AddError(FString::Printf(TEXT("MaxLateralErrorCm too large: %f"), MaxLateralErrorCm));
	}

	if (!bWithinRmsLat)
	{
		AddError(FString::Printf(TEXT("RmsLateralErrorCm too large: %f"), RmsLateralErrorCm));
	}

	if (!bWithinHeading)
	{
		AddError(FString::Printf(TEXT("MaxHeadingErrorDeg too large: %f"), MaxHeadingErrorDeg));
	}

	UTrafficAutomationLogger::LogMetric(TEXT("WithinMaxLateralTolerance"), bWithinMaxLat ? TEXT("true") : TEXT("false"));
	UTrafficAutomationLogger::LogMetric(TEXT("WithinRmsLateralTolerance"), bWithinRmsLat ? TEXT("true") : TEXT("false"));
	UTrafficAutomationLogger::LogMetric(TEXT("WithinHeadingTolerance"), bWithinHeading ? TEXT("true") : TEXT("false"));

	UTrafficAutomationLogger::EndTestLog();

	return bWithinMaxLat && bWithinRmsLat && bWithinHeading;
}

