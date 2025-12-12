#include "TrafficCalibrationSubsystem.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "RoadLanePreviewActor.h"
#include "Components/SplineComponent.h"
#include "TrafficRoadFamilySettings.h"
#include "TrafficRoadMetadataComponent.h"
#include "TrafficRuntimeModule.h"
#include "TrafficNetworkBuilder.h"
#include "TrafficNetworkAsset.h"
#include "TrafficGeometryProviderFactory.h"
#include "TrafficGeometryProvider.h"
#include "EngineUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"

void UTrafficCalibrationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTraffic, Log, TEXT("TrafficCalibrationSubsystem initialized."));
}

void UTrafficCalibrationSubsystem::SpawnPreviewForSelected()
{
#if WITH_EDITOR
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("SpawnPreviewForSelected: No editor world."));
		return;
	}

	USelection* SelectedActors = GEditor->GetSelectedActors();
	if (!SelectedActors || SelectedActors->Num() == 0)
	{
		UE_LOG(LogTraffic, Warning, TEXT("SpawnPreviewForSelected: No selected actors."));
		return;
	}

	AActor* FirstActor = Cast<AActor>(SelectedActors->GetSelectedObject(0));
	if (!FirstActor)
	{
		UE_LOG(LogTraffic, Warning, TEXT("SpawnPreviewForSelected: First selected object is not an actor."));
		return;
	}

	USplineComponent* FoundSpline = FirstActor->FindComponentByClass<USplineComponent>();
	if (!FoundSpline)
	{
		UE_LOG(LogTraffic, Warning, TEXT("SpawnPreviewForSelected: Selected actor has no USplineComponent."));
		return;
	}

	const UTrafficRoadFamilySettings* FamSettings = GetDefault<UTrafficRoadFamilySettings>();
	if (!FamSettings || FamSettings->Families.Num() == 0)
	{
		UE_LOG(LogTraffic, Warning, TEXT("SpawnPreviewForSelected: No road families configured."));
		return;
	}

	const FRoadFamilyDefinition& Family = FamSettings->Families[0];

	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = MakeUniqueObjectName(World, ARoadLanePreviewActor::StaticClass(), TEXT("RoadLanePreview"));
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ARoadLanePreviewActor* PreviewActor = World->SpawnActor<ARoadLanePreviewActor>(
		ARoadLanePreviewActor::StaticClass(),
		FirstActor->GetActorLocation(),
		FRotator::ZeroRotator,
		SpawnParams);

	if (!PreviewActor)
	{
		UE_LOG(LogTraffic, Warning, TEXT("SpawnPreviewForSelected: Failed to spawn ARoadLanePreviewActor."));
		return;
	}

	PreviewActor->BuildPreviewFromSpline(FoundSpline, Family);
	UE_LOG(LogTraffic, Log, TEXT("SpawnPreviewForSelected: Preview built using family %s."), *Family.FamilyName.ToString());
#endif
}

UTrafficRoadMetadataComponent* UTrafficCalibrationSubsystem::GetOrAddRoadMetadata(AActor* Actor) const
{
	if (!Actor)
	{
		return nullptr;
	}

	UTrafficRoadMetadataComponent* Meta = Actor->FindComponentByClass<UTrafficRoadMetadataComponent>();
	if (!Meta)
	{
		Meta = NewObject<UTrafficRoadMetadataComponent>(Actor);
		Meta->RegisterComponent();
		Actor->AddInstanceComponent(Meta);
		UE_LOG(LogTraffic, Log, TEXT("Added UTrafficRoadMetadataComponent to actor %s"), *Actor->GetName());
	}
	return Meta;
}

