#include "TrafficRuntimeWorldSubsystem.h"

#include "CityBLDRoadGeometryProvider.h"
#include "Components/SplineComponent.h"
#include "TrafficRuntimeSettings.h"
#include "TrafficCityBLDAdapterSettings.h"
#include "TrafficRoadFamilySettings.h"
#include "TrafficRoadMetadataComponent.h"
#include "TrafficSystemController.h"
#include "TrafficRuntimeModule.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Algo/Reverse.h"
#include "UObject/UnrealType.h"

namespace
{
	static bool IsCityBLDRoadCandidate_Runtime(const AActor* Actor, const UTrafficCityBLDAdapterSettings* AdapterSettings)
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
		if (ClassName.Contains(TEXT("BP_MeshRoad"), ESearchCase::IgnoreCase) ||
			ClassName.Contains(TEXT("BP_ModularRoad"), ESearchCase::IgnoreCase))
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
	}

	static bool TryReadStringLikePropertyValue(const FProperty* Prop, const void* ValuePtr, FString& OutValue)
	{
		if (!Prop || !ValuePtr)
		{
			return false;
		}

		if (const FStrProperty* StrProp = CastField<const FStrProperty>(Prop))
		{
			OutValue = StrProp->GetPropertyValue(ValuePtr);
			return true;
		}

		if (const FNameProperty* NameProp = CastField<const FNameProperty>(Prop))
		{
			OutValue = NameProp->GetPropertyValue(ValuePtr).ToString();
			return true;
		}

		if (const FTextProperty* TextProp = CastField<const FTextProperty>(Prop))
		{
			OutValue = TextProp->GetPropertyValue(ValuePtr).ToString();
			return true;
		}

		return false;
	}

	static FString FindBestStyleOrPresetNameOnObject(UObject* Obj)
	{
		if (!Obj)
		{
			return FString();
		}

		static const TArray<FName> PreferredPropertyNames = {
			FName(TEXT("StyleName")),
			FName(TEXT("StreetStyleName")),
			FName(TEXT("RoadStyleName")),
			FName(TEXT("CityBLDPresetName")),
		};

		FString Best;

		for (TFieldIterator<FProperty> It(Obj->GetClass()); It; ++It)
		{
			const FProperty* Prop = *It;
			if (!Prop)
			{
				continue;
			}

			const FName PropName = Prop->GetFName();
			const FString PropNameStr = PropName.ToString();
			const FString DisplayName = Prop->GetMetaData(TEXT("DisplayName"));

			const bool bLooksLikeStyle =
				PreferredPropertyNames.Contains(PropName) ||
				PropNameStr.Contains(TEXT("Style"), ESearchCase::IgnoreCase) ||
				PropNameStr.Contains(TEXT("Preset"), ESearchCase::IgnoreCase) ||
				DisplayName.Contains(TEXT("Style"), ESearchCase::IgnoreCase) ||
				DisplayName.Contains(TEXT("Preset"), ESearchCase::IgnoreCase);

			if (!bLooksLikeStyle)
			{
				continue;
			}

			const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Obj);
			FString Value;

			if (TryReadStringLikePropertyValue(Prop, ValuePtr, Value))
			{
				Value.TrimStartAndEndInline();
				if (Value.Len() > Best.Len())
				{
					Best = Value;
				}
				continue;
			}

			// One hop into object references (CityBLD commonly stores style data in nested objects/data assets).
			if (const FObjectPropertyBase* ObjProp = CastField<const FObjectPropertyBase>(Prop))
			{
				UObject* Ref = ObjProp->GetObjectPropertyValue(ValuePtr);
				if (!Ref)
				{
					continue;
				}

				const FString Nested = FindBestStyleOrPresetNameOnObject(Ref);
				if (Nested.Len() > Best.Len())
				{
					Best = Nested;
				}
				continue;
			}

			// Struct leaf scan for common "StyleName" fields.
			if (const FStructProperty* StructProp = CastField<const FStructProperty>(Prop))
			{
				const void* StructPtr = StructProp->ContainerPtrToValuePtr<void>(Obj);
				if (!StructPtr)
				{
					continue;
				}

				for (TFieldIterator<FProperty> InnerIt(StructProp->Struct); InnerIt; ++InnerIt)
				{
					const FProperty* Inner = *InnerIt;
					if (!Inner)
					{
						continue;
					}

					const FString InnerNameStr = Inner->GetName();
					const FString InnerDisplayName = Inner->GetMetaData(TEXT("DisplayName"));
					const bool bLooksLikeStyleLeaf =
						InnerNameStr.Equals(TEXT("StyleName"), ESearchCase::IgnoreCase) ||
						InnerNameStr.Equals(TEXT("PresetName"), ESearchCase::IgnoreCase) ||
						InnerDisplayName.Equals(TEXT("Style Name"), ESearchCase::IgnoreCase) ||
						InnerDisplayName.Equals(TEXT("Preset Name"), ESearchCase::IgnoreCase);

					if (!bLooksLikeStyleLeaf)
					{
						continue;
					}

					const void* InnerPtr = Inner->ContainerPtrToValuePtr<void>(StructPtr);
					FString InnerValue;
					if (TryReadStringLikePropertyValue(Inner, InnerPtr, InnerValue))
					{
						InnerValue.TrimStartAndEndInline();
						if (InnerValue.Len() > Best.Len())
						{
							Best = InnerValue;
						}
					}
				}
			}
		}

		return Best;
	}

	static FString TryGetCityBLDVariantKeyFromActor(const AActor* RoadActor)
	{
		if (!RoadActor)
		{
			return FString();
		}

		FString Best;
		TArray<UActorComponent*> Comps;
		RoadActor->GetComponents(Comps);
		for (UActorComponent* C : Comps)
		{
			if (!C)
			{
				continue;
			}

			const FString Candidate = FindBestStyleOrPresetNameOnObject(C);
			if (Candidate.Len() > Best.Len())
			{
				Best = Candidate;
			}
		}

		// Also scan the actor itself.
		{
			const FString Candidate = FindBestStyleOrPresetNameOnObject(const_cast<AActor*>(RoadActor));
			if (Candidate.Len() > Best.Len())
			{
				Best = Candidate;
			}
		}

		Best.TrimStartAndEndInline();
		return Best;
	}

	static void EnsureFamilyDefinitionExists(UTrafficRoadFamilySettings* RoadSettings, const FName FamilyName, const FString& FamilyNameStr)
	{
		if (!RoadSettings || FamilyName.IsNone())
		{
			return;
		}

		if (RoadSettings->FindFamilyByName(FamilyName))
		{
			return;
		}

		FRoadFamilyDefinition NewDef;
		NewDef.FamilyName = FamilyName;
		NewDef.Forward.NumLanes = 1;
		NewDef.Backward.NumLanes = 1;

		// Best-effort heuristic seeding when the project settings haven't been saved yet.
		if (FamilyNameStr.Contains(TEXT("Four Lane"), ESearchCase::IgnoreCase))
		{
			NewDef.Forward.NumLanes = 2;
			NewDef.Backward.NumLanes = 2;
		}
		else if (FamilyNameStr.Contains(TEXT("Two Lane"), ESearchCase::IgnoreCase))
		{
			NewDef.Forward.NumLanes = 1;
			NewDef.Backward.NumLanes = 1;
		}
		else if (FamilyNameStr.Contains(TEXT("3 Lane"), ESearchCase::IgnoreCase) &&
				 (FamilyNameStr.Contains(TEXT("One-Way"), ESearchCase::IgnoreCase) ||
				  FamilyNameStr.Contains(TEXT("One Way"), ESearchCase::IgnoreCase) ||
				  FamilyNameStr.Contains(TEXT("1 Way"), ESearchCase::IgnoreCase)))
		{
			NewDef.Forward.NumLanes = 3;
			NewDef.Backward.NumLanes = 0;
		}
		else if (FamilyNameStr.Contains(TEXT("Ramp"), ESearchCase::IgnoreCase))
		{
			NewDef.Forward.NumLanes = 1;
			NewDef.Backward.NumLanes = 0;
		}

		NewDef.Forward.LaneWidthCm = 350.f;
		NewDef.Backward.LaneWidthCm = 350.f;

		// Basic offset defaults: center the first lane at half a lane width.
		NewDef.Forward.InnerLaneCenterOffsetCm = NewDef.Forward.LaneWidthCm * 0.5f;
		NewDef.Backward.InnerLaneCenterOffsetCm = NewDef.Backward.LaneWidthCm * 0.5f;

		// Default ZoneGraph lane profile paths (plugin mount point). Optional; can be overridden by calibration.
		NewDef.VehicleLaneProfile = FSoftObjectPath(TEXT("/AAAtrafficSystem/ZoneProfiles/CityBLDUrbanTwoLane.CityBLDUrbanTwoLane"));
		NewDef.FootpathLaneProfile = FSoftObjectPath(TEXT("/AAAtrafficSystem/ZoneProfiles/CityBLDFootpath.CityBLDFootpath"));

		RoadSettings->Families.Add(NewDef);
	}

	static void Runtime_PrepareCityBLDRoadMetadata(UWorld& World)
	{
		const UTrafficRuntimeSettings* RuntimeSettings = GetDefault<UTrafficRuntimeSettings>();
		if (!RuntimeSettings || !RuntimeSettings->bEnableCityBLDAdapter)
		{
			return;
		}

		const UTrafficCityBLDAdapterSettings* AdapterSettings = GetDefault<UTrafficCityBLDAdapterSettings>();
		UTrafficRoadFamilySettings* RoadSettings = GetMutableDefault<UTrafficRoadFamilySettings>();

		int32 CityBLDActorsProcessed = 0;
		int32 MetadataCreated = 0;
		int32 FamilyAssignedFromVariant = 0;
		int32 MissingFamiliesAdded = 0;
		int32 RampRoleRemapped = 0;

		// Collect highway polylines for ramp role classification (best-effort; only for CityBLD modular highway setups).
		UCityBLDRoadGeometryProvider* Provider = NewObject<UCityBLDRoadGeometryProvider>();
		TArray<AActor*> HighwayActors;
		TArray<AActor*> RampActors;

		for (TActorIterator<AActor> It(&World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor)
			{
				continue;
			}

			if (!IsCityBLDRoadCandidate_Runtime(Actor, AdapterSettings))
			{
				continue;
			}

			if (!Actor->FindComponentByClass<USplineComponent>())
			{
				continue;
			}

			++CityBLDActorsProcessed;

			const FString VariantKey = TryGetCityBLDVariantKeyFromActor(Actor);
			const bool bLooksLikeRamp = VariantKey.Contains(TEXT("Ramp"), ESearchCase::IgnoreCase);
			const bool bLooksLikeHighway = !bLooksLikeRamp &&
				(VariantKey.Contains(TEXT("Freeway"), ESearchCase::IgnoreCase) ||
				 VariantKey.Contains(TEXT("Highway"), ESearchCase::IgnoreCase));

			if (bLooksLikeRamp)
			{
				RampActors.Add(Actor);
			}
			else if (bLooksLikeHighway)
			{
				HighwayActors.Add(Actor);
			}

			UTrafficRoadMetadataComponent* Meta = Actor->FindComponentByClass<UTrafficRoadMetadataComponent>();
			if (!Meta)
			{
				Meta = NewObject<UTrafficRoadMetadataComponent>(Actor);
				Meta->RegisterComponent();
				Actor->AddInstanceComponent(Meta);
				++MetadataCreated;
			}
			else if (!Meta->IsRegistered())
			{
				Meta->RegisterComponent();
			}

			// If this CityBLD actor ended up with a generic default family, prefer its CityBLD preset/style name instead.
			if (Meta->FamilyName.IsNone() || Meta->FamilyName == FName(TEXT("Urban_2x2")))
			{
				if (!VariantKey.IsEmpty())
				{
					Meta->FamilyName = FName(*VariantKey);
					++FamilyAssignedFromVariant;
				}
			}

			Meta->bIncludeInTraffic = true;

			const FString FamilyNameStr = Meta->FamilyName.ToString();
			const bool bHadFamily = (RoadSettings && RoadSettings->FindFamilyByName(Meta->FamilyName) != nullptr);
			EnsureFamilyDefinitionExists(RoadSettings, Meta->FamilyName, FamilyNameStr);
			if (!bHadFamily && RoadSettings && RoadSettings->FindFamilyByName(Meta->FamilyName))
			{
				++MissingFamiliesAdded;
			}
		}

		// Best-effort ramp role split (On/Off) based on current reverse direction and which endpoint is nearest a highway.
		if (RampActors.Num() > 0 && HighwayActors.Num() > 0)
		{
			TArray<TArray<FVector>> HighwayPolylines;
			for (AActor* Highway : HighwayActors)
			{
				TArray<FVector> Pts;
				if (Provider && Provider->GetDisplayCenterlineForActor(Highway, Pts) && Pts.Num() >= 2)
				{
					HighwayPolylines.Add(MoveTemp(Pts));
				}
			}

			auto ClosestPointDistanceSqToPolyline = [](const TArray<FVector>& Poly, const FVector& Pt) -> float
			{
				if (Poly.Num() < 2)
				{
					return TNumericLimits<float>::Max();
				}

				float BestSq = TNumericLimits<float>::Max();
				for (int32 i = 0; i + 1 < Poly.Num(); ++i)
				{
					const FVector A = Poly[i];
					const FVector B = Poly[i + 1];
					const FVector Closest = FMath::ClosestPointOnSegment(Pt, A, B);
					BestSq = FMath::Min(BestSq, FVector::DistSquared(Pt, Closest));
				}
				return BestSq;
			};

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

				TArray<FVector> RampCenterline;
				if (!Provider || !Provider->GetDisplayCenterlineForActor(Ramp, RampCenterline) || RampCenterline.Num() < 2)
				{
					continue;
				}

				// Provider returns driving-direction centerline (may already be reversed). We want raw endpoints for distance checks.
				TArray<FVector> RampRaw = RampCenterline;
				if (Meta->bReverseCenterlineDirection)
				{
					Algo::Reverse(RampRaw);
				}

				const FVector StartRaw = RampRaw[0];
				const FVector EndRaw = RampRaw.Last();

				float BestStartSq = TNumericLimits<float>::Max();
				float BestEndSq = TNumericLimits<float>::Max();
				for (const TArray<FVector>& Hwy : HighwayPolylines)
				{
					BestStartSq = FMath::Min(BestStartSq, ClosestPointDistanceSqToPolyline(Hwy, StartRaw));
					BestEndSq = FMath::Min(BestEndSq, ClosestPointDistanceSqToPolyline(Hwy, EndRaw));
				}

				const bool bHighwayAtStart = (BestStartSq <= BestEndSq);

				// Physical highway endpoint.
				const FVector HighwayEndpoint = bHighwayAtStart ? StartRaw : EndRaw;

				// Physical driving endpoints (based on reverse flag).
				const FVector DrivingStart = Meta->bReverseCenterlineDirection ? EndRaw : StartRaw;
				const FVector DrivingEnd = Meta->bReverseCenterlineDirection ? StartRaw : EndRaw;

				enum class ERampRole : uint8 { Unknown, On, Off };
				ERampRole Role = ERampRole::Unknown;
				if (DrivingEnd.Equals(HighwayEndpoint, 1.f))
				{
					Role = ERampRole::On;
				}
				else if (DrivingStart.Equals(HighwayEndpoint, 1.f))
				{
					Role = ERampRole::Off;
				}

				if (Role == ERampRole::Unknown)
				{
					continue;
				}

				FString BaseFamilyName = Meta->FamilyName.ToString();
				BaseFamilyName.TrimStartAndEndInline();
				BaseFamilyName = BaseFamilyName.LeftChop(BaseFamilyName.EndsWith(TEXT(" (On)")) ? 5 : 0);
				BaseFamilyName = BaseFamilyName.LeftChop(BaseFamilyName.EndsWith(TEXT(" (Off)")) ? 6 : 0);
				BaseFamilyName.TrimEndInline();

				const FString RoleSuffix = (Role == ERampRole::On) ? TEXT(" (On)") : TEXT(" (Off)");
				const FString VariantKey = BaseFamilyName + RoleSuffix;
				Meta->FamilyName = FName(*VariantKey);

				++RampRoleRemapped;
				EnsureFamilyDefinitionExists(RoadSettings, Meta->FamilyName, VariantKey);
			}
		}

		if (CityBLDActorsProcessed > 0)
		{
			UE_LOG(LogTraffic, Log,
				TEXT("[TrafficRuntimeWorldSubsystem] CityBLD runtime prepare: Actors=%d MetaCreated=%d FamilyFromVariant=%d FamiliesAdded=%d RampRoleRemapped=%d"),
				CityBLDActorsProcessed,
				MetadataCreated,
				FamilyAssignedFromVariant,
				MissingFamiliesAdded,
				RampRoleRemapped);
		}
	}
}

void UTrafficRuntimeWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	const EWorldType::Type WorldType = InWorld.WorldType;
	if (WorldType == EWorldType::Editor)
	{
		return;
	}

	const UTrafficRuntimeSettings* Settings = GetDefault<UTrafficRuntimeSettings>();
	if (!Settings || !Settings->bAutoSpawnTrafficOnBeginPlay)
	{
		return;
	}

	if (Settings->bAutoSpawnOnlyInPIE && WorldType != EWorldType::PIE)
	{
		return;
	}

	UE_LOG(LogTraffic, Log,
		TEXT("[TrafficRuntimeWorldSubsystem] OnWorldBeginPlay: World=%s Type=%d AutoBuild=%s AutoSpawn=%s VehiclesPerLane=%d Speed=%.1f ZoneGraph=%s"),
		*InWorld.GetName(),
		static_cast<int32>(WorldType),
		Settings->bAutoBuildNetwork ? TEXT("true") : TEXT("false"),
		Settings->bAutoSpawnVehicles ? TEXT("true") : TEXT("false"),
		Settings->VehiclesPerLaneRuntime,
		Settings->RuntimeSpeedCmPerSec,
		Settings->bGenerateZoneGraph ? TEXT("true") : TEXT("false"));

	// If the user ran the Editor Test harness before PIE, those vehicles/controllers are duplicated into the PIE world.
	// Clear any previously spawned traffic actors so we always have a clean, predictable runtime start.
	static const FName TrafficSpawnedVehicleTag(TEXT("AAA_TrafficVehicle"));
	int32 ClearedCount = 0;
	{
		TArray<AActor*> ToDestroy;
		for (TActorIterator<AActor> It(&InWorld); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor && Actor->Tags.Contains(TrafficSpawnedVehicleTag))
			{
				ToDestroy.Add(Actor);
			}
		}

		for (AActor* Actor : ToDestroy)
		{
			if (Actor && !Actor->IsActorBeingDestroyed())
			{
				InWorld.DestroyActor(Actor);
				++ClearedCount;
			}
		}
	}

	if (ClearedCount > 0)
	{
		UE_LOG(LogTraffic, Log, TEXT("[TrafficRuntimeWorldSubsystem] Cleared %d pre-existing traffic actors in PIE/Game world."), ClearedCount);
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ATrafficSystemController* Controller = nullptr;
	for (TActorIterator<ATrafficSystemController> It(&InWorld); It; ++It)
	{
		Controller = *It;
		break;
	}

	if (!Controller)
	{
		Controller = InWorld.SpawnActor<ATrafficSystemController>(ATrafficSystemController::StaticClass(), FTransform::Identity, Params);
	}

	if (!Controller)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficRuntimeWorldSubsystem] Failed to spawn TrafficSystemController."));
		return;
	}

	Controller->SetRuntimeConfigFromProjectSettings();

	// Runtime one-button safety: ensure CityBLD roads have metadata + family names before we build.
	// This prevents "everything becomes Urban_2x2" in PIE when the editor-prepared settings aren't present yet.
	Runtime_PrepareCityBLDRoadMetadata(InWorld);

	if (Settings->bAutoBuildNetwork)
	{
		Controller->Runtime_BuildTrafficNetwork();
	}

	if (Settings->bAutoSpawnVehicles)
	{
		Controller->Runtime_SpawnTraffic();
	}
}
