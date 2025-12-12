#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TrafficRoadMeshTagComponent.generated.h"

class UStaticMeshComponent;

/**
 * Component that marks a mesh component as usable for traffic geometry sampling.
 * Attach this component to any StaticMeshComponent and set bUseForTraffic=true
 * to include the mesh in calibration; otherwise it will be ignored.
 */
UCLASS(ClassGroup=(Traffic), meta=(BlueprintSpawnableComponent))
class TRAFFICRUNTIME_API UTrafficRoadMeshTagComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTrafficRoadMeshTagComponent();

	/** Whether the owning mesh should be used for traffic geometry extraction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic")
	bool bUseForTraffic = false;

	/** Optional target mesh to which this tag applies. If unset, applies to all mesh components on the actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic")
	class UStaticMeshComponent* TargetMesh = nullptr;
};
