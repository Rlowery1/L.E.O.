#include "TrafficSystemEditorSubsystem.h"
#include "TrafficCalibrationSubsystem.h"
#include "TrafficRuntimeModule.h"
#include "TrafficRoadFamilySettings.h"
#include "TrafficLaneCalibration.h"
#include "RoadFamilyRegistry.h"
#include "TrafficSystemController.h"
#include "TrafficNetworkAsset.h"
#include "TrafficVehicleManager.h"
#include "TrafficVehicleBase.h"
#include "TrafficVehicleAdapter.h"
#include "TrafficCityBLDAdapterSettings.h"
#include "TrafficRoadMetadataComponent.h"
#include "TrafficVisualSettings.h"
#include "LaneCalibrationOverlayActor.h"
#include "RoadLanePreviewActor.h"
#include "TrafficLaneEndpointMarkerActor.h"
#include "TrafficRouting.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "GameFramework/PlayerStart.h"
#include "CollisionQueryParams.h"
#include "UObject/SoftObjectPath.h"
#include "Misc/ScopeExit.h"
#include "ScopedTransaction.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "Misc/MessageDialog.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "HAL/IConsoleManager.h"
#include "Algo/Count.h"

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
#include "Algo/Reverse.h"

static TAutoConsoleVariable<int32> CVarTrafficDrawAllIntersectionDebug(
	TEXT("aaa.Traffic.Debug.DrawAllIntersectionDebug"),
	0,
	TEXT("0=draw closest/selected intersection only; 1=draw all intersections/movements."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficDebugIntersectionId(
	TEXT("aaa.Traffic.Debug.IntersectionId"),
	-1,
	TEXT("If >=0, draws this intersection id (overrides closest/selected)."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficEditorVehiclesPerLane(
	TEXT("aaa.Traffic.EditorTest.VehiclesPerLane"),
	1,
	TEXT("Vehicles per lane to spawn when using BUILD + CARS (Editor Test). Default: 1."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficEditorVehicleSpeedCmPerSec(
	TEXT("aaa.Traffic.EditorTest.SpeedCmPerSec"),
	800.f,
	TEXT("Speed (cm/s) for vehicles spawned by BUILD + CARS (Editor Test). Default: 800."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficCityBLDAutoTagCenterlineSpline(
	TEXT("aaa.Traffic.CityBLD.AutoTagCenterlineSpline"),
	1,
	TEXT("If non-zero, PREPARE MAP will auto-tag the chosen CityBLD centerline spline component with the configured RoadSplineTag (e.g. CityBLD_Centerline) when missing."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficCityBLDAutoDetectRampDirection(
	TEXT("aaa.Traffic.CityBLD.AutoDetectRampDirection"),
	1,
	TEXT("If non-zero, PREPARE MAP will try to auto-detect on- vs off-ramps and set bReverseCenterlineDirection on their metadata."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficCityBLDRampDetectMaxFreewayDistanceCm(
	TEXT("aaa.Traffic.CityBLD.RampDetectMaxFreewayDistanceCm"),
	1500.f,
	TEXT("Max distance (cm) from a ramp endpoint to a freeway/highway centerline to consider it a merge/diverge endpoint. Default: 1500."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficCityBLDSplitRampFamiliesByRole(
	TEXT("aaa.Traffic.CityBLD.SplitRampFamiliesByRole"),
	1,
	TEXT("If non-zero, PREPARE MAP will split generic CityBLD ramp families into separate '(On)' and '(Off)' families based on which end is nearest the freeway and the ramp's current driving direction (bReverseCenterlineDirection)."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficEditorAutoPrepareOnActions(
	TEXT("aaa.Traffic.Editor.AutoPrepareOnActions"),
	1,
	TEXT("If non-zero, BEGIN CALIBRATION / BUILD / FLIP will auto-run PREPARE MAP when roads changed (Undo/Redo, road actor add/remove)."),
	ECVF_Default);

const FName UTrafficSystemEditorSubsystem::RoadLabTag = FName(TEXT("AAA_RoadLab"));

static bool IsLikelyRoadActorForPrepareDirty(AActor* Actor)
{
	if (!Actor)
	{
		return false;
	}

	// Ignore AAA helpers.
	static const FName RoadLabTagLocal(TEXT("AAA_RoadLab"));
	if (Actor->Tags.Contains(RoadLabTagLocal))
	{
		return false;
	}

	// If the actor already has traffic metadata, treat it as a road relevant to PREPARE.
	if (Actor->FindComponentByClass<UTrafficRoadMetadataComponent>())
	{
		return true;
	}

	// Must have a spline to be road-like.
	if (!Actor->FindComponentByClass<USplineComponent>())
	{
		return false;
	}

	const UTrafficCityBLDAdapterSettings* AdapterSettings = GetDefault<UTrafficCityBLDAdapterSettings>();
	if (AdapterSettings && !AdapterSettings->RoadActorTag.IsNone() && Actor->ActorHasTag(AdapterSettings->RoadActorTag))
	{
		return true;
	}

	const FString ClassName = Actor->GetClass() ? Actor->GetClass()->GetName() : FString();

	// CityBLD special cases.
	if (ClassName.Contains(TEXT("BP_MeshRoad"), ESearchCase::IgnoreCase) ||
		ClassName.Contains(TEXT("BP_ModularRoad"), ESearchCase::IgnoreCase))
	{
		return true;
	}

	// Generic "road-ish" heuristics to avoid flagging arbitrary spline actors.
	if (ClassName.Contains(TEXT("Road"), ESearchCase::IgnoreCase) ||
		ClassName.Contains(TEXT("Street"), ESearchCase::IgnoreCase) ||
		ClassName.Contains(TEXT("Highway"), ESearchCase::IgnoreCase) ||
		ClassName.Contains(TEXT("Lane"), ESearchCase::IgnoreCase))
	{
		return true;
	}

#if WITH_EDITOR
	// Editor-only label hint.
	const FString Label = Actor->GetActorLabel();
	if (Label.Contains(TEXT("Road"), ESearchCase::IgnoreCase) ||
		Label.Contains(TEXT("Street"), ESearchCase::IgnoreCase) ||
		Label.Contains(TEXT("Highway"), ESearchCase::IgnoreCase))
	{
		return true;
	}
#endif

	return false;
}

void UTrafficSystemEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CalibrationOverlayActor = nullptr;
	ActiveCalibrationRoadActor = nullptr;
	ActiveFamilyId.Invalidate();
	bPrepareDirty = false;
	PrepareDirtyReason = FText::GetEmpty();

#if WITH_EDITOR
	// Mark PREPARE as dirty after Undo/Redo or when spline-road actors are added/removed.
	// CityBLD frequently re-creates BP_ModularRoad actors on Undo, which can drop metadata components.
	if (!PostUndoRedoHandle.IsValid())
	{
		// UE delegate signature is void(); (no params).
		PostUndoRedoHandle = FEditorDelegates::PostUndoRedo.AddLambda([this]()
		{
			MarkPrepareDirty(NSLOCTEXT("TrafficEditor", "PrepareDirty_UndoRedo", "Roads changed (Undo/Redo). Run PREPARE MAP."));
		});
	}

	if (GEngine)
	{
		if (!ActorAddedHandle.IsValid())
		{
			ActorAddedHandle = GEngine->OnLevelActorAdded().AddLambda([this](AActor* Actor)
			{
				if (!Actor || Actor->Tags.Contains(RoadLabTag))
				{
					return;
				}

				// Avoid nuisance warnings from unrelated spline actors (cables, camera rails, etc).
				if (IsLikelyRoadActorForPrepareDirty(Actor))
				{
					MarkPrepareDirty(NSLOCTEXT("TrafficEditor", "PrepareDirty_ActorAdded", "Road actor added/edited. Run PREPARE MAP."));
				}
			});
		}
		if (!ActorDeletedHandle.IsValid())
		{
			ActorDeletedHandle = GEngine->OnLevelActorDeleted().AddLambda([this](AActor* Actor)
			{
				if (!Actor || Actor->Tags.Contains(RoadLabTag))
				{
					return;
				}

				if (IsLikelyRoadActorForPrepareDirty(Actor))
				{
					MarkPrepareDirty(NSLOCTEXT("TrafficEditor", "PrepareDirty_ActorDeleted", "Road actor removed. Run PREPARE MAP."));
				}
			});
		}
	}
#endif
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

#if WITH_EDITOR
	if (PostUndoRedoHandle.IsValid())
	{
		FEditorDelegates::PostUndoRedo.Remove(PostUndoRedoHandle);
		PostUndoRedoHandle.Reset();
	}
	if (GEngine)
	{
		if (ActorAddedHandle.IsValid())
		{
			GEngine->OnLevelActorAdded().Remove(ActorAddedHandle);
			ActorAddedHandle.Reset();
		}
		if (ActorDeletedHandle.IsValid())
		{
			GEngine->OnLevelActorDeleted().Remove(ActorDeletedHandle);
			ActorDeletedHandle.Reset();
		}
	}
#endif

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

void UTrafficSystemEditorSubsystem::MarkPrepareDirty(const FText& Reason)
{
#if WITH_EDITOR
	bPrepareDirty = true;
	PrepareDirtyReason = Reason;
#endif
}

void UTrafficSystemEditorSubsystem::ClearPrepareDirty()
{
#if WITH_EDITOR
	bPrepareDirty = false;
	PrepareDirtyReason = FText::GetEmpty();
#endif
}

bool UTrafficSystemEditorSubsystem::EnsurePreparedForAction(const TCHAR* Context)
{
#if WITH_EDITOR
	if (!bPrepareDirty)
	{
		return true;
	}

	const bool bAuto = (CVarTrafficEditorAutoPrepareOnActions.GetValueOnGameThread() != 0);
	if (!bAuto)
	{
		const bool bAutomationOrCmdlet = IsRunningCommandlet() || GIsAutomationTesting;
		if (!bAutomationOrCmdlet)
		{
			FMessageDialog::Open(
				EAppMsgType::Ok,
				FText::Format(
					NSLOCTEXT("TrafficEditor", "PrepareDirty_Blocking",
						"{0}\n\n"
						"Run PREPARE MAP before using this action to avoid stale/missing road metadata."),
					PrepareDirtyReason.IsEmpty()
						? NSLOCTEXT("TrafficEditor", "PrepareDirty_Generic", "Roads changed. Run PREPARE MAP.")
						: PrepareDirtyReason));
		}
		return false;
	}

	UE_LOG(LogTraffic, Log, TEXT("[TrafficEditor] AutoPrepareOnActions: PREPARE MAP before '%s' (dirty reason: %s)"),
		Context ? Context : TEXT("Action"),
		*PrepareDirtyReason.ToString());

	Editor_PrepareMapForTraffic();
	return true;
#else
	return true;
#endif
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

	int32 Count = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (UTrafficRoadMetadataComponent* Meta = It->FindComponentByClass<UTrafficRoadMetadataComponent>())
		{
			if (Meta->RoadFamilyId == FamilyId && Meta->bIncludeInTraffic)
			{
				++Count;
			}
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

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (UTrafficRoadMetadataComponent* Meta = It->FindComponentByClass<UTrafficRoadMetadataComponent>())
		{
			if (Meta->RoadFamilyId == FamilyId && Meta->bIncludeInTraffic)
			{
				OutActors.Add(*It);
			}
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

#if WITH_EDITOR
	if (GEditor)
	{
		// Avoid "SelectActor invalid flags" warnings when destroying selected preview actors (e.g. calibration overlay).
		GEditor->SelectNone(/*NoteSelectionChange=*/false, /*DeselectBSPSurfs=*/true, /*WarnAboutManyActors=*/false);
	}
#endif

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
		static const FName TrafficSpawnedVehicleTag(TEXT("AAA_TrafficVehicle"));
		const bool bIsSpawnedTrafficVehicleTagged = Actor->Tags.Contains(TrafficSpawnedVehicleTag);
		const bool bIsPreview = Actor->IsA(ARoadLanePreviewActor::StaticClass());
		const bool bIsOverlay = Actor->IsA(ALaneCalibrationOverlayActor::StaticClass());
		const bool bIsVehicle = Actor->IsA(ATrafficVehicleBase::StaticClass());
		const bool bIsVehicleAdapter = Actor->IsA(ATrafficVehicleAdapter::StaticClass());
		const bool bIsVehicleMgr = Actor->IsA(ATrafficVehicleManager::StaticClass());
		const bool bIsController = Actor->IsA(ATrafficSystemController::StaticClass());

		const bool bIsUserRoadTagged = bIncludeTaggedUserRoads && (!RoadTagFromSettings.IsNone() && Actor->ActorHasTag(RoadTagFromSettings));

		if (bIsAAARoadLabTagged || bIsSpawnedTrafficVehicleTagged || bIsPreview || bIsOverlay || bIsVehicle || bIsVehicleAdapter || bIsVehicleMgr || bIsController || bIsUserRoadTagged)
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

	UTrafficRoadFamilySettings* RoadSettings = GetMutableDefault<UTrafficRoadFamilySettings>();
	bool bRoadSettingsModified = false;
	static const FSoftObjectPath DefaultVehicleProfilePath(TEXT("/AAAtrafficSystem/ZoneProfiles/CityBLDUrbanTwoLane.CityBLDUrbanTwoLane"));
	static const FSoftObjectPath DefaultFootpathProfilePath(TEXT("/AAAtrafficSystem/ZoneProfiles/CityBLDFootpath.CityBLDFootpath"));

	const UTrafficCityBLDAdapterSettings* AdapterSettings = GetDefault<UTrafficCityBLDAdapterSettings>();
	const FName RoadTagFromSettings = AdapterSettings ? AdapterSettings->RoadActorTag : NAME_None;

	auto IsCityBLDRoadCandidate = [&](const AActor* Actor) -> bool
	{
		if (!Actor)
		{
			return false;
		}

		if (AdapterSettings && !AdapterSettings->RoadActorTag.IsNone() && Actor->ActorHasTag(AdapterSettings->RoadActorTag))
		{
			return true;
		}

		const FString ClassName = Actor->GetClass()->GetName();
		if (ClassName.Contains(TEXT("BP_MeshRoad"), ESearchCase::IgnoreCase))
		{
			return true;
		}

		if (AdapterSettings)
		{
			for (const FString& Contains : AdapterSettings->RoadClassNameContains)
			{
				if (!Contains.IsEmpty() && ClassName.Contains(Contains, ESearchCase::IgnoreCase))
				{
					return true;
				}
			}
		}

		return false;
	};

	// If this level contains CityBLD BP_MeshRoad actors, be stricter about what counts as a "road"
	// to avoid picking up sidewalk/support spline actors that confuse users.
	bool bHasCityBLDRoads = false;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (AActor* Actor = *It)
		{
			if (IsCityBLDRoadCandidate(Actor))
			{
				bHasCityBLDRoads = true;
				break;
			}
		}
	}

	int32 ActorsFound = 0;
	int32 FamiliesCreated = 0;
	int32 MetadataAttached = 0;
	int32 CityBLDSplineTagsAdded = 0;
	int32 CityBLDRampAutoDirectionApplied = 0;
	int32 CityBLDRampAutoDirectionLocked = 0;
	int32 CityBLDRampAutoDirectionUnknown = 0;

	TUniquePtr<FScopedTransaction> AutoTagTransaction;

	auto EnsureCityBLDCenterlineSplineTag = [&](AActor* Actor)
	{
		if (!Actor)
		{
			return;
		}

		if (!IsCityBLDRoadCandidate(Actor))
		{
			return;
		}

		if (CVarTrafficCityBLDAutoTagCenterlineSpline.GetValueOnGameThread() == 0)
		{
			return;
		}

		const UTrafficCityBLDAdapterSettings* AdapterSettings = GetDefault<UTrafficCityBLDAdapterSettings>();
		if (!AdapterSettings || AdapterSettings->RoadSplineTag.IsNone())
		{
			return;
		}

		TArray<USplineComponent*> Splines;
		Actor->GetComponents<USplineComponent>(Splines);
		if (Splines.Num() <= 1)
		{
			return;
		}

		for (USplineComponent* Spline : Splines)
		{
			if (Spline && Spline->ComponentHasTag(AdapterSettings->RoadSplineTag))
			{
				return; // already tagged
			}
		}

		USplineComponent* Chosen = nullptr;

		// Mirror the selection heuristics used by the CityBLD geometry provider to avoid behavior changes.
		for (USplineComponent* Spline : Splines)
		{
			if (Spline && Spline->GetName().Contains(TEXT("Centerline"), ESearchCase::IgnoreCase))
			{
				Chosen = Spline;
				break;
			}
		}
		if (!Chosen)
		{
			for (USplineComponent* Spline : Splines)
			{
				if (Spline && Spline->GetName().Equals(TEXT("Spline"), ESearchCase::IgnoreCase))
				{
					Chosen = Spline;
					break;
				}
			}
		}
		if (!Chosen)
		{
			for (USplineComponent* Spline : Splines)
			{
				if (Spline && Spline->GetName().Contains(TEXT("Control"), ESearchCase::IgnoreCase))
				{
					Chosen = Spline;
					break;
				}
			}
		}
		if (!Chosen)
		{
			float BestLen = -1.f;
			for (USplineComponent* Spline : Splines)
			{
				if (!Spline)
				{
					continue;
				}
				const float Len = Spline->GetSplineLength();
				if (Len > BestLen)
				{
					BestLen = Len;
					Chosen = Spline;
				}
			}
		}

		if (!Chosen)
		{
			return;
		}

		if (!AutoTagTransaction)
		{
			AutoTagTransaction = MakeUnique<FScopedTransaction>(
				NSLOCTEXT("TrafficEditor", "AutoTagCityBLDCenterline", "AAA Traffic: Auto-tag CityBLD centerline splines"));
		}

		Chosen->Modify();
		Chosen->ComponentTags.AddUnique(AdapterSettings->RoadSplineTag);
		++CityBLDSplineTagsAdded;

		UE_LOG(LogTraffic, Log,
			TEXT("[TrafficPrep] Auto-tagged %s.%s with '%s' (CityBLD centerline spline)."),
			*Actor->GetName(),
			*Chosen->GetName(),
			*AdapterSettings->RoadSplineTag.ToString());
	};

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

		// If CityBLD roads exist, only include CityBLD road actors (or explicitly tagged/included roads).
		if (bHasCityBLDRoads)
		{
			if (!RoadTagFromSettings.IsNone() && Actor->ActorHasTag(RoadTagFromSettings))
			{
				// explicitly tagged as road
			}
			else if (UTrafficRoadMetadataComponent* ExistingMeta = Actor->FindComponentByClass<UTrafficRoadMetadataComponent>())
			{
				if (!ExistingMeta->bIncludeInTraffic)
				{
					continue;
				}
			}
			else
			{
				// Only accept classes that look like CityBLD road actors.
				if (!IsCityBLDRoadCandidate(Actor))
				{
					continue;
				}
			}
		}

		++ActorsFound;
		UE_LOG(LogTraffic, Log, TEXT("[TrafficPrep] Found road actor: %s"), *ClassName);

		EnsureCityBLDCenterlineSplineTag(Actor);

		bool bCreated = false;
		FRoadFamilyInfo* FamilyInfo = Registry->FindOrCreateFamilyForActor(Actor, &bCreated);
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

			// Ensure the runtime build settings contain a matching family entry by name.
			// The runtime traffic build uses UTrafficRoadFamilySettings, while the editor uses URoadFamilyRegistry.
			if (RoadSettings && !Meta->FamilyName.IsNone())
			{
				const FRoadFamilyDefinition* Existing = RoadSettings->FindFamilyByName(Meta->FamilyName);
				if (!Existing)
				{
					FRoadFamilyDefinition NewDef = FamilyInfo->FamilyDefinition;
					NewDef.FamilyName = Meta->FamilyName;
					if (NewDef.VehicleLaneProfile.IsNull())
					{
						NewDef.VehicleLaneProfile = DefaultVehicleProfilePath;
					}
					if (NewDef.FootpathLaneProfile.IsNull())
					{
						NewDef.FootpathLaneProfile = DefaultFootpathProfilePath;
					}

					RoadSettings->Families.Add(NewDef);
					bRoadSettingsModified = true;
					UE_LOG(LogTraffic, Log,
						TEXT("[TrafficPrep] Added family '%s' to TrafficRoadFamilySettings (Forward=%d Backward=%d Width=%.1f Offset=%.1f)."),
						*NewDef.FamilyName.ToString(),
						NewDef.Forward.NumLanes,
						NewDef.Backward.NumLanes,
						NewDef.Forward.LaneWidthCm,
						NewDef.Forward.InnerLaneCenterOffsetCm);
				}
			}
		}
	}

	// CityBLD ramps: optional auto-detect direction + optional family split into (On)/(Off).
	const bool bCityBLDAutoDetectRampDirection = (CVarTrafficCityBLDAutoDetectRampDirection.GetValueOnGameThread() != 0);
	const bool bCityBLDSplitRampFamiliesByRole = (CVarTrafficCityBLDSplitRampFamiliesByRole.GetValueOnGameThread() != 0);
	if (bHasCityBLDRoads && (bCityBLDAutoDetectRampDirection || bCityBLDSplitRampFamiliesByRole))
	{
		// Collect "highway-like" centerlines once.
		struct FPolylineCache
		{
			TArray<FVector> Points;
		};

		auto IsHighwayFamilyName = [](const FName& Name) -> bool
		{
			const FString S = Name.ToString();
			return S.Contains(TEXT("Freeway"), ESearchCase::IgnoreCase) ||
				S.Contains(TEXT("Highway"), ESearchCase::IgnoreCase);
		};

		auto IsRampFamilyName = [](const FName& Name) -> bool
		{
			const FString S = Name.ToString();
			return S.Contains(TEXT("Ramp"), ESearchCase::IgnoreCase);
		};

		enum class ERampHint : uint8 { Unknown, OnRamp, OffRamp };

		auto DetectRampHintFromText = [](const FString& InText) -> ERampHint
		{
			const FString T = InText;
			if (T.Contains(TEXT("Off Ramp"), ESearchCase::IgnoreCase) ||
				T.Contains(TEXT("Off-Ramp"), ESearchCase::IgnoreCase) ||
				T.Contains(TEXT("Exit"), ESearchCase::IgnoreCase) ||
				T.Contains(TEXT("Diverge"), ESearchCase::IgnoreCase) ||
				T.Contains(TEXT("Out Ramp"), ESearchCase::IgnoreCase) ||
				T.Contains(TEXT("Out-Ramp"), ESearchCase::IgnoreCase))
			{
				return ERampHint::OffRamp;
			}

			if (T.Contains(TEXT("On Ramp"), ESearchCase::IgnoreCase) ||
				T.Contains(TEXT("On-Ramp"), ESearchCase::IgnoreCase) ||
				T.Contains(TEXT("Entry"), ESearchCase::IgnoreCase) ||
				T.Contains(TEXT("Merge"), ESearchCase::IgnoreCase) ||
				T.Contains(TEXT("In Ramp"), ESearchCase::IgnoreCase) ||
				T.Contains(TEXT("In-Ramp"), ESearchCase::IgnoreCase))
			{
				return ERampHint::OnRamp;
			}

			return ERampHint::Unknown;
		};

		auto DetectRampHintFromActor = [&](AActor* Actor, const FName& FamilyName) -> ERampHint
		{
			if (!Actor)
			{
				return ERampHint::Unknown;
			}

			ERampHint Hint = DetectRampHintFromText(FamilyName.ToString());
			if (Hint != ERampHint::Unknown)
			{
				return Hint;
			}

			Hint = DetectRampHintFromText(Actor->GetActorLabel());
			if (Hint != ERampHint::Unknown)
			{
				return Hint;
			}

			// Check actor tags as a cheap hint source.
			for (const FName& Tag : Actor->Tags)
			{
				Hint = DetectRampHintFromText(Tag.ToString());
				if (Hint != ERampHint::Unknown)
				{
					return Hint;
				}
			}

			// Scan a small subset of actor/component properties for string-like values mentioning ramps/merge/exit.
			static const TArray<FString> InterestingPropNameNeedles = {
				TEXT("Ramp"),
				TEXT("Merge"),
				TEXT("Exit"),
				TEXT("Entry"),
				TEXT("Style"),
				TEXT("Preset"),
				TEXT("Module"),
			};

			auto ScanObjectForHint = [&](UObject* Obj) -> ERampHint
			{
				if (!Obj)
				{
					return ERampHint::Unknown;
				}

				for (TFieldIterator<FProperty> ItP(Obj->GetClass()); ItP; ++ItP)
				{
					const FProperty* Prop = *ItP;
					if (!Prop)
					{
						continue;
					}

					const FString PropName = Prop->GetName();
					bool bInteresting = false;
					for (const FString& Needle : InterestingPropNameNeedles)
					{
						if (PropName.Contains(Needle, ESearchCase::IgnoreCase))
						{
							bInteresting = true;
							break;
						}
					}
					if (!bInteresting)
					{
						continue;
					}

					const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Obj);
					FString Value;
					if (const FStrProperty* StrProp = CastField<FStrProperty>(Prop))
					{
						Value = StrProp->GetPropertyValue(ValuePtr);
					}
					else if (const FNameProperty* NameProp = CastField<FNameProperty>(Prop))
					{
						const FName NameValue = NameProp->GetPropertyValue(ValuePtr);
						Value = NameValue.ToString();
					}
					else if (const FTextProperty* TextProp = CastField<FTextProperty>(Prop))
					{
						Value = TextProp->GetPropertyValue(ValuePtr).ToString();
					}
					else if (const FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(Prop))
					{
						if (UObject* Ref = ObjProp->GetObjectPropertyValue(ValuePtr))
						{
							Value = Ref->GetName();
							ERampHint Inner = DetectRampHintFromText(Value);
							if (Inner != ERampHint::Unknown)
							{
								return Inner;
							}
						}
					}

					Hint = DetectRampHintFromText(Value);
					if (Hint != ERampHint::Unknown)
					{
						return Hint;
					}
				}

				return ERampHint::Unknown;
			};

			Hint = ScanObjectForHint(Actor);
			if (Hint != ERampHint::Unknown)
			{
				return Hint;
			}

			TArray<UActorComponent*> Comps;
			Actor->GetComponents(Comps);
			for (UActorComponent* C : Comps)
			{
				Hint = ScanObjectForHint(C);
				if (Hint != ERampHint::Unknown)
				{
					return Hint;
				}
			}

			return ERampHint::Unknown;
		};

		auto ClosestPointDistanceSqToPolyline = [](const TArray<FVector>& Points, const FVector& Query) -> float
		{
			if (Points.Num() < 2)
			{
				return TNumericLimits<float>::Max();
			}

			float Best = TNumericLimits<float>::Max();
			for (int32 i = 0; i < Points.Num() - 1; ++i)
			{
				const FVector A = Points[i];
				const FVector B = Points[i + 1];
				const FVector AB = B - A;
				const float Den = AB.SizeSquared();
				if (Den <= KINDA_SMALL_NUMBER)
				{
					continue;
				}

				const float T = FMath::Clamp(FVector::DotProduct(Query - A, AB) / Den, 0.f, 1.f);
				const FVector Closest = A + AB * T;
				Best = FMath::Min(Best, FVector::DistSquared(Query, Closest));
			}
			return Best;
		};

		TArray<AActor*> HighwayActors;
		TArray<AActor*> RampActors;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor)
			{
				continue;
			}
			UTrafficRoadMetadataComponent* Meta = Actor->FindComponentByClass<UTrafficRoadMetadataComponent>();
			if (!Meta || !Meta->bIncludeInTraffic || Meta->FamilyName.IsNone())
			{
				continue;
			}

			if (IsRampFamilyName(Meta->FamilyName))
			{
				RampActors.Add(Actor);
			}
			else if (IsHighwayFamilyName(Meta->FamilyName))
			{
				HighwayActors.Add(Actor);
			}
		}

		if (RampActors.Num() > 0 && HighwayActors.Num() > 0)
		{
			int32 CityBLDRampRoleFamiliesCreated = 0;
			int32 CityBLDRampRoleActorsRemapped = 0;
			int32 CityBLDRampsClassifiedOn = 0;
			int32 CityBLDRampsClassifiedOff = 0;

			TArray<TObjectPtr<UObject>> ProviderObjects;
			TArray<ITrafficRoadGeometryProvider*> Providers;
			UTrafficGeometryProviderFactory::CreateProviderChainForEditorWorld(World, ProviderObjects, Providers);

			TArray<TArray<FVector>> HighwayPolylines;
			for (AActor* Highway : HighwayActors)
			{
				TArray<FVector> Centerline;
				for (ITrafficRoadGeometryProvider* Provider : Providers)
				{
					if (Provider && Provider->GetDisplayCenterlineForActor(Highway, Centerline) && Centerline.Num() >= 2)
					{
						break;
					}
				}
				if (Centerline.Num() >= 2)
				{
					HighwayPolylines.Add(MoveTemp(Centerline));
				}
			}

			const float MaxDistCm = FMath::Max(0.f, CVarTrafficCityBLDRampDetectMaxFreewayDistanceCm.GetValueOnGameThread());
			const float MaxDistSq = FMath::Square(MaxDistCm);

			for (AActor* Ramp : RampActors)
			{
				if (!Ramp)
				{
					continue;
				}

				UTrafficRoadMetadataComponent* Meta = Ramp->FindComponentByClass<UTrafficRoadMetadataComponent>();
				if (!Meta || !Meta->bIncludeInTraffic || Meta->FamilyName.IsNone())
				{
					continue;
				}

				const bool bLocked = Meta->bLockReverseDirection;
				if (bLocked)
				{
					++CityBLDRampAutoDirectionLocked;
				}

				const ERampHint Hint = bCityBLDAutoDetectRampDirection ? DetectRampHintFromActor(Ramp, Meta->FamilyName) : ERampHint::Unknown;

				TArray<FVector> RampCenterline;
				for (ITrafficRoadGeometryProvider* Provider : Providers)
				{
					if (Provider && Provider->GetDisplayCenterlineForActor(Ramp, RampCenterline) && RampCenterline.Num() >= 2)
					{
						break;
					}
				}

				if (RampCenterline.Num() < 2 || HighwayPolylines.Num() == 0)
				{
					++CityBLDRampAutoDirectionUnknown;
					continue;
				}

				// IMPORTANT: providers may already reverse the returned centerline based on metadata
				// (bReverseCenterlineDirection). For detecting which physical end is near the freeway and for
				// classifying On vs Off, we want the *unreversed* (raw) endpoints.
				TArray<FVector> RampCenterlineRaw = RampCenterline;
				if (Meta->bReverseCenterlineDirection)
				{
					Algo::Reverse(RampCenterlineRaw);
				}

				const FVector StartRaw = RampCenterlineRaw[0];
				const FVector EndRaw = RampCenterlineRaw.Last();

				float BestStartSq = TNumericLimits<float>::Max();
				float BestEndSq = TNumericLimits<float>::Max();
				for (const TArray<FVector>& Hwy : HighwayPolylines)
				{
					BestStartSq = FMath::Min(BestStartSq, ClosestPointDistanceSqToPolyline(Hwy, StartRaw));
					BestEndSq = FMath::Min(BestEndSq, ClosestPointDistanceSqToPolyline(Hwy, EndRaw));
				}

				const bool bStartNearHighway = BestStartSq <= MaxDistSq;
				const bool bEndNearHighway = BestEndSq <= MaxDistSq;
				if (!bStartNearHighway && !bEndNearHighway)
				{
					++CityBLDRampAutoDirectionUnknown;
					continue;
				}

				// Which physical endpoint is closer to the freeway?
				const bool bHighwayAtStart = (BestStartSq <= BestEndSq);

				// Optional: auto-set ramp direction only when we have a reliable textual hint, and only if not locked.
				if (bCityBLDAutoDetectRampDirection && !bLocked && Hint != ERampHint::Unknown)
				{
					// Desired behavior (in terms of *physical* endpoints):
					// - On-ramp: traffic should END at the highway endpoint.
					// - Off-ramp: traffic should START at the highway endpoint.
					const bool bDesiredReverse = (Hint == ERampHint::OnRamp) ? bHighwayAtStart : !bHighwayAtStart;

					if (Meta->bReverseCenterlineDirection != bDesiredReverse)
					{
						Meta->Modify();
						Meta->bReverseCenterlineDirection = bDesiredReverse;
						++CityBLDRampAutoDirectionApplied;

						UE_LOG(LogTraffic, Log,
							TEXT("[TrafficPrep][CityBLD] Auto-set ramp direction for %s (Family=%s Hint=%s HighwayAtStart=%s Reverse=%s)"),
							*Ramp->GetName(),
							*Meta->FamilyName.ToString(),
							(Hint == ERampHint::OnRamp) ? TEXT("OnRamp") : TEXT("OffRamp"),
							bHighwayAtStart ? TEXT("true") : TEXT("false"),
							bDesiredReverse ? TEXT("true") : TEXT("false"));
					}
				}
				else if (bCityBLDAutoDetectRampDirection && !bLocked && Hint == ERampHint::Unknown)
				{
					// We had freeway geometry but couldn't figure out whether the ramp is "on" or "off" from textual hints.
					++CityBLDRampAutoDirectionUnknown;
				}

				// Optional: split generic ramp family into separate (On)/(Off) variants so users can bake different offsets.
				// This is based on the *current* driving direction (bReverseCenterlineDirection) and which endpoint is nearest the freeway.
				if (bCityBLDSplitRampFamiliesByRole)
				{
					enum class ERampRole : uint8 { Unknown, On, Off };

					FString BaseFamilyName = Meta->FamilyName.ToString();
					ERampRole ExistingRole = ERampRole::Unknown;

					auto StripRoleSuffix = [&](const TCHAR* Suffix, ERampRole Role) -> bool
					{
						const FString S(Suffix);
						if (BaseFamilyName.EndsWith(S, ESearchCase::IgnoreCase))
						{
							BaseFamilyName.LeftChopInline(S.Len());
							BaseFamilyName.TrimEndInline();
							ExistingRole = Role;
							return true;
						}
						return false;
					};

					StripRoleSuffix(TEXT(" (On)"), ERampRole::On);
					if (ExistingRole == ERampRole::Unknown)
					{
						StripRoleSuffix(TEXT(" (Off)"), ERampRole::Off);
					}

					const bool bTrafficFlowsStartToEnd = !Meta->bReverseCenterlineDirection;
					// With our convention, StartRaw/EndRaw are physical endpoints. Traffic flow is StartRaw->EndRaw
					// when not reversed, else EndRaw->StartRaw. On-ramp means destination is at freeway endpoint,
					// Off-ramp means source is at freeway endpoint.
					const bool bIsOnRamp = (Meta->bReverseCenterlineDirection == bHighwayAtStart);
					if (bIsOnRamp)
					{
						++CityBLDRampsClassifiedOn;
					}
					else
					{
						++CityBLDRampsClassifiedOff;
					}

					const FString VariantKey = BaseFamilyName + (bIsOnRamp ? TEXT(" (On)") : TEXT(" (Off)"));
					const FGuid OldFamilyId = Meta->RoadFamilyId;
					const FRoadFamilyInfo* OldInfo = Registry->FindFamilyById(OldFamilyId);

					bool bCreatedRoleFamily = false;
					FRoadFamilyInfo* RoleInfo = Registry->FindOrCreateFamilyForActorWithVariantKey(Ramp, VariantKey, &bCreatedRoleFamily);
					if (RoleInfo)
					{
						if (bCreatedRoleFamily)
						{
							++CityBLDRampRoleFamiliesCreated;

							// Seed role family from the actor's previous family calibration if we have it (better first-run UX).
							if (OldInfo)
							{
								RoleInfo->CalibrationData = OldInfo->CalibrationData;
								RoleInfo->bIsCalibrated = OldInfo->bIsCalibrated;
								RoleInfo->FamilyDefinition = OldInfo->FamilyDefinition;
								RoleInfo->FamilyDefinition.FamilyName = FName(*VariantKey);
							}
						}

						if (Meta->RoadFamilyId != RoleInfo->FamilyId)
						{
							Meta->Modify();
							Meta->RoadFamilyId = RoleInfo->FamilyId;
							Meta->FamilyName = RoleInfo->FamilyDefinition.FamilyName.IsNone() ? FName(*VariantKey) : RoleInfo->FamilyDefinition.FamilyName;
							++CityBLDRampRoleActorsRemapped;

							UE_LOG(LogTraffic, Log,
								TEXT("[TrafficPrep][CityBLD] Ramp role family: %s -> %s (HighwayAtStart=%s Reverse=%s)"),
								*Ramp->GetName(),
								*VariantKey,
								bHighwayAtStart ? TEXT("true") : TEXT("false"),
								Meta->bReverseCenterlineDirection ? TEXT("true") : TEXT("false"));
						}

						// Ensure runtime build settings contain a matching family entry by name.
						if (RoadSettings)
						{
							const FName VariantFamilyName(*VariantKey);
							if (!RoadSettings->FindFamilyByName(VariantFamilyName))
							{
								FRoadFamilyDefinition NewDef = RoleInfo->FamilyDefinition;
								NewDef.FamilyName = VariantFamilyName;
								if (NewDef.VehicleLaneProfile.IsNull())
								{
									NewDef.VehicleLaneProfile = DefaultVehicleProfilePath;
								}
								if (NewDef.FootpathLaneProfile.IsNull())
								{
									NewDef.FootpathLaneProfile = DefaultFootpathProfilePath;
								}

								RoadSettings->Families.Add(NewDef);
								bRoadSettingsModified = true;
							}
						}
					}
				}
			}

			if (bCityBLDSplitRampFamiliesByRole && (CityBLDRampRoleFamiliesCreated > 0 || CityBLDRampRoleActorsRemapped > 0))
			{
				UE_LOG(LogTraffic, Log,
					TEXT("[TrafficPrep] CityBLD: Ramp role split: FamiliesCreated=%d ActorsRemapped=%d (set aaa.Traffic.CityBLD.SplitRampFamiliesByRole=0 to disable)."),
					CityBLDRampRoleFamiliesCreated,
					CityBLDRampRoleActorsRemapped);
			}

			// UX guard: after Undo/Redo or re-drawing, users often end up with all ramps defaulting to one direction
			// (Reverse=false). If we see multiple ramps but none classified as Off, warn clearly.
			if (bCityBLDSplitRampFamiliesByRole && RampActors.Num() >= 2 && CityBLDRampsClassifiedOn > 0 && CityBLDRampsClassifiedOff == 0)
			{
				UE_LOG(LogTraffic, Warning,
					TEXT("[TrafficPrep][CityBLD] All %d ramp actors were classified as '(On)' and none as '(Off)'. ")
					TEXT("If you placed an off-ramp, select that ramp actor and click 'Flip Selected Road Direction', then run PREPARE MAP again."),
					RampActors.Num());
			}
		}
	}

	Registry->RefreshCache();
	Registry->SaveConfig();

	if (bRoadSettingsModified && RoadSettings)
	{
		RoadSettings->SaveConfig();
	}

	// Successful PREPARE clears the dirty flag.
	ClearPrepareDirty();

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
	if (CityBLDSplineTagsAdded > 0)
	{
		UE_LOG(LogTraffic, Log, TEXT("[TrafficPrep] CityBLD: Auto-tagged %d spline components with '%s'."),
			CityBLDSplineTagsAdded,
			*GetDefault<UTrafficCityBLDAdapterSettings>()->RoadSplineTag.ToString());
	}
	if (CityBLDRampAutoDirectionApplied > 0 || CityBLDRampAutoDirectionLocked > 0 || CityBLDRampAutoDirectionUnknown > 0)
	{
		UE_LOG(LogTraffic, Log,
			TEXT("[TrafficPrep] CityBLD: Auto-ramp direction: Applied=%d Locked=%d Unknown=%d (set aaa.Traffic.CityBLD.AutoDetectRampDirection=0 to disable)."),
			CityBLDRampAutoDirectionApplied,
			CityBLDRampAutoDirectionLocked,
			CityBLDRampAutoDirectionUnknown);
	}
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

	// Important: use the full AAA PREPARE MAP flow here (not the legacy calibration subsystem PrepareAllRoads).
	//
	// The legacy PrepareAllRoads assigns a default family like 'Urban_2x2' to any spline actor with metadata,
	// which can overwrite CityBLD preset-based families and cause incorrect lane counts (e.g. 4 lanes on 2-lane roads).
	UE_LOG(LogTraffic, Log, TEXT("[TrafficEditor] PREPARE: Running Editor_PrepareMapForTraffic()."));
	Editor_PrepareMapForTraffic();
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

	if (!EnsurePreparedForAction(TEXT("Build")))
	{
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

	if (!EnsurePreparedForAction(TEXT("Cars")))
	{
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
	const int32 VehiclesPerLane = FMath::Clamp(CVarTrafficEditorVehiclesPerLane.GetValueOnGameThread(), 1, 50);
	const float SpeedCmPerSec = FMath::Clamp(CVarTrafficEditorVehicleSpeedCmPerSec.GetValueOnGameThread(), 10.f, 10000.f);
	Manager->SpawnTestVehicles(VehiclesPerLane, SpeedCmPerSec);

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

void UTrafficSystemEditorSubsystem::DoClearCars()
{
#if WITH_EDITOR
	static const FName TrafficSpawnedVehicleTag(TEXT("AAA_TrafficVehicle"));

	UE_LOG(LogTraffic, Log, TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION));
	UWorld* World = GetEditorWorld();
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] DoClearCars: No editor world."));
		return;
	}

	int32 NumDestroyed = 0;
	TArray<AActor*> ToDestroy;

	// Clear via managers first (they know about adapters + spawned visual pawns).
	for (TActorIterator<ATrafficVehicleManager> It(World); It; ++It)
	{
		if (ATrafficVehicleManager* Manager = *It)
		{
			Manager->ClearVehicles();
			ToDestroy.AddUnique(Manager);
		}
	}

	// Also catch adapters explicitly (in case a manager was deleted).
	for (TActorIterator<ATrafficVehicleAdapter> It(World); It; ++It)
	{
		if (ATrafficVehicleAdapter* Adapter = *It)
		{
			ToDestroy.AddUnique(Adapter);
			if (APawn* VisualPawn = Adapter->ChaosVehicle.Get())
			{
				ToDestroy.AddUnique(VisualPawn);
			}
			if (ATrafficVehicleBase* Logic = Adapter->LogicVehicle.Get())
			{
				ToDestroy.AddUnique(Logic);
			}
		}
	}

	// Destroy any tagged spawned vehicles (covers Chaos pawns + any future spawned types).
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->Tags.Contains(TrafficSpawnedVehicleTag))
		{
			ToDestroy.AddUnique(Actor);
		}
	}

	// Best-effort legacy cleanup: any pawn owned by a TrafficVehicleManager.
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Pawn = *It;
		if (!Pawn)
		{
			continue;
		}
		if (ATrafficVehicleManager* OwnerMgr = Cast<ATrafficVehicleManager>(Pawn->GetOwner()))
		{
			if (!OwnerMgr->IsActorBeingDestroyed())
			{
				ToDestroy.AddUnique(Pawn);
			}
		}
	}

	for (AActor* Actor : ToDestroy)
	{
		if (Actor && Actor->IsValidLowLevel() && !Actor->IsActorBeingDestroyed())
		{
			World->DestroyActor(Actor);
			++NumDestroyed;
		}
	}

	if (NumDestroyed == 0)
	{
		UE_LOG(LogTraffic, Log, TEXT("[TrafficEditor] CLEAR CARS: No traffic vehicles to clear."));
	}
	else
	{
		UE_LOG(LogTraffic, Log, TEXT("[TrafficEditor] CLEAR CARS: Destroyed %d actors."), NumDestroyed);
	}
#endif
}

void UTrafficSystemEditorSubsystem::DoDrawIntersectionDebug()
{
#if WITH_EDITOR
	UE_LOG(LogTraffic, Log, TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION));
	UWorld* World = GetEditorWorld();
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] DoDrawIntersectionDebug: No editor world."));
		return;
	}

	if (!EnsurePreparedForAction(TEXT("IntersectionDebug")))
	{
		return;
	}

	if (!EnsureDetectedFamilies(TEXT("IntersectionDebug")))
	{
		return;
	}

	ATrafficSystemController* Controller = GetOrSpawnController();
	if (!Controller)
	{
		return;
	}

	if (Controller->GetNumRoads() <= 0 || Controller->GetNumLanes() <= 0 || !Controller->GetBuiltNetworkAsset())
	{
		UE_LOG(LogTraffic, Log, TEXT("[TrafficEditor] IntersectionDebug: No built network detected; building now."));
		Controller->Editor_BuildTrafficNetwork();
	}

	const UTrafficNetworkAsset* NetAsset = Controller->GetBuiltNetworkAsset();
	if (!NetAsset)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] IntersectionDebug: No network asset after build."));
		return;
	}

	const FTrafficNetwork& Network = NetAsset->Network;
	UE_LOG(LogTraffic, Log,
		TEXT("[TrafficEditor] IntersectionDebug: Drawing network debug. Roads=%d Lanes=%d Intersections=%d Movements=%d"),
		Network.Roads.Num(),
		Network.Lanes.Num(),
		Network.Intersections.Num(),
		Network.Movements.Num());

	if (Network.Intersections.Num() == 0)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] IntersectionDebug: Network contains 0 intersections."));
		return;
	}

	constexpr float LifeTimeSeconds = 30.f;
	constexpr float LaneThickness = 6.f;
	constexpr float MovementThickness = 10.f;
	constexpr float ZOffsetLane = 25.f;
	constexpr float ZOffsetMovement = 45.f;
	constexpr float ArrowLengthCm = 250.f;
	constexpr float ArrowSpacingCm = 800.f;

	auto DrawPolylineWithArrows = [&](const TArray<FVector>& InPoints, const FColor& Color, float ZOffset, float Thickness)
	{
		if (InPoints.Num() < 2)
		{
			return;
		}

		float DistanceUntilNextArrow = 0.f;
		for (int32 i = 0; i + 1 < InPoints.Num(); ++i)
		{
			const FVector A0 = InPoints[i] + FVector(0.f, 0.f, ZOffset);
			const FVector B0 = InPoints[i + 1] + FVector(0.f, 0.f, ZOffset);
			DrawDebugLine(World, A0, B0, Color, /*bPersistentLines=*/false, LifeTimeSeconds, /*DepthPriority=*/0, Thickness);

			const FVector Segment = B0 - A0;
			const float SegmentLen = Segment.Size();
			if (SegmentLen <= KINDA_SMALL_NUMBER)
			{
				continue;
			}

			const FVector Dir = Segment / SegmentLen;
			while (DistanceUntilNextArrow < SegmentLen)
			{
				const FVector Pos = A0 + Dir * DistanceUntilNextArrow;
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
	};

	const bool bDrawAll = CVarTrafficDrawAllIntersectionDebug.GetValueOnGameThread() != 0;
	const int32 ForcedIntersectionId = CVarTrafficDebugIntersectionId.GetValueOnGameThread();

	const FTrafficIntersection* TargetIntersection = nullptr;
	int32 SelectedLaneId = INDEX_NONE;
	const FTrafficMovement* SelectedMovement = nullptr;
	int32 SelectedOutgoingLaneId = INDEX_NONE;
	if (bDrawAll)
	{
		TargetIntersection = nullptr;
	}
	else if (ForcedIntersectionId >= 0)
	{
		TargetIntersection = Network.Intersections.FindByPredicate([&](const FTrafficIntersection& I)
		{
			return I.IntersectionId == ForcedIntersectionId;
		});

		if (!TargetIntersection)
		{
			UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] IntersectionDebug: Requested IntersectionId=%d not found; falling back to closest."), ForcedIntersectionId);
		}
	}

	FVector FocusLocation = FVector::ZeroVector;
	bool bHasFocusLocation = false;
	if (!bDrawAll && !TargetIntersection && GEditor && GEditor->GetSelectedActors())
	{
		for (FSelectionIterator It(*GEditor->GetSelectedActors()); It; ++It)
		{
			if (const ATrafficLaneEndpointMarkerActor* Marker = Cast<ATrafficLaneEndpointMarkerActor>(*It))
			{
				// Only treat "incoming" markers as the selection for chosen-path filtering.
				// For this debug tool, an incoming lane endpoint is the lane's travel end (bIsStart==false).
				if (!Marker->bIsStart)
				{
					SelectedLaneId = Marker->LaneId;
				}
				FocusLocation = Marker->GetActorLocation();
				bHasFocusLocation = true;
				break;
			}

			if (const AActor* SelectedActor = Cast<AActor>(*It))
			{
				FocusLocation = SelectedActor->GetActorLocation();
				bHasFocusLocation = true;
				break;
			}
		}
	}

	if (!bDrawAll && !TargetIntersection && !bHasFocusLocation && GCurrentLevelEditingViewportClient)
	{
		FocusLocation = GCurrentLevelEditingViewportClient->GetViewLocation();
		bHasFocusLocation = true;
	}

	if (!bDrawAll && !TargetIntersection && bHasFocusLocation)
	{
		float BestDistSq = TNumericLimits<float>::Max();
		for (const FTrafficIntersection& Intersection : Network.Intersections)
		{
			const float DistSq = FVector::DistSquared(FocusLocation, Intersection.Center);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				TargetIntersection = &Intersection;
			}
		}
	}

	TSet<int32> RelevantLaneIds;
	if (TargetIntersection)
	{
		RelevantLaneIds.Reserve(TargetIntersection->IncomingLaneIds.Num() + TargetIntersection->OutgoingLaneIds.Num());
		for (const int32 LaneId : TargetIntersection->IncomingLaneIds)
		{
			RelevantLaneIds.Add(LaneId);
		}
		for (const int32 LaneId : TargetIntersection->OutgoingLaneIds)
		{
			RelevantLaneIds.Add(LaneId);
		}

		UE_LOG(LogTraffic, Log,
			TEXT("[TrafficEditor] IntersectionDebug: Focus IntersectionId=%d (In=%d Out=%d). Use aaa.Traffic.Debug.DrawAllIntersectionDebug=1 to draw all."),
			TargetIntersection->IntersectionId,
			TargetIntersection->IncomingLaneIds.Num(),
			TargetIntersection->OutgoingLaneIds.Num());

		// Refresh selectable endpoint markers for this intersection to allow "click an endpoint" workflows.
		for (TActorIterator<ATrafficLaneEndpointMarkerActor> It(World); It; ++It)
		{
			if (It->Tags.Contains(RoadLabTag))
			{
				It->Destroy();
			}
		}

		for (const int32 LaneId : RelevantLaneIds)
		{
			const FTrafficLane* LanePtr = Network.Lanes.FindByPredicate([&](const FTrafficLane& L) { return L.LaneId == LaneId; });
			if (!LanePtr || LanePtr->CenterlinePoints.Num() < 2)
			{
				continue;
			}

			const FVector StartPos = LanePtr->CenterlinePoints[0];
			const FVector EndPos = LanePtr->CenterlinePoints.Last();
			const float StartDist = FVector::DistSquared(StartPos, TargetIntersection->Center);
			const float EndDist = FVector::DistSquared(EndPos, TargetIntersection->Center);
			const bool bEndpointIsStart = StartDist <= EndDist;
			const FVector MarkerPos = bEndpointIsStart ? StartPos : EndPos;

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			ATrafficLaneEndpointMarkerActor* Marker = World->SpawnActor<ATrafficLaneEndpointMarkerActor>(MarkerPos + FVector(0.f, 0.f, ZOffsetLane + 15.f), FRotator::ZeroRotator, Params);
			if (!Marker)
			{
				continue;
			}

			TagAsRoadLab(Marker);
			Marker->LaneId = LaneId;
			Marker->bIsStart = bEndpointIsStart;
			Marker->IntersectionId = TargetIntersection->IntersectionId;

			const bool bIsIncomingLane = TargetIntersection->IncomingLaneIds.Contains(LaneId);
			DrawDebugString(
				World,
				MarkerPos + FVector(0.f, 0.f, ZOffsetLane + 55.f),
				FString::Printf(TEXT("%s Lane %d"), bIsIncomingLane ? TEXT("IN") : TEXT("OUT"), LaneId),
				/*TestBaseActor=*/nullptr,
				FColor(255, 230, 80, 255),
				LifeTimeSeconds,
				/*bDrawShadow=*/true);
		}
	}

	if (SelectedLaneId != INDEX_NONE)
	{
		SelectedMovement = TrafficRouting::ChooseDefaultMovementForIncomingLane(Network, SelectedLaneId);
		SelectedOutgoingLaneId = SelectedMovement ? SelectedMovement->OutgoingLaneId : INDEX_NONE;
	}

	// Lanes: draw centerlines so you can see continuity across road segments.
	for (const FTrafficLane& Lane : Network.Lanes)
	{
		if (SelectedLaneId != INDEX_NONE && Lane.LaneId != SelectedLaneId)
		{
			// Also draw the chosen outgoing lane (if any) to make the "chosen path" obvious.
			if (SelectedOutgoingLaneId == INDEX_NONE || Lane.LaneId != SelectedOutgoingLaneId)
			{
				continue;
			}
		}

		if (!bDrawAll && TargetIntersection && !RelevantLaneIds.Contains(Lane.LaneId))
		{
			continue;
		}

		const FColor LaneColor =
			(Lane.Direction == ELaneDirection::Forward) ? FColor(40, 220, 80, 220) :
			(Lane.Direction == ELaneDirection::Backward) ? FColor(220, 60, 40, 220) :
			FColor(40, 160, 255, 220);

		DrawPolylineWithArrows(Lane.CenterlinePoints, LaneColor, ZOffsetLane, LaneThickness);

		if (Lane.CenterlinePoints.Num() >= 2)
		{
			DrawDebugSphere(World, Lane.CenterlinePoints[0] + FVector(0.f, 0.f, ZOffsetLane), /*Radius=*/35.f, /*Segments=*/8, LaneColor, false, LifeTimeSeconds, 0, 2.f);
			DrawDebugSphere(World, Lane.CenterlinePoints.Last() + FVector(0.f, 0.f, ZOffsetLane), /*Radius=*/35.f, /*Segments=*/8, LaneColor, false, LifeTimeSeconds, 0, 2.f);
		}
	}

	// Intersections: draw a simple sphere representing the clustered endpoint radius.
	for (const FTrafficIntersection& Intersection : Network.Intersections)
	{
		if (!bDrawAll && TargetIntersection && Intersection.IntersectionId != TargetIntersection->IntersectionId)
		{
			continue;
		}

		const float Radius = FMath::Max(Intersection.Radius, 50.f);
		DrawDebugSphere(World, Intersection.Center + FVector(0.f, 0.f, ZOffsetMovement), Radius, /*Segments=*/16, FColor(255, 255, 255, 60), false, LifeTimeSeconds, 0, 2.f);

		const int32 MovementCount = Algo::CountIf(Network.Movements, [&](const FTrafficMovement& M)
		{
			return M.IntersectionId == Intersection.IntersectionId;
		});

		DrawDebugString(
			World,
			Intersection.Center + FVector(0.f, 0.f, ZOffsetMovement + 60.f),
			FString::Printf(TEXT("Intersection %d\nIn=%d Out=%d Mov=%d"),
				Intersection.IntersectionId,
				Intersection.IncomingLaneIds.Num(),
				Intersection.OutgoingLaneIds.Num(),
				MovementCount),
			/*TestBaseActor=*/nullptr,
			FColor::White,
			LifeTimeSeconds,
			/*bDrawShadow=*/true);
	}

	// Movements: draw explicit turn connectors (what "going through an intersection" means in this plugin).
	for (const FTrafficMovement& Movement : Network.Movements)
	{
		if (SelectedLaneId != INDEX_NONE && Movement.IncomingLaneId != SelectedLaneId)
		{
			continue;
		}

		if (!bDrawAll && TargetIntersection && Movement.IntersectionId != TargetIntersection->IntersectionId)
		{
			continue;
		}

		const bool bIsChosen = SelectedMovement && SelectedMovement->MovementId == Movement.MovementId;
		const FColor MoveColor = bIsChosen
			? FColor::White
			: (Movement.TurnType == ETrafficTurnType::Through) ? FColor::Yellow :
			  (Movement.TurnType == ETrafficTurnType::Left) ? FColor(255, 0, 255, 255) :
			  (Movement.TurnType == ETrafficTurnType::Right) ? FColor::Cyan :
			  FColor::Red;

		DrawPolylineWithArrows(Movement.PathPoints, MoveColor, ZOffsetMovement, bIsChosen ? (MovementThickness * 1.6f) : MovementThickness);
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

	if (!EnsurePreparedForAction(TEXT("BeginCalibration")))
	{
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
	ActorsBefore.RemoveAll([&](AActor* Actor)
	{
		return !Actor || Actor->Tags.Contains(RoadLabTag);
	});

	auto WarnIfMissing = [&](const TCHAR* Context)
	{
		TArray<AActor*> ActorsAfter;
		GetActorsForFamily(FamilyId, ActorsAfter);
		ActorsAfter.RemoveAll([&](AActor* Actor)
		{
			return !Actor || Actor->Tags.Contains(RoadLabTag);
		});

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
	AActor* PreferredRoadActor = nullptr;
	AActor* PreviousActiveRoadActor = ActiveCalibrationRoadActor.IsValid() ? ActiveCalibrationRoadActor.Get() : nullptr;
	if (GEditor && GEditor->GetSelectedActors())
	{
		for (FSelectionIterator It(*GEditor->GetSelectedActors()); It; ++It)
		{
			if (AActor* Candidate = Cast<AActor>(*It))
			{
				if (!Candidate->IsA(RoadClass))
				{
					continue;
				}

				// IMPORTANT: some road kits (CityBLD BP_ModularRoad) use a single actor class for many road styles.
				// Only accept the selected actor if it is actually mapped to this family id via metadata.
				if (const UTrafficRoadMetadataComponent* Meta = Candidate->FindComponentByClass<UTrafficRoadMetadataComponent>())
				{
					if (Meta->RoadFamilyId != FamilyId || !Meta->bIncludeInTraffic)
					{
						continue;
					}
				}
				else
				{
					continue;
				}

				RoadActor = Candidate;
				break;
			}
		}
	}

	// Common UX case: we are refreshing the overlay for the *same* active family (e.g. user flipped road direction),
	// but the selection is currently the overlay actor. Prefer staying locked to the previously active road actor
	// so the camera doesn't jump to a different instance of the same family.
	if (!RoadActor && PreviousActiveRoadActor && PreviousActiveRoadActor->IsA(RoadClass) && !PreviousActiveRoadActor->Tags.Contains(RoadLabTag))
	{
		if (const UTrafficRoadMetadataComponent* PrevMeta = PreviousActiveRoadActor->FindComponentByClass<UTrafficRoadMetadataComponent>())
		{
			if (PrevMeta->RoadFamilyId == FamilyId && PrevMeta->bIncludeInTraffic)
			{
				PreferredRoadActor = PreviousActiveRoadActor;
			}
		}
	}

	// Cleanup previous preview actors.
	if (ActiveCalibrationRoadActor.IsValid())
	{
		if (AActor* Prev = ActiveCalibrationRoadActor.Get())
		{
			// Only destroy actors we spawned for the RoadLab flow. Never destroy user road actors.
			if (Prev->Tags.Contains(RoadLabTag))
			{
				Prev->Destroy();
			}
		}
		ActiveCalibrationRoadActor.Reset();
	}
	if (CalibrationOverlayActor.IsValid())
	{
		CalibrationOverlayActor->Destroy();
		CalibrationOverlayActor.Reset();
	}

	const bool bIsCityBLDLikeRoadClass =
		RoadClass->GetName().Contains(TEXT("BP_MeshRoad"), ESearchCase::IgnoreCase) ||
		RoadClass->GetName().Contains(TEXT("BP_ModularRoad"), ESearchCase::IgnoreCase);

	if (!RoadActor)
	{
		if (PreferredRoadActor)
		{
			RoadActor = PreferredRoadActor;
		}
	}

	if (!RoadActor)
	{
		// Prefer an existing road actor from the level (safer for procedural road kits that may rebuild on spawn).
		for (AActor* Existing : ActorsBefore)
		{
			if (!Existing || !Existing->IsA(RoadClass) || Existing->Tags.Contains(RoadLabTag))
			{
				continue;
			}

			// IMPORTANT: some road kits (CityBLD BP_ModularRoad/BP_MeshRoad) use a single actor class for many road styles.
			// Only accept an existing actor if it is actually mapped to this family id via metadata.
			if (const UTrafficRoadMetadataComponent* Meta = Existing->FindComponentByClass<UTrafficRoadMetadataComponent>())
			{
				if (Meta->RoadFamilyId != FamilyId || !Meta->bIncludeInTraffic)
				{
					continue;
				}
			}
			else
			{
				continue;
			}

			RoadActor = Existing;
			break;
		}

		if (!RoadActor)
		{
			// Fallback: spawn a preview instance only for non-CityBLD road classes.
			// For CityBLD modular roads, the style/preset lives on the actor instance, so spawning a blank preview can be misleading.
			if (bIsCityBLDLikeRoadClass)
			{
				UE_LOG(LogTraffic, Warning,
					TEXT("[TrafficEditor] BeginCalibrationForFamily: No actor instance found for %s. Ensure the relevant CityBLD regions are loaded and run PREPARE MAP again."),
					*FamilyInfo->DisplayName);
				return;
			}

			RoadActor = World->SpawnActor<AActor>(RoadClass, FTransform::Identity, SpawnParams);
			if (!RoadActor)
			{
				UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] BeginCalibrationForFamily: Failed to spawn road actor of class %s"), *RoadClass->GetName());
				return;
			}
			TagAsRoadLab(RoadActor);
		}
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

	const bool bIsCityBLD =
		RoadActor->GetClass()->GetName().Contains(TEXT("BP_MeshRoad"), ESearchCase::IgnoreCase) ||
		RoadActor->GetClass()->GetName().Contains(TEXT("BP_ModularRoad"), ESearchCase::IgnoreCase);
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
	if (bIsCityBLD && AdapterSettings && AdapterSettings->bUseZoneGraphLanePolylinesForCalibrationOverlay)
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

	// Seed per-lane offset adjustments from the current family definition (useful for asymmetric layouts like bus lanes).
	Overlay->ForwardLaneOffsetAdjustmentsCm = FamilyInfo->FamilyDefinition.Forward.LaneCenterOffsetAdjustmentsCm;
	Overlay->BackwardLaneOffsetAdjustmentsCm = FamilyInfo->FamilyDefinition.Backward.LaneCenterOffsetAdjustmentsCm;
	Overlay->ForwardLaneOffsetAdjustmentsCm.SetNum(FMath::Max(0, Overlay->NumLanesPerSideForward));
	Overlay->BackwardLaneOffsetAdjustmentsCm.SetNum(FMath::Max(0, Overlay->NumLanesPerSideBackward));

	bool bBuiltOverlayFromZoneGraph = false;
	if (AdapterSettings &&
		AdapterSettings->bUseZoneGraphLanePolylinesForCalibrationOverlay &&
		bIsCityBLD)
	{
		TArray<TArray<FVector>> LanePolylines;
		if (ZoneGraphLaneOverlayUtils::GetZoneGraphLanePolylinesNearActor(World, RoadActor, LanePolylines))
		{
			// If the road direction was flipped via metadata, ZoneGraph data may be stale until the next network build.
			// To keep calibration UX predictable, detect mismatched polyline direction and reverse polylines when needed.
			//
			// Only do this for one-way families; for two-way families ZoneGraph typically provides both directions and
			// reversing everything can confuse which side is "forward" in the preview.
			if ((Calib.NumLanesPerSideForward == 0) ^ (Calib.NumLanesPerSideBackward == 0))
			{
				const UTrafficRoadMetadataComponent* Meta = RoadActor->FindComponentByClass<UTrafficRoadMetadataComponent>();
				const USplineComponent* Spline = RoadActor->FindComponentByClass<USplineComponent>();
				if (Meta && Spline && LanePolylines.Num() > 0 && LanePolylines[0].Num() >= 2)
				{
					const FVector PolyDir = (LanePolylines[0][1] - LanePolylines[0][0]).GetSafeNormal();
					const float Key = Spline->FindInputKeyClosestToWorldLocation(LanePolylines[0][0]);
					FVector ExpectedDir = Spline->GetDirectionAtSplineInputKey(Key, ESplineCoordinateSpace::World).GetSafeNormal();
					if (Meta->bReverseCenterlineDirection)
					{
						ExpectedDir *= -1.f;
					}

					if (FVector::DotProduct(PolyDir, ExpectedDir) < 0.f)
					{
						for (TArray<FVector>& Poly : LanePolylines)
						{
							Algo::Reverse(Poly);
						}
					}
				}
			}

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
		if (IsValid(Overlay) && !Overlay->IsActorBeingDestroyed())
		{
			GEditor->SelectNone(/*NoteSelectionChange=*/false, /*DeselectBSPSurfs=*/true, /*WarnAboutManyActors=*/false);
			GEditor->SelectActor(Overlay, /*InSelected=*/true, /*Notify=*/true);
			FocusCameraOnActor(Overlay);
		}
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
	ActorsBefore.RemoveAll([&](AActor* Actor)
	{
		return !Actor || Actor->Tags.Contains(RoadLabTag);
	});

	const bool bAutomationOrCmdlet = IsRunningCommandlet() || GIsAutomationTesting;

	auto WarnIfMissing = [&]()
	{
		TArray<AActor*> ActorsAfter;
		GetActorsForFamily(ActiveFamilyId, ActorsAfter);
		ActorsAfter.RemoveAll([&](AActor* Actor)
		{
			return !Actor || Actor->Tags.Contains(RoadLabTag);
		});

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
	NewCalib.NumLanesPerSideForward = FMath::Clamp(CalibrationOverlayActor->NumLanesPerSideForward, 0, 5);
	NewCalib.NumLanesPerSideBackward = FMath::Clamp(CalibrationOverlayActor->NumLanesPerSideBackward, 0, 5);
	NewCalib.LaneWidthCm = FMath::Clamp(CalibrationOverlayActor->LaneWidthCm, 200.f, 500.f);
	NewCalib.CenterlineOffsetCm = CalibrationOverlayActor->CenterlineOffsetCm;

	if (NewCalib.NumLanesPerSideForward == 0 && NewCalib.NumLanesPerSideBackward == 0)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficCalib] Invalid calibration: both forward and backward lanes are 0."));
		if (!bAutomationOrCmdlet)
		{
			FMessageDialog::Open(
				EAppMsgType::Ok,
				NSLOCTEXT("TrafficSystemEditorSubsystem", "Traffic_CalibrationZeroLanes",
					"Invalid calibration: total lanes cannot be 0.\n\n"
					"Set at least one lane on either the forward or backward side."));
		}
		return;
	}

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
					"  - Lanes per side: 0 to 5 (total lanes must be > 0)\n"
					"  - Lane width: 200cm to 500cm"));
			}
			else
			{
				UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor][Automation] Calibration clamped; dialog suppressed."));
		}
	}

	Registry->ApplyCalibration(ActiveFamilyId, NewCalib);

	// Store per-lane offset adjustments in the editor registry so future calibration sessions can start from them.
	FamilyInfo->FamilyDefinition.Forward.LaneCenterOffsetAdjustmentsCm = CalibrationOverlayActor->ForwardLaneOffsetAdjustmentsCm;
	FamilyInfo->FamilyDefinition.Backward.LaneCenterOffsetAdjustmentsCm = CalibrationOverlayActor->BackwardLaneOffsetAdjustmentsCm;
	FamilyInfo->FamilyDefinition.Forward.LaneCenterOffsetAdjustmentsCm.SetNum(FMath::Max(0, NewCalib.NumLanesPerSideForward));
	FamilyInfo->FamilyDefinition.Backward.LaneCenterOffsetAdjustmentsCm.SetNum(FMath::Max(0, NewCalib.NumLanesPerSideBackward));
	Registry->SaveConfig();

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

		// Detect duplicates (same FamilyName) and offer a one-click cleanup.
		TArray<int32> MatchingIndices;
		for (int32 i = 0; i < RoadSettings->Families.Num(); ++i)
		{
			if (RoadSettings->Families[i].FamilyName == CalibFamilyName)
			{
				MatchingIndices.Add(i);
			}
		}

		int32 EffectiveIndex = FoundIndex;
		if (MatchingIndices.Num() > 1)
		{
			UE_LOG(LogTraffic, Warning,
				TEXT("[TrafficCalib] TrafficRoadFamilySettings contains %d duplicate entries for FamilyName='%s'. Using first index %d."),
				MatchingIndices.Num(),
				*CalibFamilyName.ToString(),
				MatchingIndices[0]);

			if (!bAutomationOrCmdlet)
			{
				const EAppReturnType::Type Choice = FMessageDialog::Open(
					EAppMsgType::YesNoCancel,
					FText::FromString(FString::Printf(
						TEXT("TrafficRoadFamilySettings contains %d duplicate entries named '%s'.\n\n")
						TEXT("Yes: delete duplicates (recommended)\n")
						TEXT("No: keep duplicates and continue\n")
						TEXT("Cancel: abort bake\n"),
						MatchingIndices.Num(),
						*CalibFamilyName.ToString())));

				if (Choice == EAppReturnType::Cancel)
				{
					return;
				}

				if (Choice == EAppReturnType::Yes)
				{
					// Remove duplicates, keeping the first match.
					for (int32 k = MatchingIndices.Num() - 1; k >= 1; --k)
					{
						const int32 RemoveIndex = MatchingIndices[k];
						if (RoadSettings->Families.IsValidIndex(RemoveIndex))
						{
							RoadSettings->Families.RemoveAt(RemoveIndex);
						}
					}
					RoadSettings->SaveConfig();
					UE_LOG(LogTraffic, Log, TEXT("[TrafficCalib] Deleted duplicate family entries for '%s' (kept index %d)."), *CalibFamilyName.ToString(), MatchingIndices[0]);
				}
			}

			// Recompute the effective index after any cleanup.
			EffectiveIndex = RoadSettings->Families.IndexOfByPredicate(
				[CalibFamilyName](const FRoadFamilyDefinition& Def)
				{
					return Def.FamilyName == CalibFamilyName;
				});
		}

		if (RoadSettings->Families.IsValidIndex(EffectiveIndex))
		{
			FRoadFamilyDefinition& Def = RoadSettings->Families[EffectiveIndex];
			Def.Forward.NumLanes = NewCalib.NumLanesPerSideForward;
			Def.Backward.NumLanes = NewCalib.NumLanesPerSideBackward;
			Def.Forward.LaneWidthCm = NewCalib.LaneWidthCm;
			Def.Backward.LaneWidthCm = NewCalib.LaneWidthCm;
			Def.Forward.InnerLaneCenterOffsetCm = NewCalib.CenterlineOffsetCm;
			Def.Backward.InnerLaneCenterOffsetCm = NewCalib.CenterlineOffsetCm;

			// Persist optional per-lane adjustments (for asymmetric layouts, e.g. bus lanes).
			Def.Forward.LaneCenterOffsetAdjustmentsCm = CalibrationOverlayActor->ForwardLaneOffsetAdjustmentsCm;
			Def.Backward.LaneCenterOffsetAdjustmentsCm = CalibrationOverlayActor->BackwardLaneOffsetAdjustmentsCm;
			Def.Forward.LaneCenterOffsetAdjustmentsCm.SetNum(FMath::Max(0, Def.Forward.NumLanes));
			Def.Backward.LaneCenterOffsetAdjustmentsCm.SetNum(FMath::Max(0, Def.Backward.NumLanes));

			RoadSettings->SaveConfig();
			UE_LOG(LogTraffic, Log, TEXT("[TrafficCalib] Saved calibrated layout into TrafficRoadFamilySettings for '%s' (Index=%d)."), *CalibFamilyName.ToString(), EffectiveIndex);
		}
		else
		{
			// If the family isn't present yet, create it so the build can immediately use the calibrated layout.
			static const FSoftObjectPath DefaultVehicleProfilePath(TEXT("/AAAtrafficSystem/ZoneProfiles/CityBLDUrbanTwoLane.CityBLDUrbanTwoLane"));
			static const FSoftObjectPath DefaultFootpathProfilePath(TEXT("/AAAtrafficSystem/ZoneProfiles/CityBLDFootpath.CityBLDFootpath"));

			FRoadFamilyDefinition NewDef = FamilyInfo->FamilyDefinition;
			NewDef.FamilyName = CalibFamilyName;
			NewDef.Forward.NumLanes = NewCalib.NumLanesPerSideForward;
			NewDef.Backward.NumLanes = NewCalib.NumLanesPerSideBackward;
			NewDef.Forward.LaneWidthCm = NewCalib.LaneWidthCm;
			NewDef.Backward.LaneWidthCm = NewCalib.LaneWidthCm;
			NewDef.Forward.InnerLaneCenterOffsetCm = NewCalib.CenterlineOffsetCm;
			NewDef.Backward.InnerLaneCenterOffsetCm = NewCalib.CenterlineOffsetCm;
			NewDef.Forward.LaneCenterOffsetAdjustmentsCm = CalibrationOverlayActor->ForwardLaneOffsetAdjustmentsCm;
			NewDef.Backward.LaneCenterOffsetAdjustmentsCm = CalibrationOverlayActor->BackwardLaneOffsetAdjustmentsCm;
			NewDef.Forward.LaneCenterOffsetAdjustmentsCm.SetNum(FMath::Max(0, NewDef.Forward.NumLanes));
			NewDef.Backward.LaneCenterOffsetAdjustmentsCm.SetNum(FMath::Max(0, NewDef.Backward.NumLanes));
			if (NewDef.VehicleLaneProfile.IsNull())
			{
				NewDef.VehicleLaneProfile = DefaultVehicleProfilePath;
			}
			if (NewDef.FootpathLaneProfile.IsNull())
			{
				NewDef.FootpathLaneProfile = DefaultFootpathProfilePath;
			}

			RoadSettings->Families.Add(NewDef);
			RoadSettings->SaveConfig();
			UE_LOG(LogTraffic, Log,
				TEXT("[TrafficCalib] Added missing family '%s' to TrafficRoadFamilySettings and saved calibrated layout."),
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

void UTrafficSystemEditorSubsystem::Editor_DeleteFamily(const FGuid& FamilyId, bool bExcludeActorsFromTraffic)
{
#if WITH_EDITOR
	URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get();
	if (!Registry)
	{
		return;
	}

	FRoadFamilyInfo* FamilyInfo = Registry->FindFamilyById(FamilyId);
	if (!FamilyInfo)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] DeleteFamily: Family not found: %s"), *FamilyId.ToString());
		return;
	}

	const FName FamilyName = FamilyInfo->FamilyDefinition.FamilyName.IsNone()
		? FName(*FamilyInfo->DisplayName)
		: FamilyInfo->FamilyDefinition.FamilyName;

	if (!Registry->DeleteFamilyById(FamilyId))
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficEditor] DeleteFamily: Failed to delete family: %s"), *FamilyId.ToString());
		return;
	}

	// Remove from runtime settings (by FamilyName), so builds don't keep using an orphaned entry.
	if (UTrafficRoadFamilySettings* RoadSettings = GetMutableDefault<UTrafficRoadFamilySettings>())
	{
		const int32 Removed = RoadSettings->Families.RemoveAll([&](const FRoadFamilyDefinition& Def)
		{
			return Def.FamilyName == FamilyName;
		});

		if (Removed > 0)
		{
			RoadSettings->SaveConfig();
			UE_LOG(LogTraffic, Log, TEXT("[TrafficEditor] DeleteFamily: Removed %d entries from TrafficRoadFamilySettings for '%s'."), Removed, *FamilyName.ToString());
		}
	}

	if (bExcludeActorsFromTraffic)
	{
		UWorld* World = GetEditorWorld();
		if (World)
		{
			const FScopedTransaction Tx(NSLOCTEXT("TrafficEditor", "DeleteFamilyExcludeActors", "AAA Traffic: Delete Family and Exclude Actors"));
			int32 UpdatedActors = 0;
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (UTrafficRoadMetadataComponent* Meta = It->FindComponentByClass<UTrafficRoadMetadataComponent>())
				{
					if (Meta->RoadFamilyId == FamilyId)
					{
						Meta->Modify();
						Meta->bIncludeInTraffic = false;
						Meta->RoadFamilyId.Invalidate();
						Meta->FamilyName = NAME_None;
						++UpdatedActors;
					}
				}
			}
			if (UpdatedActors > 0)
			{
				UE_LOG(LogTraffic, Log, TEXT("[TrafficEditor] DeleteFamily: Excluded %d actors from traffic for '%s'."), UpdatedActors, *FamilyName.ToString());
			}
		}
	}

	// If deleting the active calibration family, clear calibration state and overlay.
	if (ActiveFamilyId == FamilyId)
	{
		ActiveFamilyId.Invalidate();
		ActiveCalibrationRoadActor.Reset();
		if (CalibrationOverlayActor.IsValid())
		{
			CalibrationOverlayActor->Destroy();
			CalibrationOverlayActor.Reset();
		}
	}
