// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

// ZoneGraph lane direction enum + tag mask live in ZoneGraphTypes.h (ZoneGraph plugin).
#include "ZoneGraphTypes.h"

#include "TrafficZoneLaneProfile.generated.h"

/**
 * Simple data asset describing a ZoneGraph lane profile to be used by AAA Traffic when generating ZoneGraph shapes.
 *
 * NOTE: ZoneGraph's native lane profiles live in ZoneGraph plugin settings (UZoneGraphSettings). AAA Traffic uses
 * this asset as a source-of-truth and mirrors it into ZoneGraphSettings at build time.
 */
UCLASS(BlueprintType)
class TRAFFICRUNTIME_API UTrafficZoneLaneProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Optional name override for the resulting ZoneGraph profile. Defaults to this asset name when empty. */
	UPROPERTY(EditAnywhere, Category = "ZoneLaneProfile")
	FName ProfileName;

	/** Number of parallel lanes in the profile. */
	UPROPERTY(EditAnywhere, Category = "ZoneLaneProfile", meta = (ClampMin = "1", ClampMax = "8"))
	int32 NumLanes = 2;

	/** Width for each lane (cm). */
	UPROPERTY(EditAnywhere, Category = "ZoneLaneProfile", meta = (ClampMin = "10.0", ClampMax = "2000.0", Units = "cm"))
	float LaneWidthCm = 350.f;

	/** Direction for all lanes in this profile. */
	UPROPERTY(EditAnywhere, Category = "ZoneLaneProfile")
	EZoneLaneDirection Direction = EZoneLaneDirection::Forward;

	/** Tag name that should be set on each lane in the profile (e.g. Vehicles, FootTraffic). */
	UPROPERTY(EditAnywhere, Category = "ZoneLaneProfile")
	FName LaneTagName = FName(TEXT("Vehicles"));
};
