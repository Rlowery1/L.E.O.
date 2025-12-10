#include "Misc/AutomationTest.h"
#include "TrafficRoadTypes.h"
#include "TrafficRoadFamilySettings.h"
#include "TrafficNetworkBuilder.h"
#include "TrafficAutomationLogger.h"
#include "Modules/ModuleManager.h"
#include "TrafficRuntimeModule.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficFamilyAssignmentSyntheticTest,
	"Traffic.Logic.FamilyAssignment.Synthetic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FTrafficFamilyAssignmentSyntheticTest::RunTest(const FString& Parameters)
{
	const FString LocalTestName = TEXT("Traffic.Logic.FamilyAssignment.Synthetic");
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

	const TArray<FRoadFamilyDefinition>& Families = FamSettings->Families;
	const int32 FamilyCount = Families.Num();

	UTrafficAutomationLogger::LogMetric(TEXT("FamilyCount"), FString::FromInt(FamilyCount));

	TArray<FTrafficRoad> Roads;
	const int32 NumPoints = 4;
	const float SegmentLength = 1000.f;

	{
		FTrafficRoad Road;
		Road.RoadId = 0;
		Road.FamilyId = 0;
		for (int32 i = 0; i < NumPoints; ++i)
		{
			const float X = i * SegmentLength;
			Road.CenterlinePoints.Add(FVector(X, 0.f, 0.f));
		}
		Roads.Add(Road);
	}

	int32 FamilyIndexRoad1 = (FamilyCount > 1) ? 1 : 0;
	{
		FTrafficRoad Road;
		Road.RoadId = 1;
		Road.FamilyId = FamilyIndexRoad1;
		for (int32 i = 0; i < NumPoints; ++i)
		{
			const float X = i * SegmentLength;
			Road.CenterlinePoints.Add(FVector(X, 5000.f, 0.f));
		}
		Roads.Add(Road);
	}

	FTrafficNetwork Network;
	FTrafficNetworkBuilder::BuildNetworkFromRoads(Roads, FamSettings, Network);

	int32 LaneCountRoad0 = 0;
	int32 LaneCountRoad1 = 0;
	for (const FTrafficLane& Lane : Network.Lanes)
	{
		if (Lane.RoadId == 0)
		{
			++LaneCountRoad0;
		}
		else if (Lane.RoadId == 1)
		{
			++LaneCountRoad1;
		}
	}

	const FRoadFamilyDefinition& Fam0 = Families[0];
	const int32 ExpectedLanesRoad0 = Fam0.Forward.NumLanes + Fam0.Backward.NumLanes;

	int32 ExpectedLanesRoad1 = ExpectedLanesRoad0;
	if (FamilyIndexRoad1 != 0)
	{
		const FRoadFamilyDefinition& Fam1 = Families[FamilyIndexRoad1];
		ExpectedLanesRoad1 = Fam1.Forward.NumLanes + Fam1.Backward.NumLanes;
	}

	UTrafficAutomationLogger::LogMetric(TEXT("LaneCountRoad0"), FString::FromInt(LaneCountRoad0));
	UTrafficAutomationLogger::LogMetric(TEXT("ExpectedLanesRoad0"), FString::FromInt(ExpectedLanesRoad0));
	UTrafficAutomationLogger::LogMetric(TEXT("LaneCountRoad1"), FString::FromInt(LaneCountRoad1));
	UTrafficAutomationLogger::LogMetric(TEXT("ExpectedLanesRoad1"), FString::FromInt(ExpectedLanesRoad1));
	UTrafficAutomationLogger::LogMetric(TEXT("FamilyIndexRoad1"), FString::FromInt(FamilyIndexRoad1));

	TestEqual(TEXT("Road 0 lane count matches family 0 definition"), LaneCountRoad0, ExpectedLanesRoad0);
	TestEqual(TEXT("Road 1 lane count matches its family definition"), LaneCountRoad1, ExpectedLanesRoad1);

	UTrafficAutomationLogger::EndTestLog();
	return true;
}

