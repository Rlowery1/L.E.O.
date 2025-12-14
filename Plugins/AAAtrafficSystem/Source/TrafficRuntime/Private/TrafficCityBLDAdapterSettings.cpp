#include "TrafficCityBLDAdapterSettings.h"

UTrafficCityBLDAdapterSettings::UTrafficCityBLDAdapterSettings()
{
	RoadActorTag = FName(TEXT("CityBLD_Road"));
	RoadSplineTag = FName(TEXT("CityBLD_Centerline"));
	DefaultFamilyName = FName(TEXT("Urban_2x2"));
	RoadClassNameContains.Add(TEXT("CityKit_Road"));
	RoadClassNameContains.Add(TEXT("CityKit_MeshRoad"));
	RoadClassNameContains.Add(TEXT("BP_MeshRoad"));
	DefaultCityBLDVehicleLaneProfile = FSoftObjectPath(TEXT("/AAAtrafficSystem/ZoneProfiles/CityBLDUrbanTwoLane.CityBLDUrbanTwoLane"));
}
