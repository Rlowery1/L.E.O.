#include "TrafficRoadFamilySettings.h"

UTrafficRoadFamilySettings::UTrafficRoadFamilySettings()
{
	if (Families.Num() == 0)
	{
		FRoadFamilyDefinition Urban2x2;
		Urban2x2.FamilyName = TEXT("Urban_2x2");
		Urban2x2.Forward.NumLanes = 2;
		Urban2x2.Forward.LaneWidthCm = 350.f;
		Urban2x2.Forward.InnerLaneCenterOffsetCm = 175.f;
		Urban2x2.Backward.NumLanes = 2;
		Urban2x2.Backward.LaneWidthCm = 350.f;
		Urban2x2.Backward.InnerLaneCenterOffsetCm = 175.f;
		Urban2x2.DefaultSpeedLimitKmh = 50.f;

		FRoadFamilyDefinition Local1x1;
		Local1x1.FamilyName = TEXT("Local_1x1");
		Local1x1.Forward.NumLanes = 1;
		Local1x1.Forward.LaneWidthCm = 325.f;
		Local1x1.Forward.InnerLaneCenterOffsetCm = 162.5f;
		Local1x1.Backward.NumLanes = 1;
		Local1x1.Backward.LaneWidthCm = 325.f;
		Local1x1.Backward.InnerLaneCenterOffsetCm = 162.5f;
		Local1x1.DefaultSpeedLimitKmh = 30.f;

		Families.Add(Urban2x2);
		Families.Add(Local1x1);
	}
}

FName UTrafficRoadFamilySettings::GetCategoryName() const
{
	return TEXT("Plugins");
}

FName UTrafficRoadFamilySettings::GetSectionName() const
{
	return TEXT("AAA Traffic System");
}

const FRoadFamilyDefinition* UTrafficRoadFamilySettings::FindFamilyByName(FName FamilyName) const
{
	return Families.FindByPredicate(
		[FamilyName](const FRoadFamilyDefinition& Def)
		{
			return Def.FamilyName == FamilyName;
		});
}

