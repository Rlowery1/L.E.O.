#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/SplineComponent.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "EditorLoadingAndSavingUtils.h"
#include "TrafficAutomationLogger.h"
#include "TrafficSystemController.h"
#include "TrafficVehicleManager.h"
#include "TrafficVehicleAdapter.h"
#include "TrafficVehicleBase.h"
#include "TrafficVehicleSettings.h"
#include "TrafficNetworkAsset.h"
#include "TrafficLaneGeometry.h"
#include "TrafficRoadTypes.h"
#include "RoadFamilyRegistry.h"
#include "HAL/IConsoleManager.h"
#include "TrafficRuntimeModule.h"

#if WITH_EDITOR

namespace
{
	static const TCHAR* BaselineMapPackage = TEXT("/AAAtrafficSystem/Maps/Traffic_BaselineStraight");
	static const float BaselineRoadLengthCm = 10000.f;
	static const float BaselineSimSeconds = 5.0f;
	static const float BaselineTickSeconds = 0.1f;
	static const float MaxLateralToleranceCm = 30.0f;
	static const TCHAR* BaselineTestName = TEXT("Traffic.Runtime.BaselineStraightChaos");

	static FString GetBaselineMapFilename()
	{
		return FPackageName::LongPackageNameToFilename(
			BaselineMapPackage,
			FPackageName::GetMapPackageExtension());
	}

	static TSubclassOf<AActor> LoadMeshRoadClass()
	{
		TSubclassOf<AActor> RoadClass = LoadClass<AActor>(nullptr, TEXT("/CityBLD/CityBLD/Blueprints/Roads/BP_MeshRoad.BP_MeshRoad_C"));
		if (!RoadClass)
		{
			RoadClass = LoadClass<AActor>(nullptr, TEXT("/CityBLD/Blueprints/Roads/BP_MeshRoad.BP_MeshRoad_C"));
		}
		return RoadClass;
	}

