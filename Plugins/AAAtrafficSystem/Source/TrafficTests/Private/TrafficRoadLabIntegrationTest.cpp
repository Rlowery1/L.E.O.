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
#include "TrafficChaosTestUtils.h"
#include "TrafficLaneCalibration.h"
#include "RoadFamilyRegistry.h"
#include "TrafficVehicleBase.h"
#include "TrafficCalibrationTestUtils.h"
#include "Components/PrimitiveComponent.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"

namespace
{
	struct FTrafficPIETestState
	{
		bool bFailed = false;
		FString FailureMessage;
		UWorld* PIEWorld = nullptr;
		FTrafficRunMetrics Metrics;
		bool bSavedVisualMode = false;
		int32 PrevVisualMode = INDEX_NONE;
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

DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(FTrafficRoadLabWaitForPIEWorldCommand, TSharedRef<FTrafficPIETestState>, State, FAutomationTestBase*, Test, double, TimeoutSeconds, double, StartTime);
bool FTrafficRoadLabWaitForPIEWorldCommand::Update()
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
	UTrafficAutomationLogger::LogLine(TEXT("PIE_Step=BuildNetwork"));

	int32 MeshRoadCount = 0;
	for (TActorIterator<AActor> It(PIEWorld); It; ++It)
	{
		if (AActor* Actor = *It)
		{
			if (Actor->GetClass()->GetName().Contains(TEXT("BP_MeshRoad")))
			{
				++MeshRoadCount;
			}
		}
	}
	UTrafficAutomationLogger::LogMetricInt(TEXT("PIE.MeshRoadActors"), MeshRoadCount);
	if (MeshRoadCount == 0)
	{
		State->bFailed = true;
		State->FailureMessage = TEXT("NoMeshRoadActors");
		UTrafficAutomationLogger::LogLine(TEXT("Error=NoMeshRoadActors"));
		if (Test)
		{
			Test->AddError(TEXT("No CityBLD BP_MeshRoad actors present in PIE world."));
		}
		return true;
	}

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

		TrafficCalibrationTestUtils::FAlignmentEvalParams EvalParams;
		TrafficCalibrationTestUtils::FAlignmentThresholds Thresholds;
		TrafficCalibrationTestUtils::FAlignmentMetrics AlignMetrics;
		if (TrafficCalibrationTestUtils::EvaluateNetworkLaneAlignment(PIEWorld, Net->Network, EvalParams, AlignMetrics))
		{
			UTrafficAutomationLogger::LogMetricFloat(TEXT("PIE.Align.MeanDevCm"), AlignMetrics.MeanLateralDeviationCm, 2);
			UTrafficAutomationLogger::LogMetricFloat(TEXT("PIE.Align.MaxDevCm"), AlignMetrics.MaxLateralDeviationCm, 2);
			UTrafficAutomationLogger::LogMetricFloat(TEXT("PIE.Align.MaxHeadingDeg"), AlignMetrics.MaxHeadingErrorDeg, 2);
			UTrafficAutomationLogger::LogMetricFloat(TEXT("PIE.Align.CoveragePercent"), AlignMetrics.CoveragePercent, 2);

			FString FailureReason;
			if (!TrafficCalibrationTestUtils::AlignmentMeetsThresholds(AlignMetrics, Thresholds, &FailureReason))
			{
				State->bFailed = true;
				State->FailureMessage = TEXT("AlignmentThresholdsFailedInPIE");
				UTrafficAutomationLogger::LogLine(TEXT("Error=AlignmentThresholdsFailedInPIE"));
				UTrafficAutomationLogger::LogMetric(TEXT("PIE.Align.FailReason"), FailureReason);
				if (Test)
				{
					Test->AddError(TEXT("PIE alignment thresholds not met (see logged metrics)."));
				}
				return true;
			}
		}
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

	TSharedRef<FTrafficPIETestState> LocalState = State;
	auto RestoreVisualMode = [LocalState]()
	{
		if (!LocalState->bSavedVisualMode)
		{
			return;
		}
		if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Visual.Mode")))
		{
			Var->Set(LocalState->PrevVisualMode, ECVF_SetByCode);
		}
		LocalState->bSavedVisualMode = false;
	};

