#include "Misc/AutomationTest.h"
#include "TrafficSystemEditorSubsystem.h"
#include "TrafficVehicleSettings.h"
#include "TrafficVehicleAdapter.h"
#include "TrafficRuntimeModule.h"
#include "TrafficVehicleBase.h"
#include "TrafficRoadMetadataComponent.h"
#include "TrafficNetworkAsset.h"
#include "TrafficSystemController.h"
#include "TrafficCityBLDAdapterSettings.h"

#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/Package.h"
#include "Components/SplineComponent.h"
#include "Tests/AutomationEditorCommon.h"
#include "TrafficLaneCalibration.h"
#include "RoadFamilyRegistry.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrafficRoadLabPIETest, "Traffic.RoadLab.PIE",
	EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FTrafficRoadLabPIETest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	UE_LOG(LogTraffic, Log, TEXT("[TrafficPIE] Starting PIE-parity test for RoadLab Build+Cars with adapter visuals."));

	if (!GEditor)
	{
		AddError(TEXT("GEditor is null."));
		return false;
	}

	FAutomationEditorCommonUtils::CreateNewMap();

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		AddError(TEXT("Editor world is null after loading map."));
		return false;
	}

	// Vehicle visuals are now profile-driven; tests do not rely on external content.

	UTrafficSystemEditorSubsystem* Subsys = GEditor->GetEditorSubsystem<UTrafficSystemEditorSubsystem>();
	if (!Subsys)
	{
		AddError(TEXT("TrafficSystemEditorSubsystem is null."));
		return false;
	}

	// Clean any prior AAA actors.
	Subsys->Editor_ResetRoadLabHard(true);

	const UTrafficCityBLDAdapterSettings* AdapterSettings = GetDefault<UTrafficCityBLDAdapterSettings>();
	const FName RoadTag = AdapterSettings ? AdapterSettings->RoadActorTag : NAME_None;
	const FName SplineTag = AdapterSettings ? AdapterSettings->RoadSplineTag : NAME_None;

	// Spawn minimal synthetic roads for the test (dev-only).
	auto SpawnTestRoad = [&](const TArray<FVector>& Points) -> AActor*
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* RoadActor = World->SpawnActor<AActor>(Params);
		if (!RoadActor)
		{
			return nullptr;
		}

		UTrafficRoadMetadataComponent* Meta = NewObject<UTrafficRoadMetadataComponent>(RoadActor);
		Meta->RegisterComponent();
		RoadActor->AddInstanceComponent(Meta);

		USplineComponent* Spline = NewObject<USplineComponent>(RoadActor);
		Spline->RegisterComponent();
		RoadActor->AddInstanceComponent(Spline);
		RoadActor->SetRootComponent(Spline);
		Spline->ClearSplinePoints(false);
		for (const FVector& P : Points)
		{
			Spline->AddSplinePoint(P, ESplineCoordinateSpace::World, false);
		}
		Spline->SetClosedLoop(false, false);
		Spline->UpdateSpline();

		if (!RoadTag.IsNone())
		{
			RoadActor->Tags.AddUnique(RoadTag);
		}
		if (!SplineTag.IsNone())
		{
			Spline->ComponentTags.AddUnique(SplineTag);
		}

		return RoadActor;
	};

	AActor* RoadA = SpawnTestRoad({ FVector(-2000.f, 500.f, 200.f), FVector(0.f, 500.f, 200.f), FVector(2000.f, 500.f, 200.f) });
	AActor* RoadB = SpawnTestRoad({ FVector(0.f, -4000.f, 200.f), FVector(0.f, 0.f, 200.f), FVector(0.f, 4000.f, 200.f) });
	if (!RoadA || !RoadB)
	{
		AddError(TEXT("Failed to spawn synthetic test roads in PIE test."));
		return false;
	}

	Subsys->DoPrepare();

	if (URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get())
	{
		for (const FRoadFamilyInfo& Info : Registry->GetAllFamilies())
		{
			FTrafficLaneFamilyCalibration Calib;
			Registry->ApplyCalibration(Info.FamilyId, Calib);
			break;
		}
	}

	Subsys->DoBuild();
	Subsys->DoCars();

	int32 RoadCount = 0, LaneCount = 0, IntersectionCount = 0, MovementCount = 0;
	for (TActorIterator<ATrafficSystemController> It(World); It; ++It)
	{
		if (UTrafficNetworkAsset* Net = It->GetBuiltNetworkAsset())
		{
			RoadCount = Net->Network.Roads.Num();
			LaneCount = Net->Network.Lanes.Num();
			IntersectionCount = Net->Network.Intersections.Num();
			MovementCount = Net->Network.Movements.Num();
			break;
		}
	}
	UE_LOG(LogTraffic, Log, TEXT("[TrafficPIE] NetworkSummary Roads=%d Lanes=%d Intersections=%d Movements=%d"),
		RoadCount, LaneCount, IntersectionCount, MovementCount);

	// Validate that vehicles spawned.
	int32 VehicleCount = 0;
	int32 LoggedLocations = 0;
	for (TActorIterator<ATrafficVehicleBase> It(World); It; ++It)
	{
		ATrafficVehicleBase* Vehicle = *It;
		if (!Vehicle)
		{
			continue;
		}
		++VehicleCount;
		if (LoggedLocations < 3)
		{
			const FVector Loc = Vehicle->GetActorLocation();
			UE_LOG(LogTraffic, Log, TEXT("[TrafficPIE] Vehicle%d_Location=%.0f,%.0f,%.0f"), LoggedLocations, Loc.X, Loc.Y, Loc.Z);
			++LoggedLocations;
		}
	}

	UE_LOG(LogTraffic, Log, TEXT("[TrafficPIE] VehiclesSpawned=%d"), VehicleCount);

	if (VehicleCount <= 0)
	{
		AddError(TEXT("No traffic vehicles were spawned during Build+Cars."));
	}

	return !HasAnyErrors();
#else
	return true;
#endif
}
