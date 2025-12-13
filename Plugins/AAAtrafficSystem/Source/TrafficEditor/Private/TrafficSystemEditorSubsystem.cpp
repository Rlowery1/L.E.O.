#include "TrafficSystemEditorSubsystem.h"
#include "TrafficCalibrationSubsystem.h"
#include "TrafficRuntimeModule.h"
#include "TrafficRoadFamilySettings.h"
#include "TrafficLaneCalibration.h"
#include "RoadFamilyRegistry.h"
#include "TrafficSystemController.h"
#include "TrafficVehicleManager.h"
#include "TrafficVehicleBase.h"
#include "TrafficCityBLDAdapterSettings.h"
#include "TrafficRoadMetadataComponent.h"
#include "TrafficVisualSettings.h"
#include "LaneCalibrationOverlayActor.h"
#include "RoadLanePreviewActor.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "GameFramework/PlayerStart.h"
#include "CollisionQueryParams.h"
#include "UObject/SoftObjectPath.h"
#include "Misc/ScopeExit.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "Misc/MessageDialog.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"

#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "Engine/World.h"
#include "Engine/Selection.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "TrafficGeometryProviderFactory.h"
#include "TrafficGeometryProvider.h"
#include "CityBLDZoneShapeBuilder.h"
#include "ZoneGraphCalibrationUtils.h"
#include "ZoneGraphLaneOverlayUtils.h"

const FName UTrafficSystemEditorSubsystem::RoadLabTag = FName(TEXT("AAA_RoadLab"));

void UTrafficSystemEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CalibrationOverlayActor = nullptr;
	ActiveCalibrationRoadActor = nullptr;
	ActiveFamilyId.Invalidate();
	UE_LOG(LogTraffic, Log, TEXT("UTrafficSystemEditorSubsystem initialized."));
}

void UTrafficSystemEditorSubsystem::Deinitialize()
{
	if (CalibrationOverlayActor.IsValid())
	{
		CalibrationOverlayActor->Destroy();
		CalibrationOverlayActor.Reset();
	}
	ActiveCalibrationRoadActor.Reset();
	ActiveFamilyId.Invalidate();
	
	for (UProceduralMeshComponent* Mesh : RoadLabRibbonMeshes)
	{
		if (Mesh && Mesh->IsValidLowLevel())
		{
			Mesh->DestroyComponent();
		}
	}
	RoadLabRibbonMeshes.Empty();
	
	UE_LOG(LogTraffic, Log, TEXT("UTrafficSystemEditorSubsystem deinitialized."));
	Super::Deinitialize();
}

UWorld* UTrafficSystemEditorSubsystem::GetEditorWorld() const
{
#if WITH_EDITOR
	if (GEditor)
	{
		return GEditor->GetEditorWorldContext().World();
	}
#endif
	return nullptr;
}

bool UTrafficSystemEditorSubsystem::EnsureDetectedFamilies(const TCHAR* Context) const
{
	const URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get();
	const int32 NumFamilies = Registry ? Registry->GetAllFamilies().Num() : 0;
	if (NumFamilies <= 0)
	{
		UE_LOG(LogTraffic, Error, TEXT("[TrafficPrep] No road families detected. Run Prepare Map first. (%s)"), Context ? Context : TEXT("Unknown"));
		return false;
	}
	return true;
}

bool UTrafficSystemEditorSubsystem::HasAnyPreparedRoads() const
{
	UWorld* World = GetEditorWorld();
	if (!World)
	{
		return false;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->FindComponentByClass<UTrafficRoadMetadataComponent>())
		{
			return true;
		}
	}
	return false;
}

bool UTrafficSystemEditorSubsystem::HasAnyCalibratedFamilies() const
{
	const URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get();
	if (!Registry)
	{
		return false;
	}

	for (const FRoadFamilyInfo& Info : Registry->GetAllFamilies())
	{
		if (Info.bIsCalibrated)
		{
			return true;
		}
	}

	return false;
}

int32 UTrafficSystemEditorSubsystem::GetNumActorsForFamily(const FGuid& FamilyId) const
{
	UWorld* World = GetEditorWorld();
	if (!World)
	{
		return 0;
	}

	const URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get();
	if (!Registry)
	{
		return 0;
	}

	const FRoadFamilyInfo* Info = Registry->FindFamilyById(FamilyId);
	if (!Info)
	{
		return 0;
	}

	UClass* RoadClass = Info->RoadClassPath.TryLoadClass<AActor>();
	if (!RoadClass)
	{
		return 0;
	}

	int32 Count = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->IsA(RoadClass))
		{
			++Count;
		}
	}
	return Count;
}

void UTrafficSystemEditorSubsystem::GetActorsForFamily(const FGuid& FamilyId, TArray<AActor*>& OutActors) const
{
	OutActors.Reset();

	UWorld* World = GetEditorWorld();
	if (!World)
	{
		return;
	}

	const URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get();
	if (!Registry)
	{
		return;
	}

	const FRoadFamilyInfo* Info = Registry->FindFamilyById(FamilyId);
	if (!Info)
	{
		return;
	}

	UClass* RoadClass = Info->RoadClassPath.TryLoadClass<AActor>();
	if (!RoadClass)
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->IsA(RoadClass))
		{
			OutActors.Add(*It);
		}
	}
}

void UTrafficSystemEditorSubsystem::TagAsRoadLab(AActor* Actor)
{
	if (Actor && !Actor->Tags.Contains(RoadLabTag))
	{
		Actor->Tags.Add(RoadLabTag);
	}
}

void UTrafficSystemEditorSubsystem::FocusCameraOnActor(AActor* Actor)
{
#if WITH_EDITOR
	if (GEditor && Actor && Actor->IsValidLowLevel())
	{
		GEditor->MoveViewportCamerasToActor(*Actor, false);
	}
#endif
}

void UTrafficSystemEditorSubsystem::Editor_DrawCenterlineDebug(UWorld* World, const TArray<FVector>& CenterlinePoints, bool bColorByCurvature) const
{
#if WITH_EDITOR
	if (!World || CenterlinePoints.Num() < 2)
	{
		return;
	}

	constexpr float HeadingSpikeThresholdDeg = 25.f;
	constexpr float ArrowSpacingCm = 1000.f; // 10m
	constexpr float ArrowLengthCm = 250.f;
	constexpr float ZOffset = 25.f;
	constexpr float LifeTimeSeconds = 30.f;
	constexpr float Thickness = 8.f;

	float DistanceUntilNextArrow = 0.f;

	for (int32 i = 0; i + 1 < CenterlinePoints.Num(); ++i)
	{
		const FVector A = CenterlinePoints[i];
		const FVector B = CenterlinePoints[i + 1];
		const FVector Segment = B - A;
		const float SegmentLen = Segment.Size();
		if (SegmentLen <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		float HeadingDeltaDeg = 0.f;
		if (bColorByCurvature && i + 2 < CenterlinePoints.Num())
		{
			const FVector Dir0 = (CenterlinePoints[i + 1] - CenterlinePoints[i]).GetSafeNormal2D();
			const FVector Dir1 = (CenterlinePoints[i + 2] - CenterlinePoints[i + 1]).GetSafeNormal2D();
			if (!Dir0.IsNearlyZero() && !Dir1.IsNearlyZero())
			{
				const float Dot = FMath::Clamp(FVector::DotProduct(Dir0, Dir1), -1.f, 1.f);
				HeadingDeltaDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));
			}
		}

		const bool bSpike = bColorByCurvature && HeadingDeltaDeg > HeadingSpikeThresholdDeg;
		const FColor Color = bSpike ? FColor::Red : FColor::Cyan;

		DrawDebugLine(World, A, B, Color, /*bPersistentLines=*/false, LifeTimeSeconds, /*DepthPriority=*/0, Thickness);

		const FVector Dir = Segment / SegmentLen;
		while (DistanceUntilNextArrow < SegmentLen)
		{
			const FVector Pos = A + Dir * DistanceUntilNextArrow + FVector(0.f, 0.f, ZOffset);
			DrawDebugDirectionalArrow(
				World,
				Pos,
				Pos + Dir * ArrowLengthCm,
				/*ArrowSize=*/75.f,
				Color,
				/*bPersistentLines=*/false,
				LifeTimeSeconds,
				/*DepthPriority=*/0,
				Thickness);

			DistanceUntilNextArrow += ArrowSpacingCm;
		}

		DistanceUntilNextArrow = FMath::Max(0.f, DistanceUntilNextArrow - SegmentLen);
	}
