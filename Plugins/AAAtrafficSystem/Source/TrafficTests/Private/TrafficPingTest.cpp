#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"
#include "TrafficRoadTypes.h"
#include "TrafficAutomationLogger.h"
#include "TrafficRuntimeModule.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficPingTest,
	"Traffic.Ping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FTrafficPingTest::RunTest(const FString& Parameters)
{
	const FString LocalTestName = TEXT("Traffic.Ping");
	UTrafficAutomationLogger::BeginTestLog(LocalTestName);
	UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION)));
	UTrafficAutomationLogger::LogLine(TEXT("Phase=Startup"));

	if (!FModuleManager::Get().IsModuleLoaded("TrafficRuntime"))
	{
		AddError(TEXT("TrafficRuntime module is not loaded."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=TrafficRuntime module not loaded"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	UTrafficAutomationLogger::LogLine(TEXT("Info=TrafficRuntime module loaded"));

	FTrafficNetwork Network;
	const int32 RoadCount = Network.Roads.Num();
	const int32 LaneCount = Network.Lanes.Num();
	const int32 IntersectionCount = Network.Intersections.Num();
	const int32 MovementCount = Network.Movements.Num();

	TestEqual(TEXT("Initial road count"), RoadCount, 0);
	TestEqual(TEXT("Initial lane count"), LaneCount, 0);
	TestEqual(TEXT("Initial intersection count"), IntersectionCount, 0);
	TestEqual(TEXT("Initial movement count"), MovementCount, 0);

	UTrafficAutomationLogger::LogMetric(TEXT("RoadCount"), FString::FromInt(RoadCount));
	UTrafficAutomationLogger::LogMetric(TEXT("LaneCount"), FString::FromInt(LaneCount));
	UTrafficAutomationLogger::LogMetric(TEXT("IntersectionCount"), FString::FromInt(IntersectionCount));
	UTrafficAutomationLogger::LogMetric(TEXT("MovementCount"), FString::FromInt(MovementCount));

	UTrafficAutomationLogger::EndTestLog();
	return true;
}

