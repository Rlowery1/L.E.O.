#include "RoadFamilyRegistry.h"

#include "GameFramework/Actor.h"
#include "UObject/Package.h"
#include "TrafficRoadFamilySettings.h"

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
	for (int32 Index = 0; Index < Families.Num(); ++Index)
	{
		const FRoadFamilyInfo& Info = Families[Index];
		if (!Info.RoadClassPath.IsNull())
		{
			if (UClass* LoadedClass = Info.RoadClassPath.TryLoadClass<AActor>())
			{
				ClassToIndex.Add(LoadedClass, Index);
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

	if (ClassToIndex.Num() != Families.Num())
	{
		RebuildClassCache();
	}

	if (const int32* FoundIndex = ClassToIndex.Find(RoadClass))
	{
		return Families.IsValidIndex(*FoundIndex) ? &Families[*FoundIndex] : nullptr;
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