#endif
}

ATrafficSystemController* UTrafficSystemEditorSubsystem::GetOrSpawnController()
{
#if WITH_EDITOR
	UWorld* World = GetEditorWorld();
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] GetOrSpawnController: No editor world."));
		return nullptr;
	}

	for (TActorIterator<ATrafficSystemController> It(World); It; ++It)
	{
		return *It;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ATrafficSystemController* Controller = World->SpawnActor<ATrafficSystemController>(Params);
	if (Controller)
	{
		TagAsRoadLab(Controller);
		UE_LOG(LogTraffic, Log, TEXT("[TrafficEditor] Spawned new TrafficSystemController: %s"), *Controller->GetName());
	}
	else
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] Failed to spawn TrafficSystemController."));
	}

	return Controller;
#else
	return nullptr;
#endif
}

void UTrafficSystemEditorSubsystem::ResetRoadLab()
{
#if WITH_EDITOR
	UE_LOG(LogTraffic, Log, TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION));
	UWorld* World = GetEditorWorld();
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] ResetRoadLab: No editor world."));
		return;
	}

	TArray<AActor*> ActorsToDestroy;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->IsValidLowLevel() && Actor->Tags.Contains(RoadLabTag))
		{
			ActorsToDestroy.Add(Actor);
		}
	}

	if (CalibrationOverlayActor.IsValid() && CalibrationOverlayActor->IsValidLowLevel())
	{
		if (!ActorsToDestroy.Contains(CalibrationOverlayActor.Get()))
		{
			ActorsToDestroy.Add(CalibrationOverlayActor.Get());
		}
	}

	int32 DestroyedCount = 0;
	for (AActor* Actor : ActorsToDestroy)
	{
		if (Actor && Actor->IsValidLowLevel())
		{
			Actor->Destroy();
			++DestroyedCount;
		}
	}

	CalibrationOverlayActor.Reset();
	ActiveCalibrationRoadActor.Reset();
	ActiveFamilyId.Invalidate();
	RoadLabRibbonMeshes.Empty();

	UE_LOG(LogTraffic, Log, TEXT("[TrafficEditor] ResetRoadLab: Destroyed %d AAA_RoadLab actors (AAA overlay/controllers/vehicles only; user roads untouched)."), DestroyedCount);
#endif
}

void UTrafficSystemEditorSubsystem::Editor_ResetRoadLabHard(bool bIncludeTaggedUserRoads)
{
#if WITH_EDITOR
	UE_LOG(LogTraffic, Log, TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION));
	UWorld* World = GetEditorWorld();
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] Editor_ResetRoadLabHard: No editor world."));
		return;
	}

	int32 NumDestroyed = 0;

	TArray<AActor*> ActorsToDestroy;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}

		const UTrafficCityBLDAdapterSettings* AdapterSettings = GetDefault<UTrafficCityBLDAdapterSettings>();
		const FName RoadTagFromSettings = AdapterSettings ? AdapterSettings->RoadActorTag : NAME_None;

		const bool bIsAAARoadLabTagged = Actor->Tags.Contains(RoadLabTag);
		const bool bIsPreview = Actor->IsA(ARoadLanePreviewActor::StaticClass());
		const bool bIsOverlay = Actor->IsA(ALaneCalibrationOverlayActor::StaticClass());
		const bool bIsVehicle = Actor->IsA(ATrafficVehicleBase::StaticClass());
		const bool bIsVehicleMgr = Actor->IsA(ATrafficVehicleManager::StaticClass());
		const bool bIsController = Actor->IsA(ATrafficSystemController::StaticClass());

		const bool bIsUserRoadTagged = bIncludeTaggedUserRoads && (!RoadTagFromSettings.IsNone() && Actor->ActorHasTag(RoadTagFromSettings));

		if (bIsAAARoadLabTagged || bIsPreview || bIsOverlay || bIsVehicle || bIsVehicleMgr || bIsController || bIsUserRoadTagged)
		{
			ActorsToDestroy.Add(Actor);
		}
	}

	for (AActor* Actor : ActorsToDestroy)
	{
		if (Actor && Actor->IsValidLowLevel())
		{
			World->DestroyActor(Actor);
			++NumDestroyed;
		}
	}

	CalibrationOverlayActor.Reset();
	ActiveCalibrationRoadActor.Reset();
	ActiveFamilyId.Invalidate();
	RoadLabRibbonMeshes.Empty();

	UE_LOG(LogTraffic, Log, TEXT("[TrafficEditor] Editor_ResetRoadLabHard: Destroyed %d actors (include tagged user roads=%s)."),
		NumDestroyed, bIncludeTaggedUserRoads ? TEXT("true") : TEXT("false"));
#endif
}

void UTrafficSystemEditorSubsystem::ConvertSelectedSplineActors()
{
#if WITH_EDITOR
	if (!GEditor)
	{
		return;
	}

	USelection* SelectedActors = GEditor->GetSelectedActors();
	if (!SelectedActors || SelectedActors->Num() == 0)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] ConvertSelectedSplineActors: No actor selected."));
		return;
	}

	int32 ConvertedCount = 0;
	for (FSelectionIterator It(*SelectedActors); It; ++It)
	{
		AActor* Actor = Cast<AActor>(*It);
		if (!Actor)
		{
			continue;
		}

		const UTrafficCityBLDAdapterSettings* AdapterSettings = GetDefault<UTrafficCityBLDAdapterSettings>();
		const FName RoadTagFromSettings = AdapterSettings ? AdapterSettings->RoadActorTag : NAME_None;

		USplineComponent* Spline = Actor->FindComponentByClass<USplineComponent>();
		if (!Spline || Spline->GetNumberOfSplinePoints() < 2)
		{
			continue;
		}

		TagAsRoadLab(Actor);
		if (!RoadTagFromSettings.IsNone())
		{
			Actor->Tags.AddUnique(RoadTagFromSettings);
		}

		if (UTrafficRoadMetadataComponent* Meta = Actor->FindComponentByClass<UTrafficRoadMetadataComponent>())
		{
			// already has meta
		}
		else
		{
			UTrafficRoadMetadataComponent* NewMeta = NewObject<UTrafficRoadMetadataComponent>(Actor);
			NewMeta->RegisterComponent();
			Actor->AddInstanceComponent(NewMeta);
		}

		++ConvertedCount;
	}

	UE_LOG(LogTraffic, Log, TEXT("[TrafficEditor] ConvertSelectedSplineActors: Converted %d spline actors (driving uses tagged spline; originals remain for visuals)."),
		ConvertedCount);
#endif
}

