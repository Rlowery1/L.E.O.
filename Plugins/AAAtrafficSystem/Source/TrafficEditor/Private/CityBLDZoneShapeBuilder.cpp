#include "CityBLDZoneShapeBuilder.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"

#include "TrafficZoneLaneProfile.h"
#include "TrafficRuntimeModule.h"

#include "ZoneGraphData.h"
#include "ZoneGraphDelegates.h"
#include "ZoneGraphSettings.h"
#include "ZoneGraphSubsystem.h"
#include "ZoneShapeActor.h"
#include "ZoneShapeComponent.h"

namespace CityBLDZoneShapeBuilder
{
	bool IsCityBLDRoadActor(const AActor* Actor)
	{
		if (!Actor)
		{
			return false;
		}

		return Actor->GetClass()->GetName().Contains(TEXT("BP_MeshRoad"), ESearchCase::IgnoreCase);
	}

#if WITH_EDITOR
	namespace
	{
		static const FName TrafficCityBLDCalibrationZoneShapeTag(TEXT("AAA_CityBLD_CalibrationZoneGraph"));

		UObject* LoadSoftObjectSync(const FSoftObjectPath& Path)
		{
			if (!Path.IsValid())
			{
				return nullptr;
			}

			FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
			return Streamable.LoadSynchronous(Path, /*bManageActiveHandle*/ false);
		}

		FZoneGraphTag FindOrAddZoneTag(UZoneGraphSettings* Settings, const FName TagName, const FColor& DefaultColor)
		{
			if (!Settings || TagName.IsNone())
			{
				return FZoneGraphTag::None;
			}

			for (const FZoneGraphTagInfo& Info : Settings->GetTagInfos())
			{
				if (Info.Name == TagName)
				{
					return Info.Tag;
				}
			}

			FProperty* TagsProp = FindFProperty<FProperty>(UZoneGraphSettings::StaticClass(), TEXT("Tags"));
			FStructProperty* TagStructProp = CastField<FStructProperty>(TagsProp);
			if (!TagStructProp || TagStructProp->Struct != FZoneGraphTagInfo::StaticStruct())
			{
				return FZoneGraphTag::None;
			}

			const int32 NumTags = TagsProp->ArrayDim;
			for (int32 Index = 0; Index < NumTags; ++Index)
			{
				FZoneGraphTagInfo* TagInfo = TagsProp->ContainerPtrToValuePtr<FZoneGraphTagInfo>(Settings, Index);
				if (!TagInfo)
				{
					continue;
				}

				if (TagInfo->Name.IsNone())
				{
					TagInfo->Name = TagName;
					TagInfo->Color = DefaultColor;
					TagInfo->Tag = FZoneGraphTag(static_cast<uint8>(Index));
					UE::ZoneGraphDelegates::OnZoneGraphTagsChanged.Broadcast();
					return TagInfo->Tag;
				}
			}

			return FZoneGraphTag::None;
		}

		bool GetZoneGraphLaneProfilesArray(UZoneGraphSettings* Settings, FArrayProperty*& OutArrayProp, void*& OutArrayPtr)
		{
			OutArrayProp = nullptr;
			OutArrayPtr = nullptr;

			if (!Settings)
			{
				return false;
			}

			FArrayProperty* LaneProfilesProp = FindFProperty<FArrayProperty>(UZoneGraphSettings::StaticClass(), TEXT("LaneProfiles"));
			if (!LaneProfilesProp)
			{
				return false;
			}

			OutArrayProp = LaneProfilesProp;
			OutArrayPtr = LaneProfilesProp->ContainerPtrToValuePtr<void>(Settings);
			return OutArrayPtr != nullptr;
		}

