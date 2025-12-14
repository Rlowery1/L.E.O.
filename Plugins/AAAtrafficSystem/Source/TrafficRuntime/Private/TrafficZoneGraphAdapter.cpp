// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrafficZoneGraphAdapter.h"

#include "TrafficRuntimeModule.h"
#include "TrafficRoadFamilySettings.h"
#include "TrafficZoneLaneProfile.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/UnrealType.h"

#include "ZoneGraphDelegates.h"
#include "ZoneGraphSettings.h"
#include "ZoneGraphSubsystem.h"
#include "ZoneShapeActor.h"
#include "ZoneShapeComponent.h"

namespace
{
	static const FName TrafficZoneGraphTag(TEXT("TrafficZoneGraph"));

	static UObject* LoadSoftObjectSync(const FSoftObjectPath& Path)
	{
		if (!Path.IsValid())
		{
			return nullptr;
		}

		FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
		return Streamable.LoadSynchronous(Path, /*bManageActiveHandle*/ false);
	}

	static FZoneGraphTag FindOrAddZoneTag(UZoneGraphSettings* Settings, const FName TagName, const FColor& DefaultColor)
	{
		if (!Settings || TagName.IsNone())
		{
			return FZoneGraphTag::None;
		}

		// First: look for an existing tag by name.
		for (const FZoneGraphTagInfo& Info : Settings->GetTagInfos())
		{
			if (Info.Name == TagName)
			{
				return Info.Tag;
			}
		}

		// Second: try to add one via reflection (UZoneGraphSettings::Tags is protected).
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
#if WITH_EDITOR
				UE::ZoneGraphDelegates::OnZoneGraphTagsChanged.Broadcast();
#endif
				return TagInfo->Tag;
			}
		}

		return FZoneGraphTag::None;
	}

	static bool GetZoneGraphLaneProfilesArray(UZoneGraphSettings* Settings, FArrayProperty*& OutArrayProp, void*& OutArrayPtr)
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

	static FZoneLaneProfileRef FindOrAddLaneProfileFromAsset(
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

		// Look for existing profile by name.
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
		static const FName VehiclesLaneTagName(TEXT("Vehicles"));
		const bool bIsCityBLDVehicleProfile =
			ProfileName.ToString().Contains(TEXT("CityBLD"), ESearchCase::IgnoreCase) &&
			ProfileAsset->LaneTagName == VehiclesLaneTagName;

		if (bIsCityBLDVehicleProfile)
		{
			// CityBLD profiles are authored as "lanes per direction". Generate opposing lanes automatically.
			const int32 LanesPerDirection = FMath::Clamp(ProfileAsset->NumLanes, 1, 4);
			NewProfile.Lanes.Reserve(LanesPerDirection * 2);

			const EZoneLaneDirection ForwardDir = ProfileAsset->Direction;
			const EZoneLaneDirection BackwardDir = (ForwardDir == EZoneLaneDirection::Forward)
				? EZoneLaneDirection::Backward
				: (ForwardDir == EZoneLaneDirection::Backward)
					? EZoneLaneDirection::Forward
					: ForwardDir;

			for (int32 i = 0; i < LanesPerDirection; ++i)
			{
				FZoneLaneDesc Lane;
				Lane.Width = ProfileAsset->LaneWidthCm;
				Lane.Direction = ForwardDir;
				Lane.Tags = LaneTags;
				NewProfile.Lanes.Add(Lane);
			}

			for (int32 i = 0; i < LanesPerDirection; ++i)
			{
				FZoneLaneDesc Lane;
				Lane.Width = ProfileAsset->LaneWidthCm;
				Lane.Direction = BackwardDir;
				Lane.Tags = LaneTags;
				NewProfile.Lanes.Add(Lane);
			}
		}
		else
		{
			const int32 NumLanes = FMath::Clamp(ProfileAsset->NumLanes, 1, 8);
			NewProfile.Lanes.Reserve(NumLanes);

			for (int32 i = 0; i < NumLanes; ++i)
			{
				FZoneLaneDesc Lane;
				Lane.Width = ProfileAsset->LaneWidthCm;
				Lane.Direction = ProfileAsset->Direction;
				Lane.Tags = LaneTags;
				NewProfile.Lanes.Add(Lane);
			}
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

		// Update in-place if found.
		for (int32 Index = 0; Index < Helper.Num(); ++Index)
		{
			FZoneLaneProfile* Existing = reinterpret_cast<FZoneLaneProfile*>(Helper.GetRawPtr(Index));
			if (Existing && Existing->Name == ProfileName)
			{
				*Existing = NewProfile;
#if WITH_EDITOR
				UE::ZoneGraphDelegates::OnZoneGraphLaneProfileChanged.Broadcast(FZoneLaneProfileRef(*Existing));
#endif
				return FZoneLaneProfileRef(*Existing);
			}
		}

		// Add a new profile.
		const int32 NewIndex = Helper.AddValue();
		FZoneLaneProfile* Added = reinterpret_cast<FZoneLaneProfile*>(Helper.GetRawPtr(NewIndex));
		if (!Added)
		{
			return FZoneLaneProfileRef();
		}

		*Added = NewProfile;

#if WITH_EDITOR
		UE::ZoneGraphDelegates::OnZoneGraphLaneProfileChanged.Broadcast(FZoneLaneProfileRef(*Added));
#endif
		return FZoneLaneProfileRef(*Added);
	}

	static void ClearExistingTrafficZoneShapes(UWorld* World)
	{
		if (!World)
		{
			return;
		}

		TArray<AActor*> ToDestroy;
		for (TActorIterator<AZoneShape> It(World); It; ++It)
		{
			AZoneShape* ShapeActor = *It;
			if (ShapeActor && ShapeActor->ActorHasTag(TrafficZoneGraphTag))
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

	static UZoneShapeComponent* GetZoneShapeComponent(AZoneShape* ShapeActor)
	{
		return ShapeActor ? ShapeActor->FindComponentByClass<UZoneShapeComponent>() : nullptr;
	}

	static void SetSplinePoints(UZoneShapeComponent* ShapeComp, const TArray<FVector>& PointsWS)
	{
		if (!ShapeComp)
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

	static void SetPolygonPointsCircle(UZoneShapeComponent* ShapeComp, const FVector CenterWS, const float RadiusCm, int32 NumSides)
	{
		if (!ShapeComp)
		{
			return;
		}

		ShapeComp->SetShapeType(FZoneShapeType::Polygon);
		ShapeComp->SetPolygonRoutingType(EZoneShapePolygonRoutingType::Arcs);

		NumSides = FMath::Clamp(NumSides, 3, 24);
		const float Radius = FMath::Max(50.f, RadiusCm);

		TArray<FZoneShapePoint>& ShapePoints = ShapeComp->GetMutablePoints();
		ShapePoints.Reset();
		ShapePoints.Reserve(NumSides);

		for (int32 i = 0; i < NumSides; ++i)
		{
			const float Angle = (2.f * PI) * (static_cast<float>(i) / static_cast<float>(NumSides));
			const FVector P = CenterWS + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
			FZoneShapePoint Pt(P);
			Pt.Type = FZoneShapePointType::Sharp;
			ShapePoints.Add(Pt);
		}

		ShapeComp->UpdateShape();
	}

	static const FRoadFamilyDefinition* GetFamilyForRoad(const UTrafficRoadFamilySettings* FamilySettings, const FTrafficRoad& Road)
	{
		if (!FamilySettings || FamilySettings->Families.Num() == 0)
		{
			return nullptr;
		}

		const int32 FamilyIndex = (Road.FamilyId >= 0 && Road.FamilyId < FamilySettings->Families.Num()) ? Road.FamilyId : 0;
		return FamilySettings->Families.IsValidIndex(FamilyIndex) ? &FamilySettings->Families[FamilyIndex] : nullptr;
	}
}

void FTrafficZoneGraphAdapter::BuildZoneGraphForNetwork(
	UWorld* World,
	const FTrafficNetwork& Network,
	const UTrafficRoadFamilySettings* FamilySettings)
{
#if !WITH_EDITOR
	UE_LOG(LogTraffic, Warning, TEXT("[TrafficZoneGraphAdapter] ZoneGraph generation is editor-only (WITH_EDITOR=0)."));
	return;
#else
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficZoneGraphAdapter] World is null."));
		return;
	}

	// Zone shapes are editor-only authoring actors. Only attempt this in Editor/PIE worlds.
	if (World->WorldType != EWorldType::Editor && World->WorldType != EWorldType::PIE)
	{
		UE_LOG(LogTraffic, Verbose, TEXT("[TrafficZoneGraphAdapter] Skipping ZoneGraph generation for world type %d."), (int32)World->WorldType);
		return;
	}

	UZoneGraphSubsystem* ZGS = World->GetSubsystem<UZoneGraphSubsystem>();
	if (!ZGS)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficZoneGraphAdapter] ZoneGraphSubsystem not available. Is the ZoneGraph plugin enabled?"));
		return;
	}

	UZoneGraphSettings* ZoneSettings = GetMutableDefault<UZoneGraphSettings>();
	if (!ZoneSettings)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficZoneGraphAdapter] ZoneGraphSettings not available."));
		return;
	}

	// Destroy any previously spawned traffic shapes so we don't duplicate on rebuild.
	ClearExistingTrafficZoneShapes(World);

	const FZoneGraphTag VehiclesTag = FindOrAddZoneTag(ZoneSettings, FName(TEXT("Vehicles")), FColor(80, 200, 120));
	const FZoneGraphTag FootTrafficTag = FindOrAddZoneTag(ZoneSettings, FName(TEXT("FootTraffic")), FColor(80, 140, 220));

	// If tags weren't resolvable, we can still build shapes, but lane/tag filtering will not work.
	if (!VehiclesTag.IsValid())
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficZoneGraphAdapter] ZoneGraph tag 'Vehicles' not found/created; generated lanes may not be filterable by tag."));
	}
	if (!FootTrafficTag.IsValid())
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficZoneGraphAdapter] ZoneGraph tag 'FootTraffic' not found/created; generated foot lanes may not be filterable by tag."));
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Build spline zone shapes for each road.
	for (const FTrafficRoad& Road : Network.Roads)
	{
		const FRoadFamilyDefinition* Family = GetFamilyForRoad(FamilySettings, Road);

		const UTrafficZoneLaneProfile* VehicleProfileAsset = nullptr;
		if (Family && Family->VehicleLaneProfile.IsValid())
		{
			VehicleProfileAsset = Cast<UTrafficZoneLaneProfile>(LoadSoftObjectSync(Family->VehicleLaneProfile));
		}

		FZoneLaneProfileRef LaneProfileRef;
		if (VehicleProfileAsset)
		{
			LaneProfileRef = FindOrAddLaneProfileFromAsset(ZoneSettings, VehicleProfileAsset, VehiclesTag);
		}

		AZoneShape* ShapeActor = World->SpawnActor<AZoneShape>(AZoneShape::StaticClass(), FTransform::Identity, SpawnParams);
		if (!ShapeActor)
		{
			continue;
		}

		ShapeActor->Tags.AddUnique(TrafficZoneGraphTag);

		UZoneShapeComponent* ShapeComp = GetZoneShapeComponent(ShapeActor);
		if (!ShapeComp)
		{
			ShapeActor->Destroy();
			continue;
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

		SetSplinePoints(ShapeComp, Road.CenterlinePoints);
	}

	// Build polygon zone shapes for multi-lane intersections.
	// Choose a representative family from the first incoming lane's road.
	TMap<int32, int32> LaneIdToRoadId;
	LaneIdToRoadId.Reserve(Network.Lanes.Num());
	for (const FTrafficLane& Lane : Network.Lanes)
	{
		LaneIdToRoadId.Add(Lane.LaneId, Lane.RoadId);
	}

	TMap<int32, int32> RoadIdToFamilyId;
	RoadIdToFamilyId.Reserve(Network.Roads.Num());
	for (const FTrafficRoad& Road : Network.Roads)
	{
		RoadIdToFamilyId.Add(Road.RoadId, Road.FamilyId);
	}

	for (const FTrafficIntersection& Intersection : Network.Intersections)
	{
		const int32 NumConnected = Intersection.IncomingLaneIds.Num() + Intersection.OutgoingLaneIds.Num();
		if (NumConnected < 3)
		{
			continue;
		}

		int32 FamilyId = 0;
		if (Intersection.IncomingLaneIds.Num() > 0)
		{
			const int32 InLaneId = Intersection.IncomingLaneIds[0];
			if (const int32* RoadIdPtr = LaneIdToRoadId.Find(InLaneId))
			{
				if (const int32* FamIdPtr = RoadIdToFamilyId.Find(*RoadIdPtr))
				{
					FamilyId = *FamIdPtr;
				}
			}
		}

		const FRoadFamilyDefinition* Family = (FamilySettings && FamilySettings->Families.IsValidIndex(FamilyId))
			? &FamilySettings->Families[FamilyId]
			: nullptr;

		const UTrafficZoneLaneProfile* VehicleProfileAsset = nullptr;
		if (Family && Family->VehicleLaneProfile.IsValid())
		{
			VehicleProfileAsset = Cast<UTrafficZoneLaneProfile>(LoadSoftObjectSync(Family->VehicleLaneProfile));
		}

		FZoneLaneProfileRef LaneProfileRef;
		if (VehicleProfileAsset)
		{
			LaneProfileRef = FindOrAddLaneProfileFromAsset(ZoneSettings, VehicleProfileAsset, VehiclesTag);
		}

		AZoneShape* ShapeActor = World->SpawnActor<AZoneShape>(AZoneShape::StaticClass(), FTransform::Identity, SpawnParams);
		if (!ShapeActor)
		{
			continue;
		}

		ShapeActor->Tags.AddUnique(TrafficZoneGraphTag);

		UZoneShapeComponent* ShapeComp = GetZoneShapeComponent(ShapeActor);
		if (!ShapeComp)
		{
			ShapeActor->Destroy();
			continue;
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

		SetPolygonPointsCircle(ShapeComp, Intersection.Center, Intersection.Radius, /*NumSides=*/8);
	}

	// Request a rebuild so ZoneGraphData can be regenerated from the newly spawned shapes.
	UE::ZoneGraphDelegates::OnZoneGraphRequestRebuild.Broadcast();

	UE_LOG(LogTraffic, Log,
		TEXT("[TrafficZoneGraphAdapter] Spawned ZoneGraph shapes for Roads=%d Intersections=%d (tag=%s). Rebuild requested."),
		Network.Roads.Num(),
		Network.Intersections.Num(),
		*TrafficZoneGraphTag.ToString());
#endif
}