void UTrafficSystemEditorSubsystem::Editor_PrepareMapForTraffic()
{
#if WITH_EDITOR
	UE_LOG(LogTraffic, Log, TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION));
	UWorld* World = GetEditorWorld();
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] Editor_PrepareMapForTraffic: No editor world."));
		return;
	}

	URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get();
	if (!Registry)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] Editor_PrepareMapForTraffic: Registry unavailable."));
		return;
	}

	int32 ActorsFound = 0;
	int32 FamiliesCreated = 0;
	int32 MetadataAttached = 0;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}

		const FString ClassName = Actor->GetClass()->GetName();
		const bool bHasSpline = (Actor->FindComponentByClass<USplineComponent>() != nullptr);
		const bool bNameMatches =
			ClassName.Contains(TEXT("Road"), ESearchCase::IgnoreCase) ||
			ClassName.Contains(TEXT("Street"), ESearchCase::IgnoreCase) ||
			ClassName.Contains(TEXT("Highway"), ESearchCase::IgnoreCase);

		if (!bHasSpline && !bNameMatches)
		{
			continue;
		}

		++ActorsFound;
		UE_LOG(LogTraffic, Log, TEXT("[TrafficPrep] Found road actor: %s"), *ClassName);

		bool bCreated = false;
		FRoadFamilyInfo* FamilyInfo = Registry->FindOrCreateFamilyForClass(Actor->GetClass(), &bCreated);
		if (bCreated && FamilyInfo)
		{
			++FamiliesCreated;
			UE_LOG(LogTraffic, Log, TEXT("[TrafficPrep] Created family: %s for class %s"),
				*FamilyInfo->FamilyId.ToString(), *ClassName);
		}

		UTrafficRoadMetadataComponent* Meta = Actor->FindComponentByClass<UTrafficRoadMetadataComponent>();
		if (!Meta)
		{
			Meta = NewObject<UTrafficRoadMetadataComponent>(Actor);
			Meta->RegisterComponent();
			Actor->AddInstanceComponent(Meta);
			++MetadataAttached;
			UE_LOG(LogTraffic, Log, TEXT("[TrafficPrep] Attached metadata to: %s"), *Actor->GetName());
		}
		else if (!Meta->IsRegistered())
		{
			Meta->RegisterComponent();
		}

		if (FamilyInfo && Meta)
		{
			Meta->RoadFamilyId = FamilyInfo->FamilyId;
			if (FamilyInfo->FamilyDefinition.FamilyName.IsNone())
			{
				FamilyInfo->FamilyDefinition.FamilyName = FName(*FamilyInfo->DisplayName);
			}
			Meta->FamilyName = FamilyInfo->FamilyDefinition.FamilyName;
			Meta->bIncludeInTraffic = true;
		}
	}

	Registry->RefreshCache();
	Registry->SaveConfig();

	if (ActorsFound == 0)
	{
		UE_LOG(LogTraffic, Warning,
			TEXT("[TrafficEditor] PrepareAllRoads: Found 0 spline roads. If this level uses World Partition or CityBLD streaming, ensure regions containing roads are loaded."));
		const bool bAutomationOrCmdlet = IsRunningCommandlet() || GIsAutomationTesting;
		if (!bAutomationOrCmdlet)
		{
			FMessageDialog::Open(
				EAppMsgType::Ok,
				NSLOCTEXT("TrafficSystemEditorSubsystem", "Traffic_Prepare_NoSplineRoads",
					"AAA Traffic did not find any spline-based roads in this level.\n\n"
					"This usually means:\n"
					"  - Road actors have not been placed yet, or\n"
					"  - Your World Partition / CityBLD regions that contain roads are not loaded.\n\n"
					"If this map uses World Partition or CityBLD streaming, load the regions containing your roads and run PREPARE MAP again."));
		}
		else
		{
			UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor][Automation] Prepare found no spline roads; dialog suppressed."));
		}
	}

	UE_LOG(LogTraffic, Log, TEXT("[TrafficPrep] Summary: Actors=%d; FamiliesCreated=%d; MetadataAttached=%d"),
		ActorsFound, FamiliesCreated, MetadataAttached);
#endif
}

void UTrafficSystemEditorSubsystem::DoPrepare()
{
#if WITH_EDITOR
	UE_LOG(LogTraffic, Log, TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION));
	UWorld* World = GetEditorWorld();
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] DoPrepare: No editor world."));
		return;
	}

	if (UTrafficCalibrationSubsystem* Calib = GEditor->GetEditorSubsystem<UTrafficCalibrationSubsystem>())
	{
		UE_LOG(LogTraffic, Log, TEXT("[TrafficEditor] PREPARE: Running PrepareAllRoads()."));
		Calib->PrepareAllRoads();

		if (!HasAnyPreparedRoads())
		{
			UE_LOG(LogTraffic, Warning,
				TEXT("[TrafficEditor] PrepareAllRoads: Found 0 spline roads. If this level uses World Partition or CityBLD streaming, ensure regions containing roads are loaded."));
			const bool bAutomationOrCmdlet = IsRunningCommandlet() || GIsAutomationTesting;
			if (!bAutomationOrCmdlet)
			{
				FMessageDialog::Open(
					EAppMsgType::Ok,
					NSLOCTEXT("TrafficSystemEditorSubsystem", "Traffic_Prepare_NoSplineRoads",
						"AAA Traffic did not find any spline-based roads in this level.\n\n"
						"This usually means:\n"
						"  - Road actors have not been placed yet, or\n"
						"  - Your World Partition / CityBLD regions that contain roads are not loaded.\n\n"
						"If this map uses World Partition or CityBLD streaming, load the regions containing your roads and run PREPARE MAP again."));
			}
			else
			{
				UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor][Automation] Prepare found no spline roads; dialog suppressed."));
			}
		}
	}
	else
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] PREPARE: Calibration subsystem not available."));
	}
#endif
}

void UTrafficSystemEditorSubsystem::DoBuild()
{
#if WITH_EDITOR
	UE_LOG(LogTraffic, Log, TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION));
	UWorld* World = GetEditorWorld();
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] DoBuild: No editor world."));
		return;
	}

	if (!EnsureDetectedFamilies(TEXT("Build")))
	{
		return;
	}

	if (!HasAnyCalibratedFamilies())
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] DoBuild: No calibrated families. Calibrate at least one family first."));
		const bool bAutomationOrCmdlet = IsRunningCommandlet() || GIsAutomationTesting;
		if (!bAutomationOrCmdlet)
		{
			FMessageDialog::Open(
				EAppMsgType::Ok,
				NSLOCTEXT("TrafficSystemEditorSubsystem", "Traffic_NoCalibratedFamilies_Build",
					"No calibrated road families found.\n\n"
					"Run PREPARE MAP, then CALIBRATE and BAKE at least one road family before building traffic."));
		}
		else
		{
			UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor][Automation] No calibrated families; dialog suppressed."));
		}
		return;
	}

	ATrafficSystemController* Controller = GetOrSpawnController();
	if (!Controller)
	{
		return;
	}

	UE_LOG(LogTraffic, Log, TEXT("[TrafficEditor] BUILD: Calling Editor_BuildTrafficNetwork()."));
	Controller->Editor_BuildTrafficNetwork();
#endif
}

void UTrafficSystemEditorSubsystem::DoCars()
{
#if WITH_EDITOR
	UE_LOG(LogTraffic, Log, TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION));
	UWorld* World = GetEditorWorld();
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] DoCars: No editor world."));
		return;
	}

	if (!EnsureDetectedFamilies(TEXT("Cars")))
	{
		return;
	}

	if (!HasAnyPreparedRoads())
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] DoCars: No prepared roads. Run Prepare Map first."));
		return;
	}

	if (!HasAnyCalibratedFamilies())
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] DoCars: No calibrated families. Calibrate at least one family first."));
		return;
	}

	ATrafficSystemController* Controller = GetOrSpawnController();
	if (!Controller)
	{
		return;
	}

	const int32 NumRoads = Controller->GetNumRoads();
	const int32 NumLanes = Controller->GetNumLanes();
	if (NumRoads <= 0 || NumLanes <= 0)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] Build produced an empty traffic network (Roads=%d, Lanes=%d). No vehicles will be spawned."), NumRoads, NumLanes);
		const bool bAutomationOrCmdlet = IsRunningCommandlet() || GIsAutomationTesting;
		if (!bAutomationOrCmdlet)
		{
			FMessageDialog::Open(
				EAppMsgType::Ok,
				NSLOCTEXT("TrafficSystemEditorSubsystem", "Traffic_EmptyNetwork",
					"AAA Traffic could not build any roads or lanes in this level.\n\n"
					"This usually means:\n"
					"  - PREPARE MAP was not run,\n"
					"  - No road actors are tagged for traffic,\n"
					"  - Your World Partition / CityBLD regions that contain roads are not loaded, or\n"
					"  - The active geometry provider does not support your road kit.\n\n"
					"Run PREPARE MAP and ensure at least one road family is calibrated and included in traffic."));
		}
		else
		{
			UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor][Automation] Empty traffic network; dialog suppressed."));
		}
		return;
	}

	ATrafficVehicleManager* Manager = nullptr;
	for (TActorIterator<ATrafficVehicleManager> It(World); It; ++It)
	{
		Manager = *It;
		break;
	}

	if (!Manager)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Manager = World->SpawnActor<ATrafficVehicleManager>(SpawnParams);
		if (!Manager)
		{
			UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] DoCars: Failed to spawn ATrafficVehicleManager."));
			return;
		}
	}

	UE_LOG(LogTraffic, Log, TEXT("[TrafficEditor] CARS: Spawning test vehicles."));
	Manager->SpawnTestVehicles(3, 800.f);

	ATrafficVehicleBase* FirstVehicle = nullptr;
	for (TActorIterator<ATrafficVehicleBase> It(World); It; ++It)
	{
		FirstVehicle = *It;
		break;
	}

	if (FirstVehicle)
	{
		FocusCameraOnActor(FirstVehicle);
	}
	else
	{
		FocusCameraOnActor(Manager);
	}
