#pragma once

#include "CoreMinimal.h"
#include "UObject/NameTypes.h"

class UStaticMesh;

class TRAFFICRUNTIME_API FTrafficCalibrationVisuals
{
public:
	// Returns a static mesh that represents a flat arrow, created fully in code and owned by the transient package.
	static UStaticMesh* GetOrCreateCalibrationArrowMesh();
};
