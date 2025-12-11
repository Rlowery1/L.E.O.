#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"
#include "TrafficAutomationLogger.h"
#include "TrafficRuntimeModule.h"
#include "TrafficSystemController.h"
#include "TrafficVehicleManager.h"
#include "TrafficNetworkAsset.h"
#include "TrafficRoadTypes.h"
#include "TrafficVehicleBase.h"
#include "EngineUtils.h"
#include "Tests/AutomationEditorCommon.h"
#include "Engine/World.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficSpawnShortLaneTest,
	"Traffic.Spawn.ShortLane",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficSpawnNormalLaneTest,
	"Traffic.Spawn.NormalLane",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

namespace
{
	static bool ValidateRuntimeModuleLoaded(FAutomationTestBase* Test)
	{
		if (!FModuleManager::Get().IsModuleLoaded("TrafficRuntime"))
		{
			if (Test)
			{
				Test->AddError(TEXT("TrafficRuntime module is not loaded."));
			}
			UTrafficAutomationLogger::LogLine(TEXT("Error=TrafficRuntime module not loaded"));
			return false;
		}
		return true;
	}

	static UWorld* CreateEditorWorldForTest(FAutomationTestBase* Test)
	{
#if !WITH_EDITOR
		if (Test)
		{
			Test->AddError(TEXT("This test requires editor (WITH_EDITOR)."));
		}
		UTrafficAutomationLogger::LogLine(TEXT("Error=NotInEditor"));
		return nullptr;
#else
		FAutomationEditorCommonUtils::CreateNewMap();
		return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
#endif
	}

	static ATrafficVehicleManager* SpawnManagerWithNetwork(UWorld* World, const FTrafficNetwork& Network, FAutomationTestBase* Test)
	{
		if (!World)
		{
			return nullptr;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ATrafficSystemController* Controller = World->SpawnActor<ATrafficSystemController>(SpawnParams);
		if (!Controller)
		{
			if (Test)
			{
				Test->AddError(TEXT("Failed to spawn TrafficSystemController."));
			}
			return nullptr;
		}

		UTrafficNetworkAsset* NetworkAsset = NewObject<UTrafficNetworkAsset>(Controller);
		if (!NetworkAsset)
		{
			if (Test)
			{
				Test->AddError(TEXT("Failed to allocate TrafficNetworkAsset."));
			}
			return nullptr;
		}
		NetworkAsset->Network = Network;

		FObjectProperty* BuiltAssetProp = FindFProperty<FObjectProperty>(ATrafficSystemController::StaticClass(), TEXT("BuiltNetworkAsset"));
		if (!BuiltAssetProp)
		{
			if (Test)
			{
				Test->AddError(TEXT("Failed to find BuiltNetworkAsset property on ATrafficSystemController."));
			}
			return nullptr;
		}
		BuiltAssetProp->SetObjectPropertyValue_InContainer(Controller, NetworkAsset);

		ATrafficVehicleManager* Manager = World->SpawnActor<ATrafficVehicleManager>(SpawnParams);
		if (!Manager && Test)
		{
			Test->AddError(TEXT("Failed to spawn TrafficVehicleManager."));
		}

		return Manager;
	}
}

bool FTrafficSpawnShortLaneTest::RunTest(const FString& Parameters)
{
	const FString LocalTestName = TEXT("Traffic.Spawn.ShortLane");
	UTrafficAutomationLogger::BeginTestLog(LocalTestName);
	UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION)));

	if (!ValidateRuntimeModuleLoaded(this))
	{
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	UWorld* World = CreateEditorWorldForTest(this);
	if (!World)
	{
		AddError(TEXT("No editor world available."));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	FTrafficNetwork Network;
	FTrafficLane ShortLane;
	ShortLane.LaneId = 0;
	ShortLane.RoadId = 0;
	ShortLane.CenterlinePoints = { FVector::ZeroVector, FVector(1000.f, 0.f, 0.f) }; // 10m lane
	Network.Lanes.Add(ShortLane);

	ATrafficVehicleManager* Manager = SpawnManagerWithNetwork(World, Network, this);
	if (!Manager)
	{
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	Manager->SetForceLogicOnlyForTests(true);
	Manager->SpawnTestVehicles(3, 800.f);

	int32 NumVehicles = 0;
	for (TActorIterator<ATrafficVehicleBase> It(World); It; ++It)
	{
		++NumVehicles;
	}

	UTrafficAutomationLogger::LogMetric(TEXT("ShortLaneSpawnCount"), FString::FromInt(NumVehicles));
	TestTrue(TEXT("Short lane should spawn at most 1 vehicle"), NumVehicles <= 1);

	UTrafficAutomationLogger::EndTestLog();
	return true;
}

bool FTrafficSpawnNormalLaneTest::RunTest(const FString& Parameters)
{
	const FString LocalTestName = TEXT("Traffic.Spawn.NormalLane");
	UTrafficAutomationLogger::BeginTestLog(LocalTestName);
	UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION)));

	if (!ValidateRuntimeModuleLoaded(this))
	{
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	UWorld* World = CreateEditorWorldForTest(this);
	if (!World)
	{
		AddError(TEXT("No editor world available."));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	const int32 DesiredVehiclesPerLane = 3;

	FTrafficNetwork Network;
	FTrafficLane LongLane;
	LongLane.LaneId = 1;
	LongLane.RoadId = 0;
	LongLane.CenterlinePoints = { FVector::ZeroVector, FVector(10000.f, 0.f, 0.f) }; // 100m lane
	Network.Lanes.Add(LongLane);

	ATrafficVehicleManager* Manager = SpawnManagerWithNetwork(World, Network, this);
	if (!Manager)
	{
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	Manager->SetForceLogicOnlyForTests(true);
	Manager->SpawnTestVehicles(DesiredVehiclesPerLane, 800.f);

	int32 NumVehicles = 0;
	for (TActorIterator<ATrafficVehicleBase> It(World); It; ++It)
	{
		++NumVehicles;
	}

	UTrafficAutomationLogger::LogMetric(TEXT("NormalLaneSpawnCount"), FString::FromInt(NumVehicles));
	TestEqual(TEXT("Normal lane should spawn desired vehicles"), NumVehicles, DesiredVehiclesPerLane);

	UTrafficAutomationLogger::EndTestLog();
	return true;
}