void UTrafficCalibrationSubsystem::PrepareAllRoads()
{
#if WITH_EDITOR
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("PrepareAllRoads: No editor world."));
		return;
	}

	const UTrafficRoadFamilySettings* FamSettings = GetDefault<UTrafficRoadFamilySettings>();
	if (!FamSettings || FamSettings->Families.Num() == 0)
	{
		UE_LOG(LogTraffic, Warning, TEXT("PrepareAllRoads: No road families configured."));
		return;
	}

	const TArray<FRoadFamilyDefinition>& Families = FamSettings->Families;

	int32 ProcessedCount = 0;
	int32 AssignedDefaultCount = 0;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}

		USplineComponent* Spline = Actor->FindComponentByClass<USplineComponent>();
		if (!Spline)
		{
			continue;
		}

		UTrafficRoadMetadataComponent* Meta = GetOrAddRoadMetadata(Actor);
		if (!Meta)
		{
			continue;
		}

		++ProcessedCount;

		bool bNeedsAssignment = Meta->FamilyName.IsNone();
		if (!bNeedsAssignment)
		{
			const FRoadFamilyDefinition* Found = FamSettings->FindFamilyByName(Meta->FamilyName);
			if (!Found)
			{
				bNeedsAssignment = true;
			}
		}

		if (bNeedsAssignment)
		{
			Meta->FamilyName = Families[0].FamilyName;
			++AssignedDefaultCount;
			UE_LOG(LogTraffic, Log,
				TEXT("PrepareAllRoads: Assigned default family '%s' to actor %s"),
				*Families[0].FamilyName.ToString(),
				*Actor->GetName());
		}
	}

	UE_LOG(LogTraffic, Log,
		TEXT("PrepareAllRoads: Processed %d spline roads, assigned default family to %d."),
		ProcessedCount,
		AssignedDefaultCount);
#endif
}

void UTrafficCalibrationSubsystem::PreviewSelectedRoadFamily()
{
#if WITH_EDITOR
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("PreviewSelectedRoadFamily: No editor world."));
		return;
	}

	USelection* SelectedActors = GEditor->GetSelectedActors();
	if (!SelectedActors || SelectedActors->Num() == 0)
	{
		UE_LOG(LogTraffic, Warning, TEXT("PreviewSelectedRoadFamily: No selected actors."));
		return;
	}

	AActor* FirstActor = Cast<AActor>(SelectedActors->GetSelectedObject(0));
	if (!FirstActor)
	{
		UE_LOG(LogTraffic, Warning, TEXT("PreviewSelectedRoadFamily: First selected object is not an actor."));
		return;
	}

	USplineComponent* Spline = FirstActor->FindComponentByClass<USplineComponent>();
	if (!Spline)
	{
		UE_LOG(LogTraffic, Warning, TEXT("PreviewSelectedRoadFamily: Selected actor has no USplineComponent."));
		return;
	}

	const UTrafficRoadFamilySettings* FamSettings = GetDefault<UTrafficRoadFamilySettings>();
	if (!FamSettings || FamSettings->Families.Num() == 0)
	{
		UE_LOG(LogTraffic, Warning, TEXT("PreviewSelectedRoadFamily: No road families configured."));
		return;
	}

	UTrafficRoadMetadataComponent* Meta = FirstActor->FindComponentByClass<UTrafficRoadMetadataComponent>();
	if (!Meta || Meta->FamilyName.IsNone())
	{
		UE_LOG(LogTraffic, Warning,
			TEXT("PreviewSelectedRoadFamily: Actor %s has no metadata or FamilyName; running PrepareAllRoads."),
			*FirstActor->GetName());
		PrepareAllRoads();
		Meta = FirstActor->FindComponentByClass<UTrafficRoadMetadataComponent>();
	}

	const FRoadFamilyDefinition* Family = nullptr;
	if (Meta && !Meta->FamilyName.IsNone())
	{
		Family = FamSettings->FindFamilyByName(Meta->FamilyName);
	}

	if (!Family)
	{
		Family = &FamSettings->Families[0];
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = MakeUniqueObjectName(World, ARoadLanePreviewActor::StaticClass(), TEXT("RoadLanePreview"));
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ARoadLanePreviewActor* PreviewActor = World->SpawnActor<ARoadLanePreviewActor>(
		ARoadLanePreviewActor::StaticClass(),
		FirstActor->GetActorLocation(),
		FRotator::ZeroRotator,
		SpawnParams);

	if (!PreviewActor)
	{
		UE_LOG(LogTraffic, Warning, TEXT("PreviewSelectedRoadFamily: Failed to spawn ARoadLanePreviewActor."));
		return;
	}

	PreviewActor->BuildPreviewFromSpline(Spline, *Family);
	UE_LOG(LogTraffic, Log,
		TEXT("PreviewSelectedRoadFamily: Built preview for actor %s with family %s."),
		*FirstActor->GetName(),
		*Family->FamilyName.ToString());
#endif
}

