#pragma once

#include "CoreMinimal.h"
#include "TrafficGeometryProvider.h"
#include "StaticMeshRoadGeometryProvider.generated.h"

class URoadKitProfile;
class UStaticMeshComponent;

/**
 * Generic static mesh geometry provider. Extracts a centerline from meshes using configurable heuristics.
 */
UCLASS()
class TRAFFICRUNTIME_API UStaticMeshRoadGeometryProvider : public UObject, public ITrafficRoadGeometryProvider
{
	GENERATED_BODY()

public:
	UStaticMeshRoadGeometryProvider();

	virtual void CollectRoads(UWorld* World, TArray<FTrafficRoad>& OutRoads) override;
	virtual bool GetDisplayCenterlineForActor(AActor* RoadActor, TArray<FVector>& OutPoints) const override;

protected:
	bool IsComponentDrivable(const UStaticMeshComponent* Comp) const;
	bool BuildCenterlineFromActor(const AActor* Actor, TArray<FVector>& OutPoints) const;
	void ExtractCentreline(const TArray<FVector>& Vertices, TArray<FVector>& OutPoints) const;

	/** Active heuristic settings; loaded from project config or default profile. */
	UPROPERTY()
	TObjectPtr<URoadKitProfile> ActiveProfile;
};
