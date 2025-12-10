#include "TrafficVehicleSettings.h"
#include "TrafficVehicleBase.h"

UTrafficVehicleSettings::UTrafficVehicleSettings()
{
	DefaultTestVehicleClass = ATrafficVehicleBase::StaticClass();
	bUseExternalVehicleAdapter = false;
}

TSoftClassPtr<AActor> UTrafficVehicleSettings::ResolveCitySampleDefaultVisual()
{
	// Candidate Blueprint paths for the City Sample Vehicles pack (may vary by install).
	static const TCHAR* Candidates[] = {
		TEXT("/CitySampleVehicles/Blueprints/BP_CitySampleSedan.BP_CitySampleSedan_C"),
		TEXT("/CitySampleVehicles/Blueprints/BP_VehicleSedan01.BP_VehicleSedan01_C"),
		TEXT("/CitySample/Blueprints/Vehicles/BP_CitySampleSedan.BP_CitySampleSedan_C"),
		TEXT("/CitySample/Blueprints/Vehicles/BP_VehicleSedan01.BP_VehicleSedan01_C")
	};

	for (const TCHAR* Path : Candidates)
	{
		const FSoftObjectPath SoftPath(Path);
		TSoftClassPtr<AActor> ClassPtr(SoftPath);
		if (ClassPtr.IsValid())
		{
			return ClassPtr;
		}

		if (UClass* Loaded = Cast<UClass>(SoftPath.TryLoad()))
		{
			return TSoftClassPtr<AActor>(Loaded);
		}
	}

	return TSoftClassPtr<AActor>();
}
