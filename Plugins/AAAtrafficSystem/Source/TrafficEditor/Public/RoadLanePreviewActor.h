#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrafficRoadFamilySettings.h"
#include "RoadLanePreviewActor.generated.h"

class USplineComponent;

UCLASS(NotBlueprintable)
class TRAFFICEDITOR_API ARoadLanePreviewActor : public AActor
{
	GENERATED_BODY()

public:
	ARoadLanePreviewActor();

#if WITH_EDITOR
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
#endif

	void BuildPreviewFromSpline(USplineComponent* SourceSpline, const FRoadFamilyDefinition& Family);

protected:
	UPROPERTY()
	TObjectPtr<USplineComponent> CenterlinePreview;

	UPROPERTY()
	TArray<TObjectPtr<USplineComponent>> LaneSplines;

	void ClearLaneSplines();
	USplineComponent* CreateOrGetCenterlineComponent();
	USplineComponent* CreateLaneSplineComponent();

	void BuildSideLanes(
		USplineComponent* SourceSpline,
		const FTrafficLaneLayoutSide& Side,
		bool bForwardDirection,
		TArray<USplineComponent*>& OutCreatedSplines);
};

