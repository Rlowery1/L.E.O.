#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "TrafficCityBLDAdapterSettings.generated.h"

UCLASS(Config=Game, DefaultConfig)
class TRAFFICRUNTIME_API UTrafficCityBLDAdapterSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UTrafficCityBLDAdapterSettings();

	virtual FName GetCategoryName() const override
	{
		return TEXT("Plugins");
	}

	virtual FName GetSectionName() const override
	{
		return TEXT("AAA Traffic System - CityBLD Adapter");
	}

	UPROPERTY(EditAnywhere, Config, Category="Detection")
	FName RoadActorTag;

	UPROPERTY(EditAnywhere, Config, Category="Detection")
	TArray<FString> RoadClassNameContains;

	UPROPERTY(EditAnywhere, Config, Category="Detection")
	FName RoadSplineTag;

	UPROPERTY(EditAnywhere, Config, Category="Families")
	FName DefaultFamilyName;
};

