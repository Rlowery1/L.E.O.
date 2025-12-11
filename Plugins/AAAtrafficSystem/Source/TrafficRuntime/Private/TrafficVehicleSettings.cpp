#include "TrafficVehicleSettings.h"
#include "TrafficVehicleProfile.h"
#include "TrafficRuntimeModule.h"
#include "GameFramework/Pawn.h"

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
	if (!DefaultVehicleProfile.IsNull())
	{
		UObject* Loaded = DefaultVehicleProfile.TryLoad();
		if (const UTrafficVehicleProfile* Profile = Cast<UTrafficVehicleProfile>(Loaded))
		{
			return Profile;
		}
	}

	// Attempt to auto-set a Chaos vehicle if project has City Sample content installed.
	static const TArray<FSoftObjectPath> CandidateChaosPaths = {
		FSoftObjectPath(TEXT("/Game/CitySampleVehicles/vehicle07_Car/BP_vehicle07_Car.BP_vehicle07_Car_C")),
		FSoftObjectPath(TEXT("/Game/CitySample/Blueprints/Vehicles/BP_Sedan.BP_Sedan_C"))
	};

	static UTrafficVehicleProfile* AutoProfile = nullptr;

	for (const FSoftObjectPath& Path : CandidateChaosPaths)
	{
		TSoftClassPtr<APawn> ChaosPtr(Path);
		UClass* ChaosClass = ChaosPtr.IsNull() ? nullptr : ChaosPtr.LoadSynchronous();
		if (!ChaosClass)
		{
			continue;
		}

		if (!AutoProfile || AutoProfile->IsUnreachable())
		{
			AutoProfile = NewObject<UTrafficVehicleProfile>(GetTransientPackage());
		}

		if (AutoProfile)
		{
			AutoProfile->VehicleClass = ChaosPtr;
			UE_LOG(LogTraffic, Log,
				TEXT("[TrafficVehicleSettings] Auto-selected Chaos vehicle from %s"),
				*Path.ToString());
			return AutoProfile;
		}
	}

	return nullptr;
}
