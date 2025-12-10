#include "Misc/AutomationTest.h"
#include "TrafficRoadTypes.h"
#include "TrafficLaneGeometry.h"
#include "TrafficKinematicFollower.h"
#include "TrafficAutomationLogger.h"
#include "Modules/ModuleManager.h"
#include "TrafficRuntimeModule.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficLaneFollowingKinematicStraightTest,
	"Traffic.Motion.LaneFollowing.KinematicStraight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FTrafficLaneFollowingKinematicStraightTest::RunTest(const FString& Parameters)
{
	const FString LocalTestName = TEXT("Traffic.Motion.LaneFollowing.KinematicStraight");
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

	UTrafficKinematicFollower* Follower = NewObject<UTrafficKinematicFollower>();
	const float SpeedCmPerSec = 1000.f;
	Follower->InitForLane(&Lane, 0.0f, SpeedCmPerSec);

	const int32 NumSteps = 100;
	const float TotalTime = LaneLength / SpeedCmPerSec;
	const float DeltaTime = (NumSteps > 0) ? (TotalTime / NumSteps) : 0.0f;

	UTrafficAutomationLogger::LogMetric(TEXT("NumSteps"), FString::FromInt(NumSteps));
	UTrafficAutomationLogger::LogMetricFloat(TEXT("TotalTime"), TotalTime, 3);

	FTrafficRunMetrics Metrics;
	float MaxLateralErrorCm = 0.0f;
	float SumLateralErrorSq = 0.0f;
	float MaxHeadingErrorDeg = 0.0f;

	for (int32 StepIndex = 0; StepIndex < NumSteps; ++StepIndex)
	{
		Follower->Step(DeltaTime);

		FVector Pos, Tangent;
		const bool bPoseOk = Follower->GetCurrentPose(Pos, Tangent);
		if (!bPoseOk)
		{
			AddError(TEXT("UTrafficKinematicFollower::GetCurrentPose failed."));
			UTrafficAutomationLogger::LogLine(TEXT("Error=GetCurrentPose failed"));
			UTrafficAutomationLogger::EndTestLog();
			return false;
		}

		FLaneProjectionResult Proj;
		const bool bProjOk = TrafficLaneGeometry::ProjectPointOntoLane(
			Lane,
			Pos,
			Tangent,
			Proj);

		if (!bProjOk)
		{
			AddError(TEXT("ProjectPointOntoLane failed for kinematic follower."));
			UTrafficAutomationLogger::LogLine(TEXT("Error=ProjectPointOntoLane failed"));
			UTrafficAutomationLogger::EndTestLog();
			return false;
		}

		const float LateralError = Proj.LateralOffsetCm;
		const float HeadingError = Proj.HeadingErrorDeg;

		MaxLateralErrorCm = FMath::Max(MaxLateralErrorCm, FMath::Abs(LateralError));
		MaxHeadingErrorDeg = FMath::Max(MaxHeadingErrorDeg, FMath::Abs(HeadingError));
		SumLateralErrorSq += LateralError * LateralError;

		Metrics.AccumulateLateralError(FMath::Abs(LateralError));
		Metrics.AccumulateHeadingError(FMath::Abs(HeadingError));
	}

	const float RmsLateralErrorCm = (NumSteps > 0)
		? FMath::Sqrt(SumLateralErrorSq / static_cast<float>(NumSteps))
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

	UTrafficAutomationLogger::LogMetric(TEXT("WithinMaxLateralTolerance"), bWithinMaxLat ? TEXT("true") : TEXT("false"));
	UTrafficAutomationLogger::LogMetric(TEXT("WithinRmsLateralTolerance"), bWithinRmsLat ? TEXT("true") : TEXT("false"));
	UTrafficAutomationLogger::LogMetric(TEXT("WithinHeadingTolerance"), bWithinHeading ? TEXT("true") : TEXT("false"));

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

	Metrics.SimulatedSeconds = TotalTime;
	Metrics.Finalize();
	UTrafficAutomationLogger::LogRunMetrics(LocalTestName, Metrics);
	UTrafficAutomationLogger::EndTestLog();

	return bWithinMaxLat && bWithinRmsLat && bWithinHeading;
}

