#include "Misc/AutomationTest.h"
#include "TrafficRoadTypes.h"
#include "TrafficRoadFamilySettings.h"
#include "TrafficNetworkBuilder.h"
#include "TrafficMovementGeometry.h"
#include "TrafficAutomationLogger.h"
#include "Modules/ModuleManager.h"
#include "TrafficRuntimeModule.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficTurnKinematicsSimpleCrossTest,
	"Traffic.Motion.TurnKinematics.SimpleCross",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FTrafficTurnKinematicsSimpleCrossTest::RunTest(const FString& Parameters)
{
	const FString LocalTestName = TEXT("Traffic.Motion.TurnKinematics.SimpleCross");
	UTrafficAutomationLogger::BeginTestLog(LocalTestName);
	UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION)));

	if (!FModuleManager::Get().IsModuleLoaded("TrafficRuntime"))
	{
		AddError(TEXT("TrafficRuntime module is not loaded."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=TrafficRuntime module not loaded"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	const UTrafficRoadFamilySettings* FamSettings = GetDefault<UTrafficRoadFamilySettings>();
	if (!FamSettings || FamSettings->Families.Num() == 0)
	{
		AddError(TEXT("No road families configured."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=No road families configured"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	TArray<FTrafficRoad> Roads;
	const int32 NumPoints = 5;
	const float SegmentLength = 1000.f;

	{
		FTrafficRoad Road;
		Road.RoadId = 0;
		Road.FamilyId = 0;
		for (int32 i = 0; i < NumPoints; ++i)
		{
			const float X = (i - 2) * SegmentLength;
			Road.CenterlinePoints.Add(FVector(X, 0.f, 0.f));
		}
		Roads.Add(Road);
	}

	{
		FTrafficRoad Road;
		Road.RoadId = 1;
		Road.FamilyId = 0;
		for (int32 i = 0; i < NumPoints; ++i)
		{
			const float Y = (i - 2) * SegmentLength;
			Road.CenterlinePoints.Add(FVector(0.f, Y, 0.f));
		}
		Roads.Add(Road);
	}

	FTrafficNetwork Network;
	FTrafficNetworkBuilder::BuildNetworkFromRoads(Roads, FamSettings, Network);

	const int32 IntersectionCount = Network.Intersections.Num();
	const int32 MovementCount = Network.Movements.Num();
	const int32 LaneCount = Network.Lanes.Num();

	FTrafficRunMetrics Metrics;

	UTrafficAutomationLogger::LogMetric(TEXT("IntersectionCount"), FString::FromInt(IntersectionCount));
	UTrafficAutomationLogger::LogMetric(TEXT("MovementCount"), FString::FromInt(MovementCount));
	UTrafficAutomationLogger::LogMetric(TEXT("LaneCount"), FString::FromInt(LaneCount));

	TestTrue(TEXT("At least one intersection found"), IntersectionCount >= 1);
	TestTrue(TEXT("At least one movement generated"), MovementCount >= 1);

	const int32 MaxMovementsToCheck = FMath::Min(4, MovementCount);

	float GlobalMaxCurvature = 0.0f;
	float GlobalMaxHeadingStepDeg = 0.0f;

	for (int32 m = 0; m < MaxMovementsToCheck; ++m)
	{
		const FTrafficMovement& Movement = Network.Movements[m];

		TArray<FMovementSample> Samples;
		TrafficMovementGeometry::AnalyzeMovementPath(Movement, Samples);

		const int32 SampleCount = Samples.Num();
		if (SampleCount < 2)
		{
			continue;
		}

		float MaxCurvature = 0.0f;
		float MaxHeadingStepDeg = 0.0f;

		for (int32 i = 0; i < SampleCount; ++i)
		{
			MaxCurvature = FMath::Max(MaxCurvature, FMath::Abs(Samples[i].Curvature));

			if (i > 0)
			{
				const FVector PrevT = Samples[i - 1].Tangent;
				const FVector CurrT = Samples[i].Tangent;
				const float Dot = FMath::Clamp(FVector::DotProduct(PrevT, CurrT), -1.0f, 1.0f);
				const float AngleRad = FMath::Acos(Dot);
				const float AngleDeg = FMath::RadiansToDegrees(AngleRad);
				MaxHeadingStepDeg = FMath::Max(MaxHeadingStepDeg, AngleDeg);

				Metrics.AccumulateHeadingError(FMath::Abs(AngleDeg));
			}
		}

		GlobalMaxCurvature = FMath::Max(GlobalMaxCurvature, MaxCurvature);
		GlobalMaxHeadingStepDeg = FMath::Max(GlobalMaxHeadingStepDeg, MaxHeadingStepDeg);

		const FString Prefix = FString::Printf(TEXT("Movement[%d]"), Movement.MovementId);
		UTrafficAutomationLogger::LogMetricFloat(Prefix + TEXT(".MaxCurvature"), MaxCurvature, 6);
		UTrafficAutomationLogger::LogMetricFloat(Prefix + TEXT(".MaxHeadingStepDeg"), MaxHeadingStepDeg, 3);
	}

	UTrafficAutomationLogger::LogMetricFloat(TEXT("GlobalMaxCurvature"), GlobalMaxCurvature, 6);
	UTrafficAutomationLogger::LogMetricFloat(TEXT("GlobalMaxHeadingStepDeg"), GlobalMaxHeadingStepDeg, 3);

	const float MaxHeadingStepToleranceDeg = 10.0f;
	const bool bHeadingSmooth = (GlobalMaxHeadingStepDeg <= MaxHeadingStepToleranceDeg);

	if (!bHeadingSmooth)
	{
		AddError(FString::Printf(TEXT("GlobalMaxHeadingStepDeg too large: %f"), GlobalMaxHeadingStepDeg));
	}

	UTrafficAutomationLogger::LogMetric(TEXT("HeadingSmooth"), bHeadingSmooth ? TEXT("true") : TEXT("false"));

	Metrics.Finalize();
	UTrafficAutomationLogger::LogRunMetrics(LocalTestName, Metrics);
	UTrafficAutomationLogger::EndTestLog();

	return bHeadingSmooth;
}