void UTrafficCalibrationSubsystem::Editor_BuildTrafficNetwork()
{
#if WITH_EDITOR
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficCalibrationSubsystem] Editor_BuildTrafficNetwork: World is null."));
		return;
	}

	UE_LOG(LogTraffic, Log, TEXT("[TrafficCalibrationSubsystem] Editor_BuildTrafficNetwork: BEGIN"));

	const UTrafficRoadFamilySettings* FamSettings = GetDefault<UTrafficRoadFamilySettings>();
	if (!FamSettings || FamSettings->Families.Num() == 0)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficCalibrationSubsystem] No road families configured."));
		return;
	}

	ITrafficRoadGeometryProvider* ProviderInterface = nullptr;
	TObjectPtr<UObject> ProviderObject = UTrafficGeometryProviderFactory::CreateProvider(World, ProviderInterface);
	if (!ProviderObject || !ProviderInterface)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficCalibrationSubsystem] Failed to create geometry provider."));
		return;
	}

	FTrafficNetwork Network;
	FTrafficNetworkBuilder::BuildNetworkFromWorld(
		World,
		*ProviderInterface,
		FamSettings,
		Network);

	UE_LOG(LogTraffic, Log,
		TEXT("[TrafficCalibrationSubsystem] Editor_BuildTrafficNetwork: END - Roads=%d Lanes=%d Intersections=%d Movements=%d"),
		Network.Roads.Num(),
		Network.Lanes.Num(),
		Network.Intersections.Num(),
		Network.Movements.Num());
#endif
}

void UTrafficCalibrationSubsystem::Editor_BuildTrafficNetworkToAsset(const FString& AssetPath)
{
#if WITH_EDITOR
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficCalibrationSubsystem] Editor_BuildTrafficNetworkToAsset: World is null."));
		return;
	}

	FString FinalAssetPath = AssetPath;
	if (FinalAssetPath.IsEmpty())
	{
		FinalAssetPath = TEXT("/Game/Traffic/TrafficNetwork");
	}

	UE_LOG(LogTraffic, Log, TEXT("[TrafficCalibrationSubsystem] Editor_BuildTrafficNetworkToAsset: BEGIN -> %s"), *FinalAssetPath);

	const UTrafficRoadFamilySettings* FamSettings = GetDefault<UTrafficRoadFamilySettings>();
	if (!FamSettings || FamSettings->Families.Num() == 0)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficCalibrationSubsystem] No road families configured."));
		return;
	}

	ITrafficRoadGeometryProvider* ProviderInterface = nullptr;
	TObjectPtr<UObject> ProviderObject = UTrafficGeometryProviderFactory::CreateProvider(World, ProviderInterface);
	if (!ProviderObject || !ProviderInterface)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficCalibrationSubsystem] Failed to create geometry provider."));
		return;
	}

	FTrafficNetwork Network;
	FTrafficNetworkBuilder::BuildNetworkFromWorld(
		World,
		*ProviderInterface,
		FamSettings,
		Network);

	FString PackageName = FinalAssetPath;
	FString AssetName = FPackageName::GetLongPackageAssetName(FinalAssetPath);

	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficCalibrationSubsystem] Failed to create package: %s"), *PackageName);
		return;
	}

	UTrafficNetworkAsset* Asset = NewObject<UTrafficNetworkAsset>(Package, *AssetName, RF_Public | RF_Standalone);
	if (!Asset)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficCalibrationSubsystem] Failed to create UTrafficNetworkAsset."));
		return;
	}

	Asset->Network = Network;
	Asset->MarkPackageDirty();

	FAssetRegistryModule::AssetCreated(Asset);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	const FString PackageFilename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	const bool bSaved = UPackage::SavePackage(Package, Asset, *PackageFilename, SaveArgs);

	if (bSaved)
	{
		UE_LOG(LogTraffic, Log,
			TEXT("[TrafficCalibrationSubsystem] Saved TrafficNetworkAsset: %s - Roads=%d Lanes=%d Intersections=%d Movements=%d"),
			*PackageFilename,
			Network.Roads.Num(),
			Network.Lanes.Num(),
			Network.Intersections.Num(),
			Network.Movements.Num());
	}
	else
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficCalibrationSubsystem] Failed to save package: %s"), *PackageFilename);
	}
#endif
}