		FZoneLaneProfileRef FindOrAddLaneProfileFromAsset(
			UZoneGraphSettings* ZoneSettings,
			const UTrafficZoneLaneProfile* ProfileAsset,
			const FZoneGraphTag LaneTag)
		{
			if (!ZoneSettings || !ProfileAsset)
			{
				return FZoneLaneProfileRef();
			}

			const FName ProfileName = !ProfileAsset->ProfileName.IsNone()
				? ProfileAsset->ProfileName
				: FName(*ProfileAsset->GetName());

			const FZoneGraphTagMask LaneTags = LaneTag.IsValid()
				? FZoneGraphTagMask(LaneTag)
				: FZoneGraphTagMask::None;

			FGuid ExistingId;
			for (const FZoneLaneProfile& Existing : ZoneSettings->GetLaneProfiles())
			{
				if (Existing.Name == ProfileName)
				{
					ExistingId = Existing.ID;
					break;
				}
			}

			FZoneLaneProfile NewProfile;
			NewProfile.Name = ProfileName;
			if (ExistingId.IsValid())
			{
				NewProfile.ID = ExistingId;
			}

			NewProfile.Lanes.Reset();
			NewProfile.Lanes.Reserve(ProfileAsset->NumLanes);

			for (int32 i = 0; i < ProfileAsset->NumLanes; ++i)
			{
				FZoneLaneDesc Lane;
				Lane.Width = ProfileAsset->LaneWidthCm;
				Lane.Direction = ProfileAsset->Direction;
				Lane.Tags = LaneTags;
				NewProfile.Lanes.Add(Lane);
			}

			FArrayProperty* LaneProfilesProp = nullptr;
			void* LaneProfilesPtr = nullptr;
			if (!GetZoneGraphLaneProfilesArray(ZoneSettings, LaneProfilesProp, LaneProfilesPtr))
			{
				return FZoneLaneProfileRef();
			}

			FScriptArrayHelper Helper(LaneProfilesProp, LaneProfilesPtr);
			FStructProperty* InnerStruct = CastField<FStructProperty>(LaneProfilesProp->Inner);
			if (!InnerStruct || InnerStruct->Struct != FZoneLaneProfile::StaticStruct())
			{
				return FZoneLaneProfileRef();
			}

			for (int32 Index = 0; Index < Helper.Num(); ++Index)
			{
				FZoneLaneProfile* Existing = reinterpret_cast<FZoneLaneProfile*>(Helper.GetRawPtr(Index));
				if (Existing && Existing->Name == ProfileName)
				{
					*Existing = NewProfile;
					UE::ZoneGraphDelegates::OnZoneGraphLaneProfileChanged.Broadcast(FZoneLaneProfileRef(*Existing));
					return FZoneLaneProfileRef(*Existing);
				}
			}

			const int32 NewIndex = Helper.AddValue();
			FZoneLaneProfile* Added = reinterpret_cast<FZoneLaneProfile*>(Helper.GetRawPtr(NewIndex));
			if (!Added)
			{
				return FZoneLaneProfileRef();
			}

			*Added = NewProfile;
			UE::ZoneGraphDelegates::OnZoneGraphLaneProfileChanged.Broadcast(FZoneLaneProfileRef(*Added));
			return FZoneLaneProfileRef(*Added);
		}

		static FString SanitizeRoadNameForLaneProfile(const FString& RoadName)
		{
			FString Name = RoadName;

			const int32 UAIDIndex = Name.Find(TEXT("_UAID_"), ESearchCase::IgnoreCase, ESearchDir::FromStart);
			if (UAIDIndex != INDEX_NONE)
			{
				Name = Name.Left(UAIDIndex);
			}

			// Strip a trailing instance suffix like "_01" to avoid generating profiles per-instance.
			{
				int32 CharIndex = Name.Len() - 1;
				while (CharIndex >= 0 && FChar::IsDigit(Name[CharIndex]))
				{
					--CharIndex;
				}

				const bool bHadTrailingDigits = (CharIndex >= 0 && CharIndex < Name.Len() - 1);
				if (bHadTrailingDigits)
				{
					int32 RemoveFrom = CharIndex + 1;
					if (Name[CharIndex] == TEXT('_') || Name[CharIndex] == TEXT('-'))
					{
						RemoveFrom = CharIndex;
					}
					Name = Name.Left(RemoveFrom);
				}
			}

			Name.ReplaceInline(TEXT(" "), TEXT("_"));

			for (int32 i = 0; i < Name.Len(); ++i)
			{
				TCHAR& Ch = Name[i];
				if (!(FChar::IsAlnum(Ch) || Ch == TEXT('_')))
				{
					Ch = TEXT('_');
				}
			}

			while (Name.Contains(TEXT("__")))
			{
				Name.ReplaceInline(TEXT("__"), TEXT("_"));
			}

			Name.TrimCharInline(TEXT('_'), nullptr);
			return Name.IsEmpty() ? TEXT("Road") : Name;
		}

