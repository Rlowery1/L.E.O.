#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TrafficDrivableSurfaceComponent.generated.h"

/**
 * Tag component marking a mesh as drivable so the geometry provider can skip heuristic filtering.
 */
UCLASS(ClassGroup=(Traffic), meta=(BlueprintSpawnableComponent))
class TRAFFICRUNTIME_API UTrafficDrivableSurfaceComponent : public UActorComponent
{
	GENERATED_BODY()
};
