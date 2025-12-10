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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 LaneId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RoadId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SideIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Width = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector> CenterlinePoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ELaneDirection Direction = ELaneDirection::Forward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PrevLaneId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 NextLaneId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpeedLimitKmh = 50.f;
};

USTRUCT(BlueprintType)
struct TRAFFICRUNTIME_API FTrafficRoad
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RoadId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 FamilyId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector> CenterlinePoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FTrafficLane> Lanes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<AActor> SourceActor;
};

USTRUCT(BlueprintType)
struct TRAFFICRUNTIME_API FTrafficIntersection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 IntersectionId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int32> IncomingLaneIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int32> OutgoingLaneIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Center = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Radius = 0.f;
};

USTRUCT(BlueprintType)
struct TRAFFICRUNTIME_API FTrafficMovement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MovementId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 IntersectionId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 IncomingLaneId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 OutgoingLaneId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ETrafficTurnType TurnType = ETrafficTurnType::Through;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector> PathPoints;
};

USTRUCT(BlueprintType)
struct TRAFFICRUNTIME_API FTrafficNetwork
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FTrafficRoad> Roads;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FTrafficLane> Lanes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FTrafficIntersection> Intersections;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FTrafficMovement> Movements;
};

