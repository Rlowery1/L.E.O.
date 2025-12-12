#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "TrafficGeometryProviderFactory.h"
#include "TrafficGeometryProvider.h"
#include "TrafficRoadTypes.h"
#include "TrafficRuntimeModule.h"
#include "UObject/StrongObjectPtr.h"

// Utility to load a map, run the static mesh provider and perform basic assertions.
static void StaticMeshProvider_RunOnMap(FAutomationTestBase* Test, const FString& MapPath)
{
	if (!AutomationOpenMap(MapPath))
	{
		Test->AddError(FString::Printf(TEXT("Map %s could not be loaded."), *MapPath));
		return;
	}

	UWorld* World = nullptr;
	if (GEditor)
	{
		World = GEditor->GetEditorWorldContext().World();
	}

	if (!World)
	{
		Test->AddError(TEXT("Editor world was null after loading the map."));
		return;
	}

	ITrafficRoadGeometryProvider* Provider = nullptr;
	TObjectPtr<UObject> ProviderObject = UTrafficGeometryProviderFactory::CreateProvider(World, Provider);
	TStrongObjectPtr<UObject> ProviderKeeper(ProviderObject);

	if (!Provider)
	{
		Test->AddError(TEXT("Failed to create static mesh road geometry provider."));
		return;
	}

	TArray<FTrafficRoad> Roads;
	Provider->CollectRoads(World, Roads);

	const int32 MinCenterlinePoints = 2;
	Test->TestTrue(TEXT("At least one road was extracted"), Roads.Num() >= 1);
	for (int32 RoadIdx = 0; RoadIdx < Roads.Num(); ++RoadIdx)
	{
		const int32 NumPoints = Roads[RoadIdx].CenterlinePoints.Num();
		Test->TestTrue(
			*FString::Printf(TEXT("Road %d has at least %d centerline points (found %d)"), RoadIdx, MinCenterlinePoints, NumPoints),
			NumPoints >= MinCenterlinePoints);
	}
}

// Run the static mesh provider against both baseline maps (straight and curved).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficStaticMeshProviderBaselineTest,
	"Traffic.Integration.StaticMeshProvider.BaselineMap.StraightAndCurved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficStaticMeshProviderBaselineTest::RunTest(const FString& Parameters)
{
	AddExpectedError(TEXT("The Editor is currently in a play mode."), EAutomationExpectedErrorFlags::Contains, 5);

	StaticMeshProvider_RunOnMap(this, TEXT("/AAAtrafficSystem/Maps/Traffic_BaselineStraight"));
	StaticMeshProvider_RunOnMap(this, TEXT("/AAAtrafficSystem/Maps/Traffic_BaselineCurve"));
	return true;
}
