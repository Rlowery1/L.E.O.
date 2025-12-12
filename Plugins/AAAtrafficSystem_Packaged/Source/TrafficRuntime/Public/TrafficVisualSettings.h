#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "TrafficVisualSettings.generated.h"

class UStaticMesh;
class UMaterialInterface;

UCLASS(Config=Game, DefaultConfig)
class TRAFFICRUNTIME_API UTrafficVisualSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UTrafficVisualSettings();

	UPROPERTY(EditAnywhere, Config, Category="Lanes")
	TSoftObjectPtr<UStaticMesh> ForwardLaneArrowMesh;

	UPROPERTY(EditAnywhere, Config, Category="Lanes")
	TSoftObjectPtr<UStaticMesh> BackwardLaneArrowMesh;

	UPROPERTY(EditAnywhere, Config, Category="Road")
	TSoftObjectPtr<UMaterialInterface> RoadRibbonMaterial;

	UPROPERTY(EditAnywhere, Config, Category="Road", meta=(ClampMin="0.01", ClampMax="100.0"))
	float RoadRibbonUVScale = 1.0f;

	UPROPERTY(EditAnywhere, Config, Category="Lane Arrows", meta=(ClampMin="100.0", ClampMax="1000.0"))
	float ArrowLength = 450.f;

	UPROPERTY(EditAnywhere, Config, Category="Lane Arrows", meta=(ClampMin="50.0", ClampMax="500.0"))
	float ArrowWidth = 180.f;

	UPROPERTY(EditAnywhere, Config, Category="Lane Arrows", meta=(ClampMin="200.0", ClampMax="1500.0"))
	float ArrowSpacing = 600.f;

	UPROPERTY(EditAnywhere, Config, Category="Debug")
	bool bShowDebugSplines = false;

	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;

#if WITH_EDITOR
	virtual FText GetSectionText() const override;
#endif
};

