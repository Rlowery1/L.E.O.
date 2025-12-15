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
	TEXT("Distance (cm) before the lane end where vehicles stop while yielding."),
	ECVF_Default);

ATrafficVehicleBase::ATrafficVehicleBase()
{
	PrimaryActorTick.bCanEverTick = true;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	SetRootComponent(Body);

	if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
	{
		Body->SetStaticMesh(Mesh);
		Body->SetWorldScale3D(FVector(200.f, 90.f, 60.f) / 100.f);
	}

	Follower = nullptr;
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

	if (UTrafficKinematicFollower* F = EnsureFollower())
	{
		F->InitForLane(Lane, InitialS, SpeedCmPerSec);
	}
}

void ATrafficVehicleBase::InitializeOnMovement(const FTrafficMovement* Movement, float InitialS, float SpeedCmPerSec)
{
	bUseZoneGraphLane = false;
	ZoneGraph = nullptr;
	ZoneSpeedCmPerSec = 0.f;
	ZoneLaneLocation = FZoneGraphLaneLocation();
	CruiseSpeedCmPerSec = SpeedCmPerSec;

	if (UTrafficKinematicFollower* F = EnsureFollower())
	{
		F->InitForMovement(Movement, InitialS, SpeedCmPerSec);
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
				const float StopS = FMath::Max(0.f, LaneLen - StopLineOffset);

				// If we're waiting, hold position and retry reservation.
				if (bWaitingForIntersection)
				{
					Follower->SetSpeedCmPerSec(0.f);
					if (PreState.S > StopS)
					{
						Follower->SetDistanceAlongTarget(StopS);
					}

					const float HoldSeconds = CVarTrafficIntersectionReservationHoldSeconds.GetValueOnGameThread();
					if (const FTrafficMovement* WaitingMove = TrafficRouting::FindMovementById(Net, WaitingMovementId))
					{
						if (TrafficController->TryReserveIntersection(WaitingIntersectionId, this, WaitingMove->MovementId, HoldSeconds))
						{
							bWaitingForIntersection = false;
							bHasIntersectionReservation = true;
							ReservedIntersectionId = WaitingIntersectionId;
							ReservedMovementId = WaitingMove->MovementId;
							ReservedOutgoingLaneId = WaitingOutgoingLaneId;

							WaitingIntersectionId = INDEX_NONE;
							WaitingMovementId = INDEX_NONE;
							WaitingOutgoingLaneId = INDEX_NONE;

							Follower->SetSpeedCmPerSec(CruiseSpeedCmPerSec);
						}
					}
				}
				else if (!bHasIntersectionReservation && StopLineOffset > 0.f && PreState.S >= StopS)
				{
					// We're at the stop line: attempt to reserve. If we can't, stop and wait.
					if (const FTrafficMovement* NextMovement = TrafficRouting::ChooseDefaultMovementForIncomingLane(Net, Lane->LaneId))
					{
						const float HoldSeconds = CVarTrafficIntersectionReservationHoldSeconds.GetValueOnGameThread();
						if (TrafficController->TryReserveIntersection(NextMovement->IntersectionId, this, NextMovement->MovementId, HoldSeconds))
						{
							bHasIntersectionReservation = true;
							ReservedIntersectionId = NextMovement->IntersectionId;
							ReservedMovementId = NextMovement->MovementId;
							ReservedOutgoingLaneId = NextMovement->OutgoingLaneId;
						}
						else
						{
							bWaitingForIntersection = true;
							WaitingIntersectionId = NextMovement->IntersectionId;
							WaitingMovementId = NextMovement->MovementId;
							WaitingOutgoingLaneId = NextMovement->OutgoingLaneId;

							Follower->SetSpeedCmPerSec(0.f);
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
								const float StopS = FMath::Max(0.f, LaneLen - StopLineOffset);
								bWaitingForIntersection = true;
								WaitingIntersectionId = NextMovement->IntersectionId;
								WaitingMovementId = NextMovement->MovementId;
								WaitingOutgoingLaneId = NextMovement->OutgoingLaneId;
								Follower->SetSpeedCmPerSec(0.f);
								Follower->SetDistanceAlongTarget(StopS);
							}
						}
					}
					else if (const FTrafficMovement* NextMovement = TrafficRouting::ChooseDefaultMovementForIncomingLane(Net, Lane->LaneId))
					{
						PendingOutgoingLaneId = NextMovement->OutgoingLaneId;
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
						InitializeOnLane(OutLane, /*InitialS=*/0.0f, CurrentSpeed);
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
		SetActorRotation(Tangent.Rotation());

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
		Body->SetVisibility(bVisible, true);
		Body->SetHiddenInGame(!bVisible);
	}
}