#endif
}

void UTrafficSystemEditorSubsystem::Editor_ToggleReverseDirectionForSelectedRoads()
{
#if WITH_EDITOR
	UWorld* World = GetEditorWorld();
	if (!World)
	{
		return;
	}

	if (!EnsurePreparedForAction(TEXT("FlipRoadDirection")))
	{
		return;
	}

	const bool bAutomationOrCmdlet = IsRunningCommandlet() || GIsAutomationTesting;

	if (!GEditor || !GEditor->GetSelectedActors())
	{
		return;
	}

	URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get();
	if (!Registry)
	{
		return;
	}

	auto IsEligibleRoadActor = [&](AActor* Actor) -> bool
	{
		if (!Actor)
		{
			return false;
		}

		// Never operate on AAA RoadLab helper actors.
		const UClass* ActorClass = Actor->GetClass();
		const FString ClassName = ActorClass ? ActorClass->GetName() : FString();
		if (Actor->Tags.Contains(RoadLabTag) ||
			Actor->IsA(ALaneCalibrationOverlayActor::StaticClass()) ||
			Actor->IsA(ARoadLanePreviewActor::StaticClass()) ||
			Actor->IsA(ATrafficVehicleBase::StaticClass()) ||
			Actor->IsA(ATrafficVehicleManager::StaticClass()) ||
			Actor->IsA(ATrafficSystemController::StaticClass()) ||
			// Extra safety: if a blueprint or renamed class slips through, still avoid touching overlay actors.
			ClassName.Contains(TEXT("LaneCalibrationOverlayActor"), ESearchCase::IgnoreCase))
		{
			return false;
		}

		// Only meaningful for spline-driven roads.
		return Actor->FindComponentByClass<USplineComponent>() != nullptr;
	};

	TArray<AActor*> TargetActors;
	for (FSelectionIterator It(*GEditor->GetSelectedActors()); It; ++It)
	{
		if (AActor* Actor = Cast<AActor>(*It))
		{
			if (IsEligibleRoadActor(Actor))
			{
				TargetActors.Add(Actor);
			}
		}
	}

	// Common UX pitfall: Begin Calibration selects the overlay actor. If no eligible actors are selected,
	// fall back to the active calibration road actor (if available).
	if (TargetActors.Num() == 0 && ActiveCalibrationRoadActor.IsValid())
	{
		if (AActor* ActiveRoad = ActiveCalibrationRoadActor.Get())
		{
			if (IsEligibleRoadActor(ActiveRoad))
			{
				TargetActors.Add(ActiveRoad);
			}
		}
	}

	if (TargetActors.Num() == 0)
	{
		if (!bAutomationOrCmdlet)
		{
			FMessageDialog::Open(
				EAppMsgType::Ok,
				NSLOCTEXT("TrafficEditor", "ToggleReverse_NoSelection",
					"Select one or more spline road actors (e.g. CityBLD ramp pieces) first.\n\n"
					"Tip: if you just clicked BEGIN CALIBRATION, the overlay actor is selected. Click the actual road actor and try again."));
		}
		return;
	}

	int32 Toggled = 0;
	int32 MissingMeta = 0;
	FGuid RefreshFamily;
	bool bToggledActiveCalibrationRoad = false;

	const FScopedTransaction Tx(NSLOCTEXT("TrafficEditor", "ToggleReverseDirection", "AAA Traffic: Toggle Road Direction"));
	for (AActor* Actor : TargetActors)
	{
		if (!Actor)
		{
			continue;
		}

		UTrafficRoadMetadataComponent* Meta = Actor->FindComponentByClass<UTrafficRoadMetadataComponent>();
		if (!Meta)
		{
			// Be user-proof: attach metadata on demand for selected roads, so users don't have to re-run PREPARE MAP
			// after placing new CityBLD pieces.
			Meta = NewObject<UTrafficRoadMetadataComponent>(Actor);
			if (!Meta)
			{
				++MissingMeta;
				continue;
			}

			Meta->RegisterComponent();
			Actor->AddInstanceComponent(Meta);

			bool bCreated = false;
			FRoadFamilyInfo* FamilyInfo = Registry->FindOrCreateFamilyForActor(Actor, &bCreated);
			if (FamilyInfo)
			{
				Meta->Modify();
				Meta->RoadFamilyId = FamilyInfo->FamilyId;
				Meta->FamilyName = FamilyInfo->FamilyDefinition.FamilyName.IsNone()
					? FName(*FamilyInfo->DisplayName)
					: FamilyInfo->FamilyDefinition.FamilyName;
				Meta->bIncludeInTraffic = true;
				Meta->bLockReverseDirection = true;

				UE_LOG(LogTraffic, Log,
					TEXT("[TrafficEditor] ToggleReverse: Attached metadata to %s (Family=%s)."),
					*Actor->GetName(),
					*Meta->FamilyName.ToString());
			}
			else
			{
				++MissingMeta;
				continue;
			}
		}

		Meta->Modify();
		Meta->bReverseCenterlineDirection = !Meta->bReverseCenterlineDirection;
		Meta->bLockReverseDirection = true;
		++Toggled;

		if (!RefreshFamily.IsValid() && Meta->RoadFamilyId.IsValid())
		{
			RefreshFamily = Meta->RoadFamilyId;
		}

		UE_LOG(LogTraffic, Log,
			TEXT("[TrafficEditor] Toggled bReverseCenterlineDirection for %s -> %s"),
			*Actor->GetName(),
			Meta->bReverseCenterlineDirection ? TEXT("true") : TEXT("false"));

		if (ActiveCalibrationRoadActor.IsValid() && Actor == ActiveCalibrationRoadActor.Get())
		{
			bToggledActiveCalibrationRoad = true;
		}
	}

	if (MissingMeta > 0 && !bAutomationOrCmdlet)
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			NSLOCTEXT("TrafficEditor", "ToggleReverse_MissingMeta",
				"Some selected actors could not be updated.\n\n"
				"AAA Traffic tried to attach metadata automatically, but a few actors still failed.\n"
				"Try running PREPARE MAP and re-selecting the ramp actors."));
	}

	if (Toggled == 0)
	{
		return;
	}

	// Refresh overlay if the active calibration family is affected (so users immediately see arrow direction flip).
	if (bToggledActiveCalibrationRoad || (ActiveFamilyId.IsValid() && (RefreshFamily == ActiveFamilyId)))
	{
		Editor_BeginCalibrationForFamily(ActiveFamilyId);
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
        FamilyInfo = Registry->FindOrCreateFamilyForActor(RoadActor, nullptr);
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