#endif
}

void UTrafficSystemEditorSubsystem::BuildRoadRibbonForActor(AActor* RoadActor, const FRoadFamilyDefinition& Family)
{
	if (!RoadActor)
	{
		return;
	}

	USplineComponent* Spline = RoadActor->FindComponentByClass<USplineComponent>();
	if (!Spline || Spline->GetNumberOfSplinePoints() < 2)
	{
		return;
	}

	const UTrafficVisualSettings* VisualSettings = GetDefault<UTrafficVisualSettings>();

	const float TotalWidth = (Family.Forward.NumLanes * Family.Forward.LaneWidthCm) + 
	                         (Family.Backward.NumLanes * Family.Backward.LaneWidthCm);
	const float HalfWidth = TotalWidth * 0.5f;

	UProceduralMeshComponent* RibbonMesh = NewObject<UProceduralMeshComponent>(RoadActor);
	RibbonMesh->SetupAttachment(RoadActor->GetRootComponent());
	RibbonMesh->RegisterComponent();
	RibbonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RibbonMesh->SetCastShadow(false);

	const float SplineLength = Spline->GetSplineLength();
	const float SampleStep = 200.f;
	const int32 NumSamples = FMath::Max(2, FMath::CeilToInt(SplineLength / SampleStep) + 1);

	const float UVScale = (VisualSettings && VisualSettings->RoadRibbonUVScale > 0.f)
		? VisualSettings->RoadRibbonUVScale
		: 1.0f;

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	TArray<FProcMeshTangent> Tangents;

	const float ZOffset = 5.f; // lift slightly above the template floor to avoid z-fighting
	FColor RoadColor(80, 80, 80, 255);

	for (int32 i = 0; i < NumSamples; ++i)
	{
		float Distance = (i == NumSamples - 1) ? SplineLength : (i * SampleStep);
		Distance = FMath::Clamp(Distance, 0.f, SplineLength);

		FVector Position = Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
		FVector Right = Spline->GetRightVectorAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);

		FVector LeftVertex = Position - Right * HalfWidth + FVector(0, 0, ZOffset);
		FVector RightVertex = Position + Right * HalfWidth + FVector(0, 0, ZOffset);

		Vertices.Add(LeftVertex);
		Vertices.Add(RightVertex);

		Normals.Add(FVector::UpVector);
		Normals.Add(FVector::UpVector);

		const float V = (Distance / 100.0f) * UVScale;
		UVs.Add(FVector2D(0.f, V));
		UVs.Add(FVector2D(1.f, V));

		Colors.Add(RoadColor);
		Colors.Add(RoadColor);

		// Tangent points along the spline direction for lighting with normal maps
		FVector TangentDir = Spline->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
		FProcMeshTangent Tangent(TangentDir, false);
		Tangents.Add(Tangent);
		Tangents.Add(Tangent);
	}

	for (int32 i = 0; i < NumSamples - 1; ++i)
	{
		int32 BL = i * 2;
		int32 BR = i * 2 + 1;
		int32 TL = (i + 1) * 2;
		int32 TR = (i + 1) * 2 + 1;

		Triangles.Add(BL);
		Triangles.Add(TL);
		Triangles.Add(BR);

		Triangles.Add(BR);
		Triangles.Add(TL);
		Triangles.Add(TR);
	}

	RibbonMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);

	// Supply tangents to improve material lighting when normal maps are used.
	RibbonMesh->UpdateMeshSection(0, Vertices, Normals, UVs, Colors, Tangents);

	// Resolve configured material, with a fallback to an engine template asphalt-like material, and log outcome.
	UMaterialInterface* RoadMat = nullptr;
	FSoftObjectPath ConfigPath;
	if (VisualSettings)
	{
		ConfigPath = VisualSettings->RoadRibbonMaterial.ToSoftObjectPath();
	}
	if (!ConfigPath.IsNull())
	{
		RoadMat = Cast<UMaterialInterface>(ConfigPath.TryLoad());
		if (RoadMat)
		{
			UE_LOG(LogTraffic, Log, TEXT("[RoadRibbon] Using configured material: %s"), *RoadMat->GetFullName());
		}
		else
		{
			UE_LOG(LogTraffic, Warning, TEXT("[RoadRibbon] Failed to load configured material: %s"), *ConfigPath.ToString());
		}
	}
	else
	{
		UE_LOG(LogTraffic, Warning, TEXT("[RoadRibbon] No configured material set in Visual Settings; will use fallback."));
	}
	if (!RoadMat)
	{
		static const FSoftObjectPath FallbackPath(TEXT("/Engine/MapTemplates/Materials/BasicAsset03.BasicAsset03"));
		RoadMat = Cast<UMaterialInterface>(FallbackPath.TryLoad());
		if (RoadMat)
		{
			UE_LOG(LogTraffic, Log, TEXT("[RoadRibbon] Using fallback material: %s"), *RoadMat->GetFullName());
		}
		else
		{
			UE_LOG(LogTraffic, Warning, TEXT("[RoadRibbon] Fallback material missing; ribbon will show checkerboard."));
		}
	}

	if (RoadMat)
	{
		RibbonMesh->SetMaterial(0, RoadMat);
	}

	RoadLabRibbonMeshes.Add(RibbonMesh);
}

float UTrafficSystemEditorSubsystem::TraceGroundHeight(UWorld* World, float X, float Y, float FallbackZ)
{
	if (!World)
	{
		return FallbackZ;
	}

	FVector TraceStart(X, Y, 10000.f);
	FVector TraceEnd(X, Y, -10000.f);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RoadLabTrace), false);

	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
	{
		return Hit.Location.Z + 5.f;
	}

	return FallbackZ + 5.f;
}

void UTrafficSystemEditorSubsystem::Editor_CreateRoadLabMap(bool bOverwriteExisting)
{
#if WITH_EDITOR
	UE_LOG(LogTraffic, Log, TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION));

	const FString MapPath = TEXT("/Game/Maps/Test_Maps/RoadLab/Traffic_RoadLab");
	const FString MapFilename = FPackageName::LongPackageNameToFilename(
		MapPath,
		FPackageName::GetMapPackageExtension());

	const bool bMapExists = FPaths::FileExists(MapFilename);

	if (bMapExists && !bOverwriteExisting)
	{
		UE_LOG(LogTraffic, Log, TEXT("[TrafficEditor] RoadLab map already exists at %s (skipping)."), *MapFilename);
		return;
	}

	IFileManager::Get().MakeDirectory(*FPaths::GetPath(MapFilename), true);

	if (bMapExists && bOverwriteExisting)
	{
		IFileManager::Get().Delete(*MapFilename, false, true, true);
	}

	const FString TemplateFilename = FPaths::Combine(FPaths::EngineContentDir(), TEXT("Maps/Templates/OpenWorld.umap"));
	if (!FPaths::FileExists(TemplateFilename))
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] OpenWorld template missing at %s"), *TemplateFilename);
		return;
	}

	UWorld* NewWorld = UEditorLoadingAndSavingUtils::NewMapFromTemplate(TemplateFilename, /*bSaveExistingMap*/ false);
	if (!NewWorld)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] Failed to create RoadLab map from template."));
		return;
	}

	if (!UEditorLoadingAndSavingUtils::SaveMap(NewWorld, MapPath))
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] Failed to save RoadLab map to %s"), *MapFilename);
		return;
	}

	FEditorFileUtils::LoadMap(MapFilename, false, true);

	UE_LOG(LogTraffic, Log, TEXT("[TrafficEditor] RoadLab map created at %s (overwrite=%s)."),
		*MapFilename, bOverwriteExisting ? TEXT("true") : TEXT("false"));