	if (!State->bSavedVisualMode)
	{
		if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Visual.Mode")))
		{
			State->PrevVisualMode = Var->GetInt();
			State->bSavedVisualMode = true;
			Var->Set(2, ECVF_SetByCode);
		}
	}

	Manager->SetForceLogicOnlyForTests(false);
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
		RestoreVisualMode();
		return true;
	}

        UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("PIE_VehicleCount=%d"), VehicleCount));

        State->Metrics.VehiclesSpawned = VehicleCount;

        bool bChaosMissing = false;
        for (TActorIterator<ATrafficVehicleBase> It(PIEWorld); It; ++It)
        {
                ATrafficVehicleAdapter* Adapter = nullptr;
                APawn* Chaos = nullptr;
                FString ChaosError;
                if (!TrafficChaosTestUtils::EnsureChaosForLogicVehicle(*PIEWorld, **It, Adapter, Chaos, ChaosError))
                {
                        bChaosMissing = true;
                        if (Test)
                        {
                                Test->AddError(FString::Printf(TEXT("PIE Chaos missing: %s"), *ChaosError));
                        }
                        break;
                }
        }
        if (bChaosMissing)
        {
                State->bFailed = true;
                State->FailureMessage = TEXT("ChaosMissing");
                UTrafficAutomationLogger::LogLine(TEXT("Error=ChaosMissing"));
                RestoreVisualMode();
                return true;
        }

        const float SimDuration = 3.0f;
        const float SimStep = 0.1f;
        float Simulated = 0.0f;
        const int32 MaxSteps = 200;
        int32 StepsTaken = 0;
        const double SimDeadline = FPlatformTime::Seconds() + 15.0;
        while (Simulated < SimDuration && StepsTaken < MaxSteps)
        {
                PIEWorld->Tick(LEVELTICK_All, SimStep);
                SampleVehicleMetrics(PIEWorld, State->Metrics, SimStep);
                Simulated += SimStep;
                ++StepsTaken;

                if (FPlatformTime::Seconds() > SimDeadline)
                {
                        State->bFailed = true;
                        State->FailureMessage = TEXT("SimLoopTimeout");
                        UTrafficAutomationLogger::LogLine(TEXT("Error=SimLoopTimeout"));
                        if (Test)
                        {
                                Test->AddError(TEXT("PIE simulation loop exceeded wall-clock timeout."));
                        }
                        break;
                }
        }
        if (Simulated < SimDuration && !State->bFailed)
        {
                State->bFailed = true;
                State->FailureMessage = TEXT("SimLoopEarlyExit");
                UTrafficAutomationLogger::LogLine(TEXT("Error=SimLoopEarlyExit"));
                if (Test)
                {
                        Test->AddError(TEXT("PIE simulation loop stopped before expected duration."));
                }
        }
        State->Metrics.SimulatedSeconds = Simulated;
        State->Metrics.Finalize();
        UTrafficAutomationLogger::LogRunMetrics(TEXT("Traffic.Calibration.RoadLab.PIE"), State->Metrics);
        RestoreVisualMode();

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
"Traffic.Calibration.RoadLab.Integration",
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

	const FString LocalTestName = TEXT("Traffic.Calibration.RoadLab.Integration");
	UTrafficAutomationLogger::BeginTestLog(LocalTestName);
	UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION)));
	UTrafficAutomationLogger::LogLine(TEXT("# RoadLab Integration Test"));
	UTrafficAutomationLogger::LogLine(TEXT("VisualBaseline=UserRoadsOnly_NoSynthetic"));

	int32 PrevVisualMode = INDEX_NONE;
	bool bSavedVisualMode = false;
	auto RestoreVisualMode = [&]()
	{
		if (!bSavedVisualMode)
		{
			return;
		}
		if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Visual.Mode")))
		{
			Var->Set(PrevVisualMode, ECVF_SetByCode);
		}
		bSavedVisualMode = false;
	};
	auto SetChaosVisualMode = [&]()
	{
		if (bSavedVisualMode)
		{
			return;
		}
		if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Visual.Mode")))
		{
			PrevVisualMode = Var->GetInt();
			bSavedVisualMode = true;
			Var->Set(2, ECVF_SetByCode);
		}
	};

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

	// Spawn a minimal cross layout using CityBLD BP_MeshRoad so the forced CityBLD provider can build a network.
	UClass* MeshRoadClass = nullptr;
	FString MeshRoadClassPath;
	{
		static const TCHAR* CandidatePaths[] =
		{
			TEXT("/CityBLD/Blueprints/Roads/BP_MeshRoad.BP_MeshRoad_C"),
			TEXT("/CityBLD/CityBLD/Blueprints/Roads/BP_MeshRoad.BP_MeshRoad_C"),
		};

		for (const TCHAR* Path : CandidatePaths)
		{
			if (UClass* Loaded = LoadClass<AActor>(nullptr, Path))
			{
				MeshRoadClass = Loaded;
				MeshRoadClassPath = Path;
				break;
			}
		}
	}
	if (!MeshRoadClass)
	{
		AddError(TEXT("Failed to load CityBLD BP_MeshRoad class (tried /CityBLD/Blueprints/... and /CityBLD/CityBLD/Blueprints/...)."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=LoadBP_MeshRoadFailed"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}
	UTrafficAutomationLogger::LogMetric(TEXT("CityBLD.MeshRoadClassPath"), MeshRoadClassPath);

	auto SpawnCityBLDRoad = [&](const FTransform& Xform) -> AActor*
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* RoadActor = World->SpawnActor<AActor>(MeshRoadClass, Xform, Params);
		if (!RoadActor)
		{
			return nullptr;
		}

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
		AddError(TEXT("Failed to spawn CityBLD BP_MeshRoad actors for automation run."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=NoRoadsSpawned"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}
	UTrafficAutomationLogger::LogLine(TEXT("Phase=RoadsSpawned"));

	UTrafficAutomationLogger::LogLine(TEXT("Phase=Prepare"));
	Subsys->DoPrepare();
	Subsys->Editor_PrepareMapForTraffic();
	TickEditorWorld(World, 0.1f);

	FTrafficRunMetrics Metrics;

	UTrafficAutomationLogger::LogLine(TEXT("Phase=Calibrate"));
	URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get();
	if (!Registry || Registry->GetAllFamilies().Num() == 0)
	{
		AddError(TEXT("No road families detected for calibration."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=NoFamilies"));
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
		UTrafficAutomationLogger::LogLine(TEXT("Error=NoCityBLDFamily"));
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
		TEXT("Calibration.RoadLab.Integration"),
		FamilyId,
		/*MaxIterations=*/5,
		EvalParams,
		Thresholds,
		AlignMetrics,
		FinalCalib);

	if (!bAlignOk)
	{
		AddError(TEXT("Calibration alignment thresholds not met (see logged metrics)."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=CalibrationThresholdsFailed"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
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
		SetChaosVisualMode();
		Manager->SetActiveRunMetrics(&Metrics);
		Manager->SetForceLogicOnlyForTests(false);
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

	bool bChaosMissing = false;
	for (TActorIterator<ATrafficVehicleBase> It(World); It; ++It)
	{
		ATrafficVehicleAdapter* Adapter = nullptr;
		APawn* Chaos = nullptr;
		FString ChaosError;
		if (!TrafficChaosTestUtils::EnsureChaosForLogicVehicle(*World, **It, Adapter, Chaos, ChaosError))
		{
			bChaosMissing = true;
			AddError(FString::Printf(TEXT("RoadLab Chaos missing: %s"), *ChaosError));
			break;
		}
	}
	if (bChaosMissing)
	{
		UTrafficAutomationLogger::LogLine(TEXT("Error=ChaosMissing"));
	}

        // Run a short simulated loop to gather motion metrics.
        const float SimDuration = 3.0f;
        const float SimStep = 0.1f;
        float Simulated = 0.0f;
        const int32 MaxSteps = 200;
        int32 StepsTaken = 0;
        const double SimDeadline = FPlatformTime::Seconds() + 15.0;
        while (Simulated < SimDuration && StepsTaken < MaxSteps)
        {
                TickEditorWorld(World, SimStep);
                SampleVehicleMetrics(World, Metrics, SimStep);
                Simulated += SimStep;
                ++StepsTaken;

                if (FPlatformTime::Seconds() > SimDeadline)
                {
                        AddError(TEXT("RoadLab integration simulation exceeded wall-clock timeout."));
                        UTrafficAutomationLogger::LogLine(TEXT("Error=SimLoopTimeout"));
                        break;
                }
        }
        if (Simulated < SimDuration)
        {
                UTrafficAutomationLogger::LogLine(TEXT("Error=SimLoopEarlyExit"));
                AddError(TEXT("RoadLab integration simulation stopped before expected duration."));
        }
        Metrics.SimulatedSeconds = Simulated;
	Metrics.Finalize();
	UTrafficAutomationLogger::LogRunMetrics(LocalTestName, Metrics);

	UTrafficAutomationLogger::LogLine(TEXT("Result=Success"));
	RestoreVisualMode();
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
"Traffic.Calibration.RoadLab.PIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FTrafficRoadLabIntegrationPIETest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	const FString MapPath = TEXT("/Game/Maps/Test_Maps/RoadLab/Traffic_RoadLab");
	const FString TemplateMap = FPaths::Combine(FPaths::EngineContentDir(), TEXT("Maps/Templates/OpenWorld.umap"));
	TSharedRef<FTrafficPIETestState> State = MakeShared<FTrafficPIETestState>();

	UTrafficAutomationLogger::BeginTestLog(TEXT("Traffic.Calibration.RoadLab.PIE"));
	UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION)));
	UTrafficAutomationLogger::LogLine(TEXT("# RoadLab Integration PIE Test"));
	AddExpectedError(TEXT("The Editor is currently in a play mode."), EAutomationExpectedErrorFlags::Contains, 1);

	UTrafficAutomationLogger::LogLine(TEXT("PIE_Begin"));

	// Load the map and run calibration in the Editor world BEFORE entering PIE. If calibration fails, bail out early.
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

	UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!EditorWorld)
	{
		AddError(TEXT("No editor world available after loading map for RoadLab PIE test."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=NoEditorWorld"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	UTrafficSystemEditorSubsystem* Subsys = GEditor->GetEditorSubsystem<UTrafficSystemEditorSubsystem>();
	if (!Subsys)
	{
		AddError(TEXT("TrafficSystemEditorSubsystem unavailable for pre-PIE calibration."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=NoEditorSubsystem"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	Subsys->Editor_ResetRoadLabHard(false);

	UTrafficAutomationLogger::LogLine(TEXT("PIE_Prepare=DoPrepare"));
	Subsys->DoPrepare();
	Subsys->Editor_PrepareMapForTraffic();

	URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get();
	if (!Registry || Registry->GetAllFamilies().Num() == 0)
	{
		AddError(TEXT("No road families detected for pre-PIE calibration."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=NoFamilies"));
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
		AddError(TEXT("Could not find a CityBLD road family id for pre-PIE calibration."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=NoCityBLDFamily"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	TrafficCalibrationTestUtils::FAlignmentEvalParams EvalParams;
	TrafficCalibrationTestUtils::FAlignmentThresholds Thresholds;
	TrafficCalibrationTestUtils::FAlignmentMetrics AlignMetrics;
	FTrafficLaneFamilyCalibration FinalCalib;

	const bool bAlignOk = TrafficCalibrationTestUtils::RunEditorCalibrationLoop(
		this,
		EditorWorld,
		Subsys,
		TEXT("Calibration.RoadLab.PIE.PrePIE"),
		FamilyId,
		/*MaxIterations=*/5,
		EvalParams,
		Thresholds,
		AlignMetrics,
		FinalCalib);

	if (!bAlignOk)
	{
		AddError(TEXT("Pre-PIE calibration alignment thresholds not met (see logged metrics)."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=PrePIECalibrationThresholdsFailed"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	Subsys->ResetRoadLab();
	UTrafficAutomationLogger::LogLine(TEXT("PIE_PreCalibration=Pass"));

	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficRoadLabWaitForPIEWorldCommand(State, this, 10.0, FPlatformTime::Seconds()));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficPIESpawnAndRunCommand(State, this));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficPIEEndCommand(State));
	return true;
#else
	return false;
#endif
}
