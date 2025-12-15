#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TrafficRuntimeWorldSubsystem.generated.h"

/**
 * Runtime bootstrap for AAA Traffic.
 *
 * When enabled in Project Settings, this will auto-spawn a TrafficSystemController and start traffic
 * at BeginPlay in PIE/Game worlds (one-button runtime setup for end users).
 */
UCLASS()
class TRAFFICRUNTIME_API UTrafficRuntimeWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
};

