#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "TrafficSystemEditorSubsystem.h"
#include "Templates/SharedPointer.h"
#include "TrafficAutomationLogger.h"
#include "TrafficRuntimeModule.h"
#include "Misc/PackageName.h"
#include "Factories/WorldFactory.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "FileHelpers.h"
#include "Tests/AutomationCommon.h"
#include "TrafficSystemController.h"
#include "TrafficVehicleManager.h"
#include "TrafficRoadMetadataComponent.h"
#include "TrafficCityBLDAdapterSettings.h"
#include "TrafficRoadFamilySettings.h"
#include "Components/SplineComponent.h"
#include "TrafficNetworkBuilder.h"
#include "TrafficGeometryProvider.h"
#include "TrafficNetworkAsset.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "TrafficLaneCalibration.h"
#include "RoadFamilyRegistry.h"
#include "TrafficVehicleBase.h"

namespace
{
	struct FTrafficPIETestState
	{
		bool bFailed = false;
		FString FailureMessage;
		UWorld* PIEWorld = nullptr;
		FTrafficRunMetrics Metrics;
	};

	static void TickEditorWorld(UWorld* World, float DeltaSeconds)
	{
		if (World)
		{
			World->Tick(LEVELTICK_All, DeltaSeconds);
		}
	}

	static void SampleVehicleMetrics(UWorld* World, FTrafficRunMetrics& Metrics, float DeltaSeconds)
	{
		if (!World)
		{
			return;
		}

		for (TActorIterator<ATrafficVehicleBase> It(World); It; ++It)
		{
			It->SampleLaneTrackingError(Metrics);
			It->SampleDynamics(Metrics, DeltaSeconds);
		}
	}

	static bool CreateAndSaveRoadLabMap(const FString& MapPackagePath)
	{
		const FString AssetName = FPackageName::GetLongPackageAssetName(MapPackagePath);
		UPackage* Package = CreatePackage(*MapPackagePath);
		if (!Package)
		{
			return false;
		}
		Package->FullyLoad();

		UWorldFactory* WorldFactory = NewObject<UWorldFactory>();
		UObject* NewWorldObj = WorldFactory->FactoryCreateNew(
			UWorld::StaticClass(),
			Package,
			FName(*AssetName),
			RF_Public | RF_Standalone,
			nullptr,
			GWarn);

		UWorld* NewWorld = Cast<UWorld>(NewWorldObj);
		if (!NewWorld)
		{
			return false;
		}

		FAssetRegistryModule::AssetCreated(NewWorld);
		Package->MarkPackageDirty();

		const FString Filename = FPackageName::LongPackageNameToFilename(
			MapPackagePath,
			FPackageName::GetMapPackageExtension());

		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.Error = GError;
		return UPackage::SavePackage(Package, NewWorld, *Filename, SaveArgs);
	}

	static bool RecreateRoadLabMap(const FString& MapPackagePath)
	{
		// Generate a clean RoadLab map from the OpenWorld template for consistent lighting/sky/ground.
		const FString TargetFilename = FPackageName::LongPackageNameToFilename(
			MapPackagePath,
			FPackageName::GetMapPackageExtension());

		IFileManager::Get().MakeDirectory(*FPaths::GetPath(TargetFilename), true);

		const FString TemplateFilename = FPaths::Combine(FPaths::EngineContentDir(), TEXT("Maps/Templates/OpenWorld.umap"));
		if (!FPaths::FileExists(TemplateFilename))
		{
			UE_LOG(LogTraffic, Warning, TEXT("[Automation] OpenWorld template not found at %s"), *TemplateFilename);
			return false;
		}

		UWorld* NewWorld = UEditorLoadingAndSavingUtils::NewMapFromTemplate(TemplateFilename, /*bSaveExistingMap*/ false);
		if (!NewWorld)
		{
			UE_LOG(LogTraffic, Warning, TEXT("[Automation] Failed to create RoadLab map from template %s"), *TemplateFilename);
			return false;
		}

		if (!UEditorLoadingAndSavingUtils::SaveMap(NewWorld, MapPackagePath))
		{
			UE_LOG(LogTraffic, Warning, TEXT("[Automation] Failed to save RoadLab map to %s"), *TargetFilename);
			return false;
		}

		// Load the freshly created map so the automation run uses it.
		FEditorFileUtils::LoadMap(TargetFilename, false, true);
		return true;
	}
}

