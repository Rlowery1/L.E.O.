#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "TrafficGeometryProviderFactory.h"
#include "TrafficRuntimeModule.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficStaticMeshProviderBaselineTest,
	"Traffic.Integration.StaticMeshProvider.BaselineMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficStaticMeshProviderBaselineTest::RunTest(const FString& Parameters)
{
	const FString MapPackage = TEXT("/AAAtrafficSystem/Maps/Traffic_BaselineStraight");

	if (!AutomationOpenMap(MapPackage))
	{
		AddError(FString::Printf(TEXT("Map %s could not be loaded."), *MapPackage));
		return false;
	}

	UWorld* World = nullptr;
	if (GEditor)
	{
		World = GEditor->GetEditorWorldContext().World();
	}

	if (!World)
	{
		AddError(TEXT("Editor world was null after loading the baseline map."));
		return false;
	}

	ITrafficRoadGeometryProvider* Provider = nullptr;
	TObjectPtr<UObject> ProviderObject = UTrafficGeometryProviderFactory::CreateProvider(World, Provider);
	TStrongObjectPtr<UObject> ProviderKeeper(ProviderObject);

	if (!Provider)
	{
		AddError(TEXT("Failed to create static mesh road geometry provider."));
		return false;
	}

	TArray<FTrafficRoad> Roads;
	Provider->CollectRoads(World, Roads);

	const int32 ExpectedRoads = 1;
	TestEqual(TEXT("Baseline map road count"), Roads.Num(), ExpectedRoads);

	const int32 MinCenterlinePoints = 2;
	for (int32 RoadIdx = 0; RoadIdx < Roads.Num(); ++RoadIdx)
	{
		const int32 NumPoints = Roads[RoadIdx].CenterlinePoints.Num();
		TestTrue(
			*FString::Printf(TEXT("Road %d has at least %d centerline points (found %d)"), RoadIdx, MinCenterlinePoints, NumPoints),
			NumPoints >= MinCenterlinePoints);
	}

	return true;
}
