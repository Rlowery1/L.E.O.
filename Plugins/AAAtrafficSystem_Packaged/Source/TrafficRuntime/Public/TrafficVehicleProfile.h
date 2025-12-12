#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TrafficVehicleProfile.generated.h"

/**
 * Data-driven vehicle profile describing the visual/Chaos pawn and basic dimensions.
 */
UCLASS(BlueprintType)
class TRAFFICRUNTIME_API UTrafficVehicleProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Visual/Chaos pawn class to use for this profile. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TrafficVehicle")
	TSoftClassPtr<APawn> VehicleClass;

	/** Optional category tag (Car, Truck, Bus, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TrafficVehicle")
	FName CategoryTag;

	/** Approximate physical dimensions (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TrafficVehicle")
	float LengthCm = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TrafficVehicle")
	float WidthCm = 180.f;

	/** Recommended headway multiplier for this profile (for later tuning). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TrafficVehicle")
	float HeadwayScale = 1.0f;
};
