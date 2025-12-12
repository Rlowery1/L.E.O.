#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RoadKitProfile.generated.h"

/**
 * Data asset defining heuristic filters and defaults for a road kit.
 */
UCLASS(BlueprintType)
class TRAFFICRUNTIME_API URoadKitProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Keywords that identify drivable materials (e.g. Asphalt, Road). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic")
	TArray<FString> DrivableMaterialKeywords;

	/** Mesh name keywords to exclude (e.g. Tree, Lamp). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic")
	TArray<FString> ExcludedMeshNameKeywords;

	/** Maximum lateral offset from the actor origin to consider (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic")
	float MaxLateralOffset = 500.f;

	/** Maximum height above actor origin to consider (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic")
	float MaxHeight = 100.f;

	/** Default number of lanes for kits without width estimation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic")
	int32 DefaultLaneCount = 2;

	/** Default lane width (cm) used when the mesh width cannot be inferred. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic")
	float DefaultLaneWidth = 350.f;
};
