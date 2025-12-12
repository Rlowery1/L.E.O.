#include "TrafficRoadMetadataComponent.h"

UTrafficRoadMetadataComponent::UTrafficRoadMetadataComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = false;
	FamilyName = NAME_None;
	bIncludeInTraffic = true;
	RoadFamilyId.Invalidate();
}
