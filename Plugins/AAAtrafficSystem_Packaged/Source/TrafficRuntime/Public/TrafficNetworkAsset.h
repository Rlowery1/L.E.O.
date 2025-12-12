#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TrafficRoadTypes.h"
#include "TrafficNetworkAsset.generated.h"

UCLASS(BlueprintType)
class TRAFFICRUNTIME_API UTrafficNetworkAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traffic")
	FTrafficNetwork Network;
};

