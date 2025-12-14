#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrafficLaneEndpointMarkerActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class TRAFFICEDITOR_API ATrafficLaneEndpointMarkerActor : public AActor
{
	GENERATED_BODY()

public:
	ATrafficLaneEndpointMarkerActor();

	UPROPERTY(EditAnywhere, Category="Traffic")
	int32 LaneId = INDEX_NONE;

	UPROPERTY(EditAnywhere, Category="Traffic")
	bool bIsStart = true;

	UPROPERTY(EditAnywhere, Category="Traffic")
	int32 IntersectionId = INDEX_NONE;

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> MarkerMesh;
};

