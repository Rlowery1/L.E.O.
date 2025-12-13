#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TrafficAutomationLogger.h"
#include "TrafficSystemController.h"
#include "TrafficVehicleAdapter.h"
#include "TrafficVehicleBase.h"
#include "Components/StaticMeshComponent.h"
#include "TrafficNetworkAsset.h"
#include "TrafficLaneGeometry.h"
#include "TrafficRoadTypes.h"
#include "HAL/IConsoleManager.h"
#include "TrafficRuntimeModule.h"
#include "TrafficVehicleSettings.h"
#include "TrafficVehicleProfile.h"
#include "TrafficVehicleManager.h"
#include "GameFramework/Pawn.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Engine/AssetManager.h"
#include "UObject/SoftObjectPtr.h"
#include "RoadFamilyRegistry.h"
#include "TrafficLaneCalibration.h"
#include "TrafficCalibrationTestUtils.h"
#include "TrafficSystemEditorSubsystem.h"

#if WITH_EDITOR

namespace
{
	static const TCHAR* BaselineCurveMapPackage = TEXT("/AAAtrafficSystem/Maps/Traffic_BaselineCurve");
	static const float BaselineCurveSimSeconds = 5.0f;
	static const float BaselineCurveTickSeconds = 0.1f;
	static const float BaselineCurveMaxLateralErrorCm = 20.0f;
	static const TCHAR* BaselineCurveTestName = TEXT("Traffic.Calibration.BaselineCurveChaos");

	struct FTrafficBaselineCurveState
	{
		bool bFailed = false;
		UWorld* PIEWorld = nullptr;
		FTrafficRunMetrics Metrics;
		float MaxLateralError = 0.f;
		float MinS = TNumericLimits<float>::Max();
		float MaxS = 0.f;
		float FinalSpeed = 0.f;
		int32 ChaosCount = 0;
		int32 LogicCount = 0;
		bool bLogicMeshHidden = true;
		bool bMonotonicS = true;
	};

	static bool FindBestLaneProjection_Curve(
		const TArray<FTrafficLane>& Lanes,
		const FVector& Location,
		const FVector& Forward,
		FLaneProjectionResult& OutProj)
	{
		bool bFound = false;
		float BestDistSq = TNumericLimits<float>::Max();
		for (const FTrafficLane& Lane : Lanes)
		{
			FLaneProjectionResult Proj;
			if (TrafficLaneGeometry::ProjectPointOntoLane(Lane, Location, Forward, Proj))
			{
				const float DistSq = FVector::DistSquared(Location, Proj.ClosestPoint);
				if (DistSq < BestDistSq)
				{
					BestDistSq = DistSq;
					OutProj = Proj;
					bFound = true;
				}
			}
		}
		return bFound;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(FTrafficBaselineCurveWaitForPIEWorldCommand, TSharedRef<FTrafficBaselineCurveState>, State, FAutomationTestBase*, Test, double, TimeoutSeconds, double, StartTime);
	bool FTrafficBaselineCurveWaitForPIEWorldCommand::Update()
	{
		if (GEditor && GEditor->PlayWorld)
		{
			State->PIEWorld = GEditor->PlayWorld;
			return true;
		}

		const double Elapsed = FPlatformTime::Seconds() - StartTime;
		if (Elapsed > TimeoutSeconds)
		{
			State->bFailed = true;
			if (Test)
			{
				Test->AddError(TEXT("PIE world did not start for curve baseline test."));
			}
			return true;
		}

		return false;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FTrafficBaselineCurveRunCommand, TSharedRef<FTrafficBaselineCurveState>, State, FAutomationTestBase*, Test);
	bool FTrafficBaselineCurveRunCommand::Update()
	{
		if (State->bFailed || !State->PIEWorld)
		{
			return true;
		}

		UWorld* World = State->PIEWorld;
		ATrafficSystemController* Controller = nullptr;
		for (TActorIterator<ATrafficSystemController> It(World); It; ++It)
		{
			Controller = *It;
			break;
		}

		// Test-only: remove sidewalk modules that spam null world-context warnings in curve map.
		if (GIsAutomationTesting)
		{
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				AActor* Actor = *It;
				if (!Actor)
				{
					continue;
				}
				const FString Name = Actor->GetName();
				if (Name.Contains(TEXT("Sidewalk")) || Name.Contains(TEXT("RoadModuleBP_AdvancedSidewalk")))
				{
					Actor->Destroy();
				}
			}

			// Ensure a TrafficSystemController exists to build and spawn traffic.
			bool bHasController = false;
			for (TActorIterator<ATrafficSystemController> ItCtrl(World); ItCtrl; ++ItCtrl)
			{
				bHasController = true;
				Controller = *ItCtrl;
				break;
			}
			if (!bHasController)
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				ATrafficSystemController* SpawnedController =
					World->SpawnActor<ATrafficSystemController>(ATrafficSystemController::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
				if (SpawnedController)
				{
					Controller = SpawnedController;
					SpawnedController->Runtime_BuildTrafficNetwork();
					UE_LOG(LogTraffic, Log, TEXT("[TrafficBaselineCurveTest] Spawned controller; Runtime_BuildTrafficNetwork invoked."));
					SpawnedController->Runtime_SpawnTraffic();
					UE_LOG(LogTraffic, Log, TEXT("[TrafficBaselineCurveTest] Runtime_SpawnTraffic invoked on spawned controller."));
				}
				else
				{
					UE_LOG(LogTraffic, Warning, TEXT("[TrafficBaselineCurveTest] Failed to spawn TrafficSystemController in automation."));
				}
			}
		}

		if (!Controller)
		{
			State->bFailed = true;
			if (Test)
			{
				Test->AddError(TEXT("No TrafficSystemController found in curve baseline PIE world."));
			}
			UE_LOG(LogTraffic, Warning, TEXT("[TrafficBaselineCurveTest] No TrafficSystemController available after automation setup."));
			return true;
		}

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
			Manager = World->SpawnActor<ATrafficVehicleManager>(ATrafficVehicleManager::StaticClass(), FTransform::Identity, Params);
		}
		if (!Manager)
		{
			State->bFailed = true;
			if (Test)
			{
				Test->AddError(TEXT("TrafficVehicleManager was not available for curve baseline test."));
			}
			return true;
		}

