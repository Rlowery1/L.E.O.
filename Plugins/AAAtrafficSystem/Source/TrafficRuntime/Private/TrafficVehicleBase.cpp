#include "TrafficVehicleBase.h"
#include "TrafficKinematicFollower.h"
#include "TrafficRoadTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "TrafficAutomationLogger.h"
#include "TrafficLaneGeometry.h"
#include "TrafficKinematicFollower.h"

ATrafficVehicleBase::ATrafficVehicleBase()
{
	PrimaryActorTick.bCanEverTick = true;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	SetRootComponent(Body);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (MeshFinder.Succeeded())
	{
		Body->SetStaticMesh(MeshFinder.Object);
		Body->SetWorldScale3D(FVector(200.f, 90.f, 60.f) / 100.f);
	}

	Follower = nullptr;
}

void ATrafficVehicleBase::BeginPlay()
{
	Super::BeginPlay();
	EnsureFollower();
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
	if (UTrafficKinematicFollower* F = EnsureFollower())
	{
		F->InitForLane(Lane, InitialS, SpeedCmPerSec);
	}
}

void ATrafficVehicleBase::InitializeOnMovement(const FTrafficMovement* Movement, float InitialS, float SpeedCmPerSec)
{
	if (UTrafficKinematicFollower* F = EnsureFollower())
	{
		F->InitForMovement(Movement, InitialS, SpeedCmPerSec);
	}
}

void ATrafficVehicleBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Follower)
	{
		return;
	}

	Follower->Step(DeltaSeconds);

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