	static void ConfigureSplineIfPresent(AActor* RoadActor)
	{
		if (!RoadActor)
		{
			return;
		}

		if (USplineComponent* Spline = RoadActor->FindComponentByClass<USplineComponent>())
		{
			Spline->ClearSplinePoints(false);
			Spline->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::World, false);
			Spline->AddSplinePoint(FVector(BaselineRoadLengthCm, 0.f, 0.f), ESplineCoordinateSpace::World, false);
			Spline->SetClosedLoop(false, false);
			Spline->UpdateSpline();
		}
	}

	static void SetControllerRuntimeFlags(ATrafficSystemController* Controller)
	{
		if (!Controller)
		{
			return;
		}

		if (FBoolProperty* AutoBuildProp = FindFProperty<FBoolProperty>(ATrafficSystemController::StaticClass(), TEXT("bAutoBuildOnBeginPlay")))
		{
			AutoBuildProp->SetPropertyValue_InContainer(Controller, true);
		}
		if (FBoolProperty* AutoSpawnProp = FindFProperty<FBoolProperty>(ATrafficSystemController::StaticClass(), TEXT("bAutoSpawnOnBeginPlay")))
		{
			AutoSpawnProp->SetPropertyValue_InContainer(Controller, true);
		}
		if (FIntProperty* VehiclesPerLaneProp = FindFProperty<FIntProperty>(ATrafficSystemController::StaticClass(), TEXT("VehiclesPerLaneRuntime")))
		{
			VehiclesPerLaneProp->SetPropertyValue_InContainer(Controller, 1);
		}
		if (FFloatProperty* SpeedProp = FindFProperty<FFloatProperty>(ATrafficSystemController::StaticClass(), TEXT("RuntimeSpeedCmPerSec")))
		{
			SpeedProp->SetPropertyValue_InContainer(Controller, 800.f);
		}
	}

	static bool BuildBaselineMapAsset(FAutomationTestBase* Test)
	{
		FAutomationEditorCommonUtils::CreateNewMap();

		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (!World)
		{
			if (Test)
			{
				Test->AddError(TEXT("No editor world available while building baseline map."));
			}
			return false;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		TSubclassOf<AActor> RoadClass = LoadMeshRoadClass();
		if (!RoadClass)
		{
			if (Test)
			{
				Test->AddError(TEXT("BP_MeshRoad class not found for baseline map."));
			}
			return false;
		}

		AActor* RoadActor = World->SpawnActor<AActor>(RoadClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (!RoadActor)
		{
			if (Test)
			{
				Test->AddError(TEXT("Failed to spawn BP_MeshRoad in baseline map."));
			}
			return false;
		}
		ConfigureSplineIfPresent(RoadActor);

		ATrafficSystemController* Controller = World->SpawnActor<ATrafficSystemController>(SpawnParams);
		if (!Controller)
		{
			if (Test)
			{
				Test->AddError(TEXT("Failed to spawn TrafficSystemController in baseline map."));
			}
			return false;
		}
		Controller->SetActorLabel(TEXT("TrafficController_Baseline"));
		SetControllerRuntimeFlags(Controller);

		const FString MapFilename = GetBaselineMapFilename();
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(MapFilename), true);

		if (!UEditorLoadingAndSavingUtils::SaveMap(World, BaselineMapPackage))
		{
			if (Test)
			{
				Test->AddError(TEXT("Failed to save baseline map to plugin content."));
			}
			return false;
		}

		return true;
	}

	struct FTrafficBaselinePIEState
	{
		bool bFailed = false;
		FString FailureMessage;
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

	static bool FindBestLaneProjection(
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

	DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(FTrafficBaselineWaitForPIEWorldCommand, TSharedRef<FTrafficBaselinePIEState>, State, FAutomationTestBase*, Test, double, TimeoutSeconds, double, StartTime);
	bool FTrafficBaselineWaitForPIEWorldCommand::Update()
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
			State->FailureMessage = TEXT("NoPIEWorld");
			if (Test)
			{
				Test->AddError(TEXT("PIE world did not start for baseline test."));
			}
			return true;
		}

		return false;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FTrafficBaselineRunCommand, TSharedRef<FTrafficBaselinePIEState>, State, FAutomationTestBase*, Test);
	bool FTrafficBaselineRunCommand::Update()
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

		if (!Controller)
		{
			State->bFailed = true;
			State->FailureMessage = TEXT("NoController");
			if (Test)
			{
				Test->AddError(TEXT("No TrafficSystemController found in PIE world."));
			}
			return true;
		}

		UTrafficNetworkAsset* Net = Controller->GetBuiltNetworkAsset();
		if (!Net || Net->Network.Lanes.Num() == 0)
		{
			State->bFailed = true;
			State->FailureMessage = TEXT("NoNetwork");
			if (Test)
			{
				Test->AddError(TEXT("Baseline network not built during PIE."));
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

		while (Simulated < BaselineSimSeconds)
		{
			World->Tick(LEVELTICK_All, BaselineTickSeconds);
			Simulated += BaselineTickSeconds;

			State->LogicCount = 0;
			for (TActorIterator<ATrafficVehicleBase> It(World); It; ++It)
			{
				ATrafficVehicleBase* Vehicle = *It;
				++State->LogicCount;

				FLaneProjectionResult Proj;
				if (FindBestLaneProjection(Lanes, Vehicle->GetActorLocation(), Vehicle->GetActorForwardVector(), Proj))
				{
					State->MaxLateralError = FMath::Max(State->MaxLateralError, FMath::Abs(Proj.LateralOffsetCm));
					State->MinS = FMath::Min(State->MinS, Proj.S);
					State->MaxS = FMath::Max(State->MaxS, Proj.S);

					const float* Prev = PrevS.Find(Vehicle);
					if (Prev)
					{
						const float Speed = (BaselineTickSeconds > KINDA_SMALL_NUMBER)
							? (Proj.S - *Prev) / BaselineTickSeconds
							: 0.f;
						State->FinalSpeed = Speed;
						if (Speed < 0.f && Test)
						{
							Test->AddWarning(TEXT("Observed negative speed sample during baseline run."));
						}
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

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FTrafficBaselineEndPIECommand, TSharedRef<FTrafficBaselinePIEState>, State, FAutomationTestBase*, Test);
	bool FTrafficBaselineEndPIECommand::Update()
	{
		if (GEditor)
		{
			GEditor->EndPlayMap();
		}

		const bool bHasVehicle = (State->LogicCount == 1);
		const bool bHasChaos = (State->ChaosCount == 1);
		const bool bMovedForward = (State->MaxS > State->MinS) && (State->MaxS > 0.f);
		const bool bLateralOk = (State->MaxLateralError <= MaxLateralToleranceCm);
		const bool bDebugMeshOk = State->bLogicMeshHidden;
		const bool bSpeedPositive = (State->FinalSpeed > 0.f);
		const bool bMonotonic = State->bMonotonicS;
		const bool bNoStuckFlags = (State->Metrics.VehiclesStuck == 0 && State->Metrics.VehiclesCulled == 0);

		UE_LOG(LogTraffic, Display, TEXT("[TrafficBaseline] VehiclesSpawned=%d"), State->LogicCount);
		UE_LOG(LogTraffic, Display, TEXT("[TrafficBaseline] ChaosVehicles=%d"), State->ChaosCount);
		UE_LOG(LogTraffic, Display, TEXT("[TrafficBaseline] MaxLateralErrorCm=%.2f"), State->MaxLateralError);
		UE_LOG(LogTraffic, Display, TEXT("[TrafficBaseline] FinalSpeedCmPerSec=%.2f"), State->FinalSpeed);
		UE_LOG(LogTraffic, Display, TEXT("[TrafficBaseline] FinalDistanceAlongLaneCm=%.2f"), State->MaxS);

		if (!bHasVehicle && Test)
		{
			Test->AddError(TEXT("Expected exactly one TrafficVehicleBase in baseline run."));
		}
		if (!bHasChaos && Test)
		{
			Test->AddError(TEXT("Expected exactly one Chaos visual pawn via TrafficVehicleAdapter."));
		}
		if (!bMovedForward && Test)
		{
			Test->AddError(TEXT("Baseline vehicle did not advance along the lane."));
		}
		if (!bLateralOk && Test)
		{
			Test->AddError(TEXT("Baseline lateral error exceeded tolerance."));
		}
		if (!bDebugMeshOk && Test)
		{
			Test->AddError(TEXT("Logic debug mesh remained visible while Chaos pawn was present."));
		}
		if (!bSpeedPositive && Test)
		{
			Test->AddError(TEXT("Baseline vehicle reported non-positive final speed."));
		}
		if (!bMonotonic && Test)
		{
			Test->AddError(TEXT("Distance along lane decreased during simulation (expected monotonic increase)."));
		}
		if (!bNoStuckFlags && Test)
		{
			Test->AddError(TEXT("Baseline vehicle reported stuck or cull flags."));
		}

		State->Metrics.MaxLateralErrorCm = State->MaxLateralError;
		State->Metrics.Finalize();
		UTrafficAutomationLogger::LogRunMetrics(BaselineTestName, State->Metrics);
		UTrafficAutomationLogger::EndTestLog();

		const bool bPass = !State->bFailed && bHasVehicle && bHasChaos && bMovedForward && bLateralOk && bDebugMeshOk && bSpeedPositive && bMonotonic && bNoStuckFlags;
		UE_LOG(LogTraffic, Display, TEXT("[TrafficBaseline] Result=%s"), bPass ? TEXT("PASS") : TEXT("FAIL"));
		return true;
	}
}

#endif // WITH_EDITOR

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficBaselineRuntimeTest,
	"Traffic.Runtime.BaselineStraightChaos",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficBaselineRuntimeTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	UTrafficAutomationLogger::BeginTestLog(BaselineTestName);
	UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION)));

	if (!BuildBaselineMapAsset(this))
	{
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	FAutomationEditorCommonUtils::LoadMap(BaselineMapPackage);

	TSharedRef<FTrafficBaselinePIEState> State = MakeShared<FTrafficBaselinePIEState>();
	AddExpectedError(TEXT("The Editor is currently in a play mode."), EAutomationExpectedErrorFlags::Contains, 1);

	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficBaselineWaitForPIEWorldCommand(State, this, 10.0, FPlatformTime::Seconds()));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficBaselineRunCommand(State, this));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficBaselineEndPIECommand(State, this));

	return true;
#else
	return false;
#endif
}