		UTrafficNetworkAsset* Net = Controller->GetBuiltNetworkAsset();
		if (!Net || Net->Network.Lanes.Num() == 0)
		{
			State->bFailed = true;
			if (Test)
			{
				Test->AddError(TEXT("Curve baseline network not built during PIE."));
			}
			return true;
		}

		const TArray<FTrafficLane>& Lanes = Net->Network.Lanes;
		int32 DebugMeshCVar = 0;
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.ShowLogicDebugMesh")))
		{
			DebugMeshCVar = CVar->GetInt();
		}

		TMap<ATrafficVehicleBase*, float> PrevS;
		float Simulated = 0.f;

		while (Simulated < BaselineCurveSimSeconds)
		{
			World->Tick(LEVELTICK_All, BaselineCurveTickSeconds);
			Simulated += BaselineCurveTickSeconds;

			State->LogicCount = 0;
			for (TActorIterator<ATrafficVehicleBase> It(World); It; ++It)
			{
				ATrafficVehicleBase* Vehicle = *It;
				++State->LogicCount;

				FLaneProjectionResult Proj;
				if (FindBestLaneProjection_Curve(Lanes, Vehicle->GetActorLocation(), Vehicle->GetActorForwardVector(), Proj))
				{
					State->MaxLateralError = FMath::Max(State->MaxLateralError, FMath::Abs(Proj.LateralOffsetCm));
					State->MinS = FMath::Min(State->MinS, Proj.S);
					State->MaxS = FMath::Max(State->MaxS, Proj.S);

					const float* Prev = PrevS.Find(Vehicle);
					if (Prev)
					{
						const float Speed = (BaselineCurveTickSeconds > KINDA_SMALL_NUMBER)
							? (Proj.S - *Prev) / BaselineCurveTickSeconds
							: 0.f;
						State->FinalSpeed = Speed;
						if (Proj.S + 1.0f < *Prev)
						{
							State->bMonotonicS = false;
						}
					}
					PrevS.Add(Vehicle, Proj.S);
				}
			}
		}

		State->Metrics.VehiclesSpawned = State->LogicCount;
		State->Metrics.SimulatedSeconds = Simulated;
		State->Metrics.MaxLateralErrorCm = State->MaxLateralError;

		State->ChaosCount = 0;
		State->bLogicMeshHidden = true;
		for (TActorIterator<ATrafficVehicleAdapter> It(World); It; ++It)
		{
			if (It->ChaosVehicle.IsValid())
			{
				++State->ChaosCount;
			}
			if (It->LogicVehicle.IsValid() && It->ChaosVehicle.IsValid() && DebugMeshCVar <= 0)
			{
				const UStaticMeshComponent* Body = It->LogicVehicle->FindComponentByClass<UStaticMeshComponent>();
				if (Body && Body->IsVisible())
				{
					State->bLogicMeshHidden = false;
				}
			}
		}

