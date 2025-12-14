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

		// Prefer the City Kit Element Component when present (it's where CityBLD exposes "Street Style -> Style Name").
		FString Best;

		TArray<UActorComponent*> Comps;
		RoadActor->GetComponents(Comps);
		for (UActorComponent* C : Comps)
		{
			if (!C)
			{
				continue;
			}

			const FString ClassName = C->GetClass()->GetName();
			if (!ClassName.Contains(TEXT("CityKit"), ESearchCase::IgnoreCase) &&
				!ClassName.Contains(TEXT("Element"), ESearchCase::IgnoreCase))
			{
				continue;
			}

			const FString Candidate = FindBestStyleOrPresetNameOnObject(C);
			if (Candidate.Len() > Best.Len())
			{
				Best = Candidate;
			}
		}

		// Fallback: scan the actor itself.
		if (Best.IsEmpty())
		{
			Best = FindBestStyleOrPresetNameOnObject(const_cast<AActor*>(RoadActor));
		}

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
	if (RoadClassName.Contains(TEXT("BP_MeshRoad"), ESearchCase::IgnoreCase))
	{
		VariantKey = TryGetCityBLDVariantKeyFromActor(RoadActor);
	}

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
				 VariantKey.Contains(TEXT("One-Way"), ESearchCase::IgnoreCase))
		{
			DefaultCalib.NumLanesPerSideForward = 3;
			DefaultCalib.NumLanesPerSideBackward = 0;
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

	FRoadFamilyInfo NewInfo;
	NewInfo.FamilyId = FGuid::NewGuid();
	NewInfo.DisplayName = VariantKey.IsEmpty() ? SanitizeDisplayName(RoadClass->GetName()) : VariantKey;
	NewInfo.RoadClassPath = RoadClass;
	NewInfo.VariantKey = VariantKey;
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