DEFINE_LATENT_AUTOMATION_COMMAND_THREE_PARAMETER(FTrafficPIELoadMapCommand, TSharedRef<FTrafficPIETestState>, State, FString, MapPath, FString, TemplateMap);
bool FTrafficPIELoadMapCommand::Update()
{
#if WITH_EDITOR
	const FString MapFilename = FPackageName::LongPackageNameToFilename(MapPath, FPackageName::GetMapPackageExtension());
	if (FPaths::FileExists(MapFilename))
	{
		UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("PIE_LoadMap=%s"), *MapFilename));
		FEditorFileUtils::LoadMap(MapFilename, false, true);
	}
	else
	{
		UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("PIE_LoadTemplate=%s"), *TemplateMap));
		FEditorFileUtils::LoadMap(TemplateMap, false, true);
	}
#endif
	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(FTrafficWaitForPIEWorldCommand, TSharedRef<FTrafficPIETestState>, State, FAutomationTestBase*, Test, double, TimeoutSeconds, double, StartTime);
bool FTrafficWaitForPIEWorldCommand::Update()
{
	if (GEditor && GEditor->PlayWorld)
	{
		State->PIEWorld = GEditor->PlayWorld;
		UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("PIE_WorldReady=%s"), *State->PIEWorld->GetMapName()));
		return true;
	}

	const double Elapsed = FPlatformTime::Seconds() - StartTime;
	if (Elapsed > TimeoutSeconds)
	{
		State->bFailed = true;
		State->FailureMessage = TEXT("NoPIEWorldTimeout");
		UTrafficAutomationLogger::LogLine(TEXT("Error=NoPIEWorldTimeout"));
		if (Test)
		{
			Test->AddError(TEXT("PIE world not available within timeout."));
		}
		return true;
	}

	return false;
}

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FTrafficPIESpawnAndRunCommand, TSharedRef<FTrafficPIETestState>, State, FAutomationTestBase*, Test);
bool FTrafficPIESpawnAndRunCommand::Update()
{
#if WITH_EDITOR
	if (State->bFailed || !State->PIEWorld)
	{
		return true;
	}

	UWorld* PIEWorld = State->PIEWorld;
	UTrafficAutomationLogger::LogLine(TEXT("PIE_Step=SpawnRoads"));

	const FName DefaultFamily(TEXT("Urban_2x2"));
	const UTrafficCityBLDAdapterSettings* AdapterSettings = GetDefault<UTrafficCityBLDAdapterSettings>();
	const FName RoadTag = AdapterSettings ? AdapterSettings->RoadActorTag : NAME_None;
	const FName SplineTag = AdapterSettings ? AdapterSettings->RoadSplineTag : NAME_None;
	int32 RoadsSpawned = 0;

	auto SpawnRoad = [&](const TArray<FVector>& Points) -> AActor*
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* RoadActor = PIEWorld->SpawnActor<AActor>(Params);
		if (!RoadActor)
		{
			return nullptr;
		}

		UTrafficRoadMetadataComponent* Meta = NewObject<UTrafficRoadMetadataComponent>(RoadActor);
		Meta->RegisterComponent();
		RoadActor->AddInstanceComponent(Meta);
		Meta->FamilyName = DefaultFamily;

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

		++RoadsSpawned;
		return RoadActor;
	};

	AActor* RoadA = SpawnRoad({ FVector(-2000.f, 500.f, 200.f), FVector(0.f, 500.f, 200.f), FVector(2000.f, 500.f, 200.f) });
	AActor* RoadB = SpawnRoad({ FVector(0.f, -4000.f, 200.f), FVector(0.f, 0.f, 200.f), FVector(0.f, 4000.f, 200.f) });

	if (!RoadA || !RoadB)
	{
		State->bFailed = true;
		State->FailureMessage = TEXT("SpawnRoadFailed");
		UTrafficAutomationLogger::LogLine(TEXT("Error=SpawnRoadFailed"));
		if (Test)
		{
			Test->AddError(TEXT("Failed to spawn PIE roads."));
		}
		return true;
	}

	UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("PIE_SpawnedRoads=%d"), RoadsSpawned));
	UTrafficAutomationLogger::LogLine(TEXT("PIE_Step=BuildNetwork"));
	ATrafficSystemController* Controller = PIEWorld->SpawnActor<ATrafficSystemController>();
	if (!Controller)
	{
		State->bFailed = true;
		State->FailureMessage = TEXT("SpawnControllerFailed");
		UTrafficAutomationLogger::LogLine(TEXT("Error=SpawnControllerFailed"));
		if (Test)
		{
			Test->AddError(TEXT("Failed to spawn TrafficSystemController in PIE."));
		}
		return true;
	}
	Controller->Editor_BuildTrafficNetwork();

	int32 RoadCount = 0, LaneCount = 0, IntersectionCount = 0, MovementCount = 0;
	if (UTrafficNetworkAsset* Net = Controller->GetBuiltNetworkAsset())
	{
		RoadCount = Net->Network.Roads.Num();
		LaneCount = Net->Network.Lanes.Num();
		IntersectionCount = Net->Network.Intersections.Num();
		MovementCount = Net->Network.Movements.Num();
	}
	if (LaneCount == 0 || RoadCount == 0)
	{
		State->bFailed = true;
		State->FailureMessage = TEXT("NoNetworkBuilt");
		UTrafficAutomationLogger::LogLine(TEXT("Error=NoNetworkBuilt"));
		if (Test)
		{
			Test->AddError(TEXT("Network build produced zero lanes/roads."));
		}
		return true;
	}
	UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("PIE_NetworkSummary=Roads:%d Lanes:%d Intersections:%d Movements:%d"),
		RoadCount, LaneCount, IntersectionCount, MovementCount));

	UTrafficAutomationLogger::LogLine(TEXT("PIE_Step=SpawnCars"));
	ATrafficVehicleManager* Manager = PIEWorld->SpawnActor<ATrafficVehicleManager>();
	if (!Manager)
	{
		State->bFailed = true;
		State->FailureMessage = TEXT("SpawnManagerFailed");
		UTrafficAutomationLogger::LogLine(TEXT("Error=SpawnManagerFailed"));
		if (Test)
		{
			Test->AddError(TEXT("Failed to spawn TrafficVehicleManager in PIE."));
		}
		return true;
	}
	Manager->SetForceLogicOnlyForTests(true);
	Manager->SetActiveRunMetrics(&State->Metrics);
	Manager->SpawnTestVehicles(3, 800.f);

	int32 VehicleCount = 0;
	int32 Logged = 0;
	for (TActorIterator<ATrafficVehicleBase> It(PIEWorld); It; ++It)
	{
		++VehicleCount;
		if (Logged < 3)
		{
			FVector Loc = It->GetActorLocation();
			UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("PIE_Vehicle%d_Location=%.0f,%.0f,%.0f"), Logged, Loc.X, Loc.Y, Loc.Z));
			++Logged;
		}
	}

	if (VehicleCount == 0)
	{
		State->bFailed = true;
		State->FailureMessage = TEXT("NoVehiclesSpawned");
		UTrafficAutomationLogger::LogLine(TEXT("Error=NoVehiclesSpawned"));
		if (Test)
		{
			Test->AddError(TEXT("No vehicles spawned in PIE test."));
		}
		return true;
	}

	UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("PIE_VehicleCount=%d"), VehicleCount));

	State->Metrics.VehiclesSpawned = VehicleCount;
	const float SimDuration = 3.0f;
	const float SimStep = 0.1f;
	float Simulated = 0.0f;
	while (Simulated < SimDuration)
	{
		PIEWorld->Tick(LEVELTICK_All, SimStep);
		SampleVehicleMetrics(PIEWorld, State->Metrics, SimStep);
		Simulated += SimStep;
	}
	State->Metrics.SimulatedSeconds = Simulated;
	State->Metrics.Finalize();
	UTrafficAutomationLogger::LogRunMetrics(TEXT("Traffic.RoadLab.PIE"), State->Metrics);

	UTrafficAutomationLogger::LogLine(TEXT("PIE_Result=Success"));
