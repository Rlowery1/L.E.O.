#include "RoadFamilyRegistry.h"

#include "GameFramework/Actor.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "TrafficRoadFamilySettings.h"
#include "TrafficCityBLDAdapterSettings.h"

URoadFamilyRegistry* URoadFamilyRegistry::Get()
{
	return GetMutableDefault<URoadFamilyRegistry>();
}

FString URoadFamilyRegistry::SanitizeDisplayName(const FString& InClassName) const
{
	FString Out = InClassName;
	Out.ReplaceInline(TEXT("Blueprint"), TEXT(""));
	Out.ReplaceInline(TEXT("BP_"), TEXT(""));
	Out.TrimStartAndEndInline();
	return Out.IsEmpty() ? TEXT("Road") : Out;
}

void URoadFamilyRegistry::RebuildClassCache() const
{
	ClassToIndex.Empty();
	KeyToIndex.Empty();
	for (int32 Index = 0; Index < Families.Num(); ++Index)
	{
		const FRoadFamilyInfo& Info = Families[Index];
		if (!Info.RoadClassPath.IsNull())
		{
			if (UClass* LoadedClass = Info.RoadClassPath.TryLoadClass<AActor>())
			{
				const FString Key = LoadedClass->GetPathName() + TEXT("|") + Info.VariantKey;
				KeyToIndex.Add(Key, Index);

				// Legacy lookup: only map class->index for the "default" (non-variant) entry.
				if (Info.VariantKey.IsEmpty())
				{
					ClassToIndex.Add(LoadedClass, Index);
				}
			}
		}
	}
}

void URoadFamilyRegistry::RefreshCache() const
{
	RebuildClassCache();
}

const FRoadFamilyInfo* URoadFamilyRegistry::FindFamilyByClass(UClass* RoadClass) const
{
	if (!RoadClass)
	{
		return nullptr;
	}

	if (KeyToIndex.Num() != Families.Num())
	{
		RebuildClassCache();
	}

	// Prefer the legacy "default" family for this class (no variant key) if present.
	if (const int32* FoundIndex = ClassToIndex.Find(RoadClass))
	{
		return Families.IsValidIndex(*FoundIndex) ? &Families[*FoundIndex] : nullptr;
	}

	// Fallback: return the first family that matches this class (any variant key).
	for (const FRoadFamilyInfo& Info : Families)
	{
		if (!Info.RoadClassPath.IsNull())
		{
			if (UClass* LoadedClass = Info.RoadClassPath.TryLoadClass<AActor>())
			{
				if (LoadedClass == RoadClass)
				{
					return &Info;
				}
			}
		}
	}

	return nullptr;
}

FRoadFamilyInfo* URoadFamilyRegistry::FindFamilyById(const FGuid& FamilyId)
{
	for (FRoadFamilyInfo& Info : Families)
	{
		if (Info.FamilyId == FamilyId)
		{
			return &Info;
		}
	}
	return nullptr;
}

const FRoadFamilyInfo* URoadFamilyRegistry::FindFamilyById(const FGuid& FamilyId) const
{
	for (const FRoadFamilyInfo& Info : Families)
	{
		if (Info.FamilyId == FamilyId)
		{
			return &Info;
		}
	}
	return nullptr;
}

