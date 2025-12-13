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
			if (!World)
			{
				return false;
			}

			for (TActorIterator<AZoneGraphData> It(World); It; ++It)
			{
				if (*It)
				{
					return true;
				}
			}

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.ObjectFlags |= RF_Transient;

			AZoneGraphData* DataActor = World->SpawnActor<AZoneGraphData>(AZoneGraphData::StaticClass(), FTransform::Identity, Params);
			if (!DataActor)
			{
				return false;
			}

			DataActor->Tags.AddUnique(TrafficCityBLDCalibrationZoneShapeTag);
			return true;
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

		const UTrafficZoneLaneProfile* ProfileAsset = Cast<UTrafficZoneLaneProfile>(LoadSoftObjectSync(VehicleLaneProfileAssetPath));
		if (!ProfileAsset)
		{
			UE_LOG(LogTraffic, Warning,
				TEXT("[CityBLDZoneShapeBuilder] Failed to load UTrafficZoneLaneProfile at '%s'."),
				*VehicleLaneProfileAssetPath.ToString());
		}

		FZoneLaneProfileRef LaneProfileRef;
		if (ProfileAsset)
		{
			LaneProfileRef = FindOrAddLaneProfileFromAsset(ZoneSettings, ProfileAsset, VehiclesTag);
		}

		ClearExistingCalibrationZoneShapes(World);

		if (!EnsureAtLeastOneZoneGraphDataActor(World))
		{
			UE_LOG(LogTraffic, Warning, TEXT("[CityBLDZoneShapeBuilder] Failed to ensure a ZoneGraphData actor exists."));
			return false;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.ObjectFlags |= RF_Transient;

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
		CollectZoneGraphDataActors(World, DataActors);
		if (DataActors.Num() == 0)
		{
			return false;
		}

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
