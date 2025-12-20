#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"
#include "TrafficAutomationLogger.h"
#include "TrafficChaosTestUtils.h"
#include "TrafficRuntimeModule.h"
#include "TrafficSystemController.h"
#include "TrafficVehicleManager.h"
#include "TrafficNetworkAsset.h"
#include "TrafficRoadTypes.h"
#include "TrafficVehicleBase.h"
#include "EngineUtils.h"
#include "Tests/AutomationEditorCommon.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
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
	bool bSavedVisualMode = false;
	int32 PrevVisualMode = 0;
	auto RestoreVisualMode = [&]()
	{
		if (bSavedVisualMode)
		{
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Visual.Mode")))
			{
				Var->Set(PrevVisualMode, ECVF_SetByCode);
			}
			bSavedVisualMode = false;
		}
	};

	if (!ValidateRuntimeModuleLoaded(this))
	{
		RestoreVisualMode();
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	UWorld* World = CreateEditorWorldForTest(this);
	if (!World)
	{
		AddError(TEXT("No editor world available."));
		RestoreVisualMode();
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Visual.Mode")))
	{
		PrevVisualMode = Var->GetInt();
		Var->Set(2, ECVF_SetByCode);
		bSavedVisualMode = true;
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
		RestoreVisualMode();
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	Manager->SetForceLogicOnlyForTests(false);
	Manager->SpawnTestVehicles(3, 800.f);

	int32 NumVehicles = 0;
	bool bChaosOk = true;
	for (TActorIterator<ATrafficVehicleBase> It(World); It; ++It)
	{
		ATrafficVehicleBase* Logic = *It;
		if (Logic)
		{
			ATrafficVehicleAdapter* Adapter = nullptr;
			APawn* Chaos = nullptr;
			FString Error;
			if (!TrafficChaosTestUtils::EnsureChaosForLogicVehicle(*World, *Logic, Adapter, Chaos, Error))
			{
				bChaosOk = false;
				AddError(Error);
			}
		}
		++NumVehicles;
	}

	UTrafficAutomationLogger::LogMetric(TEXT("ShortLaneSpawnCount"), FString::FromInt(NumVehicles));
	TestTrue(TEXT("Short lane spawn requires Chaos for logic vehicles"), bChaosOk);
	TestTrue(TEXT("Short lane should spawn at most 1 vehicle"), NumVehicles <= 1);

	RestoreVisualMode();
	UTrafficAutomationLogger::EndTestLog();
	return true;
}

bool FTrafficSpawnNormalLaneTest::RunTest(const FString& Parameters)
{
	const FString LocalTestName = TEXT("Traffic.Spawn.NormalLane");
	UTrafficAutomationLogger::BeginTestLog(LocalTestName);
	UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION)));
	bool bSavedVisualMode = false;
	int32 PrevVisualMode = 0;
	auto RestoreVisualMode = [&]()
	{
		if (bSavedVisualMode)
		{
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Visual.Mode")))
			{
				Var->Set(PrevVisualMode, ECVF_SetByCode);
			}
			bSavedVisualMode = false;
		}
	};

	if (!ValidateRuntimeModuleLoaded(this))
	{
		RestoreVisualMode();
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	UWorld* World = CreateEditorWorldForTest(this);
	if (!World)
	{
		AddError(TEXT("No editor world available."));
		RestoreVisualMode();
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Visual.Mode")))
	{
		PrevVisualMode = Var->GetInt();
		Var->Set(2, ECVF_SetByCode);
		bSavedVisualMode = true;
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
		RestoreVisualMode();
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	Manager->SetForceLogicOnlyForTests(false);
	Manager->SpawnTestVehicles(DesiredVehiclesPerLane, 800.f);

	int32 NumVehicles = 0;
	bool bChaosOk = true;
	for (TActorIterator<ATrafficVehicleBase> It(World); It; ++It)
	{
		ATrafficVehicleBase* Logic = *It;
		if (Logic)
		{
			ATrafficVehicleAdapter* Adapter = nullptr;
			APawn* Chaos = nullptr;
			FString Error;
			if (!TrafficChaosTestUtils::EnsureChaosForLogicVehicle(*World, *Logic, Adapter, Chaos, Error))
			{
				bChaosOk = false;
				AddError(Error);
			}
		}
		++NumVehicles;
	}

	UTrafficAutomationLogger::LogMetric(TEXT("NormalLaneSpawnCount"), FString::FromInt(NumVehicles));
	TestTrue(TEXT("Normal lane spawn requires Chaos for logic vehicles"), bChaosOk);
	TestEqual(TEXT("Normal lane should spawn desired vehicles"), NumVehicles, DesiredVehiclesPerLane);

	RestoreVisualMode();
	UTrafficAutomationLogger::EndTestLog();
	return true;
}
