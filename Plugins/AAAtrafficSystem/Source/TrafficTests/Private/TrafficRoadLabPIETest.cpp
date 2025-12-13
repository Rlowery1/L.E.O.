#include "Misc/AutomationTest.h"
#include "TrafficSystemEditorSubsystem.h"
#include "TrafficVehicleSettings.h"
#include "TrafficVehicleAdapter.h"
#include "TrafficRuntimeModule.h"
#include "TrafficVehicleBase.h"
#include "TrafficRoadMetadataComponent.h"
#include "TrafficNetworkAsset.h"
#include "TrafficSystemController.h"
#include "TrafficAutomationLogger.h"
#include "TrafficCalibrationTestUtils.h"
#include "TrafficCityBLDAdapterSettings.h"

#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/Package.h"
#include "Components/SplineComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Tests/AutomationEditorCommon.h"
#include "TrafficLaneCalibration.h"
#include "RoadFamilyRegistry.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrafficRoadLabPIETest, "Traffic.Calibration.RoadLab.Editor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficRoadLabPIETest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	const FString LocalTestName = TEXT("Traffic.Calibration.RoadLab.Editor");
	UTrafficAutomationLogger::BeginTestLog(LocalTestName);
	UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION)));
	UTrafficAutomationLogger::LogLine(TEXT("[TrafficCalib] RoadLab editor calibration test starting."));

	if (!GEditor)
	{
		AddError(TEXT("GEditor is null."));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	FAutomationEditorCommonUtils::CreateNewMap();

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		AddError(TEXT("Editor world is null after loading map."));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	UTrafficSystemEditorSubsystem* Subsys = GEditor->GetEditorSubsystem<UTrafficSystemEditorSubsystem>();
	if (!Subsys)
	{
		AddError(TEXT("TrafficSystemEditorSubsystem is null."));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	// Clean any prior AAA overlay/controller actors (do not delete user roads).
	Subsys->Editor_ResetRoadLabHard(false);

	// Spawn a couple of CityBLD roads so the forced CityBLD provider can build a network.
	UClass* MeshRoadClass = LoadClass<AActor>(nullptr, TEXT("/CityBLD/Blueprints/Roads/BP_MeshRoad.BP_MeshRoad_C"));
	if (!MeshRoadClass)
	{
		AddError(TEXT("Failed to load CityBLD BP_MeshRoad class at /CityBLD/Blueprints/Roads/BP_MeshRoad.BP_MeshRoad_C"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	auto SpawnCityBLDRoad = [&](const FTransform& Xform) -> AActor*
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* RoadActor = World->SpawnActor<AActor>(MeshRoadClass, Xform, Params);
		if (!RoadActor)
		{
			return nullptr;
		}

		// Ensure collision queries can hit the generated mesh components in automation.
		TArray<UPrimitiveComponent*> PrimComps;
		RoadActor->GetComponents<UPrimitiveComponent>(PrimComps);
		for (UPrimitiveComponent* Prim : PrimComps)
		{
			if (!Prim)
			{
				continue;
			}
			Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Prim->SetCollisionObjectType(ECC_WorldStatic);
			Prim->SetCollisionResponseToAllChannels(ECR_Block);
		}

		return RoadActor;
	};

	AActor* RoadA = SpawnCityBLDRoad(FTransform(FRotator::ZeroRotator, FVector::ZeroVector));
	AActor* RoadB = SpawnCityBLDRoad(FTransform(FRotator(0.f, 90.f, 0.f), FVector::ZeroVector));
	if (!RoadA || !RoadB)
	{
		AddError(TEXT("Failed to spawn CityBLD BP_MeshRoad actors."));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	Subsys->DoPrepare();
	Subsys->Editor_PrepareMapForTraffic();

	URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get();
	if (!Registry || Registry->GetAllFamilies().Num() == 0)
	{
		AddError(TEXT("No road families detected for calibration."));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	FGuid FamilyId;
	for (const FRoadFamilyInfo& Info : Registry->GetAllFamilies())
	{
		const FString ClassName = Info.RoadClassPath.GetAssetName();
		if (ClassName.Contains(TEXT("BP_MeshRoad")) || ClassName.Contains(TEXT("MeshRoad")))
		{
			if (Subsys->GetNumActorsForFamily(Info.FamilyId) > 0)
			{
				FamilyId = Info.FamilyId;
				break;
			}
		}
	}
	if (!FamilyId.IsValid())
	{
		AddError(TEXT("Could not find a CityBLD road family id for calibration."));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	TrafficCalibrationTestUtils::FAlignmentEvalParams EvalParams;
	TrafficCalibrationTestUtils::FAlignmentThresholds Thresholds;
	TrafficCalibrationTestUtils::FAlignmentMetrics AlignMetrics;
	FTrafficLaneFamilyCalibration FinalCalib;

	const bool bAlignOk = TrafficCalibrationTestUtils::RunEditorCalibrationLoop(
		this,
		World,
		Subsys,
		TEXT("Calibration.RoadLab"),
		FamilyId,
		/*MaxIterations=*/5,
		EvalParams,
		Thresholds,
		AlignMetrics,
		FinalCalib);

	if (!bAlignOk)
	{
		AddError(TEXT("Calibration alignment thresholds not met (see logged metrics)."));
		UTrafficAutomationLogger::EndTestLog();
		return false;
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
	UTrafficAutomationLogger::LogMetricInt(TEXT("Network.Roads"), RoadCount);
	UTrafficAutomationLogger::LogMetricInt(TEXT("Network.Lanes"), LaneCount);
	UTrafficAutomationLogger::LogMetricInt(TEXT("Network.Intersections"), IntersectionCount);
	UTrafficAutomationLogger::LogMetricInt(TEXT("Network.Movements"), MovementCount);

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
	UTrafficAutomationLogger::LogMetricInt(TEXT("VehiclesSpawned"), VehicleCount);

	if (VehicleCount <= 0)
	{
		AddError(TEXT("No traffic vehicles were spawned during Build+Cars."));
	}

	UTrafficAutomationLogger::EndTestLog();
	return !HasAnyErrors();
#else
	return true;
#endif
}
