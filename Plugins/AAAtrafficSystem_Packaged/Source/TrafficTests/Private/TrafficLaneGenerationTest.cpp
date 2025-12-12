#include "Misc/AutomationTest.h"
#include "TrafficRoadTypes.h"
#include "TrafficRoadFamilySettings.h"
#include "TrafficNetworkBuilder.h"
#include "TrafficAutomationLogger.h"
#include "TrafficRuntimeModule.h"
#include "Modules/ModuleManager.h"
#include "TrafficRuntimeModule.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficLaneGenerationSimpleStraightTest,
	"Traffic.Logic.LaneGeneration.SimpleStraight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FTrafficLaneGenerationSimpleStraightTest::RunTest(const FString& Parameters)
{
	const FString LocalTestName = TEXT("Traffic.Logic.LaneGeneration.SimpleStraight");
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

	const FRoadFamilyDefinition& Family = FamSettings->Families[0];

	FTrafficRoad Road;
	Road.RoadId = 0;
	Road.FamilyId = 0;

	const int32 NumPoints = 6;
	const float SegmentLength = 2000.f;
	for (int32 i = 0; i < NumPoints; ++i)
	{
		const float X = i * SegmentLength;
		Road.CenterlinePoints.Add(FVector(X, 0.f, 0.f));
	}

	TArray<FTrafficRoad> Roads;
	Roads.Add(Road);

	FTrafficNetwork Network;
	FTrafficNetworkBuilder::BuildNetworkFromRoads(Roads, FamSettings, Network);

	const int32 ExpectedLaneCount = Family.Forward.NumLanes + Family.Backward.NumLanes;
	const int32 ActualLaneCount = Network.Lanes.Num();

	UTrafficAutomationLogger::LogMetric(TEXT("ExpectedLaneCount"), FString::FromInt(ExpectedLaneCount));
	UTrafficAutomationLogger::LogMetric(TEXT("ActualLaneCount"), FString::FromInt(ActualLaneCount));

	TestEqual(TEXT("Lane count matches family definition"), ActualLaneCount, ExpectedLaneCount);

	if (ActualLaneCount > 0)
	{
		const FTrafficLane& FirstLane = Network.Lanes[0];
		const int32 LanePoints = FirstLane.CenterlinePoints.Num();

		UTrafficAutomationLogger::LogMetric(TEXT("LanePoints"), FString::FromInt(LanePoints));

		TestEqual(TEXT("Lane has same number of points as road"), LanePoints, NumPoints);

		if (LanePoints > 0)
		{
			const FVector CenterStart = Road.CenterlinePoints[0];
			const FVector LaneStart = FirstLane.CenterlinePoints[0];
			const float LateralDistance = FVector::Dist2D(CenterStart, LaneStart);

			UTrafficAutomationLogger::LogMetricFloat(TEXT("LateralOffsetStartCm"), LateralDistance, 2);

			TestTrue(TEXT("Lane lateral offset > 50cm"), LateralDistance > 50.f);
		}
	}

	UTrafficAutomationLogger::EndTestLog();
	return true;
}