FRoadFamilyInfo* URoadFamilyRegistry::FindOrCreateFamilyForClass(UClass* RoadClass, bool* bOutCreated)
{
	if (bOutCreated)
	{
		*bOutCreated = false;
	}

	if (!RoadClass)
	{
		return nullptr;
	}

	// Class-only calls map to the default (non-variant) family entry.
	if (KeyToIndex.Num() != Families.Num())
	{
		RebuildClassCache();
	}

	const FString Key = RoadClass->GetPathName() + TEXT("|");
	if (const int32* FoundIndex = KeyToIndex.Find(Key))
	{
		return Families.IsValidIndex(*FoundIndex) ? &Families[*FoundIndex] : nullptr;
	}

	if (FRoadFamilyInfo* Existing = const_cast<FRoadFamilyInfo*>(FindFamilyByClass(RoadClass)))
	{
		return Existing;
	}

	FTrafficLaneFamilyCalibration DefaultCalib;
	FRoadFamilyDefinition DefaultFamily;
	DefaultFamily.FamilyName = FName(*SanitizeDisplayName(RoadClass->GetName()));
	DefaultFamily.Forward.NumLanes = DefaultCalib.NumLanesPerSideForward;
	DefaultFamily.Forward.LaneWidthCm = DefaultCalib.LaneWidthCm;
	DefaultFamily.Forward.InnerLaneCenterOffsetCm = DefaultCalib.CenterlineOffsetCm;
	DefaultFamily.Backward.NumLanes = DefaultCalib.NumLanesPerSideBackward;
	DefaultFamily.Backward.LaneWidthCm = DefaultCalib.LaneWidthCm;
	DefaultFamily.Backward.InnerLaneCenterOffsetCm = DefaultCalib.CenterlineOffsetCm;

	FRoadFamilyInfo NewInfo;
	NewInfo.FamilyId = FGuid::NewGuid();
	NewInfo.DisplayName = SanitizeDisplayName(RoadClass->GetName());
	NewInfo.RoadClassPath = RoadClass;
	NewInfo.VariantKey = FString();
	NewInfo.FamilyDefinition = DefaultFamily;
	NewInfo.CalibrationData = DefaultCalib;
	NewInfo.bIsCalibrated = false;

	const int32 NewIndex = Families.Add(NewInfo);
	if (bOutCreated)
	{
		*bOutCreated = true;
	}

	RebuildClassCache();
	SaveConfig();
	return Families.IsValidIndex(NewIndex) ? &Families[NewIndex] : nullptr;
}

namespace
{
	static bool TryReadStringLikePropertyValue(const FProperty* Prop, const void* ValuePtr, FString& OutValue)
	{
		if (!Prop || !ValuePtr)
		{
			return false;
		}

		if (const FStrProperty* StrProp = CastField<FStrProperty>(Prop))
		{
			OutValue = StrProp->GetPropertyValue(ValuePtr);
			return !OutValue.IsEmpty();
		}

		if (const FNameProperty* NameProp = CastField<FNameProperty>(Prop))
		{
			const FName NameValue = NameProp->GetPropertyValue(ValuePtr);
			if (!NameValue.IsNone())
			{
				OutValue = NameValue.ToString();
				return !OutValue.IsEmpty();
			}
			return false;
		}

		if (const FTextProperty* TextProp = CastField<FTextProperty>(Prop))
		{
			OutValue = TextProp->GetPropertyValue(ValuePtr).ToString();
			return !OutValue.IsEmpty();
		}

		return false;
	}

