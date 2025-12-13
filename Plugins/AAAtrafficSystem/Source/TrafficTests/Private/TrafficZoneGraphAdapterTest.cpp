// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "TrafficAutomationLogger.h"
#include "TrafficRuntimeModule.h"
#include "TrafficRoadFamilySettings.h"
#include "TrafficNetworkBuilder.h"
#include "TrafficZoneGraphAdapter.h"

#include "Tests/AutomationEditorCommon.h"

#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"

#include "ZoneShapeActor.h"
#include "ZoneShapeComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficZoneGraphAdapterBuildShapesTest,
	"Traffic.ZoneGraph.Adapter.BuildShapes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FTrafficZoneGraphAdapterBuildShapesTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	const FString LocalTestName = TEXT("Traffic.ZoneGraph.Adapter.BuildShapes");
	UTrafficAutomationLogger::BeginTestLog(LocalTestName);
	UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION)));

	FAutomationEditorCommonUtils::CreateNewMap();

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		AddError(TEXT("No editor world available."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=NoEditorWorld"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	const UTrafficRoadFamilySettings* FamilySettings = GetDefault<UTrafficRoadFamilySettings>();
	if (!FamilySettings)
	{
		AddError(TEXT("UTrafficRoadFamilySettings missing."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=NoTrafficRoadFamilySettings"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	// Build a simple synthetic network (3 roads meeting near the origin) so intersections exist.
	TArray<FTrafficRoad> Roads;

	{
		FTrafficRoad R;
		R.RoadId = 0;
		R.FamilyId = 0;
		R.CenterlinePoints = { FVector(-5000.f, 0.f, 0.f), FVector(0.f, 0.f, 0.f), FVector(5000.f, 0.f, 0.f) };
		Roads.Add(R);
	}
	{
		FTrafficRoad R;
		R.RoadId = 1;
		R.FamilyId = 0;
		R.CenterlinePoints = { FVector(0.f, -5000.f, 0.f), FVector(0.f, 0.f, 0.f), FVector(0.f, 5000.f, 0.f) };
		Roads.Add(R);
	}
	{
		FTrafficRoad R;
		R.RoadId = 2;
		R.FamilyId = 0;
		R.CenterlinePoints = { FVector(-3000.f, -3000.f, 0.f), FVector(0.f, 0.f, 0.f), FVector(3000.f, 3000.f, 0.f) };
		Roads.Add(R);
	}

	FTrafficNetwork Network;
	FTrafficNetworkBuilder::BuildNetworkFromRoads(Roads, FamilySettings, Network);

	UTrafficAutomationLogger::LogMetricInt(TEXT("Roads"), Network.Roads.Num());
	UTrafficAutomationLogger::LogMetricInt(TEXT("Intersections"), Network.Intersections.Num());
	UTrafficAutomationLogger::LogMetricInt(TEXT("Lanes"), Network.Lanes.Num());

	FTrafficZoneGraphAdapter::BuildZoneGraphForNetwork(World, Network, FamilySettings);

	const FName TrafficZoneGraphTag(TEXT("TrafficZoneGraph"));

	int32 ExpectedPolygons = 0;
	for (const FTrafficIntersection& Intersection : Network.Intersections)
	{
		const int32 Connected = Intersection.IncomingLaneIds.Num() + Intersection.OutgoingLaneIds.Num();
		if (Connected >= 3)
		{
			++ExpectedPolygons;
		}
	}

	int32 SplineShapes = 0;
	int32 PolygonShapes = 0;
	for (TActorIterator<AZoneShape> It(World); It; ++It)
	{
		AZoneShape* Shape = *It;
		if (!Shape || !Shape->ActorHasTag(TrafficZoneGraphTag))
		{
			continue;
		}

		const UZoneShapeComponent* ShapeComp = Shape->FindComponentByClass<UZoneShapeComponent>();
		if (!ShapeComp)
		{
			continue;
		}

		if (ShapeComp->GetShapeType() == FZoneShapeType::Spline)
		{
			++SplineShapes;
		}
		else
		{
			++PolygonShapes;
		}
	}

	UTrafficAutomationLogger::LogMetricInt(TEXT("SpawnedSplineShapes"), SplineShapes);
	UTrafficAutomationLogger::LogMetricInt(TEXT("SpawnedPolygonShapes"), PolygonShapes);
	UTrafficAutomationLogger::LogMetricInt(TEXT("ExpectedPolygons"), ExpectedPolygons);

	TestEqual(TEXT("Spawned spline zone shapes == roads"), SplineShapes, Roads.Num());
	TestEqual(TEXT("Spawned polygon zone shapes == expected"), PolygonShapes, ExpectedPolygons);

	UTrafficAutomationLogger::EndTestLog();
	return !HasAnyErrors();
#else
	return true;
#endif
}