#endif
}

void UTrafficSystemEditorSubsystem::Editor_BeginCalibrationForFamily(const FGuid& FamilyId)
{
#if WITH_EDITOR
	UE_LOG(LogTraffic, Log, TEXT("[TrafficEditor] BeginCalibrationForFamily: %s"), *FamilyId.ToString());

	UWorld* World = GetEditorWorld();
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] BeginCalibrationForFamily: No editor world."));
		return;
	}

	URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get();
	if (!Registry)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] BeginCalibrationForFamily: Registry unavailable."));
		return;
	}

	FRoadFamilyInfo* FamilyInfo = Registry->FindFamilyById(FamilyId);
	if (!FamilyInfo)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] BeginCalibrationForFamily: Family not found."));
		return;
	}

	if (FamilyInfo->FamilyDefinition.FamilyName.IsNone())
	{
		FamilyInfo->FamilyDefinition.FamilyName = FName(*FamilyInfo->DisplayName);
	}

	const bool bAutomationOrCmdlet = IsRunningCommandlet() || GIsAutomationTesting;

	const int32 NumInstances = GetNumActorsForFamily(FamilyId);
	if (NumInstances <= 0)
	{
		UE_LOG(LogTraffic, Warning,
			TEXT("[TrafficEditor] BeginCalibrationForFamily: Family %s has 0 actors in this level."),
			*FamilyId.ToString());

		if (!bAutomationOrCmdlet)
		{
			FMessageDialog::Open(
				EAppMsgType::Ok,
				NSLOCTEXT("TrafficSystemEditorSubsystem", "Traffic_NoActorsForFamily",
					"There are no actors for this road family in this level.\n\n"
					"This usually means:\n"
					"  - You deleted all instances of this road class from the map, or\n"
					"  - The World Partition / CityBLD regions that contain them are not loaded.\n\n"
					"Place or load at least one road actor of this family before calibrating it."));
		}
		else
		{
			UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor][Automation] No actors for family; dialog suppressed."));
		}
		return;
	}

	UClass* RoadClass = FamilyInfo->RoadClassPath.TryLoadClass<AActor>();
	if (!RoadClass)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] BeginCalibrationForFamily: Failed to load road class for %s"), *FamilyInfo->DisplayName);
		return;
	}

	TArray<AActor*> ActorsBefore;
	GetActorsForFamily(FamilyId, ActorsBefore);

	auto WarnIfMissing = [&](const TCHAR* Context)
	{
		TArray<AActor*> ActorsAfter;
		GetActorsForFamily(FamilyId, ActorsAfter);

		int32 MissingCount = 0;
		for (AActor* Actor : ActorsBefore)
		{
			if (Actor && !ActorsAfter.Contains(Actor))
			{
				++MissingCount;
			}
		}

		if (MissingCount > 0)
		{
			UE_LOG(LogTraffic, Warning,
				TEXT("[TrafficEditor] Road actors disappeared during calibration for family %s. Before=%d After=%d Context=%s"),
				*FamilyId.ToString(), ActorsBefore.Num(), ActorsAfter.Num(), Context ? Context : TEXT("BeginCalibration"));

			if (!bAutomationOrCmdlet)
			{
				FMessageDialog::Open(
					EAppMsgType::Ok,
					NSLOCTEXT("TrafficEditor", "Traffic_RoadsDisappeared",
						"One or more road actors disappeared during calibration for this family.\n"
						"This is unexpected. Please check your road kit tools and streaming state."));
			}
			else
			{
				UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor][Automation] Roads disappeared during calibration; dialog suppressed."));
			}
		}
	};

	ON_SCOPE_EXIT
	{
		WarnIfMissing(TEXT("EndBeginCalibration"));
	};

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Try to use a selected actor of this family if present; otherwise spawn a preview instance.
	AActor* RoadActor = nullptr;
	if (GEditor && GEditor->GetSelectedActors())
	{
		for (FSelectionIterator It(*GEditor->GetSelectedActors()); It; ++It)
		{
			if (AActor* Candidate = Cast<AActor>(*It))
			{
				if (Candidate->IsA(RoadClass))
				{
					RoadActor = Candidate;
					break;
				}
			}
		}
	}

	// Cleanup previous preview actors.
	if (ActiveCalibrationRoadActor.IsValid())
	{
		ActiveCalibrationRoadActor->Destroy();
	}
	if (CalibrationOverlayActor.IsValid())
	{
		CalibrationOverlayActor->Destroy();
		CalibrationOverlayActor.Reset();
	}

	if (!RoadActor)
	{
		RoadActor = World->SpawnActor<AActor>(RoadClass, FTransform::Identity, SpawnParams);
		if (!RoadActor)
		{
			UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] BeginCalibrationForFamily: Failed to spawn road actor of class %s"), *RoadClass->GetName());
			return;
		}
		TagAsRoadLab(RoadActor);
	}

	TArray<TObjectPtr<UObject>> ProviderObjects;
	TArray<ITrafficRoadGeometryProvider*> Providers;
	UTrafficGeometryProviderFactory::CreateProviderChainForEditorWorld(World, ProviderObjects, Providers);

	TArray<FVector> CenterlinePoints;
	for (ITrafficRoadGeometryProvider* Provider : Providers)
	{
		if (Provider && Provider->GetDisplayCenterlineForActor(RoadActor, CenterlinePoints) && CenterlinePoints.Num() >= 2)
		{
			break;
		}
	}

	if (CenterlinePoints.Num() < 2)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] BeginCalibrationForFamily: Provider could not supply a display centerline for %s."), *GetNameSafe(RoadActor));
		if (RoadActor == ActiveCalibrationRoadActor.Get())
		{
			RoadActor = nullptr;
		}
		else if (RoadActor && RoadActor->Tags.Contains(RoadLabTag))
		{
			RoadActor->Destroy();
		}
		return;
	}

	const UTrafficCityBLDAdapterSettings* AdapterSettings = GetDefault<UTrafficCityBLDAdapterSettings>();
	if (AdapterSettings && AdapterSettings->bDrawCalibrationCenterlineDebug)
	{
		Editor_DrawCenterlineDebug(World, CenterlinePoints, /*bColorByCurvature=*/true);
	}

	const bool bIsCityBLD = RoadActor->GetClass()->GetName().Contains(TEXT("BP_MeshRoad"), ESearchCase::IgnoreCase);
	if (bIsCityBLD && AdapterSettings && AdapterSettings->bUseZoneGraphLanePolylinesForCalibrationOverlay)
	{
		// Ensure a lane profile is present so ZoneShapes can generate lanes deterministically.
		if (!FamilyInfo->FamilyDefinition.VehicleLaneProfile.IsValid() && AdapterSettings->DefaultCityBLDVehicleLaneProfile.IsValid())
		{
			FamilyInfo->FamilyDefinition.VehicleLaneProfile = AdapterSettings->DefaultCityBLDVehicleLaneProfile;
			Registry->SaveConfig();
		}

		const FSoftObjectPath LaneProfilePath = FamilyInfo->FamilyDefinition.VehicleLaneProfile.IsValid()
			? FamilyInfo->FamilyDefinition.VehicleLaneProfile
			: AdapterSettings->DefaultCityBLDVehicleLaneProfile;

		CityBLDZoneShapeBuilder::BuildVehicleZoneGraphForCityBLDRoad(World, RoadActor, CenterlinePoints, LaneProfilePath);
	}

	const bool bHasUnderlyingMesh = (RoadActor->FindComponentByClass<UStaticMeshComponent>() != nullptr);

	ALaneCalibrationOverlayActor* Overlay = World->SpawnActor<ALaneCalibrationOverlayActor>(
		FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!Overlay)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] BeginCalibrationForFamily: Failed to spawn overlay actor."));
		RoadActor->Destroy();
		return;
	}

	TagAsRoadLab(Overlay);
	FTrafficLaneFamilyCalibration Calib;
	bool bUsedZoneGraph = false;
	if (bIsCityBLD)
	{
		bUsedZoneGraph = ComputeCalibrationFromZoneGraph(World, CenterlinePoints, Calib);
	}

	if (!bUsedZoneGraph)
	{
		Calib.NumLanesPerSideForward = FMath::Clamp(FamilyInfo->FamilyDefinition.Forward.NumLanes, 1, 5);
		Calib.NumLanesPerSideBackward = FMath::Clamp(FamilyInfo->FamilyDefinition.Backward.NumLanes, 0, 5);
		Calib.LaneWidthCm = FMath::Clamp(FamilyInfo->FamilyDefinition.Forward.LaneWidthCm, 200.f, 500.f);
		Calib.CenterlineOffsetCm = FamilyInfo->FamilyDefinition.Forward.InnerLaneCenterOffsetCm;
	}

	Overlay->ApplyCalibrationSettings(
		Calib.NumLanesPerSideForward,
		Calib.NumLanesPerSideBackward,
		Calib.LaneWidthCm,
		Calib.CenterlineOffsetCm);

	bool bBuiltOverlayFromZoneGraph = false;
	if (AdapterSettings &&
		AdapterSettings->bUseZoneGraphLanePolylinesForCalibrationOverlay &&
		bIsCityBLD)
	{
		TArray<TArray<FVector>> LanePolylines;
		if (ZoneGraphLaneOverlayUtils::GetZoneGraphLanePolylinesNearActor(World, RoadActor, LanePolylines))
		{
			Overlay->BuildFromLanePolylines(LanePolylines, RoadActor->GetActorTransform());
			bBuiltOverlayFromZoneGraph = true;
		}
	}

	if (!bBuiltOverlayFromZoneGraph)
	{
		Overlay->BuildFromCenterline(CenterlinePoints, Calib, RoadActor->GetActorTransform());
	}