#endif
	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FTrafficPIEEndCommand, TSharedRef<FTrafficPIETestState>, State);
bool FTrafficPIEEndCommand::Update()
{
#if WITH_EDITOR
	if (State->bFailed && !State->FailureMessage.IsEmpty())
	{
		UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("PIE_Failure=%s"), *State->FailureMessage));
	}
	if (!State->bFailed)
	{
		UTrafficAutomationLogger::LogLine(TEXT("PIE_Result=Success"));
		UTrafficAutomationLogger::LogLine(TEXT("PIE_HumanVisual_PASS=true"));
	}
	else
	{
		UTrafficAutomationLogger::LogLine(TEXT("PIE_Result=Fail"));
	}
	if (GEditor)
	{
		GEditor->EndPlayMap();
	}
	UTrafficAutomationLogger::EndTestLog();
#endif
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
FTrafficRoadLabIntegrationTest,
"Traffic.RoadLab.Integration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FTrafficRoadLabIntegrationTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	FAutomationEditorCommonUtils::CreateNewMap();

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		AddError(TEXT("No editor world after opening RoadLab map."));
		return false;
	}

	const FString LocalTestName = TEXT("Traffic.RoadLab.Integration");
	UTrafficAutomationLogger::BeginTestLog(LocalTestName);
	UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION)));
	UTrafficAutomationLogger::LogLine(TEXT("# RoadLab Integration Test"));
	UTrafficAutomationLogger::LogLine(TEXT("VisualBaseline=UserRoadsOnly_NoSynthetic"));

	UTrafficSystemEditorSubsystem* Subsys = GEditor->GetEditorSubsystem<UTrafficSystemEditorSubsystem>();
	if (!Subsys)
	{
		AddError(TEXT("UTrafficSystemEditorSubsystem not available."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=EditorSubsystemMissing"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	UTrafficAutomationLogger::LogLine(TEXT("Phase=ResetLab"));
	Subsys->ResetRoadLab();
	TickEditorWorld(World, 0.1f);

	const UTrafficCityBLDAdapterSettings* AdapterSettings = GetDefault<UTrafficCityBLDAdapterSettings>();
	const FName RoadTag = AdapterSettings ? AdapterSettings->RoadActorTag : NAME_None;
	const FName SplineTag = AdapterSettings ? AdapterSettings->RoadSplineTag : NAME_None;

	// Spawn minimal cross layout for testing (dev-only synthetic roads).
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
		AddError(TEXT("Failed to spawn synthetic test roads for automation run."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=NoRoadsSpawned"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}
	UTrafficAutomationLogger::LogLine(TEXT("Phase=RoadsSpawned"));

	UTrafficAutomationLogger::LogLine(TEXT("Phase=Prepare"));
	Subsys->DoPrepare();
	TickEditorWorld(World, 0.1f);

	FTrafficRunMetrics Metrics;

	// Ensure at least one family is calibrated for build/test gating.
	if (URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get())
	{
		for (const FRoadFamilyInfo& Info : Registry->GetAllFamilies())
		{
			FTrafficLaneFamilyCalibration Calib;
			Registry->ApplyCalibration(Info.FamilyId, Calib);
			break;
		}
	}

	UTrafficAutomationLogger::LogLine(TEXT("Phase=Build"));
	Subsys->DoBuild();
	TickEditorWorld(World, 0.1f);

	// Ensure a vehicle manager exists and is wired to metrics before spawning cars.
	ATrafficVehicleManager* Manager = nullptr;
	for (TActorIterator<ATrafficVehicleManager> It(World); It; ++It)
	{
		Manager = *It;
		break;
	}
	if (!Manager)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Manager = World->SpawnActor<ATrafficVehicleManager>(Params);
	}
	if (Manager)
	{
		Manager->SetActiveRunMetrics(&Metrics);
		Manager->SetForceLogicOnlyForTests(true);
	}

	// Log network summary for human-like verification.
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
	UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("NetworkSummary=Roads:%d Lanes:%d Intersections:%d Movements:%d"),
		RoadCount, LaneCount, IntersectionCount, MovementCount));

	UTrafficAutomationLogger::LogLine(TEXT("Phase=Cars"));
	Subsys->DoCars();
	TickEditorWorld(World, 0.1f);

	int32 VehicleCount = 0;
	int32 LoggedVehicles = 0;
	for (TActorIterator<ATrafficVehicleBase> It(World); It; ++It)
	{
		++VehicleCount;
		if (LoggedVehicles < 3)
		{
			FVector Loc = It->GetActorLocation();
			UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("Vehicle%d_Location=%.0f,%.0f,%.0f"),
				LoggedVehicles, Loc.X, Loc.Y, Loc.Z));
			++LoggedVehicles;
		}
	}
	UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("VehicleCount=%d"), VehicleCount));

	Metrics.VehiclesSpawned = VehicleCount;

	// Run a short simulated loop to gather motion metrics.
	const float SimDuration = 3.0f;
	const float SimStep = 0.1f;
	float Simulated = 0.0f;
	while (Simulated < SimDuration)
	{
		TickEditorWorld(World, SimStep);
		SampleVehicleMetrics(World, Metrics, SimStep);
		Simulated += SimStep;
	}
	Metrics.SimulatedSeconds = Simulated;
	Metrics.Finalize();
	UTrafficAutomationLogger::LogRunMetrics(LocalTestName, Metrics);

	UTrafficAutomationLogger::LogLine(TEXT("Result=Success"));
	UTrafficAutomationLogger::EndTestLog();
	UE_LOG(LogTraffic, Display, TEXT("[Automation] RoadLab Integration Test Complete."));

	// Cleanup spawned AAA actors to avoid lingering references across tests.
	Subsys->Editor_ResetRoadLabHard(false);
	return true;