		return true;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FTrafficBaselineCurveEndPIECommand, TSharedRef<FTrafficBaselineCurveState>, State, FAutomationTestBase*, Test);
	bool FTrafficBaselineCurveEndPIECommand::Update()
	{
		if (GEditor)
		{
			GEditor->EndPlayMap();
		}

		const bool bHasVehicle = (State->LogicCount > 0);
		const bool bHasChaos = (State->ChaosCount > 0) && (State->ChaosCount == State->LogicCount);
		const bool bMovedForward = (State->MaxS > State->MinS) && (State->MaxS > 0.f);
		const bool bLateralOk = (State->MaxLateralError <= BaselineCurveMaxLateralErrorCm);
		const bool bDebugMeshOk = State->bLogicMeshHidden;
		const bool bMonotonic = State->bMonotonicS;
		const bool bNoStuckFlags = (State->Metrics.VehiclesStuck == 0 && State->Metrics.VehiclesCulled == 0);

		UE_LOG(LogTraffic, Display, TEXT("[TrafficBaselineCurve] VehiclesSpawned=%d"), State->LogicCount);
		UE_LOG(LogTraffic, Display, TEXT("[TrafficBaselineCurve] ChaosVehicles=%d"), State->ChaosCount);
		UE_LOG(LogTraffic, Display, TEXT("[TrafficBaselineCurve] MaxLateralErrorCm=%.2f"), State->MaxLateralError);
		UE_LOG(LogTraffic, Display, TEXT("[TrafficBaselineCurve] FinalSpeedCmPerSec=%.2f"), State->FinalSpeed);
		UE_LOG(LogTraffic, Display, TEXT("[TrafficBaselineCurve] FinalDistanceAlongLaneCm=%.2f"), State->MaxS);

		if (!bHasVehicle && Test)
		{
			Test->AddError(TEXT("Expected at least one TrafficVehicleBase in curve baseline run."));
		}
		if (!bHasChaos && Test)
		{
			Test->AddError(TEXT("Expected Chaos visual pawns for all curve baseline vehicles (check DefaultVehicleProfile VehicleClass)."));
		}
		if (!bMovedForward && Test)
		{
			Test->AddError(TEXT("Curve baseline vehicle did not advance along the lane."));
		}
		if (!bLateralOk && Test)
		{
			Test->AddError(TEXT("Curve baseline lateral error exceeded tolerance."));
		}
		if (!bDebugMeshOk && Test)
		{
			Test->AddError(TEXT("Logic debug mesh remained visible while Chaos pawn was present (curve)."));
		}
		if (!bMonotonic && Test)
		{
			Test->AddError(TEXT("Curve baseline distance along lane decreased during simulation."));
		}
		if (!bNoStuckFlags && Test)
		{
			Test->AddError(TEXT("Curve baseline reported stuck/cull flags."));
		}

		State->Metrics.Finalize();
		UTrafficAutomationLogger::LogRunMetrics(BaselineCurveTestName, State->Metrics);
		const bool bPass = !State->bFailed && bHasVehicle && bHasChaos && bMovedForward && bLateralOk && bDebugMeshOk && bMonotonic && bNoStuckFlags;
		UE_LOG(LogTraffic, Display, TEXT("[TrafficBaselineCurve] Result=%s"), bPass ? TEXT("PASS") : TEXT("FAIL"));
		return true;
	}
}