	static bool TryReadObjectReferencedStyleName(const FProperty* Prop, const void* ValuePtr, FString& OutValue)
	{
		if (!Prop || !ValuePtr)
		{
			return false;
		}

		const FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(Prop);
		if (!ObjProp)
		{
			return false;
		}

		UObject* Referenced = ObjProp->GetObjectPropertyValue(ValuePtr);
		if (!Referenced)
		{
			return false;
		}

		// Recurse a single hop into referenced objects (common in CityBLD where "Style" is a data asset/object).
		static const TArray<FName> DirectCandidates = {
			FName(TEXT("StyleName")),
			FName(TEXT("StreetStyleName")),
			FName(TEXT("PresetName")),
			FName(TEXT("RoadPresetName")),
			FName(TEXT("StreetPresetName")),
			FName(TEXT("RoadStyleName")),
			FName(TEXT("CityBLDPresetName")),
		};

		for (const FName& CandidateName : DirectCandidates)
		{
			if (const FProperty* InnerProp = Referenced->GetClass()->FindPropertyByName(CandidateName))
			{
				const void* InnerPtr = InnerProp->ContainerPtrToValuePtr<void>(Referenced);
				FString Value;
				if (TryReadStringLikePropertyValue(InnerProp, InnerPtr, Value) && !Value.IsEmpty())
				{
					OutValue = Value;
					return true;
				}
			}
		}

		for (TFieldIterator<FProperty> It(Referenced->GetClass()); It; ++It)
		{
			const FProperty* Inner = *It;
			if (!Inner)
			{
				continue;
			}

			const FString DisplayName = Inner->HasMetaData(TEXT("DisplayName")) ? Inner->GetMetaData(TEXT("DisplayName")) : FString();
			const FString PropNameStr = Inner->GetName();
			const bool bLooksLikeStyle =
				PropNameStr.Contains(TEXT("Style"), ESearchCase::IgnoreCase) ||
				PropNameStr.Contains(TEXT("Preset"), ESearchCase::IgnoreCase) ||
				DisplayName.Contains(TEXT("Style"), ESearchCase::IgnoreCase) ||
				DisplayName.Contains(TEXT("Preset"), ESearchCase::IgnoreCase);

			if (!bLooksLikeStyle)
			{
				continue;
			}

			const void* InnerPtr = Inner->ContainerPtrToValuePtr<void>(Referenced);
			FString Value;
			if (TryReadStringLikePropertyValue(Inner, InnerPtr, Value) && !Value.IsEmpty())
			{
				OutValue = Value;
				return true;
			}
		}

		return false;
	}

	static FString FindBestStyleOrPresetNameOnObject(UObject* Obj)
	{
		if (!Obj)
		{
			return FString();
		}

		// First try a small set of common property names.
		static const TArray<FName> DirectCandidates = {
			FName(TEXT("StyleName")),
			FName(TEXT("StreetStyleName")),
			FName(TEXT("PresetName")),
			FName(TEXT("RoadPresetName")),
			FName(TEXT("StreetPresetName")),
			FName(TEXT("RoadStyleName")),
			FName(TEXT("CityBLDPresetName")),
		};

		for (const FName& PropName : DirectCandidates)
		{
			if (const FProperty* Prop = Obj->GetClass()->FindPropertyByName(PropName))
			{
				const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Obj);
				FString Value;
				if (TryReadStringLikePropertyValue(Prop, ValuePtr, Value) && !Value.IsEmpty())
				{
					return Value;
				}
			}
		}

		// Then scan for anything that looks like "Style Name" via metadata/display name, and handle common structs.
		FString Best;
		for (TFieldIterator<FProperty> It(Obj->GetClass()); It; ++It)
		{
			const FProperty* Prop = *It;
			if (!Prop)
			{
				continue;
			}

			const FString DisplayName = Prop->HasMetaData(TEXT("DisplayName")) ? Prop->GetMetaData(TEXT("DisplayName")) : FString();
			const FString PropNameStr = Prop->GetName();
			const bool bLooksLikeStyle =
				PropNameStr.Contains(TEXT("Style"), ESearchCase::IgnoreCase) ||
				PropNameStr.Contains(TEXT("Preset"), ESearchCase::IgnoreCase) ||
				DisplayName.Contains(TEXT("Style"), ESearchCase::IgnoreCase) ||
				DisplayName.Contains(TEXT("Preset"), ESearchCase::IgnoreCase);

			if (!bLooksLikeStyle)
			{
				continue;
			}

			// Direct string-like property.
			{
				const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Obj);
				FString Value;
				if (TryReadStringLikePropertyValue(Prop, ValuePtr, Value))
				{
					if (Value.Len() > Best.Len())
					{
						Best = Value;
					}
				}
			}

