#pragma once

#include "CoreMinimal.h"
#include "TrafficRoadTypes.generated.h"

UENUM(BlueprintType)
enum class ELaneDirection : uint8
{
	Forward,
	Backward,
	Bidirectional
};

UENUM(BlueprintType)
enum class ETrafficTurnType : uint8
{
	Through,
	Left,
	Right,
	UTurn
};

USTRUCT(BlueprintType)
struct TRAFFICRUNTIME_API FTrafficLane
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Lane")
	int32 LaneId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Lane")
	int32 RoadId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Lane")
	int32 SideIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Lane")
	float Width = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Lane")
	TArray<FVector> CenterlinePoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Lane")
	ELaneDirection Direction = ELaneDirection::Forward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Lane")
	int32 PrevLaneId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Lane")
	int32 NextLaneId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Lane")
	float SpeedLimitKmh = 50.f;
};

USTRUCT(BlueprintType)
struct TRAFFICRUNTIME_API FTrafficRoad
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Road")
	int32 RoadId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Road")
	int32 FamilyId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Road")
	TArray<FVector> CenterlinePoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Road")
	TArray<FTrafficLane> Lanes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Road")
	TSoftObjectPtr<AActor> SourceActor;
};

USTRUCT(BlueprintType)
struct TRAFFICRUNTIME_API FTrafficIntersection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Intersection")
	int32 IntersectionId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Intersection")
	TArray<int32> IncomingLaneIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Intersection")
	TArray<int32> OutgoingLaneIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Intersection")
	FVector Center = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Intersection")
	float Radius = 0.f;
};

USTRUCT(BlueprintType)
struct TRAFFICRUNTIME_API FTrafficMovement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Movement")
	int32 MovementId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Movement")
	int32 IntersectionId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Movement")
	int32 IncomingLaneId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Movement")
	int32 OutgoingLaneId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Movement")
	ETrafficTurnType TurnType = ETrafficTurnType::Through;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Movement")
	TArray<FVector> PathPoints;
};

USTRUCT(BlueprintType)
struct TRAFFICRUNTIME_API FTrafficNetwork
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Network")
	TArray<FTrafficRoad> Roads;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Network")
	TArray<FTrafficLane> Lanes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Network")
	TArray<FTrafficIntersection> Intersections;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic|Network")
	TArray<FTrafficMovement> Movements;
};