#if WITH_EDITOR
	if (GEditor)
	{
		GEditor->SelectNone(/*NoteSelectionChange=*/false, /*DeselectBSPSurfs=*/true, /*WarnAboutManyActors=*/false);
		GEditor->SelectActor(Overlay, /*InSelected=*/true, /*Notify=*/true);
		FocusCameraOnActor(Overlay);
	}
#endif
	Overlay->SetActorHiddenInGame(false);
	Overlay->SetIsTemporarilyHiddenInEditor(false);

	const int32 NumArrowInstances = Overlay->GetArrowInstanceCount();
	if (NumArrowInstances <= 0)
	{
		if (!bAutomationOrCmdlet)
		{
			FMessageDialog::Open(
				EAppMsgType::Ok,
				NSLOCTEXT("TrafficSystemEditorSubsystem", "Traffic_OverlayZeroInstances",
					"Calibration overlay did not place any arrows.\n\n"
					"This usually means the road's display centerline is degenerate (near-zero length)\n"
					"or the calibration inputs are invalid.\n\n"
					"Ensure the selected road piece has a non-zero length and valid lane settings, then try again."));
		}
		else
		{
			UE_LOG(LogTraffic, Warning,
				TEXT("[TrafficEditor][Automation] Overlay produced zero arrow instances; dialog suppressed."));
		}
	}

	ActiveFamilyId = FamilyId;
	ActiveCalibrationRoadActor = RoadActor;
	CalibrationOverlayActor = Overlay;
#endif
}

void UTrafficSystemEditorSubsystem::Editor_BakeCalibrationForActiveFamily()
{
#if WITH_EDITOR
	if (!ActiveFamilyId.IsValid())
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] BakeCalibration: No active family selected."));
		return;
	}

	URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get();
	if (!Registry)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] BakeCalibration: Registry unavailable."));
		return;
	}

	FRoadFamilyInfo* FamilyInfo = Registry->FindFamilyById(ActiveFamilyId);
	if (!FamilyInfo)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] BakeCalibration: Active family not found."));
		return;
	}

	if (!CalibrationOverlayActor.IsValid())
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] BakeCalibration: No active overlay actor."));
		return;
	}

	TArray<AActor*> ActorsBefore;
	GetActorsForFamily(ActiveFamilyId, ActorsBefore);

	const bool bAutomationOrCmdlet = IsRunningCommandlet() || GIsAutomationTesting;

	auto WarnIfMissing = [&]()
	{
		TArray<AActor*> ActorsAfter;
		GetActorsForFamily(ActiveFamilyId, ActorsAfter);

		int32 MissingCount = 0;
		for (AActor* Actor : ActorsBefore)
		{
			if (Actor && !ActorsAfter.Contains(Actor))
			{
				++MissingCount;
			}
		}

		if (MissingCount > 0)
		{
			UE_LOG(LogTraffic, Warning,
				TEXT("[TrafficEditor] Road actors disappeared during calibration bake for family %s. Before=%d After=%d"),
				*ActiveFamilyId.ToString(), ActorsBefore.Num(), ActorsAfter.Num());

			if (!bAutomationOrCmdlet)
			{
				FMessageDialog::Open(
					EAppMsgType::Ok,
					NSLOCTEXT("TrafficEditor", "Traffic_RoadsDisappeared",
						"One or more road actors disappeared during calibration for this family.\n"
						"This is unexpected. Please check your road kit tools and streaming state."));
			}
			else
			{
				UE_LOG(LogTraffic, Warning,
					TEXT("[TrafficEditor][Automation] Road actors disappeared during calibration bake; dialog suppressed."));
			}
		}
	};

	ON_SCOPE_EXIT
	{
		WarnIfMissing();
	};

	FTrafficLaneFamilyCalibration NewCalib;
	NewCalib.NumLanesPerSideForward = FMath::Clamp(CalibrationOverlayActor->NumLanesPerSideForward, 1, 5);
	NewCalib.NumLanesPerSideBackward = FMath::Clamp(CalibrationOverlayActor->NumLanesPerSideBackward, 1, 5);
	NewCalib.LaneWidthCm = FMath::Clamp(CalibrationOverlayActor->LaneWidthCm, 200.f, 500.f);
	NewCalib.CenterlineOffsetCm = CalibrationOverlayActor->CenterlineOffsetCm;

	if (NewCalib.NumLanesPerSideForward != CalibrationOverlayActor->NumLanesPerSideForward ||
		NewCalib.NumLanesPerSideBackward != CalibrationOverlayActor->NumLanesPerSideBackward ||
		!FMath::IsNearlyEqual(NewCalib.LaneWidthCm, CalibrationOverlayActor->LaneWidthCm))
	{
		UE_LOG(LogTraffic, Warning,
			TEXT("[TrafficCalib] Clamped calibration values for safety. Forward=%d Backward=%d Width=%.1f"),
			NewCalib.NumLanesPerSideForward,
			NewCalib.NumLanesPerSideBackward,
			NewCalib.LaneWidthCm);

		if (!bAutomationOrCmdlet)
		{
			FMessageDialog::Open(
				EAppMsgType::Ok,
				NSLOCTEXT("TrafficSystemEditorSubsystem", "Traffic_CalibrationClamped",
					"Calibration values were out of the supported range and were clamped.\n\n"
					"Supported ranges:\n"
					"  - Lanes per side: 1 to 5\n"
					"  - Lane width: 200cm to 500cm"));
		}
		else
		{
			UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor][Automation] Calibration clamped; dialog suppressed."));
		}
	}

	Registry->ApplyCalibration(ActiveFamilyId, NewCalib);

	// Persist calibration into game-visible TrafficRoadFamilySettings so builds use the calibrated layout.
	if (UTrafficRoadFamilySettings* RoadSettings = GetMutableDefault<UTrafficRoadFamilySettings>())
	{
		const FName CalibFamilyName = FamilyInfo->FamilyDefinition.FamilyName.IsNone()
			? FName(*FamilyInfo->DisplayName)
			: FamilyInfo->FamilyDefinition.FamilyName;

		const int32 FoundIndex = RoadSettings->Families.IndexOfByPredicate(
			[CalibFamilyName](const FRoadFamilyDefinition& Def)
			{
				return Def.FamilyName == CalibFamilyName;
			});

		if (RoadSettings->Families.IsValidIndex(FoundIndex))
		{
			FRoadFamilyDefinition& Def = RoadSettings->Families[FoundIndex];
			Def.Forward.NumLanes = NewCalib.NumLanesPerSideForward;
			Def.Backward.NumLanes = NewCalib.NumLanesPerSideBackward;
			Def.Forward.LaneWidthCm = NewCalib.LaneWidthCm;
			Def.Backward.LaneWidthCm = NewCalib.LaneWidthCm;
			Def.Forward.InnerLaneCenterOffsetCm = NewCalib.CenterlineOffsetCm;
			Def.Backward.InnerLaneCenterOffsetCm = NewCalib.CenterlineOffsetCm;

			RoadSettings->SaveConfig();
			UE_LOG(LogTraffic, Log, TEXT("[TrafficCalib] Saved calibrated layout into TrafficRoadFamilySettings for '%s' (Index=%d)."), *CalibFamilyName.ToString(), FoundIndex);
		}
		else
		{
			UE_LOG(LogTraffic, Warning,
				TEXT("[TrafficCalib] Could not find matching family '%s' in TrafficRoadFamilySettings; calibration will not affect builds until you add it."),
				*CalibFamilyName.ToString());
		}
	}

	// Update any existing metadata components to keep display name in sync.
	if (FamilyInfo->FamilyDefinition.FamilyName.IsNone())
	{
		FamilyInfo->FamilyDefinition.FamilyName = FName(*FamilyInfo->DisplayName);
	}

	UWorld* World = GetEditorWorld();
	if (World)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor)
			{
				continue;
			}
			if (UTrafficRoadMetadataComponent* Meta = Actor->FindComponentByClass<UTrafficRoadMetadataComponent>())
			{
				if (Meta->RoadFamilyId == ActiveFamilyId)
				{
					Meta->FamilyName = FamilyInfo->FamilyDefinition.FamilyName;
				}
			}
		}
	}

	UE_LOG(LogTraffic, Log, TEXT("[TrafficCalib] Baked calibration for family %s (Fwd=%d Back=%d Width=%.1fcm Offset=%.1fcm)"),
		*FamilyInfo->DisplayName,
		NewCalib.NumLanesPerSideForward,
		NewCalib.NumLanesPerSideBackward,
		NewCalib.LaneWidthCm,
		NewCalib.CenterlineOffsetCm);