		/**
		 * Determine lane configuration for a CityBLD road based off its name.
		 * Defaults to one lane per side with a 3.5m width.
		 */
		static void GetLaneConfigFromRoadName(const FString& RoadName, int32& OutLanesPerSide, float& OutLaneWidthCm)
		{
			OutLanesPerSide = 1;
			OutLaneWidthCm = 350.f;

			if (RoadName.Contains(TEXT("FourLane"), ESearchCase::IgnoreCase) ||
				RoadName.Contains(TEXT("HighwayConnector_2Lane"), ESearchCase::IgnoreCase))
			{
				OutLanesPerSide = 2;
			}
			else if (RoadName.Contains(TEXT("TwoLane"), ESearchCase::IgnoreCase))
			{
				OutLanesPerSide = 1;
			}
			else if (RoadName.Contains(TEXT("SuburbanStreet"), ESearchCase::IgnoreCase))
			{
				OutLanesPerSide = 1;
			}
		}

		/**
		 * Create a lane profile directly in ZoneGraph settings for a given road name.
		 * This bypasses the need for a lane profile asset. All lanes use the same width and tag.
		 */
		static FZoneLaneProfileRef CreateAutoLaneProfileForRoad(
			UZoneGraphSettings* ZoneSettings,
			const FString& RoadName,
			const FZoneGraphTag VehiclesTag)
		{
			if (!ZoneSettings)
			{
				return FZoneLaneProfileRef();
			}

			int32 LanesPerSide = 1;
			float LaneWidthCm = 350.f;
			GetLaneConfigFromRoadName(RoadName, LanesPerSide, LaneWidthCm);
			LanesPerSide = FMath::Clamp(LanesPerSide, 1, 8);
			LaneWidthCm = FMath::Clamp(LaneWidthCm, 10.f, 2000.f);

			const FZoneGraphTagMask LaneTags = VehiclesTag.IsValid()
				? FZoneGraphTagMask(VehiclesTag)
				: FZoneGraphTagMask::None;

			const FString SanitizedRoadName = SanitizeRoadNameForLaneProfile(RoadName);
			const FString ProfileNameString = FString::Printf(TEXT("AutoProfile_%s_%dLane"), *SanitizedRoadName, LanesPerSide);
			const FName ProfileName(*ProfileNameString);

			for (const FZoneLaneProfile& Existing : ZoneSettings->GetLaneProfiles())
			{
				if (Existing.Name == ProfileName)
				{
					return FZoneLaneProfileRef(Existing);
				}
			}

			FZoneLaneProfile NewProfile;
			NewProfile.Name = ProfileName;
			NewProfile.Lanes.Reset();
			NewProfile.Lanes.Reserve(LanesPerSide * 2);

			for (int32 i = 0; i < LanesPerSide; ++i)
			{
				FZoneLaneDesc Lane;
				Lane.Width = LaneWidthCm;
				Lane.Direction = EZoneLaneDirection::Forward;
				Lane.Tags = LaneTags;
				NewProfile.Lanes.Add(Lane);
			}

			for (int32 i = 0; i < LanesPerSide; ++i)
			{
				FZoneLaneDesc Lane;
				Lane.Width = LaneWidthCm;
				Lane.Direction = EZoneLaneDirection::Backward;
				Lane.Tags = LaneTags;
				NewProfile.Lanes.Add(Lane);
			}

			FArrayProperty* LaneProfilesProp = nullptr;
			void* LaneProfilesPtr = nullptr;
			if (!GetZoneGraphLaneProfilesArray(ZoneSettings, LaneProfilesProp, LaneProfilesPtr))
			{
				return FZoneLaneProfileRef();
			}

			FScriptArrayHelper Helper(LaneProfilesProp, LaneProfilesPtr);
			FStructProperty* InnerStruct = CastField<FStructProperty>(LaneProfilesProp->Inner);
			if (!InnerStruct || InnerStruct->Struct != FZoneLaneProfile::StaticStruct())
			{
				return FZoneLaneProfileRef();
			}

			const int32 NewIndex = Helper.AddValue();
			FZoneLaneProfile* Added = reinterpret_cast<FZoneLaneProfile*>(Helper.GetRawPtr(NewIndex));
			if (!Added)
			{
				return FZoneLaneProfileRef();
			}

			*Added = NewProfile;
			UE::ZoneGraphDelegates::OnZoneGraphLaneProfileChanged.Broadcast(FZoneLaneProfileRef(*Added));
			return FZoneLaneProfileRef(*Added);
		}