			// Object property: the style/preset may live on a referenced data asset/object.
			{
				const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Obj);
				FString Value;
				if (TryReadObjectReferencedStyleName(Prop, ValuePtr, Value))
				{
					if (Value.Len() > Best.Len())
					{
						Best = Value;
					}
				}
			}

			// Struct property: try to find an inner "StyleName"/"PresetName".
			if (const FStructProperty* StructProp = CastField<FStructProperty>(Prop))
			{
				const void* StructPtr = StructProp->ContainerPtrToValuePtr<void>(Obj);
				for (TFieldIterator<FProperty> Sit(StructProp->Struct); Sit; ++Sit)
				{
					const FProperty* Inner = *Sit;
					if (!Inner)
					{
						continue;
					}
					const FString InnerDisplayName = Inner->HasMetaData(TEXT("DisplayName")) ? Inner->GetMetaData(TEXT("DisplayName")) : FString();
					const FString InnerNameStr = Inner->GetName();
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
					FString Value;
					if (TryReadStringLikePropertyValue(Inner, InnerPtr, Value))
					{
						if (Value.Len() > Best.Len())
						{
							Best = Value;
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

		auto ScoreVariantCandidate = [](const FString& Candidate, const FString& OwnerClassName) -> int32
		{
			if (Candidate.IsEmpty())
			{
				return 0;
			}

			int32 Score = Candidate.Len();

			// Prefer human-readable preset-like strings.
			if (Candidate.Contains(TEXT(" - "), ESearchCase::IgnoreCase))
			{
				Score += 20;
			}

			// Common road keywords.
			static const TCHAR* Keywords[] = {
				TEXT("Lane"),
				TEXT("Road"),
				TEXT("Street"),
				TEXT("Highway"),
				TEXT("Freeway"),
				TEXT("Ramp"),
				TEXT("One Way"),
				TEXT("One-Way"),
			};
			for (const TCHAR* K : Keywords)
			{
				if (Candidate.Contains(K, ESearchCase::IgnoreCase))
				{
					Score += 10;
				}
			}

			// Prefer values coming from road-kit components rather than generic engine components.
			if (OwnerClassName.Contains(TEXT("City"), ESearchCase::IgnoreCase) ||
				OwnerClassName.Contains(TEXT("Kit"), ESearchCase::IgnoreCase) ||
				OwnerClassName.Contains(TEXT("BLD"), ESearchCase::IgnoreCase) ||
				OwnerClassName.Contains(TEXT("Element"), ESearchCase::IgnoreCase) ||
				OwnerClassName.Contains(TEXT("Modular"), ESearchCase::IgnoreCase))
			{
				Score += 15;
			}

			return Score;
		};

		FString Best;
		int32 BestScore = 0;

		TArray<UActorComponent*> Comps;
		RoadActor->GetComponents(Comps);
		for (UActorComponent* C : Comps)
		{
			if (!C)
			{
				continue;
			}

			const FString Candidate = FindBestStyleOrPresetNameOnObject(C);
			const int32 Score = ScoreVariantCandidate(Candidate, C->GetClass()->GetName());
			if (Score > BestScore)
			{
				Best = Candidate;
				BestScore = Score;
			}
		}

		// Also scan the actor itself (lower weight than components).
		{
			const FString Candidate = FindBestStyleOrPresetNameOnObject(const_cast<AActor*>(RoadActor));
			const int32 Score = ScoreVariantCandidate(Candidate, RoadActor->GetClass()->GetName()) - 5;
			if (Score > BestScore)
			{
				Best = Candidate;
				BestScore = Score;
			}
		}

#if WITH_EDITOR
		// Final fallback (editor only): CityBLD often bakes the chosen style into the actor label, even when the style lives in nested objects.
		if (Best.IsEmpty())
		{
			const FString Label = RoadActor->GetActorLabel();
			const int32 Score = ScoreVariantCandidate(Label, RoadActor->GetClass()->GetName()) - 10;
			if (Score > BestScore)
			{
				Best = Label;
				BestScore = Score;
			}
		}
#endif

		return Best;
	}
}

FRoadFamilyInfo* URoadFamilyRegistry::FindOrCreateFamilyForActor(AActor* RoadActor, bool* bOutCreated)
{
	if (bOutCreated)
	{
		*bOutCreated = false;
	}

	if (!RoadActor)
	{
		return nullptr;
	}

	UClass* RoadClass = RoadActor->GetClass();
	if (!RoadClass)
	{
		return nullptr;
	}

	FString VariantKey;
	const FString RoadClassName = RoadClass->GetName();
	if (RoadClassName.Contains(TEXT("BP_MeshRoad"), ESearchCase::IgnoreCase) ||
		RoadClassName.Contains(TEXT("BP_ModularRoad"), ESearchCase::IgnoreCase))
	{
		VariantKey = TryGetCityBLDVariantKeyFromActor(RoadActor);
	}

	VariantKey.TrimStartAndEndInline();
	return FindOrCreateFamilyForActorWithVariantKey(RoadActor, VariantKey, bOutCreated);
}

FRoadFamilyInfo* URoadFamilyRegistry::FindOrCreateFamilyForActorWithVariantKey(AActor* RoadActor, const FString& VariantKeyOverride, bool* bOutCreated)
{
	if (bOutCreated)
	{
		*bOutCreated = false;
	}

	if (!RoadActor)
	{
		return nullptr;
	}

	UClass* RoadClass = RoadActor->GetClass();
	if (!RoadClass)
	{
		return nullptr;
	}

	FString VariantKey = VariantKeyOverride;
	VariantKey.TrimStartAndEndInline();

	if (KeyToIndex.Num() != Families.Num())
	{
		RebuildClassCache();
	}

	const FString Key = RoadClass->GetPathName() + TEXT("|") + VariantKey;
	if (const int32* FoundIndex = KeyToIndex.Find(Key))
	{
		return Families.IsValidIndex(*FoundIndex) ? &Families[*FoundIndex] : nullptr;
	}

	FTrafficLaneFamilyCalibration DefaultCalib;
	if (!VariantKey.IsEmpty())
	{
		// Seed more helpful defaults based on common CityBLD preset/style names.
		// Users can still calibrate/bake to override these.
		if (VariantKey.Contains(TEXT("Four Lane"), ESearchCase::IgnoreCase))
		{
			DefaultCalib.NumLanesPerSideForward = 2;
			DefaultCalib.NumLanesPerSideBackward = 2;
		}
		else if (VariantKey.Contains(TEXT("Two Lane"), ESearchCase::IgnoreCase))
		{
			DefaultCalib.NumLanesPerSideForward = 1;
			DefaultCalib.NumLanesPerSideBackward = 1;
		}
		else if (VariantKey.Contains(TEXT("3 Lane"), ESearchCase::IgnoreCase) &&
				 (VariantKey.Contains(TEXT("One-Way"), ESearchCase::IgnoreCase) ||
				  VariantKey.Contains(TEXT("One Way"), ESearchCase::IgnoreCase) ||
				  VariantKey.Contains(TEXT("1 Way"), ESearchCase::IgnoreCase)))
		{
			DefaultCalib.NumLanesPerSideForward = 3;
			DefaultCalib.NumLanesPerSideBackward = 0;
		}
		else if (VariantKey.Contains(TEXT("Ramp"), ESearchCase::IgnoreCase))
		{
			// Most modular ramps are one-way. Default to forward-only unless the style explicitly says otherwise.
			DefaultCalib.NumLanesPerSideBackward = 0;

			if (VariantKey.Contains(TEXT("2 Lane"), ESearchCase::IgnoreCase))
			{
				DefaultCalib.NumLanesPerSideForward = 2;
			}
			else
			{
				DefaultCalib.NumLanesPerSideForward = 1;
			}
		}
		else if (VariantKey.Contains(TEXT("8 Lane"), ESearchCase::IgnoreCase))
		{
			// Common freeway preset: 4 lanes each direction.
			DefaultCalib.NumLanesPerSideForward = 4;
			DefaultCalib.NumLanesPerSideBackward = 4;
		}
	}

	// Improve default centering for one-way presets:
	// - Two-way roads: inner lane center is half a lane from centerline.
	// - One-way roads: center the lane group around the centerline.
	//
	// Examples (LaneWidth=W):
	// - 1 lane one-way: offset=0
	// - 2 lanes one-way: offset=-W/2  (lane centers at -W/2, +W/2)
	// - 3 lanes one-way: offset=-W    (lane centers at -W, 0, +W)
	{
		const int32 Fwd = DefaultCalib.NumLanesPerSideForward;
		const int32 Back = DefaultCalib.NumLanesPerSideBackward;
		const float W = DefaultCalib.LaneWidthCm;

		if (Back > 0)
		{
			DefaultCalib.CenterlineOffsetCm = W * 0.5f;
		}
		else if (Fwd > 0)
		{
			DefaultCalib.CenterlineOffsetCm = -W * 0.5f * static_cast<float>(Fwd - 1);
		}
	}

	FRoadFamilyDefinition DefaultFamily;

	const FString DisplayBase = VariantKey.IsEmpty() ? SanitizeDisplayName(RoadClass->GetName()) : VariantKey;
	DefaultFamily.FamilyName = FName(*DisplayBase);
	DefaultFamily.Forward.NumLanes = DefaultCalib.NumLanesPerSideForward;
	DefaultFamily.Forward.LaneWidthCm = DefaultCalib.LaneWidthCm;
	DefaultFamily.Forward.InnerLaneCenterOffsetCm = DefaultCalib.CenterlineOffsetCm;
	DefaultFamily.Backward.NumLanes = DefaultCalib.NumLanesPerSideBackward;
	DefaultFamily.Backward.LaneWidthCm = DefaultCalib.LaneWidthCm;
	DefaultFamily.Backward.InnerLaneCenterOffsetCm = DefaultCalib.CenterlineOffsetCm;

	// If the runtime settings already contain a family definition with this name (e.g. shipped presets),
	// seed the registry from that so first-run behavior matches the shipped defaults.
	bool bSeededFromRuntimeSettings = false;
	if (const UTrafficRoadFamilySettings* RuntimeSettings = GetDefault<UTrafficRoadFamilySettings>())
	{
		if (const FRoadFamilyDefinition* Existing = RuntimeSettings->FindFamilyByName(DefaultFamily.FamilyName))
		{
			DefaultFamily = *Existing;
			DefaultCalib.NumLanesPerSideForward = Existing->Forward.NumLanes;
			DefaultCalib.NumLanesPerSideBackward = Existing->Backward.NumLanes;
			DefaultCalib.LaneWidthCm = Existing->Forward.LaneWidthCm;
			DefaultCalib.CenterlineOffsetCm = Existing->Forward.InnerLaneCenterOffsetCm;
			bSeededFromRuntimeSettings = true;
		}
	}

	FRoadFamilyInfo NewInfo;
	NewInfo.FamilyId = FGuid::NewGuid();
	NewInfo.DisplayName = VariantKey.IsEmpty() ? SanitizeDisplayName(RoadClass->GetName()) : VariantKey;
	NewInfo.RoadClassPath = RoadClass;
	NewInfo.VariantKey = VariantKey;
	NewInfo.FamilyDefinition = DefaultFamily;
	NewInfo.CalibrationData = DefaultCalib;
	NewInfo.bIsCalibrated = bSeededFromRuntimeSettings;

	const int32 NewIndex = Families.Add(NewInfo);
	if (bOutCreated)
	{
		*bOutCreated = true;
	}

	RebuildClassCache();
	SaveConfig();
	return Families.IsValidIndex(NewIndex) ? &Families[NewIndex] : nullptr;
}

bool URoadFamilyRegistry::RenameFamily(const FGuid& FamilyId, const FString& NewDisplayName)
{
	for (FRoadFamilyInfo& Info : Families)
	{
		if (Info.FamilyId == FamilyId)
		{
			Info.DisplayName = NewDisplayName.IsEmpty() ? SanitizeDisplayName(Info.RoadClassPath.GetAssetName()) : NewDisplayName;
			Info.FamilyDefinition.FamilyName = FName(*Info.DisplayName);
			SaveConfig();
			return true;
		}
	}
	return false;
}

void URoadFamilyRegistry::ApplyCalibration(const FGuid& FamilyId, const FTrafficLaneFamilyCalibration& NewCalibration)
{
	for (FRoadFamilyInfo& Info : Families)
	{
		if (Info.FamilyId == FamilyId)
		{
			// Backup current calibration before overwriting.
			Info.BackupCalibration = Info.CalibrationData;
			Info.bHasBackupCalibration = Info.bIsCalibrated;

			Info.CalibrationData = NewCalibration;
			Info.bIsCalibrated = true;

			// Mirror into FamilyDefinition for compatibility with existing code paths.
			Info.FamilyDefinition.FamilyName = Info.FamilyDefinition.FamilyName.IsNone()
				? FName(*Info.DisplayName)
				: Info.FamilyDefinition.FamilyName;
			Info.FamilyDefinition.Forward.NumLanes = NewCalibration.NumLanesPerSideForward;
			Info.FamilyDefinition.Backward.NumLanes = NewCalibration.NumLanesPerSideBackward;
			Info.FamilyDefinition.Forward.LaneWidthCm = NewCalibration.LaneWidthCm;
			Info.FamilyDefinition.Backward.LaneWidthCm = NewCalibration.LaneWidthCm;
			Info.FamilyDefinition.Forward.InnerLaneCenterOffsetCm = NewCalibration.CenterlineOffsetCm;
			Info.FamilyDefinition.Backward.InnerLaneCenterOffsetCm = NewCalibration.CenterlineOffsetCm;

			SaveConfig();
			return;
		}
	}
}

bool URoadFamilyRegistry::RestoreLastCalibration(const FGuid& FamilyId)
{
	for (FRoadFamilyInfo& Info : Families)
	{
		if (Info.FamilyId == FamilyId)
		{
			if (!Info.bHasBackupCalibration)
			{
				return false;
			}

			Swap(Info.CalibrationData, Info.BackupCalibration);
			Info.bIsCalibrated = true;

			Info.FamilyDefinition.FamilyName = Info.FamilyDefinition.FamilyName.IsNone()
				? FName(*Info.DisplayName)
				: Info.FamilyDefinition.FamilyName;
			Info.FamilyDefinition.Forward.NumLanes = Info.CalibrationData.NumLanesPerSideForward;
			Info.FamilyDefinition.Backward.NumLanes = Info.CalibrationData.NumLanesPerSideBackward;
			Info.FamilyDefinition.Forward.LaneWidthCm = Info.CalibrationData.LaneWidthCm;
			Info.FamilyDefinition.Backward.LaneWidthCm = Info.CalibrationData.LaneWidthCm;
			Info.FamilyDefinition.Forward.InnerLaneCenterOffsetCm = Info.CalibrationData.CenterlineOffsetCm;
			Info.FamilyDefinition.Backward.InnerLaneCenterOffsetCm = Info.CalibrationData.CenterlineOffsetCm;

			SaveConfig();
			return true;
		}
	}
	return false;
}

bool URoadFamilyRegistry::DeleteFamilyById(const FGuid& FamilyId)
{
	for (int32 Index = 0; Index < Families.Num(); ++Index)
	{
		if (Families[Index].FamilyId == FamilyId)
		{
			Families.RemoveAt(Index);
			RebuildClassCache();
			SaveConfig();
			return true;
		}
	}
	return false;
}
