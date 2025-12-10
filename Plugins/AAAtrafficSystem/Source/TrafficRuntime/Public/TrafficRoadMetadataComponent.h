#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TrafficRoadMetadataComponent.generated.h"

UCLASS(ClassGroup=(Traffic), Blueprintable, meta=(BlueprintSpawnableComponent))
class TRAFFICRUNTIME_API UTrafficRoadMetadataComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTrafficRoadMetadataComponent();

	UPROPERTY(EditAnywhere, Category="Traffic")
	FName FamilyName;

	UPROPERTY(EditAnywhere, Category="Traffic")
	bool bIncludeInTraffic = true;

	UPROPERTY(EditAnywhere, Category="Traffic")
	FGuid RoadFamilyId;
};