		void ClearExistingCalibrationZoneShapes(UWorld* World)
		{
			if (!World)
			{
				return;
			}

			TArray<AActor*> ToDestroy;
			for (TActorIterator<AZoneShape> It(World); It; ++It)
			{
				AZoneShape* ShapeActor = *It;
				if (ShapeActor && ShapeActor->ActorHasTag(TrafficCityBLDCalibrationZoneShapeTag))
				{
					ToDestroy.Add(ShapeActor);
				}
			}

			for (AActor* Actor : ToDestroy)
			{
				if (Actor)
				{
					Actor->Destroy();
				}
			}
		}

		UZoneShapeComponent* GetZoneShapeComponent(AZoneShape* ShapeActor)
		{
			return ShapeActor ? ShapeActor->FindComponentByClass<UZoneShapeComponent>() : nullptr;
		}

		void SetSplinePoints(UZoneShapeComponent* ShapeComp, const TArray<FVector>& PointsWS)
		{
			if (!ShapeComp || PointsWS.Num() < 2)
			{
				return;
			}

			ShapeComp->SetShapeType(FZoneShapeType::Spline);
			TArray<FZoneShapePoint>& ShapePoints = ShapeComp->GetMutablePoints();
			ShapePoints.Reset();
			ShapePoints.Reserve(PointsWS.Num());

			for (int32 i = 0; i < PointsWS.Num(); ++i)
			{
				FZoneShapePoint Pt(PointsWS[i]);
				Pt.Type = (i == 0 || i == PointsWS.Num() - 1)
					? FZoneShapePointType::Sharp
					: FZoneShapePointType::AutoBezier;
				ShapePoints.Add(Pt);
			}

			ShapeComp->UpdateShape();
		}

		bool EnsureAtLeastOneZoneGraphDataActor(UWorld* World)
		{
			// Deprecated: use EnsureCalibrationZoneGraphDataActorForLevel(). Kept for reference only.
			return World != nullptr;
		}

		AZoneGraphData* EnsureCalibrationZoneGraphDataActorForLevel(UWorld* World, ULevel* Level)
		{
			if (!World)
			{
				return nullptr;
			}

			ULevel* TargetLevel = Level ? Level : World->PersistentLevel.Get();

			for (TActorIterator<AZoneGraphData> It(World); It; ++It)
			{
				AZoneGraphData* DataActor = *It;
				if (!DataActor)
				{
					continue;
				}

				// Only reuse transient, calibration-tagged data actors so we never mutate user-authored ZoneGraphData.
				if (!DataActor->HasAnyFlags(RF_Transient) ||
					!DataActor->ActorHasTag(TrafficCityBLDCalibrationZoneShapeTag))
				{
					continue;
				}

				if (DataActor->GetLevel() == TargetLevel)
				{
					return DataActor;
				}
			}

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.ObjectFlags |= RF_Transient;
			Params.OverrideLevel = TargetLevel;
#if WITH_EDITOR
			Params.bTemporaryEditorActor = true;
			Params.bHideFromSceneOutliner = true;
#endif

			AZoneGraphData* DataActor = World->SpawnActor<AZoneGraphData>(AZoneGraphData::StaticClass(), FTransform::Identity, Params);
			if (!DataActor)
			{
				return nullptr;
			}

			DataActor->Tags.AddUnique(TrafficCityBLDCalibrationZoneShapeTag);
			return DataActor;
		}

		void CollectZoneGraphDataActors(UWorld* World, TArray<AZoneGraphData*>& OutDataActors)
		{
			OutDataActors.Reset();
			if (!World)
			{
				return;
			}

			for (TActorIterator<AZoneGraphData> It(World); It; ++It)
			{
				if (AZoneGraphData* DataActor = *It)
				{
					OutDataActors.Add(DataActor);
				}
			}
		}
	}