#endif
}

void UTrafficSystemEditorSubsystem::Editor_RestoreCalibrationForFamily(const FGuid& FamilyId)
{
#if WITH_EDITOR
	URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get();
	if (!Registry)
	{
		return;
	}

	if (!Registry->RestoreLastCalibration(FamilyId))
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficCalib] No backup calibration available for family %s."), *FamilyId.ToString());
		return;
	}

	UE_LOG(LogTraffic, Log, TEXT("[TrafficCalib] Restored last calibration for family %s."), *FamilyId.ToString());

	// Refresh overlay if restoring the active family.
	if (ActiveFamilyId == FamilyId && ActiveFamilyId.IsValid())
	{
		Editor_BeginCalibrationForFamily(FamilyId);
	}
#endif
}

bool UTrafficSystemEditorSubsystem::ExtractCalibrationSnippet(AActor* RoadActor, FCalibrationSnippet& OutSnippet)
{
	if (!RoadActor)
	{
		return false;
	}

	USplineComponent* Spline = RoadActor->FindComponentByClass<USplineComponent>();
	if (!Spline || Spline->GetNumberOfSplinePoints() < 2)
	{
		return false;
	}

	const float SplineLength = Spline->GetSplineLength();
	const float TargetSnippetLength = FMath::Clamp(SplineLength * 0.3f, SnippetMinLength, SnippetMaxLength);

	float BestStart = 0.f;
	float BestCurvatureSum = FLT_MAX;

	const float SampleStep = 200.f;
	const int32 NumSamples = FMath::Max(1, FMath::FloorToInt((SplineLength - TargetSnippetLength) / SampleStep));

	for (int32 i = 0; i <= NumSamples; ++i)
	{
		float StartDist = i * SampleStep;
		float EndDist = FMath::Min(StartDist + TargetSnippetLength, SplineLength);

		float CurvatureSum = 0.f;
		const float CurveSampleStep = 100.f;
		int32 CurveSamples = 0;

		for (float D = StartDist; D <= EndDist; D += CurveSampleStep)
		{
			FVector Tangent = Spline->GetTangentAtDistanceAlongSpline(D, ESplineCoordinateSpace::World);
			FVector NextTangent = Spline->GetTangentAtDistanceAlongSpline(
				FMath::Min(D + CurveSampleStep, SplineLength), ESplineCoordinateSpace::World);
			
			float DotProduct = FMath::Clamp(FVector::DotProduct(Tangent.GetSafeNormal(), NextTangent.GetSafeNormal()), -1.f, 1.f);
			float AngleDiff = FMath::Acos(DotProduct);
			CurvatureSum += AngleDiff;
			++CurveSamples;
		}

		if (CurveSamples > 0)
		{
			float AvgCurvature = CurvatureSum / CurveSamples;
			if (AvgCurvature < BestCurvatureSum)
			{
				BestCurvatureSum = AvgCurvature;
				BestStart = StartDist;
			}
		}
	}

	float SnippetStart = BestStart;
	float SnippetEnd = FMath::Min(SnippetStart + TargetSnippetLength, SplineLength);
	OutSnippet.SnippetLength = SnippetEnd - SnippetStart;

	const float PointSpacing = 200.f;
	OutSnippet.SnippetPoints.Empty();
	for (float D = SnippetStart; D <= SnippetEnd; D += PointSpacing)
	{
		OutSnippet.SnippetPoints.Add(Spline->GetLocationAtDistanceAlongSpline(D, ESplineCoordinateSpace::World));
	}

	if (OutSnippet.SnippetPoints.Num() > 0)
	{
		FVector LastPoint = Spline->GetLocationAtDistanceAlongSpline(SnippetEnd, ESplineCoordinateSpace::World);
		if (FVector::DistSquared(OutSnippet.SnippetPoints.Last(), LastPoint) > 100.f)
		{
			OutSnippet.SnippetPoints.Add(LastPoint);
		}
	}

	return OutSnippet.SnippetPoints.Num() >= 2;
}

bool UTrafficSystemEditorSubsystem::CanCalibrateSelectedRoad() const
{
#if WITH_EDITOR
	if (!GEditor)
	{
		return false;
	}

	USelection* SelectedActors = GEditor->GetSelectedActors();
	if (!SelectedActors || SelectedActors->Num() == 0)
	{
		return false;
	}

	AActor* RoadActor = Cast<AActor>(SelectedActors->GetSelectedObject(0));
	if (!RoadActor)
	{
		return false;
	}

	return (RoadActor->FindComponentByClass<USplineComponent>() != nullptr);
#else
	return false;
#endif
}

