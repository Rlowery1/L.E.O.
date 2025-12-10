#include "TrafficSystemEditorSubsystem.h"
#include "TrafficCalibrationSubsystem.h"
#include "TrafficRuntimeModule.h"
#include "TrafficRoadFamilySettings.h"
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
#include "FileHelpers.h"
#include "HAL/FileManager.h"

#include "Editor.h"
#include "Engine/World.h"
#include "Engine/Selection.h"
#include "EngineUtils.h"
#include "TrafficGeometryProviderFactory.h"
#include "TrafficGeometryProvider.h"

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

	UE_LOG(LogTraffic, Log, TEXT("[TrafficEditor] ResetRoadLab: Destroyed %d AAA_RoadLab actors."), DestroyedCount);
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

	if (!HasAnyCalibratedFamilies())
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] DoCars: No calibrated families. Calibrate at least one family first."));
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

	UClass* RoadClass = FamilyInfo->RoadClassPath.TryLoadClass<AActor>();
	if (!RoadClass)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] BeginCalibrationForFamily: Failed to load road class for %s"), *FamilyInfo->DisplayName);
		return;
	}

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
	Overlay->ApplyCalibrationSettings(
		FamilyInfo->FamilyDefinition.Forward.NumLanes,
		FamilyInfo->FamilyDefinition.Backward.NumLanes,
		FamilyInfo->FamilyDefinition.Forward.LaneWidthCm,
		FamilyInfo->FamilyDefinition.Forward.InnerLaneCenterOffsetCm);

	Overlay->BuildForRoad(CenterlinePoints, FamilyInfo->FamilyDefinition, RoadActor->GetActorTransform(), bHasUnderlyingMesh);
	FocusCameraOnActor(Overlay);

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

	FTrafficLaneFamilyCalibration NewCalib;
	NewCalib.NumLanesPerSideForward = CalibrationOverlayActor->NumLanesPerSideForward;
	NewCalib.NumLanesPerSideBackward = CalibrationOverlayActor->NumLanesPerSideBackward;
	NewCalib.LaneWidthCm = CalibrationOverlayActor->LaneWidthCm;
	NewCalib.CenterlineOffsetCm = CalibrationOverlayActor->CenterlineOffsetCm;

	Registry->ApplyCalibration(ActiveFamilyId, NewCalib);

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

    USplineComponent* Spline = RoadActor->FindComponentByClass<USplineComponent>();
    if (!Spline)
    {
        UE_LOG(LogTraffic, Warning,
            TEXT("[TrafficEditor] CalibrateSelectedRoad: Actor '%s' has no USplineComponent."),
            *RoadActor->GetName());
        return;
    }

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

    FCalibrationSnippet Snippet;
    TArray<FVector> CenterlinePoints;

    if (ExtractCalibrationSnippet(RoadActor, Snippet) && Snippet.SnippetPoints.Num() >= 2)
    {
        CenterlinePoints = Snippet.SnippetPoints;
        UE_LOG(LogTraffic, Log,
            TEXT("[TrafficEditor] CalibrateSelectedRoad: Using snippet of %.0fcm with %d points."),
            Snippet.SnippetLength, Snippet.SnippetPoints.Num());
    }
    else
    {
        const int32 NumSplinePoints = Spline->GetNumberOfSplinePoints();
        for (int32 i = 0; i < NumSplinePoints; ++i)
        {
            CenterlinePoints.Add(Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World));
        }
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
		CalibrationOverlayActor->BuildForRoad(CenterlinePoints, Family, RoadActor->GetActorTransform(), bHasUnderlyingMesh);
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

