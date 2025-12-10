#include "TrafficVehicleSettings.h"

UTrafficVehicleSettings::UTrafficVehicleSettings()
{
	DefaultVehicleProfile = FSoftObjectPath();
}

const UTrafficVehicleSettings* UTrafficVehicleSettings::Get()
{
	return GetDefault<UTrafficVehicleSettings>();
}
