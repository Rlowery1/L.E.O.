#include "TrafficVisualSettings.h"
#include "UObject/SoftObjectPath.h"

UTrafficVisualSettings::UTrafficVisualSettings()
{
	ForwardLaneArrowMesh = nullptr;
	BackwardLaneArrowMesh = nullptr;
	// Default to a dark template material shipped with the engine so ribbons look asphalt-like out of the box.
	// Users can override in Project Settings → Plugins → AAA Traffic Visual Settings.
	RoadRibbonMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Engine/MapTemplates/Materials/BasicAsset03.BasicAsset03")));
}

FName UTrafficVisualSettings::GetCategoryName() const
{
	return FName(TEXT("Plugins"));
}

FName UTrafficVisualSettings::GetSectionName() const
{
	return FName(TEXT("AAA Traffic Visual Settings"));
}

#if WITH_EDITOR
FText UTrafficVisualSettings::GetSectionText() const
{
	return FText::FromString(TEXT("AAA Traffic Visual Settings"));
}
#endif