#endif // WITH_EDITOR

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficBaselineCurveRuntimeTest,
	"Traffic.Calibration.BaselineCurveChaos",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficBaselineCurveRuntimeTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	UTrafficAutomationLogger::BeginTestLog(BaselineCurveTestName);
	UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION)));

	if (GConfig)
	{
		GConfig->LoadGlobalIniFile(GGameIni, TEXT("Game"), nullptr, true);
		GConfig->LoadGlobalIniFile(GEngineIni, TEXT("Engine"), nullptr, true);
	}

	if (UTrafficVehicleSettings* MutableSettings = GetMutableDefault<UTrafficVehicleSettings>())
	{
		MutableSettings->LoadConfig();
		MutableSettings->ReloadConfig();
		const FString ProjectConfig = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("DefaultGame.ini"));
		if (GConfig && !ProjectConfig.IsEmpty())
		{
			GConfig->LoadFile(ProjectConfig);
			MutableSettings->LoadConfig(UTrafficVehicleSettings::StaticClass(), *ProjectConfig);
		}
	}

	const UTrafficVehicleSettings* VehicleSettings = GetDefault<UTrafficVehicleSettings>();
	FSoftObjectPath ProfilePath;
	if (GConfig)
	{
		FString PathString;
		if (GConfig->GetString(TEXT("/Script/TrafficRuntime.TrafficVehicleSettings"), TEXT("DefaultVehicleProfile"), PathString, GGameIni))
		{
			ProfilePath.SetPath(PathString);
		}
	}
	if (ProfilePath.IsNull() && VehicleSettings)
	{
		ProfilePath = VehicleSettings->DefaultVehicleProfile;
	}

	const UTrafficVehicleProfile* DefaultProfile = VehicleSettings ? Cast<UTrafficVehicleProfile>(ProfilePath.TryLoad()) : nullptr;
	if (!DefaultProfile || !DefaultProfile->VehicleClass.IsValid())
	{
		static const TCHAR* DevProfilePath = TEXT("/Game/Traffic/Profiles/DA_VehicleProfile_CitySampleSedan.DA_VehicleProfile_CitySampleSedan");
		UE_LOG(LogTraffic, Warning,
			TEXT("[BaselineCurveChaos] DefaultVehicleProfile invalid from settings. Attempting to load dev profile asset at %s"),
			DevProfilePath);

		FSoftObjectPath Path(DevProfilePath);
		UObject* LoadedObj = Path.TryLoad();
		UTrafficVehicleProfile* DevProfile = Cast<UTrafficVehicleProfile>(LoadedObj);
		if (DevProfile && DevProfile->VehicleClass.IsValid())
		{
			DefaultProfile = DevProfile;
			UE_LOG(LogTraffic, Warning,
				TEXT("[BaselineCurveChaos] Using dev profile asset %s with VehicleClass %s"),
				DevProfilePath,
				*DevProfile->VehicleClass.ToString());
		}
	}

	if (!DefaultProfile || !DefaultProfile->VehicleClass.IsValid())
	{
		static const TCHAR* ChaosClassPaths[] = {
			TEXT("/Game/CitySample/Blueprints/Vehicles/BP_Sedan.BP_Sedan_C"),
			TEXT("/Game/CitySampleVehicles/vehicle07_Car/BP_vehicle07_Car.BP_vehicle07_Car_C")
		};

		for (const TCHAR* ChaosClassPath : ChaosClassPaths)
		{
			if (UClass* ChaosClass = LoadClass<APawn>(nullptr, ChaosClassPath))
			{
				UTrafficVehicleProfile* TempProfile = NewObject<UTrafficVehicleProfile>(GetTransientPackage());
				TempProfile->VehicleClass = ChaosClass;
				DefaultProfile = TempProfile;

				UE_LOG(LogTraffic, Warning,
					TEXT("[BaselineCurveChaos] Using Chaos class fallback at %s"),
					ChaosClassPath);
				break;
			}
		}
	}

	if (!DefaultProfile || !DefaultProfile->VehicleClass.IsValid())
	{
		UE_LOG(LogTraffic, Warning, TEXT("[BaselineCurveChaos] DefaultVehicleProfile is not set or VehicleClass is invalid. Continuing to rely on VehicleManager automation fallback."));
	}

	if (!AutomationOpenMap(BaselineCurveMapPackage))
	{
		UTrafficAutomationLogger::LogLine(TEXT("[TrafficBaselineCurve] Map /Plugins/AAAtrafficSystem/Maps/Traffic_BaselineCurve could not be loaded."));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	// --- Calibration + lane-on-road alignment metrics (pre-PIE) ---
	UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!EditorWorld)
	{
		AddError(TEXT("No editor world available after opening baseline curve map."));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	UTrafficSystemEditorSubsystem* Subsys = GEditor->GetEditorSubsystem<UTrafficSystemEditorSubsystem>();
	if (!Subsys)
	{
		AddError(TEXT("TrafficSystemEditorSubsystem unavailable for calibration."));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	Subsys->DoPrepare();
	Subsys->Editor_PrepareMapForTraffic();

	URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get();
	if (!Registry || Registry->GetAllFamilies().Num() == 0)
	{
		AddError(TEXT("No road families detected for calibration. Ensure CityBLD roads are loaded."));
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
		for (const FRoadFamilyInfo& Info : Registry->GetAllFamilies())
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
		AddError(TEXT("No road family with instances found for calibration."));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	TrafficCalibrationTestUtils::FAlignmentEvalParams EvalParams;
	TrafficCalibrationTestUtils::FAlignmentThresholds Thresholds;
	TrafficCalibrationTestUtils::FAlignmentMetrics AlignMetrics;
	FTrafficLaneFamilyCalibration FinalCalib;

	const FString MetricPrefix = FString::Printf(TEXT("%s.Align"), BaselineCurveTestName);
	const bool bAlignOk = TrafficCalibrationTestUtils::RunEditorCalibrationLoop(
		this,
		EditorWorld,
		Subsys,
		MetricPrefix,
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

	Subsys->ResetRoadLab();

	TSharedRef<FTrafficBaselineCurveState> State = MakeShared<FTrafficBaselineCurveState>();
	AddExpectedError(TEXT("The Editor is currently in a play mode."), EAutomationExpectedErrorFlags::Contains, 3);

	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficBaselineCurveWaitForPIEWorldCommand(State, this, 10.0, FPlatformTime::Seconds()));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficBaselineCurveRunCommand(State, this));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficBaselineCurveEndPIECommand(State, this));
	return true;
#else
	return false;
#endif
}
