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

	/**
	 * If true, reverses the actor's sampled centerline before lane generation.
	 *
	 * Use this for one-way pieces (e.g. CityBLD ramps) where two visually-similar actors should drive in opposite
	 * directions, but still share the same family calibration (lane width/offsets).
	 */
	UPROPERTY(EditAnywhere, Category="Traffic")
	bool bReverseCenterlineDirection = false;

	/** If true, PREPARE MAP will not auto-adjust bReverseCenterlineDirection for this actor. */
	UPROPERTY(EditAnywhere, Category="Traffic")
	bool bLockReverseDirection = false;

	UPROPERTY(EditAnywhere, Category="Traffic")
	FGuid RoadFamilyId;
};
