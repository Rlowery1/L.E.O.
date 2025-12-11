#include "TrafficVehicleSettings.h"
#include "TrafficVehicleProfile.h"

UTrafficVehicleSettings::UTrafficVehicleSettings()
{
	DefaultVehicleProfile = FSoftObjectPath();
}

const UTrafficVehicleSettings* UTrafficVehicleSettings::Get()
{
	return GetDefault<UTrafficVehicleSettings>();
}

const UTrafficVehicleProfile* UTrafficVehicleSettings::GetDefaultVehicleProfile() const
{
	if (DefaultVehicleProfile.IsNull())
	{
		return nullptr;
	}

	UObject* Loaded = DefaultVehicleProfile.TryLoad();
	return Cast<UTrafficVehicleProfile>(Loaded);
}
