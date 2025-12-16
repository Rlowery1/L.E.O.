#include "TrafficVehicleAdapter.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/MovementComponent.h"
#include "Components/PrimitiveComponent.h"
#include "TrafficRuntimeModule.h"
#include "TrafficVehicleBase.h"
#include "HAL/IConsoleManager.h"
#include "UObject/UnrealType.h"
#include "Engine/CollisionProfile.h"

static TAutoConsoleVariable<int32> CVarTrafficKinematicChaosVisuals(
	TEXT("aaa.Traffic.Visual.KinematicChaosVisuals"),
	1,
	TEXT("If non-zero, AAA Traffic will treat spawned Chaos vehicle pawns as kinematic visuals:\n")
	TEXT("  - disables physics simulation on their primitive components\n")
	TEXT("  - disables movement components\n")
	TEXT("  - keeps collision enabled\n")
	TEXT("This prevents jitter/spinning caused by teleport-following active Chaos physics vehicles.\n")
	TEXT("Default: 1"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficVisualMode(
	TEXT("aaa.Traffic.Visual.Mode"),
	1,
	TEXT("Traffic visual mode:\n")
	TEXT("  0 = LogicOnly (no spawned visual pawns)\n")
	TEXT("  1 = KinematicVisual (teleport-follow; physics off)\n")
	TEXT("  2 = ChaosDrive (apply throttle/brake/steer to follow logic)\n")
	TEXT("Default: 1"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveLookaheadCm(
	TEXT("aaa.Traffic.Visual.ChaosDrive.LookaheadCm"),
	1200.f,
	TEXT("Lookahead distance (cm) when steering a Chaos vehicle to follow the logic vehicle."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveSteerGain(
	TEXT("aaa.Traffic.Visual.ChaosDrive.SteerGain"),
	1.0f,
	TEXT("Steering gain (multiplies yaw error in radians)."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveThrottleGain(
	TEXT("aaa.Traffic.Visual.ChaosDrive.ThrottleGain"),
	0.0010f,
	TEXT("Throttle gain (multiplies positive speed error in cm/sec)."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveBrakeGain(
	TEXT("aaa.Traffic.Visual.ChaosDrive.BrakeGain"),
	0.0015f,
	TEXT("Brake gain (multiplies negative speed error in cm/sec)."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveInputSlewRatePerSec(
	TEXT("aaa.Traffic.Visual.ChaosDrive.InputSlewRatePerSec"),
	5.0f,
	TEXT("Max change per second for throttle/brake/steer inputs (helps prevent spin-outs)."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveInitialBrakeSeconds(
	TEXT("aaa.Traffic.Visual.ChaosDrive.InitialBrakeSeconds"),
	0.5f,
	TEXT("Seconds to hold full brake after spawn (lets physics settle before driving)."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveDisableMovementSeconds(
	TEXT("aaa.Traffic.Visual.ChaosDrive.DisableMovementSeconds"),
	0.f,
	TEXT("If >0, keeps the Chaos vehicle movement component deactivated for this many seconds after spawn (debug tool to isolate wheel/road instability)."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDrivePhysicsWarmupSeconds(
	TEXT("aaa.Traffic.Visual.ChaosDrive.PhysicsWarmupSeconds"),
	0.5f,
	TEXT("Seconds after spawn to keep physics simulation disabled on the Chaos pawn, while holding it in place (reduces spawn impulses/flip-outs)."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveUprightFixSeconds(
	TEXT("aaa.Traffic.Visual.ChaosDrive.UprightFixSeconds"),
	0.0f,
	TEXT("Seconds after spawn during which AAA Traffic will auto-correct a Chaos vehicle that ends up upside-down."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveUprightFixMinUpZ(
	TEXT("aaa.Traffic.Visual.ChaosDrive.UprightFixMinUpZ"),
	-0.2f,
	TEXT("If the Chaos pawn's UpVector.Z is below this value during UprightFixSeconds, it is considered upside-down and will be corrected."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveUprightFixLiftCm(
	TEXT("aaa.Traffic.Visual.ChaosDrive.UprightFixLiftCm"),
	30.0f,
	TEXT("Z lift (cm) applied when correcting an upside-down Chaos pawn (reduces re-penetration impulses)."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveUprightFixMaxSpeedCmPerSec(
	TEXT("aaa.Traffic.Visual.ChaosDrive.UprightFixMaxSpeedCmPerSec"),
	200.0f,
	TEXT("Max speed (cm/s) at which an upside-down Chaos pawn will be auto-corrected (prevents mid-drive teleport fixes)."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficChaosDriveDebug(
	TEXT("aaa.Traffic.Visual.ChaosDrive.Debug"),
	0,
	TEXT("If non-zero, logs ChaosDrive adapter initialization and missing bindings."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficCollisionIgnoreOtherTraffic(
	TEXT("aaa.Traffic.Collision.IgnoreOtherTraffic"),
	0,
	TEXT("If non-zero, spawned traffic vehicles ignore collisions with other traffic vehicles.\n")
	TEXT("This uses the collision profile 'AAA_TrafficVehicle' (configured via plugin Config/DefaultEngine.ini).\n")
	TEXT("Default: 0"),
	ECVF_Default);

namespace
{
	enum class ETrafficVisualMode : int32
	{
		LogicOnly = 0,
		KinematicVisual = 1,
		ChaosDrive = 2,
	};

	static ETrafficVisualMode GetTrafficVisualMode()
	{
		const int32 Raw = CVarTrafficVisualMode.GetValueOnGameThread();
		if (Raw <= 0)
		{
			return ETrafficVisualMode::LogicOnly;
		}
		if (Raw == 2)
		{
			return ETrafficVisualMode::ChaosDrive;
		}
		return ETrafficVisualMode::KinematicVisual;
	}

	static UMovementComponent* FindChaosMovementComponent(APawn* Pawn)
	{
		if (!Pawn)
		{
			return nullptr;
		}

		TArray<UMovementComponent*> MoveComps;
		Pawn->GetComponents(MoveComps);

		auto LooksLikeChaosWheeled = [](const UMovementComponent* Move) -> bool
		{
			if (!Move)
			{
				return false;
			}
			const FString Name = Move->GetClass()->GetName();
			return Name.Contains(TEXT("ChaosWheeledVehicleMovementComponent"), ESearchCase::IgnoreCase);
		};

		for (UMovementComponent* Move : MoveComps)
		{
			if (LooksLikeChaosWheeled(Move))
			{
				return Move;
			}
		}

		// Fallback: first movement component that has the expected input functions.
		for (UMovementComponent* Move : MoveComps)
		{
			if (!Move)
			{
				continue;
			}
			if (Move->FindFunction(FName(TEXT("SetThrottleInput"))) &&
				Move->FindFunction(FName(TEXT("SetSteeringInput"))) &&
				Move->FindFunction(FName(TEXT("SetBrakeInput"))))
			{
				return Move;
			}
		}

		return nullptr;
	}

static FString ListMovementComponents(APawn* Pawn)
{
		if (!Pawn)
		{
			return TEXT("<null pawn>");
		}

		TArray<UMovementComponent*> MoveComps;
		Pawn->GetComponents(MoveComps);

		TArray<FString> Names;
		Names.Reserve(MoveComps.Num());
		for (const UMovementComponent* Move : MoveComps)
		{
			if (Move)
			{
				Names.Add(Move->GetClass()->GetName());
			}
		}

		return (Names.Num() > 0) ? FString::Join(Names, TEXT(", ")) : TEXT("<none>");
	}

	static float SlewTo(float Current, float Target, float RatePerSec, float DeltaSeconds)
	{
		const float MaxDelta = FMath::Max(0.f, RatePerSec) * FMath::Max(0.f, DeltaSeconds);
		return FMath::Clamp(Target, Current - MaxDelta, Current + MaxDelta);
	}

	static bool HasCollisionProfile(const FName ProfileName)
	{
		const UCollisionProfile* Profile = UCollisionProfile::Get();
		if (!Profile)
		{
			return false;
		}

		FCollisionResponseTemplate Template;
		return Profile->GetProfileTemplate(ProfileName, Template);
	}

	static void ApplyTrafficCollisionProfile(APawn* Pawn)
	{
		if (!Pawn)
		{
			return;
		}

		if (CVarTrafficCollisionIgnoreOtherTraffic.GetValueOnGameThread() == 0)
		{
			return;
		}

		static const FName TrafficProfileName(TEXT("AAA_TrafficVehicle"));
		if (!HasCollisionProfile(TrafficProfileName))
		{
			UE_LOG(LogTraffic, Warning,
				TEXT("[TrafficVehicleAdapter] Collision profile '%s' not found. Traffic-vs-traffic collision ignoring is disabled.\n")
				TEXT("Ensure plugin Config/DefaultEngine.ini is merged into your project config."),
				*TrafficProfileName.ToString());
			return;
		}

		// Apply to root and any primitive with collision enabled.
		if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Pawn->GetRootComponent()))
		{
			RootPrim->SetCollisionProfileName(TrafficProfileName);
		}

		TArray<UPrimitiveComponent*> PrimComps;
		Pawn->GetComponents(PrimComps);
		for (UPrimitiveComponent* Prim : PrimComps)
		{
			if (!Prim)
			{
				continue;
			}

			if (Prim->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
			{
				continue;
			}

			Prim->SetCollisionProfileName(TrafficProfileName);
		}
	}

	static bool CallSingleParam(UObject* Obj, const FName FuncName, float FloatValue, bool BoolValue)
	{
		if (!Obj)
		{
			return false;
		}

		UFunction* Fn = Obj->FindFunction(FuncName);
		if (!Fn)
		{
			return false;
		}

		FProperty* Param = nullptr;
		int32 ParamCount = 0;
		for (TFieldIterator<FProperty> It(Fn); It; ++It)
		{
			FProperty* P = *It;
			if (!P)
			{
				continue;
			}
			if (P->HasAnyPropertyFlags(CPF_Parm) && !P->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				Param = P;
				++ParamCount;
			}
		}
		if (ParamCount != 1 || !Param)
		{
			return false;
		}

		TArray<uint8> Buffer;
		Buffer.SetNumZeroed(Fn->ParmsSize);

		if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Param))
		{
			FloatProp->SetPropertyValue_InContainer(Buffer.GetData(), FloatValue);
		}
		else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Param))
		{
			BoolProp->SetPropertyValue_InContainer(Buffer.GetData(), BoolValue);
		}
		else
		{
			return false;
		}

		Obj->ProcessEvent(Fn, Buffer.GetData());
		return true;
	}

	static void ApplyInitialBraking(UMovementComponent* Move)
	{
		if (!Move)
		{
			return;
		}

		CallSingleParam(Move, FName(TEXT("SetSteeringInput")), 0.f, false);
		CallSingleParam(Move, FName(TEXT("SetThrottleInput")), 0.f, false);
		CallSingleParam(Move, FName(TEXT("SetBrakeInput")), 1.f, false);

		// Some Chaos vehicles expose a handbrake input as well.
		CallSingleParam(Move, FName(TEXT("SetHandbrakeInput")), 1.f, true);
	}
}

static void ZeroPhysicsVelocities(APawn& Pawn)
{
	TArray<UPrimitiveComponent*> PrimComps;
	Pawn.GetComponents(PrimComps);
	for (UPrimitiveComponent* Prim : PrimComps)
	{
		if (!Prim || !Prim->IsSimulatingPhysics())
		{
			continue;
		}

		Prim->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Prim->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		Prim->PutRigidBodyToSleep();
	}
}

static void ForceUpright(APawn& Pawn, float LiftCm)
{
	TArray<UPrimitiveComponent*> SimComps;
	{
		TArray<UPrimitiveComponent*> PrimComps;
		Pawn.GetComponents(PrimComps);
		for (UPrimitiveComponent* Prim : PrimComps)
		{
			if (Prim && Prim->IsSimulatingPhysics())
			{
				SimComps.Add(Prim);
				Prim->SetSimulatePhysics(false);
			}
		}
	}

	const FVector Loc = Pawn.GetActorLocation() + FVector(0.f, 0.f, FMath::Max(0.f, LiftCm));
	const float Yaw = Pawn.GetActorRotation().Yaw;
	Pawn.SetActorLocationAndRotation(Loc, FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);

	for (UPrimitiveComponent* Prim : SimComps)
	{
		if (Prim)
		{
			Prim->SetSimulatePhysics(true);
		}
	}

	ZeroPhysicsVelocities(Pawn);
}

ATrafficVehicleAdapter::ATrafficVehicleAdapter()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATrafficVehicleAdapter::Initialize(ATrafficVehicleBase* InLogic, APawn* InChaos)
{
	LogicVehicle = InLogic;
	ChaosVehicle = InChaos;
	bLoggedChaosDriveInit = false;
	bLoggedChaosDriveMissingMovement = false;
	bChaosDriveMovementSuspended = false;
	ChaosDriveMovementResumeAgeSeconds = 0.f;
	bChaosDriveUprightFixed = false;
	bChaosDrivePhysicsSuspended = false;
	ChaosDrivePhysicsResumeAgeSeconds = 0.f;
	ChaosDrivePhysicsComps.Reset();
	PrevSteer = 0.f;
	PrevThrottle = 0.f;
	PrevBrake = 0.f;

	if (ChaosVehicle.IsValid())
	{
		const ETrafficVisualMode Mode = GetTrafficVisualMode();
		const bool bDebug = (CVarTrafficChaosDriveDebug.GetValueOnGameThread() != 0);

		// By default, use kinematic-follow mode for Chaos pawns. Teleporting a fully simulated Chaos vehicle each tick
		// tends to cause spinning/jitter and looks broken in PIE.
		const bool bKinematic =
			(Mode == ETrafficVisualMode::KinematicVisual) &&
			(CVarTrafficKinematicChaosVisuals.GetValueOnGameThread() != 0);

		ChaosVehicle->SetActorEnableCollision(true);

		if (bKinematic)
		{
			TArray<UMovementComponent*> MoveComps;
			ChaosVehicle->GetComponents(MoveComps);
			for (UMovementComponent* Move : MoveComps)
			{
				if (Move)
				{
					Move->Deactivate();
					Move->SetComponentTickEnabled(false);
				}
			}

			TArray<UPrimitiveComponent*> PrimComps;
			ChaosVehicle->GetComponents(PrimComps);
			for (UPrimitiveComponent* Prim : PrimComps)
			{
				if (!Prim)
				{
					continue;
				}
				if (Prim->IsSimulatingPhysics())
				{
					Prim->SetSimulatePhysics(false);
				}
				Prim->SetEnableGravity(false);
				Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			}
		}
		else if (Mode == ETrafficVisualMode::ChaosDrive)
		{
			// In drive mode, the logic vehicle is just a "ghost" path target and should not collide with the physical pawn.
			if (LogicVehicle.IsValid())
			{
				LogicVehicle->SetActorEnableCollision(false);
			}

			ApplyTrafficCollisionProfile(ChaosVehicle.Get());

			const float PhysicsWarmupSeconds = FMath::Max(0.f, CVarTrafficChaosDrivePhysicsWarmupSeconds.GetValueOnGameThread());
			if (PhysicsWarmupSeconds > 0.f)
			{
				bChaosDrivePhysicsSuspended = true;
				ChaosDrivePhysicsResumeAgeSeconds = PhysicsWarmupSeconds;

				TArray<UPrimitiveComponent*> PrimComps;
				ChaosVehicle->GetComponents(PrimComps);
				for (UPrimitiveComponent* Prim : PrimComps)
				{
					if (!Prim)
					{
						continue;
					}

					FChaosDrivePrimWarmupState State;
					State.Prim = Prim;
					State.bWasSimulatingPhysics = Prim->IsSimulatingPhysics();
					State.bWasGravityEnabled = Prim->IsGravityEnabled();
					ChaosDrivePhysicsComps.Add(State);

					Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					Prim->SetEnableGravity(false);
					if (State.bWasSimulatingPhysics)
					{
						Prim->SetSimulatePhysics(false);
					}
				}

				// Movement must stay off while physics is suspended.
				TArray<UMovementComponent*> MoveComps;
				ChaosVehicle->GetComponents(MoveComps);
				for (UMovementComponent* Move : MoveComps)
				{
					if (Move)
					{
						Move->Deactivate();
						Move->SetComponentTickEnabled(false);
					}
				}
			}

			const float DisableMoveSeconds = FMath::Max(0.f, CVarTrafficChaosDriveDisableMovementSeconds.GetValueOnGameThread());
			if (DisableMoveSeconds > 0.f)
			{
				bChaosDriveMovementSuspended = true;
				ChaosDriveMovementResumeAgeSeconds = DisableMoveSeconds;

				TArray<UMovementComponent*> MoveComps;
				ChaosVehicle->GetComponents(MoveComps);
				for (UMovementComponent* Move : MoveComps)
				{
					if (Move)
					{
						Move->Deactivate();
						Move->SetComponentTickEnabled(false);
					}
				}
			}
			else
			{
				// Ensure movement components are active.
				TArray<UMovementComponent*> MoveComps;
				ChaosVehicle->GetComponents(MoveComps);
				for (UMovementComponent* Move : MoveComps)
				{
					if (Move)
					{
						if (!bChaosDrivePhysicsSuspended)
						{
							Move->Activate(true);
							Move->SetComponentTickEnabled(true);
						}
					}
				}
			}

			// Ensure primitive components are allowed to simulate physics if configured in the pawn/mesh.
			TArray<UPrimitiveComponent*> PrimComps;
			ChaosVehicle->GetComponents(PrimComps);
			for (UPrimitiveComponent* Prim : PrimComps)
			{
				if (!Prim)
				{
					continue;
				}
				if (!bChaosDrivePhysicsSuspended)
				{
					Prim->SetEnableGravity(true);
				}
				Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			}

			// Apply an initial "full brake" immediately so the first physics tick doesn't start with residual inputs.
			if (UMovementComponent* Move = FindChaosMovementComponent(ChaosVehicle.Get()))
			{
				if (!bChaosDrivePhysicsSuspended)
				{
					ApplyInitialBraking(Move);
				}
			}

			if (bDebug)
			{
				UMovementComponent* Move = FindChaosMovementComponent(ChaosVehicle.Get());
				UE_LOG(LogTraffic, Log,
					TEXT("[TrafficVehicleAdapter] ChaosDrive init: Logic=%s Chaos=%s Move=%s MoveComps=[%s]"),
					LogicVehicle.IsValid() ? *LogicVehicle->GetName() : TEXT("<null>"),
					ChaosVehicle.IsValid() ? *ChaosVehicle->GetName() : TEXT("<null>"),
					Move ? *Move->GetClass()->GetName() : TEXT("<none>"),
					*ListMovementComponents(ChaosVehicle.Get()));
			}
		}
	}
}

void ATrafficVehicleAdapter::BeginPlay()
{
	Super::BeginPlay();
}

void ATrafficVehicleAdapter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!LogicVehicle.IsValid() || !ChaosVehicle.IsValid())
	{
		return;
	}

	const ETrafficVisualMode Mode = GetTrafficVisualMode();

	if (Mode == ETrafficVisualMode::ChaosDrive)
	{
		// If physics is suspended, hold the pawn steady until the warmup expires.
		if (bChaosDrivePhysicsSuspended && GetWorld())
		{
			const float Age = ChaosVehicle->GetGameTimeSinceCreation();
			if (Age >= ChaosDrivePhysicsResumeAgeSeconds)
			{
				bChaosDrivePhysicsSuspended = false;

				for (const FChaosDrivePrimWarmupState& State : ChaosDrivePhysicsComps)
				{
					if (UPrimitiveComponent* Prim = State.Prim.Get())
					{
						Prim->SetEnableGravity(State.bWasGravityEnabled);
						if (State.bWasSimulatingPhysics)
						{
							Prim->SetSimulatePhysics(true);
						}
					}
				}
				ChaosDrivePhysicsComps.Reset();

				ZeroPhysicsVelocities(*ChaosVehicle.Get());

				// Now it is safe to enable movement if not explicitly suspended.
				if (!bChaosDriveMovementSuspended)
				{
					TArray<UMovementComponent*> MoveComps;
					ChaosVehicle->GetComponents(MoveComps);
					for (UMovementComponent* Move : MoveComps)
					{
						if (Move)
						{
							Move->Activate(true);
							Move->SetComponentTickEnabled(true);
						}
					}

					if (UMovementComponent* Move = FindChaosMovementComponent(ChaosVehicle.Get()))
					{
						ApplyInitialBraking(Move);
					}
				}
			}
			else
			{
				ZeroPhysicsVelocities(*ChaosVehicle.Get());
				return;
			}
		}

		// Spawn stabilization: if the pawn ends up upside-down immediately after spawn, force it upright once.
		{
			const float FixSeconds = FMath::Max(0.f, CVarTrafficChaosDriveUprightFixSeconds.GetValueOnGameThread());
			if (!bChaosDriveUprightFixed && FixSeconds > 0.f && GetWorld())
			{
				const float Age = ChaosVehicle->GetGameTimeSinceCreation();
				if (Age <= FixSeconds)
				{
					const float MinUpZ = CVarTrafficChaosDriveUprightFixMinUpZ.GetValueOnGameThread();
					const float UpZ = ChaosVehicle->GetActorUpVector().Z;
					const float Speed = ChaosVehicle->GetVelocity().Size();
					const float MaxSpeed = FMath::Max(0.f, CVarTrafficChaosDriveUprightFixMaxSpeedCmPerSec.GetValueOnGameThread());
					if (UpZ < MinUpZ && Speed <= MaxSpeed)
					{
						const float Lift = CVarTrafficChaosDriveUprightFixLiftCm.GetValueOnGameThread();
						ForceUpright(*ChaosVehicle.Get(), Lift);
						bChaosDriveUprightFixed = true;

						if (CVarTrafficChaosDriveDebug.GetValueOnGameThread() != 0)
						{
							UE_LOG(LogTraffic, Warning,
								TEXT("[TrafficVehicleAdapter] UprightFix applied to %s (age=%.2fs upZ=%.2f speed=%.1fcm/s lift=%.1fcm)"),
								*ChaosVehicle->GetName(),
								Age,
								UpZ,
								Speed,
								Lift);
						}
					}
				}
			}
		}

		if (bChaosDriveMovementSuspended && GetWorld())
		{
			const float Age = ChaosVehicle->GetGameTimeSinceCreation();
			if (Age >= ChaosDriveMovementResumeAgeSeconds)
			{
				bChaosDriveMovementSuspended = false;

				TArray<UMovementComponent*> MoveComps;
				ChaosVehicle->GetComponents(MoveComps);
				for (UMovementComponent* Move : MoveComps)
				{
					if (Move)
					{
						Move->Activate(true);
						Move->SetComponentTickEnabled(true);
					}
				}

				PrevSteer = 0.f;
				PrevThrottle = 0.f;
				PrevBrake = 1.f;
			}
			else
			{
				// Keep doing nothing while movement is disabled (isolates physics/collision issues from drive inputs).
				return;
			}
		}

		UMovementComponent* Move = FindChaosMovementComponent(ChaosVehicle.Get());
		if (!Move)
		{
			if (!bLoggedChaosDriveMissingMovement && CVarTrafficChaosDriveDebug.GetValueOnGameThread() != 0)
			{
				bLoggedChaosDriveMissingMovement = true;
				UE_LOG(LogTraffic, Warning,
					TEXT("[TrafficVehicleAdapter] ChaosDrive: No suitable movement component on %s. MoveComps=[%s]"),
					*ChaosVehicle->GetName(),
					*ListMovementComponents(ChaosVehicle.Get()));
			}
			return;
		}

		const float InitialBrakeSeconds = FMath::Max(0.f, CVarTrafficChaosDriveInitialBrakeSeconds.GetValueOnGameThread());
		if (InitialBrakeSeconds > 0.f && GetWorld())
		{
			const float Age = ChaosVehicle->GetGameTimeSinceCreation();
			if (Age < InitialBrakeSeconds)
			{
				ApplyInitialBraking(Move);
				return;
			}
		}

		const FVector ChaosPos = ChaosVehicle->GetActorLocation();
		const FVector ChaosFwd = ChaosVehicle->GetActorForwardVector();
		const FVector LogicPos = LogicVehicle->GetActorLocation();
		const FVector LogicFwd = LogicVehicle->GetActorForwardVector();

		const float Lookahead = FMath::Max(0.f, CVarTrafficChaosDriveLookaheadCm.GetValueOnGameThread());
		const FVector Target = LogicPos + LogicFwd * Lookahead;

		const FVector2D ToTarget2D(Target.X - ChaosPos.X, Target.Y - ChaosPos.Y);
		const FVector2D Fwd2D(ChaosFwd.X, ChaosFwd.Y);

		const float ToLen = ToTarget2D.Size();
		const FVector2D ToN = (ToLen > KINDA_SMALL_NUMBER) ? (ToTarget2D / ToLen) : FVector2D::ZeroVector;
		const FVector2D FwdN = Fwd2D.GetSafeNormal();

		const float Cross = (FwdN.X * ToN.Y) - (FwdN.Y * ToN.X);
		const float Dot = (FwdN.X * ToN.X) + (FwdN.Y * ToN.Y);
		const float YawErrorRad = FMath::Atan2(Cross, Dot);

		const float SteerGain = CVarTrafficChaosDriveSteerGain.GetValueOnGameThread();
		float Steering = FMath::Clamp(YawErrorRad * SteerGain, -1.f, 1.f);

		const float DesiredSpeed = LogicVehicle->GetPlannedSpeedCmPerSec();
		const FVector Vel = ChaosVehicle->GetVelocity();
		const float CurrentForwardSpeed = FVector::DotProduct(Vel, ChaosFwd); // signed cm/s
		const float CurrentSpeed = FMath::Abs(CurrentForwardSpeed);
		const float SpeedError = DesiredSpeed - CurrentSpeed;

		const float ThrottleGain = FMath::Max(0.f, CVarTrafficChaosDriveThrottleGain.GetValueOnGameThread());
		const float BrakeGain = FMath::Max(0.f, CVarTrafficChaosDriveBrakeGain.GetValueOnGameThread());

		float Throttle = 0.f;
		float Brake = 0.f;
		if (SpeedError >= 0.f)
		{
			Throttle = FMath::Clamp(SpeedError * ThrottleGain, 0.f, 1.f);
		}
		else
		{
			Brake = FMath::Clamp((-SpeedError) * BrakeGain, 0.f, 1.f);
		}

		// Prevent aggressive oscillations/spin-outs by limiting per-frame input changes.
		const float SlewRate = CVarTrafficChaosDriveInputSlewRatePerSec.GetValueOnGameThread();
		Steering = SlewTo(PrevSteer, Steering, SlewRate, DeltaSeconds);
		Throttle = SlewTo(PrevThrottle, Throttle, SlewRate, DeltaSeconds);
		Brake = SlewTo(PrevBrake, Brake, SlewRate, DeltaSeconds);
		PrevSteer = Steering;
		PrevThrottle = Throttle;
		PrevBrake = Brake;

		const bool bSteerOk = CallSingleParam(Move, FName(TEXT("SetSteeringInput")), Steering, false);
		const bool bThrottleOk = CallSingleParam(Move, FName(TEXT("SetThrottleInput")), Throttle, false);
		const bool bBrakeOk = CallSingleParam(Move, FName(TEXT("SetBrakeInput")), Brake, false);
		if ((!bSteerOk || !bThrottleOk || !bBrakeOk) && !bLoggedChaosDriveInit && CVarTrafficChaosDriveDebug.GetValueOnGameThread() != 0)
		{
			bLoggedChaosDriveInit = true;
			UE_LOG(LogTraffic, Warning,
				TEXT("[TrafficVehicleAdapter] ChaosDrive: Missing input bindings on %s (Move=%s steer=%d throttle=%d brake=%d)"),
				*ChaosVehicle->GetName(),
				Move ? *Move->GetClass()->GetName() : TEXT("<none>"),
				bSteerOk ? 1 : 0,
				bThrottleOk ? 1 : 0,
				bBrakeOk ? 1 : 0);
		}
		return;
	}

	// Kinematic transform-follow mode for deterministic tests.
	ChaosVehicle->SetActorLocationAndRotation(
		LogicVehicle->GetActorLocation(),
		LogicVehicle->GetActorRotation(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
}
