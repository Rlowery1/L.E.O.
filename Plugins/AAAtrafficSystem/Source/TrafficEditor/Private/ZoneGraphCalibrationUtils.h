// Utility functions for ZoneGraph-based calibration.
#pragma once

#include "CoreMinimal.h"
#include "TrafficLaneCalibration.h"

/**
 * Computes lane layout for a given road based on ZoneGraph data.
 * @param World The editor world containing ZoneGraph data.
 * @param RoadCenterlinePoints Points along the road centerline (world space).
 * @param OutCalib (output) The computed calibration (lanes per direction, width, offset).
 * @return true if a matching ZoneGraph lane layout was found; false otherwise.
 */
bool ComputeCalibrationFromZoneGraph(
	UWorld* World,
	const TArray<FVector>& RoadCenterlinePoints,
	FTrafficLaneFamilyCalibration& OutCalib);

