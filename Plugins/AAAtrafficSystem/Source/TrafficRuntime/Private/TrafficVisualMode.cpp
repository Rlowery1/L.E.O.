#include "TrafficVisualMode.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarTrafficVisualMode(
	TEXT("aaa.Traffic.Visual.Mode"),
	2,
	TEXT("Traffic visual mode:\n")
	TEXT("  0 = LogicOnly (no spawned visual pawns)\n")
	TEXT("  1 = KinematicVisual (teleport-follow; physics off)\n")
	TEXT("  2 = ChaosDrive (apply throttle/brake/steer to follow logic)\n")
	TEXT("Default: 2"),
	ECVF_Default);

int32 GetTrafficVisualModeRaw()
{
	return CVarTrafficVisualMode.GetValueOnGameThread();
}

ETrafficVisualMode GetTrafficVisualMode()
{
	const int32 Raw = GetTrafficVisualModeRaw();
	if (Raw <= 0)
	{
		return ETrafficVisualMode::LogicOnly;
	}
	if (Raw == 2)
	{
		return ETrafficVisualMode::ChaosDrive;
	}
	return ETrafficVisualMode::KinematicVisual;
}

