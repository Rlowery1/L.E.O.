#pragma once

#include "CoreMinimal.h"

enum class ETrafficVisualMode : int32
{
	LogicOnly = 0,
	KinematicVisual = 1,
	ChaosDrive = 2,
};

/** Returns the current traffic visual mode (derived from `aaa.Traffic.Visual.Mode`). */
TRAFFICRUNTIME_API ETrafficVisualMode GetTrafficVisualMode();

/** Returns the raw int value of `aaa.Traffic.Visual.Mode` (unclamped). */
TRAFFICRUNTIME_API int32 GetTrafficVisualModeRaw();