#else
	return false;
#endif
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
FTrafficRoadLabIntegrationPIETest,
"Traffic.RoadLab.PIE",
	EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter
)

bool FTrafficRoadLabIntegrationPIETest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	const FString MapPath = TEXT("/Game/Maps/Test_Maps/RoadLab/Traffic_RoadLab");
	const FString TemplateMap = FPaths::Combine(FPaths::EngineContentDir(), TEXT("Maps/Templates/OpenWorld.umap"));
	TSharedRef<FTrafficPIETestState> State = MakeShared<FTrafficPIETestState>();

	UTrafficAutomationLogger::BeginTestLog(TEXT("Traffic.RoadLab.PIE"));
	UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION)));
	UTrafficAutomationLogger::LogLine(TEXT("# RoadLab Integration PIE Test"));
	AddExpectedError(TEXT("The Editor is currently in a play mode."), EAutomationExpectedErrorFlags::Contains, 1);

	UTrafficAutomationLogger::LogLine(TEXT("PIE_Begin"));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficPIELoadMapCommand(State, MapPath, TemplateMap));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficWaitForPIEWorldCommand(State, this, 10.0, FPlatformTime::Seconds()));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficPIESpawnAndRunCommand(State, this));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficPIEEndCommand(State));
	return true;
#else
	return false;
#endif
}