	bool BuildVehicleZoneGraphForCityBLDRoad(
		UWorld* World,
		const AActor* RoadActor,
		const TArray<FVector>& RoadCenterlinePoints,
		const FSoftObjectPath& VehicleLaneProfileAssetPath)
	{
		if (!World || !RoadActor || !IsCityBLDRoadActor(RoadActor) || RoadCenterlinePoints.Num() < 2)
		{
			return false;
		}

		if (World->WorldType != EWorldType::Editor && World->WorldType != EWorldType::PIE)
		{
			return false;
		}

		UZoneGraphSubsystem* ZGS = World->GetSubsystem<UZoneGraphSubsystem>();
		if (!ZGS)
		{
			UE_LOG(LogTraffic, Warning, TEXT("[CityBLDZoneShapeBuilder] ZoneGraphSubsystem missing. Enable ZoneGraph plugin."));
			return false;
		}

		UZoneGraphSettings* ZoneSettings = GetMutableDefault<UZoneGraphSettings>();
		if (!ZoneSettings)
		{
			UE_LOG(LogTraffic, Warning, TEXT("[CityBLDZoneShapeBuilder] ZoneGraphSettings not available."));
			return false;
		}

		const FZoneGraphTag VehiclesTag = FindOrAddZoneTag(ZoneSettings, FName(TEXT("Vehicles")), FColor(80, 200, 120));
		if (!VehiclesTag.IsValid())
		{
			UE_LOG(LogTraffic, Warning, TEXT("[CityBLDZoneShapeBuilder] ZoneGraph tag 'Vehicles' not found/created; lane filtering may not work."));
		}

		FZoneLaneProfileRef LaneProfileRef;
		const UTrafficZoneLaneProfile* ProfileAsset = Cast<UTrafficZoneLaneProfile>(LoadSoftObjectSync(VehicleLaneProfileAssetPath));
		if (ProfileAsset)
		{
			LaneProfileRef = FindOrAddLaneProfileFromAsset(ZoneSettings, ProfileAsset, VehiclesTag);
		}
		else
		{
			UE_LOG(LogTraffic, Warning,
				TEXT("[CityBLDZoneShapeBuilder] Failed to load UTrafficZoneLaneProfile at '%s'. Creating auto lane profile."),
				*VehicleLaneProfileAssetPath.ToString());

			LaneProfileRef = CreateAutoLaneProfileForRoad(ZoneSettings, RoadActor->GetName(), VehiclesTag);
		}

		if (!LaneProfileRef.ID.IsValid())
		{
			UE_LOG(LogTraffic, Warning,
				TEXT("[CityBLDZoneShapeBuilder] No valid lane profile available; ZoneGraph lanes may use defaults."));
		}

		ClearExistingCalibrationZoneShapes(World);

		AZoneGraphData* CalibrationDataActor = EnsureCalibrationZoneGraphDataActorForLevel(World, RoadActor->GetLevel());
		if (!CalibrationDataActor)
		{
			UE_LOG(LogTraffic, Warning, TEXT("[CityBLDZoneShapeBuilder] Failed to ensure a calibration ZoneGraphData actor exists in the road's level."));
			return false;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.ObjectFlags |= RF_Transient;
		SpawnParams.OverrideLevel = RoadActor->GetLevel();
#if WITH_EDITOR
		SpawnParams.bTemporaryEditorActor = true;
		SpawnParams.bHideFromSceneOutliner = true;
#endif

		AZoneShape* ShapeActor = World->SpawnActor<AZoneShape>(AZoneShape::StaticClass(), FTransform::Identity, SpawnParams);
		if (!ShapeActor)
		{
			return false;
		}

		ShapeActor->Tags.AddUnique(TrafficCityBLDCalibrationZoneShapeTag);

		UZoneShapeComponent* ShapeComp = GetZoneShapeComponent(ShapeActor);
		if (!ShapeComp)
		{
			ShapeActor->Destroy();
			return false;
		}

		if (LaneProfileRef.ID.IsValid())
		{
			ShapeComp->SetCommonLaneProfile(LaneProfileRef);
			ShapeComp->SetReverseLaneProfile(false);
		}

		FZoneGraphTagMask ShapeTags = FZoneGraphTagMask::None;
		if (VehiclesTag.IsValid())
		{
			ShapeTags.Add(VehiclesTag);
		}
		ShapeComp->SetTags(ShapeTags);

		SetSplinePoints(ShapeComp, RoadCenterlinePoints);

		TArray<AZoneGraphData*> DataActors;
		DataActors.Add(CalibrationDataActor);

		ZGS->GetBuilder().BuildAll(DataActors, /*bForceRebuild=*/true);

		UE_LOG(LogTraffic, Log,
			TEXT("[CityBLDZoneShapeBuilder] Built transient ZoneShape + ZoneGraph for '%s' (Points=%d Profile=%s)."),
			*GetNameSafe(RoadActor),
			RoadCenterlinePoints.Num(),
			*VehicleLaneProfileAssetPath.ToString());

		return true;
	}
#endif
}