void UTrafficSystemEditorSubsystem::CalibrateSelectedRoad()
{
#if WITH_EDITOR
    UE_LOG(LogTraffic, Log, TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION));
    UWorld* World = GetEditorWorld();
    if (!World)
    {
        UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] CalibrateSelectedRoad: No editor world."));
        return;
    }

    if (!GEditor)
    {
        UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] CalibrateSelectedRoad: No GEditor."));
        return;
    }

    if (!EnsureDetectedFamilies(TEXT("Calibrate")))
    {
        return;
    }

    USelection* SelectedActors = GEditor->GetSelectedActors();
    if (!SelectedActors || SelectedActors->Num() == 0)
    {
        UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] CalibrateSelectedRoad: No actor selected."));
        return;
    }

    AActor* RoadActor = Cast<AActor>(SelectedActors->GetSelectedObject(0));
    if (!RoadActor)
    {
        UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] CalibrateSelectedRoad: Selected object is not an actor."));
        return;
    }

    // NOTE: DO NOT rely on raw spline points for calibration on CityBLD.
    // CityBLD control splines frequently contain sharp corners (90-degree segments).
    // We must use the provider chain to get the *smoothed* display centerline.

    UTrafficRoadMetadataComponent* Meta = RoadActor->FindComponentByClass<UTrafficRoadMetadataComponent>();
    if (!Meta)
    {
        Meta = NewObject<UTrafficRoadMetadataComponent>(RoadActor);
        Meta->RegisterComponent();
        RoadActor->AddInstanceComponent(Meta);
        UE_LOG(LogTraffic, Log,
            TEXT("[TrafficEditor] CalibrateSelectedRoad: Added metadata component to '%s'."),
            *RoadActor->GetName());
    }

    URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get();
    FRoadFamilyInfo* FamilyInfo = nullptr;

    if (Meta->RoadFamilyId.IsValid())
    {
        FamilyInfo = Registry ? Registry->FindFamilyById(Meta->RoadFamilyId) : nullptr;
    }

    if (!FamilyInfo && Registry)
    {
        FamilyInfo = Registry->FindOrCreateFamilyForClass(RoadActor->GetClass(), nullptr);
    }

    if (!FamilyInfo)
    {
        UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] CalibrateSelectedRoad: Could not resolve family for '%s'."), *RoadActor->GetName());
        return;
    }

    if (FamilyInfo->FamilyDefinition.FamilyName.IsNone())
    {
        FamilyInfo->FamilyDefinition.FamilyName = FName(*FamilyInfo->DisplayName);
    }

    Meta->RoadFamilyId = FamilyInfo->FamilyId;
    Meta->FamilyName = FamilyInfo->FamilyDefinition.FamilyName;

    FRoadFamilyDefinition Family = FamilyInfo->FamilyDefinition;
    if (Family.FamilyName.IsNone())
    {
        Family.FamilyName = FName(*FamilyInfo->DisplayName);
    }

    // === Get smoothed display centerline from provider chain (same logic as Editor_BeginCalibrationForFamily) ===
    TArray<TObjectPtr<UObject>> ProviderObjects;
    TArray<ITrafficRoadGeometryProvider*> Providers;
    UTrafficGeometryProviderFactory::CreateProviderChainForEditorWorld(World, ProviderObjects, Providers);

    TArray<FVector> CenterlinePoints;
    for (ITrafficRoadGeometryProvider* Provider : Providers)
    {
        if (Provider && Provider->GetDisplayCenterlineForActor(RoadActor, CenterlinePoints) && CenterlinePoints.Num() >= 2)
        {
            break;
        }
    }

    if (CenterlinePoints.Num() < 2)
    {
        UE_LOG(LogTraffic, Warning,
            TEXT("[TrafficEditor] CalibrateSelectedRoad: No provider could supply a display centerline for '%s'."),
            *GetNameSafe(RoadActor));
        return;
    }

    // Optional: allow snippet extraction for non-CityBLD spline roads (fast UX); never for CityBLD BP_MeshRoad.
    const bool bIsCityBLD = RoadActor->GetClass()->GetName().Contains(TEXT("BP_MeshRoad"), ESearchCase::IgnoreCase);
    if (!bIsCityBLD)
    {
        FCalibrationSnippet Snippet;
        if (ExtractCalibrationSnippet(RoadActor, Snippet) && Snippet.SnippetPoints.Num() >= 2)
        {
            CenterlinePoints = Snippet.SnippetPoints;
            UE_LOG(LogTraffic, Log,
                TEXT("[TrafficEditor] CalibrateSelectedRoad: Using snippet of %.0fcm with %d points."),
                Snippet.SnippetLength, Snippet.SnippetPoints.Num());
        }
    }

	const UTrafficCityBLDAdapterSettings* AdapterSettings = GetDefault<UTrafficCityBLDAdapterSettings>();
	if (AdapterSettings && AdapterSettings->bDrawCalibrationCenterlineDebug)
	{
		Editor_DrawCenterlineDebug(World, CenterlinePoints, /*bColorByCurvature=*/true);
	}

	if (bIsCityBLD && AdapterSettings && AdapterSettings->bUseZoneGraphLanePolylinesForCalibrationOverlay)
	{
		// Ensure a lane profile is present so ZoneShapes can generate lanes deterministically.
		if (FamilyInfo && !FamilyInfo->FamilyDefinition.VehicleLaneProfile.IsValid() && AdapterSettings->DefaultCityBLDVehicleLaneProfile.IsValid())
		{
			FamilyInfo->FamilyDefinition.VehicleLaneProfile = AdapterSettings->DefaultCityBLDVehicleLaneProfile;
			if (Registry)
			{
				Registry->SaveConfig();
			}
		}

		const FSoftObjectPath LaneProfilePath = (FamilyInfo && FamilyInfo->FamilyDefinition.VehicleLaneProfile.IsValid())
			? FamilyInfo->FamilyDefinition.VehicleLaneProfile
			: AdapterSettings->DefaultCityBLDVehicleLaneProfile;

		CityBLDZoneShapeBuilder::BuildVehicleZoneGraphForCityBLDRoad(World, RoadActor, CenterlinePoints, LaneProfilePath);
	}

    const bool bHasUnderlyingMesh = (RoadActor->FindComponentByClass<UStaticMeshComponent>() != nullptr);

	if (CalibrationOverlayActor.IsValid() && CalibrationOverlayActor->IsValidLowLevel())
	{
		CalibrationOverlayActor->Destroy();
		CalibrationOverlayActor.Reset();
	}

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    CalibrationOverlayActor = World->SpawnActor<ALaneCalibrationOverlayActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);

	if (CalibrationOverlayActor.IsValid())
	{
		TagAsRoadLab(CalibrationOverlayActor.Get());

		FTrafficLaneFamilyCalibration Calib;
		Calib.NumLanesPerSideForward = Family.Forward.NumLanes;
		Calib.NumLanesPerSideBackward = Family.Backward.NumLanes;
		Calib.LaneWidthCm = Family.Forward.LaneWidthCm;
		Calib.CenterlineOffsetCm = Family.Forward.InnerLaneCenterOffsetCm;

		bool bBuiltOverlayFromZoneGraph = false;
		if (AdapterSettings &&
			AdapterSettings->bUseZoneGraphLanePolylinesForCalibrationOverlay &&
			bIsCityBLD)
		{
			TArray<TArray<FVector>> LanePolylines;
			if (ZoneGraphLaneOverlayUtils::GetZoneGraphLanePolylinesNearActor(World, RoadActor, LanePolylines))
			{
				CalibrationOverlayActor->BuildFromLanePolylines(LanePolylines, RoadActor->GetActorTransform());
				bBuiltOverlayFromZoneGraph = true;
			}
		}

		if (!bBuiltOverlayFromZoneGraph)
		{
			CalibrationOverlayActor->BuildFromCenterline(CenterlinePoints, Calib, RoadActor->GetActorTransform());
		}
		FocusCameraOnActor(CalibrationOverlayActor.Get());
		ActiveFamilyId = FamilyInfo->FamilyId;
		ActiveCalibrationRoadActor = RoadActor;
	}

    const int32 TotalLanes = Family.Forward.NumLanes + Family.Backward.NumLanes;
    UE_LOG(LogTraffic, Log,
        TEXT("[TrafficEditor] CalibrateSelectedRoad: Actor '%s' calibrated with family '%s'. "
             "Lanes: %d (Fwd:%d, Bwd:%d), Width: %.0fcm. Overlay applied."),
        *RoadActor->GetName(), *Family.FamilyName.ToString(),
        TotalLanes, Family.Forward.NumLanes, Family.Backward.NumLanes,
        Family.Forward.LaneWidthCm);
#endif
}
