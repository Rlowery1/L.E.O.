#include "TrafficVehicleBase.h"
#include "TrafficKinematicFollower.h"
#include "TrafficRoadTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "TrafficAutomationLogger.h"
#include "TrafficLaneGeometry.h"
#include "TrafficKinematicFollower.h"
#include "TrafficMovementGeometry.h"
#include "TrafficNetworkAsset.h"
#include "TrafficRouting.h"
#include "TrafficRuntimeModule.h"
#include "TrafficSystemController.h"
#include "ZoneGraphSubsystem.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarTrafficIntersectionReservationEnabled(
	TEXT("aaa.Traffic.Intersections.ReservationEnabled"),
	1,
	TEXT("If non-zero, vehicles reserve intersections before entering (safe default to prevent conflicting overlap)."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficIntersectionReservationHoldSeconds(
	TEXT("aaa.Traffic.Intersections.ReservationHoldSeconds"),
	3.0f,
	TEXT("Seconds to hold an intersection reservation before it expires."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficIntersectionStopLineOffsetCm(
	TEXT("aaa.Traffic.Intersections.StopLineOffsetCm"),
	300.0f,
	TEXT("Distance (cm) before the intersection boundary where vehicles stop while yielding (approximate stop line)."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficIntersectionDebugReservation(
	TEXT("aaa.Traffic.Intersections.DebugReservation"),
	0,
	TEXT("If non-zero, logs reservation/wait events for vehicles at intersections."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficIntersectionDebugStopLine(
	TEXT("aaa.Traffic.Intersections.DebugStopLine"),
	0,
	TEXT("If non-zero, logs computed stop line distances for vehicles near intersections."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficIntersectionRequireFullStop(
	TEXT("aaa.Traffic.Intersections.RequireFullStop"),
	0,
	TEXT("If non-zero, vehicles will come to a full stop at the stop line even when they successfully reserve the intersection (4-way-stop style)."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficIntersectionFullStopSeconds(
	TEXT("aaa.Traffic.Intersections.FullStopSeconds"),
	0.75f,
	TEXT("Seconds to remain fully stopped at the stop line when RequireFullStop=1."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficFollowingTimeHeadwaySeconds(
	TEXT("aaa.Traffic.Following.TimeHeadwaySeconds"),
	1.6f,
	TEXT("Desired time headway to the lead vehicle when following (seconds)."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficFollowingMinGapCm(
	TEXT("aaa.Traffic.Following.MinGapCm"),
	250.f,
	TEXT("Minimum standstill gap to the lead vehicle when following (cm)."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficFollowingMaxAccelCmPerSec2(
	TEXT("aaa.Traffic.Following.MaxAccelCmPerSec2"),
	220.f,
	TEXT("Max acceleration when adjusting speed for following (cm/s^2)."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficFollowingMaxDecelCmPerSec2(
	TEXT("aaa.Traffic.Following.MaxDecelCmPerSec2"),
	500.f,
	TEXT("Max deceleration when adjusting speed for following (cm/s^2)."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficFollowingEnabled(
	TEXT("aaa.Traffic.Following.Enabled"),
	1,
	TEXT("If non-zero, vehicles adjust speed to follow a lead vehicle on the same lane/movement."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficDebugSampleVehicleCount(
	TEXT("aaa.Traffic.Debug.SampleVehicleCount"),
	3,
	TEXT("Number of vehicles (by spawn order) that emit always-on intersection transition logs.\n")
	TEXT("Set to 0 to disable. Default: 3"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficKinematicMaxYawRateDegPerSec(
	TEXT("aaa.Traffic.Kinematic.MaxYawRateDegPerSec"),
	720.f,
	TEXT("Max yaw rate (deg/sec) applied to the logic vehicle when aligning to lane/movement tangents.\n")
	TEXT("Helps prevent sudden spins when tangents are discontinuous (intersections). Default: 720"),
	ECVF_Default);

static const TCHAR* TurnTypeToString(ETrafficTurnType TurnType)
{
	switch (TurnType)
	{
	case ETrafficTurnType::Through: return TEXT("Through");
	case ETrafficTurnType::Left: return TEXT("Left");
	case ETrafficTurnType::Right: return TEXT("Right");
	case ETrafficTurnType::UTurn: return TEXT("UTurn");
	default: break;
	}
	return TEXT("Unknown");
}

static bool ShouldSampleVehicleLogs(const ATrafficVehicleBase& Vehicle)
{
	const int32 Count = FMath::Max(0, CVarTrafficDebugSampleVehicleCount.GetValueOnGameThread());
	return (Count > 0) && (Vehicle.GetDebugSpawnIndex() >= 0) && (Vehicle.GetDebugSpawnIndex() < Count);
}

static bool TryComputeStopSFromIntersectionBoundary(
	const FTrafficNetwork& Net,
	const FTrafficLane& Lane,
	const int32 IntersectionId,
	const float StopLineOffsetCm,
	float& OutStopS)
{
	const FTrafficIntersection* Intersection = Net.Intersections.FindByPredicate(
		[&](const FTrafficIntersection& I) { return I.IntersectionId == IntersectionId; });
	if (!Intersection)
	{
		return false;
	}

	const float Radius = Intersection->Radius;
	if (Radius <= KINDA_SMALL_NUMBER || Lane.CenterlinePoints.Num() < 2)
	{
		return false;
	}

	const float EffectiveRadius = Radius + FMath::Max(0.f, StopLineOffsetCm);
	const float EffectiveRadiusSq = EffectiveRadius * EffectiveRadius;

	TArray<float> CumulativeS;
	CumulativeS.SetNum(Lane.CenterlinePoints.Num());
	CumulativeS[0] = 0.f;
	for (int32 i = 1; i < Lane.CenterlinePoints.Num(); ++i)
	{
		CumulativeS[i] = CumulativeS[i - 1] + FVector::Distance(Lane.CenterlinePoints[i - 1], Lane.CenterlinePoints[i]);
	}

	const FVector Center = Intersection->Center;

	// Find the entry point where the lane crosses from outside -> inside an expanded intersection sphere.
	// We expand by StopLineOffsetCm so the stop happens *before* the intersection proper, even if the stored
	// intersection radius is tight.
	for (int32 i = Lane.CenterlinePoints.Num() - 2; i >= 0; --i)
	{
		const FVector P0 = Lane.CenterlinePoints[i];
		const FVector P1 = Lane.CenterlinePoints[i + 1];
		const float D0Sq = FVector::DistSquared(P0, Center);
		const float D1Sq = FVector::DistSquared(P1, Center);

		const bool bP0Outside = (D0Sq > EffectiveRadiusSq);
		const bool bP1InsideOrOn = (D1Sq <= EffectiveRadiusSq);
		if (!(bP0Outside && bP1InsideOrOn))
		{
			continue;
		}

		const FVector Segment = (P1 - P0);
		const float SegmentLen = Segment.Size();
		if (SegmentLen <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		// Segment-sphere intersection: |(P0 + t*D) - C|^2 = R^2, t in [0, 1]
		const FVector D = Segment / SegmentLen;
		const FVector M = P0 - Center;
		const float A = FVector::DotProduct(D, D); // ~1
		const float B = 2.0f * FVector::DotProduct(D, M);
		const float CCoef = FVector::DotProduct(M, M) - EffectiveRadiusSq;
		const float Discriminant = (B * B) - (4.0f * A * CCoef);

		float T = -1.f;
		if (Discriminant >= 0.f && A > KINDA_SMALL_NUMBER)
		{
			const float SqrtDisc = FMath::Sqrt(Discriminant);
			const float T0 = (-B - SqrtDisc) / (2.0f * A);
			const float T1 = (-B + SqrtDisc) / (2.0f * A);

			// Pick the first intersection along the segment direction.
			if (T0 >= 0.f && T0 <= SegmentLen)
			{
				T = T0;
			}
			else if (T1 >= 0.f && T1 <= SegmentLen)
			{
				T = T1;
			}
		}

		if (T < 0.f)
		{
			// Fallback: distance-based interpolation.
			const float D0 = FMath::Sqrt(D0Sq);
			const float D1 = FMath::Sqrt(D1Sq);
			const float Denom = (D0 - D1);
			const float Alpha = (FMath::Abs(Denom) > KINDA_SMALL_NUMBER) ? (D0 - EffectiveRadius) / Denom : 1.f;
			T = FMath::Clamp(Alpha * SegmentLen, 0.f, SegmentLen);
		}

		const float EntryS = CumulativeS[i] + T;
		OutStopS = FMath::Max(0.f, EntryS);
		return true;
	}

	return false;
}
ATrafficVehicleBase::ATrafficVehicleBase()
{
	PrimaryActorTick.bCanEverTick = true;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	SetRootComponent(Body);
	Body->SetMobility(EComponentMobility::Movable);
	// This actor represents the traffic "logic" vehicle and should not physically interact with Chaos vehicles/world.
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetGenerateOverlapEvents(false);

	if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
	{
		Body->SetStaticMesh(Mesh);
		Body->SetWorldScale3D(FVector(200.f, 90.f, 60.f) / 100.f);
	}

	Follower = nullptr;
}

bool ATrafficVehicleBase::GetFollowTarget(EPathFollowTargetType& OutType, int32& OutTargetId, float& OutS) const
{
	if (!Follower)
	{
		return false;
	}

	const FPathFollowState& State = Follower->GetState();
	if (State.TargetType != EPathFollowTargetType::Lane && State.TargetType != EPathFollowTargetType::Movement)
	{
		return false;
	}

	OutType = State.TargetType;
	OutTargetId = State.TargetId;
	OutS = State.S;
	return true;
}

bool ATrafficVehicleBase::SampleFollowPoseAheadOf(
	const FVector& WorldLocation,
	const FVector& WorldForward,
	float LookaheadCm,
	FVector& OutPosition,
	FVector& OutTangent) const
{
	if (bUseZoneGraphLane || !Follower)
	{
		return false;
	}

	const float Lookahead = FMath::Max(0.f, LookaheadCm);
	const FPathFollowState& State = Follower->GetState();

	if (State.TargetType == EPathFollowTargetType::Lane)
	{
		const FTrafficLane* Lane = Follower->GetCurrentLane();
		if (!Lane)
		{
			return false;
		}

		FLaneProjectionResult Proj;
		if (TrafficLaneGeometry::ProjectPointOntoLane(*Lane, WorldLocation, WorldForward, Proj))
		{
			return TrafficLaneGeometry::SamplePoseAtS(*Lane, Proj.S + Lookahead, OutPosition, OutTangent);
		}

		return TrafficLaneGeometry::SamplePoseAtS(*Lane, State.S + Lookahead, OutPosition, OutTangent);
	}

	if (State.TargetType == EPathFollowTargetType::Movement)
	{
		const FTrafficMovement* Movement = Follower->GetCurrentMovement();
		if (!Movement)
		{
			return false;
		}

		float ProjS = 0.f;
		if (TrafficMovementGeometry::ProjectPointOntoMovement(*Movement, WorldLocation, ProjS))
		{
			return TrafficMovementGeometry::SamplePoseAtS(*Movement, ProjS + Lookahead, OutPosition, OutTangent);
		}

		return TrafficMovementGeometry::SamplePoseAtS(*Movement, State.S + Lookahead, OutPosition, OutTangent);
	}

	return false;
}

void ATrafficVehicleBase::BeginPlay()
{
	Super::BeginPlay();
	if (!bUseZoneGraphLane)
	{
		EnsureFollower();
	}
}

void ATrafficVehicleBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ActiveReservedIntersectionId != INDEX_NONE && TrafficController.IsValid())
	{
		TrafficController->ReleaseIntersection(ActiveReservedIntersectionId, this);
		ActiveReservedIntersectionId = INDEX_NONE;
	}

	Super::EndPlay(EndPlayReason);
}

UTrafficKinematicFollower* ATrafficVehicleBase::EnsureFollower()
{
	if (!Follower)
	{
		Follower = NewObject<UTrafficKinematicFollower>(this, TEXT("KinematicFollower"));
	}
	return Follower;
}

void ATrafficVehicleBase::InitializeOnLane(const FTrafficLane* Lane, float InitialS, float SpeedCmPerSec)
{
	bUseZoneGraphLane = false;
	ZoneGraph = nullptr;
	ZoneSpeedCmPerSec = 0.f;
	ZoneLaneLocation = FZoneGraphLaneLocation();
	PendingOutgoingLaneId = INDEX_NONE;
	bWaitingForIntersection = false;
	WaitingIntersectionId = INDEX_NONE;
	WaitingMovementId = INDEX_NONE;
	WaitingOutgoingLaneId = INDEX_NONE;
	bHasIntersectionReservation = false;
	ReservedIntersectionId = INDEX_NONE;
	ReservedMovementId = INDEX_NONE;
	ReservedOutgoingLaneId = INDEX_NONE;
	ActiveReservedIntersectionId = INDEX_NONE;
	CruiseSpeedCmPerSec = SpeedCmPerSec;
	StopUntilTimeSeconds = -1.f;

	if (UTrafficKinematicFollower* F = EnsureFollower())
	{
		F->InitForLane(Lane, InitialS, SpeedCmPerSec);
	}

	if (Lane && ShouldSampleVehicleLogs(*this))
	{
		UE_LOG(LogTraffic, Log,
			TEXT("[TrafficVehicle] Sample[%d] %s InitOnLane lane=%d S=%.1f speed=%.1f"),
			GetDebugSpawnIndex(),
			*GetName(),
			Lane->LaneId,
			InitialS,
			SpeedCmPerSec);
	}
}

void ATrafficVehicleBase::InitializeOnMovement(const FTrafficMovement* Movement, float InitialS, float SpeedCmPerSec)
{
	bUseZoneGraphLane = false;
	ZoneGraph = nullptr;
	ZoneSpeedCmPerSec = 0.f;
	ZoneLaneLocation = FZoneGraphLaneLocation();
	CruiseSpeedCmPerSec = SpeedCmPerSec;
	StopUntilTimeSeconds = -1.f;

	if (UTrafficKinematicFollower* F = EnsureFollower())
	{
		F->InitForMovement(Movement, InitialS, SpeedCmPerSec);
	}

	if (Movement && ShouldSampleVehicleLogs(*this))
	{
		UE_LOG(LogTraffic, Log,
			TEXT("[TrafficVehicle] Sample[%d] %s InitOnMovement move=%d int=%d turn=%s inLane=%d outLane=%d speed=%.1f"),
			GetDebugSpawnIndex(),
			*GetName(),
			Movement->MovementId,
			Movement->IntersectionId,
			TurnTypeToString(Movement->TurnType),
			Movement->IncomingLaneId,
			Movement->OutgoingLaneId,
			SpeedCmPerSec);
	}
}

void ATrafficVehicleBase::SetNetworkAsset(UTrafficNetworkAsset* InNetworkAsset)
{
	NetworkAsset = InNetworkAsset;
}

void ATrafficVehicleBase::InitializeOnZoneGraphLane(UZoneGraphSubsystem* ZoneGraphSubsystem, const FZoneGraphLaneHandle& LaneHandle, float InitialDistanceCm, float SpeedCmPerSec)
{
	bUseZoneGraphLane = true;
	ZoneGraph = ZoneGraphSubsystem;
	ZoneSpeedCmPerSec = SpeedCmPerSec;
	ZoneLaneLocation = FZoneGraphLaneLocation();
	PendingOutgoingLaneId = INDEX_NONE;

	if (!ZoneGraph || !LaneHandle.IsValid())
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficVehicleBase] InitializeOnZoneGraphLane: Invalid ZoneGraph subsystem or lane handle."));
		bUseZoneGraphLane = false;
		return;
	}

	FZoneGraphLaneLocation StartLoc;
	if (!ZoneGraph->CalculateLocationAlongLane(LaneHandle, InitialDistanceCm, StartLoc))
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficVehicleBase] InitializeOnZoneGraphLane: Failed to calculate initial lane location (lane=%s)."), *LaneHandle.ToString());
		bUseZoneGraphLane = false;
		return;
	}

	ZoneLaneLocation = StartLoc;
	SetActorLocationAndRotation(StartLoc.Position, StartLoc.Direction.Rotation());
}

void ATrafficVehicleBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bUseZoneGraphLane)
	{
		if (!ZoneGraph || !ZoneLaneLocation.LaneHandle.IsValid() || DeltaSeconds <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		FZoneGraphLaneLocation NewLoc;
		const float AdvanceDistance = ZoneSpeedCmPerSec * DeltaSeconds;
		if (!ZoneGraph->AdvanceLaneLocation(ZoneLaneLocation, AdvanceDistance, NewLoc))
		{
			return;
		}

		ZoneLaneLocation = NewLoc;

		SetActorLocationAndRotation(NewLoc.Position, NewLoc.Direction.Rotation());

		const float PrevSpeed = LastSpeed;
		const float PrevAccel = LastAccel;
		const float Speed = ZoneSpeedCmPerSec;
		const float Accel = (DeltaSeconds > KINDA_SMALL_NUMBER) ? (Speed - PrevSpeed) / DeltaSeconds : 0.f;
		const float Jerk = (DeltaSeconds > KINDA_SMALL_NUMBER) ? (Accel - PrevAccel) / DeltaSeconds : 0.f;

		LastSpeed = Speed;
		LastAccel = Accel;
		LastJerk = Jerk;
		LastPos = NewLoc.Position;

		return;
	}

	if (!Follower)
	{
		return;
	}

	// Intersection reservation (safe default): stop at a virtual stop line near lane end until we can reserve the intersection.
	// This prevents multiple vehicles from overlapping conflicting movements in the middle of the junction.
	const bool bReservationEnabled =
		(CVarTrafficIntersectionReservationEnabled.GetValueOnGameThread() != 0) &&
		TrafficController.IsValid() &&
		NetworkAsset;

	float DesiredSpeedCmPerSec = CruiseSpeedCmPerSec;

	if (bReservationEnabled)
	{
		const FTrafficNetwork& Net = NetworkAsset->Network;
		const FPathFollowState& PreState = Follower->GetState();
		if (PreState.TargetType == EPathFollowTargetType::Lane)
		{
			const FTrafficLane* Lane = Follower->GetCurrentLane();
			if (Lane)
			{
				const float LaneLen = TrafficLaneGeometry::ComputeLaneLengthCm(*Lane);
				const float StopLineOffset = FMath::Max(0.f, CVarTrafficIntersectionStopLineOffsetCm.GetValueOnGameThread());
				const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

				int32 StopLineIntersectionId = INDEX_NONE;
				const FTrafficMovement* NextMovement = nullptr;
				if (bWaitingForIntersection)
				{
					StopLineIntersectionId = WaitingIntersectionId;
				}
				else if (bHasIntersectionReservation)
				{
					StopLineIntersectionId = ReservedIntersectionId;
				}
				else
				{
					NextMovement = TrafficRouting::ChooseDefaultMovementForIncomingLane(Net, Lane->LaneId);
					StopLineIntersectionId = NextMovement ? NextMovement->IntersectionId : INDEX_NONE;
				}

				float StopS = FMath::Max(0.f, LaneLen - StopLineOffset);
				bool bUsedBoundaryStopS = false;
				float IntersectionRadiusCm = 0.f;
				if (StopLineIntersectionId != INDEX_NONE)
				{
					float BoundaryStopS = 0.f;
					if (TryComputeStopSFromIntersectionBoundary(Net, *Lane, StopLineIntersectionId, StopLineOffset, BoundaryStopS))
					{
						StopS = FMath::Clamp(BoundaryStopS, 0.f, LaneLen);
						bUsedBoundaryStopS = true;
					}

					if (const FTrafficIntersection* I = Net.Intersections.FindByPredicate([&](const FTrafficIntersection& X) { return X.IntersectionId == StopLineIntersectionId; }))
					{
						IntersectionRadiusCm = I->Radius;
					}
				}

				if (CVarTrafficIntersectionDebugStopLine.GetValueOnGameThread() != 0)
				{
					const float DebugNow = Now;
					const float DebugCooldown = 0.75f;
					if (DebugNow - LastStopLineDebugTimeSeconds > DebugCooldown && PreState.S >= FMath::Max(0.f, StopS - 2000.f))
					{
						LastStopLineDebugTimeSeconds = DebugNow;
						UE_LOG(LogTraffic, Log, TEXT("[TrafficVehicle] StopLine %s lane=%d S=%.1f StopS=%.1f (method=%s) int=%d radius=%.1f effRadius=%.1f offset=%.1f laneLen=%.1f"),
							*GetName(),
							Lane->LaneId,
							PreState.S,
							StopS,
							bUsedBoundaryStopS ? TEXT("boundary") : TEXT("laneEnd"),
							StopLineIntersectionId,
							IntersectionRadiusCm,
							IntersectionRadiusCm + StopLineOffset,
							StopLineOffset,
							LaneLen);
					}
				}

				// Optional 4-way-stop style full stop even when reservation succeeds.
				if (StopUntilTimeSeconds > Now && StopLineOffset > 0.f)
				{
					DesiredSpeedCmPerSec = 0.f;
					if (PreState.S > StopS)
					{
						Follower->SetDistanceAlongTarget(StopS);
					}
				}
				else if (StopUntilTimeSeconds > 0.f && StopUntilTimeSeconds <= Now)
				{
					StopUntilTimeSeconds = -1.f;
					// Resume normal following behavior below (speed will be restored smoothly).
				}

				// If we're waiting, hold position and retry reservation.
				if (bWaitingForIntersection)
				{
					DesiredSpeedCmPerSec = 0.f;
					if (PreState.S > StopS)
					{
						Follower->SetDistanceAlongTarget(StopS);
					}

					const float HoldSeconds = CVarTrafficIntersectionReservationHoldSeconds.GetValueOnGameThread();
					if (const FTrafficMovement* WaitingMove = TrafficRouting::FindMovementById(Net, WaitingMovementId))
					{
						if (TrafficController->TryReserveIntersection(WaitingIntersectionId, this, WaitingMove->MovementId, HoldSeconds))
						{
							if (CVarTrafficIntersectionDebugReservation.GetValueOnGameThread() != 0)
							{
								UE_LOG(LogTraffic, Log, TEXT("[TrafficVehicle] %s reserved intersection %d (movement=%d) after waiting."),
									*GetName(), WaitingIntersectionId, WaitingMove->MovementId);
							}
							if (ShouldSampleVehicleLogs(*this))
							{
								UE_LOG(LogTraffic, Log,
									TEXT("[TrafficVehicle] Sample[%d] %s Reserved(after-wait) int=%d move=%d turn=%s"),
									GetDebugSpawnIndex(),
									*GetName(),
									WaitingIntersectionId,
									WaitingMove->MovementId,
									TurnTypeToString(WaitingMove->TurnType));
							}

							bWaitingForIntersection = false;
							bHasIntersectionReservation = true;
							ReservedIntersectionId = WaitingIntersectionId;
							ReservedMovementId = WaitingMove->MovementId;
							ReservedOutgoingLaneId = WaitingOutgoingLaneId;

							WaitingIntersectionId = INDEX_NONE;
							WaitingMovementId = INDEX_NONE;
							WaitingOutgoingLaneId = INDEX_NONE;

							if (CVarTrafficIntersectionRequireFullStop.GetValueOnGameThread() != 0 && StopLineOffset > 0.f)
							{
								const float StopSeconds = FMath::Max(0.f, CVarTrafficIntersectionFullStopSeconds.GetValueOnGameThread());
								StopUntilTimeSeconds = Now + StopSeconds;
								DesiredSpeedCmPerSec = 0.f;
								Follower->SetDistanceAlongTarget(StopS);
							}
						}
					}
				}
				else if (!bHasIntersectionReservation && StopLineOffset > 0.f && PreState.S >= StopS)
				{
					// We're at the stop line: attempt to reserve. If we can't, stop and wait.
					if (!NextMovement)
					{
						NextMovement = TrafficRouting::ChooseDefaultMovementForIncomingLane(Net, Lane->LaneId);
					}

					if (NextMovement)
					{
						const float HoldSeconds = CVarTrafficIntersectionReservationHoldSeconds.GetValueOnGameThread();
						if (TrafficController->TryReserveIntersection(NextMovement->IntersectionId, this, NextMovement->MovementId, HoldSeconds))
						{
							if (CVarTrafficIntersectionDebugReservation.GetValueOnGameThread() != 0)
							{
								UE_LOG(LogTraffic, Log, TEXT("[TrafficVehicle] %s reserved intersection %d (movement=%d)."),
									*GetName(), NextMovement->IntersectionId, NextMovement->MovementId);
							}
							if (ShouldSampleVehicleLogs(*this))
							{
								UE_LOG(LogTraffic, Log,
									TEXT("[TrafficVehicle] Sample[%d] %s Reserved int=%d move=%d turn=%s"),
									GetDebugSpawnIndex(),
									*GetName(),
									NextMovement->IntersectionId,
									NextMovement->MovementId,
									TurnTypeToString(NextMovement->TurnType));
							}

							bHasIntersectionReservation = true;
							ReservedIntersectionId = NextMovement->IntersectionId;
							ReservedMovementId = NextMovement->MovementId;
							ReservedOutgoingLaneId = NextMovement->OutgoingLaneId;

							if (CVarTrafficIntersectionRequireFullStop.GetValueOnGameThread() != 0)
							{
								const float StopSeconds = FMath::Max(0.f, CVarTrafficIntersectionFullStopSeconds.GetValueOnGameThread());
								StopUntilTimeSeconds = Now + StopSeconds;
								DesiredSpeedCmPerSec = 0.f;
								Follower->SetDistanceAlongTarget(StopS);
							}
						}
						else
						{
							if (CVarTrafficIntersectionDebugReservation.GetValueOnGameThread() != 0)
							{
								UE_LOG(LogTraffic, Log, TEXT("[TrafficVehicle] %s waiting for intersection %d (movement=%d)."),
									*GetName(), NextMovement->IntersectionId, NextMovement->MovementId);
							}
							if (ShouldSampleVehicleLogs(*this))
							{
								UE_LOG(LogTraffic, Log,
									TEXT("[TrafficVehicle] Sample[%d] %s Waiting int=%d move=%d turn=%s"),
									GetDebugSpawnIndex(),
									*GetName(),
									NextMovement->IntersectionId,
									NextMovement->MovementId,
									TurnTypeToString(NextMovement->TurnType));
							}

							bWaitingForIntersection = true;
							WaitingIntersectionId = NextMovement->IntersectionId;
							WaitingMovementId = NextMovement->MovementId;
							WaitingOutgoingLaneId = NextMovement->OutgoingLaneId;

							DesiredSpeedCmPerSec = 0.f;
							if (PreState.S > StopS)
							{
								Follower->SetDistanceAlongTarget(StopS);
							}
						}
					}
				}
			}
		}
	}

	// Simple car-following: cap desired speed based on a lead vehicle on the same lane/movement.
	if (CVarTrafficFollowingEnabled.GetValueOnGameThread() != 0 && Follower)
	{
		EPathFollowTargetType MyType;
		int32 MyTargetId;
		float MyS;
		if (GetFollowTarget(MyType, MyTargetId, MyS))
		{
			float BestDeltaS = TNumericLimits<float>::Max();
			float LeadSpeed = 0.f;
			float LeadLength = 0.f;

			for (TActorIterator<ATrafficVehicleBase> It(GetWorld()); It; ++It)
			{
				ATrafficVehicleBase* Other = *It;
				if (!Other || Other == this)
				{
					continue;
				}

				EPathFollowTargetType OtherType;
				int32 OtherTargetId;
				float OtherS;
				if (!Other->GetFollowTarget(OtherType, OtherTargetId, OtherS))
				{
					continue;
				}

				if (OtherType != MyType || OtherTargetId != MyTargetId)
				{
					continue;
				}

				const float DeltaS = OtherS - MyS;
				if (DeltaS <= 0.f)
				{
					continue;
				}

				if (DeltaS < BestDeltaS)
				{
					BestDeltaS = DeltaS;
					LeadSpeed = Other->GetPlannedSpeedCmPerSec();
					LeadLength = Other->ApproxVehicleLengthCm;
				}
			}

			if (BestDeltaS < TNumericLimits<float>::Max())
			{
				const float TimeHeadway = FMath::Max(0.f, CVarTrafficFollowingTimeHeadwaySeconds.GetValueOnGameThread());
				const float MinGap = FMath::Max(0.f, CVarTrafficFollowingMinGapCm.GetValueOnGameThread());

				const float EgoSpeed = Follower->GetCurrentSpeedCmPerSec();
				const float DesiredGap = MinGap + TimeHeadway * EgoSpeed;
				const float AvailableGap = FMath::Max(0.f, BestDeltaS - LeadLength);

				// If too close, cap speed (and potentially stop).
				if (AvailableGap <= DesiredGap)
				{
					DesiredSpeedCmPerSec = FMath::Min(DesiredSpeedCmPerSec, LeadSpeed);
					if (AvailableGap <= MinGap * 0.5f)
					{
						DesiredSpeedCmPerSec = 0.f;
					}
				}
			}
		}
	}

	// Apply acceleration limits for smoother motion (and better ChaosDrive targets).
	if (Follower)
	{
		const float Current = Follower->GetCurrentSpeedCmPerSec();
		const float Target = FMath::Max(0.f, DesiredSpeedCmPerSec);
		const float MaxAccel = FMath::Max(0.f, CVarTrafficFollowingMaxAccelCmPerSec2.GetValueOnGameThread());
		const float MaxDecel = FMath::Max(0.f, CVarTrafficFollowingMaxDecelCmPerSec2.GetValueOnGameThread());

		const float MaxDeltaUp = MaxAccel * FMath::Max(0.f, DeltaSeconds);
		const float MaxDeltaDown = MaxDecel * FMath::Max(0.f, DeltaSeconds);

		float NewSpeed = Target;
		if (NewSpeed > Current)
		{
			NewSpeed = FMath::Min(NewSpeed, Current + MaxDeltaUp);
		}
		else
		{
			NewSpeed = FMath::Max(NewSpeed, Current - MaxDeltaDown);
		}

		Follower->SetSpeedCmPerSec(NewSpeed);
	}

	const float CurrentSpeed = Follower->GetCurrentSpeedCmPerSec();
	Follower->Step(DeltaSeconds);

	// Simple lane->movement->lane transitions (non-ZoneGraph mode).
	// This makes vehicles continue through intersections instead of clamping at lane ends.
	if (NetworkAsset)
	{
		const FTrafficNetwork& Net = NetworkAsset->Network;
		const FPathFollowState& State = Follower->GetState();

		if (State.TargetType == EPathFollowTargetType::Lane)
		{
			const FTrafficLane* Lane = Follower->GetCurrentLane();
			if (Lane)
			{
				const float LaneLen = TrafficLaneGeometry::ComputeLaneLengthCm(*Lane);
				constexpr float EndToleranceCm = 10.f;
				if (LaneLen > KINDA_SMALL_NUMBER && State.S >= (LaneLen - EndToleranceCm))
				{
					// Reservation-enabled: only enter the movement when we already reserved it.
					if (bReservationEnabled)
					{
						const float StopLineOffset = FMath::Max(0.f, CVarTrafficIntersectionStopLineOffsetCm.GetValueOnGameThread());
						if (bHasIntersectionReservation && ReservedMovementId != INDEX_NONE)
						{
							if (const FTrafficMovement* ReservedMove = TrafficRouting::FindMovementById(Net, ReservedMovementId))
							{
								if (ShouldSampleVehicleLogs(*this))
								{
									UE_LOG(LogTraffic, Log,
										TEXT("[TrafficVehicle] Sample[%d] %s EnterIntersection int=%d move=%d turn=%s -> outLane=%d"),
										GetDebugSpawnIndex(),
										*GetName(),
										ReservedMove->IntersectionId,
										ReservedMove->MovementId,
										TurnTypeToString(ReservedMove->TurnType),
										ReservedOutgoingLaneId);
								}

								PendingOutgoingLaneId = ReservedOutgoingLaneId;
								ActiveReservedIntersectionId = ReservedIntersectionId;

								bHasIntersectionReservation = false;
								ReservedIntersectionId = INDEX_NONE;
								ReservedMovementId = INDEX_NONE;
								ReservedOutgoingLaneId = INDEX_NONE;

								InitializeOnMovement(ReservedMove, /*InitialS=*/0.0f, CruiseSpeedCmPerSec);
							}
							else
							{
								bHasIntersectionReservation = false;
								ReservedIntersectionId = INDEX_NONE;
								ReservedMovementId = INDEX_NONE;
								ReservedOutgoingLaneId = INDEX_NONE;
							}
						}
						else if (!bWaitingForIntersection && StopLineOffset > 0.f)
						{
							// We reached lane end without having a reservation; stop and wait.
							if (const FTrafficMovement* NextMovement = TrafficRouting::ChooseDefaultMovementForIncomingLane(Net, Lane->LaneId))
							{
								float StopS = FMath::Max(0.f, LaneLen - StopLineOffset);
								float BoundaryStopS = 0.f;
								if (TryComputeStopSFromIntersectionBoundary(Net, *Lane, NextMovement->IntersectionId, StopLineOffset, BoundaryStopS))
								{
									StopS = FMath::Clamp(BoundaryStopS, 0.f, LaneLen);
								}

								bWaitingForIntersection = true;
								WaitingIntersectionId = NextMovement->IntersectionId;
								WaitingMovementId = NextMovement->MovementId;
								WaitingOutgoingLaneId = NextMovement->OutgoingLaneId;

								if (ShouldSampleVehicleLogs(*this))
								{
									UE_LOG(LogTraffic, Log,
										TEXT("[TrafficVehicle] Sample[%d] %s LaneEnd->Wait int=%d move=%d turn=%s stopS=%.1f lane=%d laneLen=%.1f"),
										GetDebugSpawnIndex(),
										*GetName(),
										NextMovement->IntersectionId,
										NextMovement->MovementId,
										TurnTypeToString(NextMovement->TurnType),
										StopS,
										Lane->LaneId,
										LaneLen);
								}

								Follower->SetSpeedCmPerSec(0.f);
								Follower->SetDistanceAlongTarget(StopS);
							}
						}
					}
					else if (const FTrafficMovement* NextMovement = TrafficRouting::ChooseDefaultMovementForIncomingLane(Net, Lane->LaneId))
					{
						PendingOutgoingLaneId = NextMovement->OutgoingLaneId;
						if (ShouldSampleVehicleLogs(*this))
						{
							UE_LOG(LogTraffic, Log,
								TEXT("[TrafficVehicle] Sample[%d] %s LaneEnd->Movement move=%d int=%d turn=%s -> outLane=%d"),
								GetDebugSpawnIndex(),
								*GetName(),
								NextMovement->MovementId,
								NextMovement->IntersectionId,
								TurnTypeToString(NextMovement->TurnType),
								NextMovement->OutgoingLaneId);
						}
						InitializeOnMovement(NextMovement, /*InitialS=*/0.0f, CurrentSpeed);
					}
				}
			}
		}
		else if (State.TargetType == EPathFollowTargetType::Movement)
		{
			const FTrafficMovement* Movement = Follower->GetCurrentMovement();
			if (Movement && PendingOutgoingLaneId != INDEX_NONE)
			{
				TArray<FMovementSample> Samples;
				TrafficMovementGeometry::AnalyzeMovementPath(*Movement, Samples);
				const float MovementLen = (Samples.Num() > 0) ? Samples.Last().S : 0.f;
				constexpr float EndToleranceCm = 10.f;
				if (MovementLen > KINDA_SMALL_NUMBER && State.S >= (MovementLen - EndToleranceCm))
				{
					// Release intersection reservation once we clear the movement.
					if (ActiveReservedIntersectionId != INDEX_NONE && TrafficController.IsValid())
					{
						TrafficController->ReleaseIntersection(ActiveReservedIntersectionId, this);
						ActiveReservedIntersectionId = INDEX_NONE;
					}

					if (const FTrafficLane* OutLane = TrafficRouting::FindLaneById(Net, PendingOutgoingLaneId))
					{
						if (ShouldSampleVehicleLogs(*this))
						{
							UE_LOG(LogTraffic, Log,
								TEXT("[TrafficVehicle] Sample[%d] %s ExitMovement -> outLane=%d speed=%.1f"),
								GetDebugSpawnIndex(),
								*GetName(),
								OutLane->LaneId,
								CurrentSpeed);
						}

						// Avoid visual pops at movement->lane transition by projecting our current pose onto the outgoing lane.
						float OutS = 0.0f;
						{
							FVector MovePos, MoveTangent;
							if (TrafficMovementGeometry::SamplePoseAtS(*Movement, State.S, MovePos, MoveTangent))
							{
								FLaneProjectionResult Proj;
								if (TrafficLaneGeometry::ProjectPointOntoLane(*OutLane, MovePos, MoveTangent, Proj))
								{
									OutS = Proj.S;
								}
							}
						}

						InitializeOnLane(OutLane, /*InitialS=*/OutS, CurrentSpeed);
					}
					else
					{
						PendingOutgoingLaneId = INDEX_NONE;
					}
				}
			}
		}
	}

	FVector Pos;
	FVector Tangent;
	if (Follower->GetCurrentPose(Pos, Tangent))
	{
		SetActorLocation(Pos);

		// Keep traffic visuals upright and smooth yaw changes to reduce intersection "spin" artifacts.
		const FVector Tangent2D = FVector(Tangent.X, Tangent.Y, 0.f).GetSafeNormal();
		const float DesiredYaw = Tangent2D.IsNearlyZero() ? GetActorRotation().Yaw : Tangent2D.Rotation().Yaw;
		const float MaxYawRate = FMath::Max(0.f, CVarTrafficKinematicMaxYawRateDegPerSec.GetValueOnGameThread());
		const float CurrentYaw = GetActorRotation().Yaw;
		const float NewYaw = (MaxYawRate > 0.f) ? FMath::FixedTurn(CurrentYaw, DesiredYaw, MaxYawRate * FMath::Max(0.f, DeltaSeconds)) : DesiredYaw;
		SetActorRotation(FRotator(0.f, NewYaw, 0.f));

		// Update dynamics samples.
		const float PrevSpeed = LastSpeed;
		const float PrevAccel = LastAccel;
		const float Speed = Follower->GetCurrentSpeedCmPerSec();
		const float Accel = (DeltaSeconds > KINDA_SMALL_NUMBER) ? (Speed - PrevSpeed) / DeltaSeconds : 0.f;
		const float Jerk = (DeltaSeconds > KINDA_SMALL_NUMBER) ? (Accel - PrevAccel) / DeltaSeconds : 0.f;

		LastSpeed = Speed;
		LastAccel = Accel;
		LastJerk = Jerk;
		LastPos = Pos;
	}
}

void ATrafficVehicleBase::SampleLaneTrackingError(FTrafficRunMetrics& Metrics) const
{
	if (!Follower)
	{
		return;
	}

	const FTrafficLane* Lane = Follower->GetCurrentLane();
	if (!Lane || Lane->CenterlinePoints.Num() < 2)
	{
		return;
	}

	float S = Follower->GetDistanceAlongLane();
	FVector LanePos, LaneTangent;
	if (!TrafficLaneGeometry::SamplePoseAtS(*Lane, S, LanePos, LaneTangent))
	{
		return;
	}

	const FVector VehPos = GetActorLocation();
	const FVector VehForward = GetActorForwardVector().GetSafeNormal();

	const FVector LaneRight = FVector::CrossProduct(FVector::UpVector, LaneTangent.GetSafeNormal()).GetSafeNormal();
	const float LateralError = FVector::DotProduct(VehPos - LanePos, LaneRight); // signed

	const float HeadingDot = FVector::DotProduct(VehForward, LaneTangent.GetSafeNormal());
	const float HeadingAngleRad = FMath::Acos(FMath::Clamp(HeadingDot, -1.f, 1.f));
	const float HeadingErrorDeg = FMath::RadiansToDegrees(HeadingAngleRad);

	Metrics.AccumulateLateralError(FMath::Abs(LateralError));
	Metrics.AccumulateHeadingError(FMath::Abs(HeadingErrorDeg));
}

void ATrafficVehicleBase::SampleDynamics(FTrafficRunMetrics& Metrics, float DeltaSeconds)
{
	if (DeltaSeconds <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	Metrics.AccumulateAccel(FMath::Abs(LastAccel));
	Metrics.AccumulateJerk(FMath::Abs(LastJerk));
}

void ATrafficVehicleBase::SetDebugBodyVisible(bool bVisible)
{
	if (Body)
	{
		// Even when visible for debugging, keep collision disabled to avoid pushing Chaos traffic vehicles.
		Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Body->SetGenerateOverlapEvents(false);
		Body->SetVisibility(bVisible, true);
		Body->SetHiddenInGame(!bVisible);
	}
}

float ATrafficVehicleBase::GetPlannedSpeedCmPerSec() const
{
	if (bUseZoneGraphLane)
	{
		return FMath::Max(0.f, ZoneSpeedCmPerSec);
	}

	return Follower ? FMath::Max(0.f, Follower->GetCurrentSpeedCmPerSec()) : 0.f;
}
