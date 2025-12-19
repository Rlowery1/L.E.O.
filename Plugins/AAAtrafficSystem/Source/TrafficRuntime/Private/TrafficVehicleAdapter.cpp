#include "TrafficVehicleAdapter.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/MovementComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "TrafficRuntimeModule.h"
#include "TrafficVehicleBase.h"
#include "TrafficKinematicFollower.h"
#include "TrafficVisualMode.h"
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

static TAutoConsoleVariable<float> CVarTrafficChaosDriveDriveDelaySeconds(
	TEXT("aaa.Traffic.Visual.ChaosDrive.DriveDelaySeconds"),
	0.25f,
	TEXT("Seconds after spawn to apply full brake and no throttle (lets wheels/contact settle before driving)."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveDriveRampSeconds(
	TEXT("aaa.Traffic.Visual.ChaosDrive.DriveRampSeconds"),
	1.0f,
	TEXT("Seconds after DriveDelaySeconds to ramp desired speed from 0 to planned speed (reduces spin-outs at spawn)."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficChaosDriveWheelTraceChannelOverride(
	TEXT("aaa.Traffic.Visual.ChaosDrive.WheelTraceChannelOverride"),
	-1,
	TEXT("Override the collision channel used for Chaos wheel/suspension traces and AAA Traffic grounding checks.\n")
	TEXT("Set to -1 to auto-detect from the vehicle movement component (WheelTraceCollisionChannel), otherwise use an ECollisionChannel value.\n")
	TEXT("Common values: 0=WorldStatic 1=WorldDynamic 3=Visibility 6=Vehicle. Default: -1"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficChaosDriveRequireGroundedToDrive(
	TEXT("aaa.Traffic.Visual.ChaosDrive.RequireGroundedToDrive"),
	1,
	TEXT("If non-zero, AAA Traffic holds full brake and zero throttle until the Chaos pawn is detected as grounded (via downward trace).\n")
	TEXT("Prevents wheel-in-air torque, spawn flip-outs, and endless spinning before first ground contact.\n")
	TEXT("Default: 1"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficChaosDriveAutoDetectWheelTraceChannel(
	TEXT("aaa.Traffic.Visual.ChaosDrive.AutoDetectWheelTraceChannel"),
	1,
	TEXT("If non-zero, AAA Traffic will auto-detect a wheel/suspension trace channel that hits the ground under the lane target.\n")
	TEXT("This can override bad vehicle/road collision channel mismatches and improves road-kit agnostic Chaos stability.\n")
	TEXT("Default: 1"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveRequireGroundedMaxSeconds(
	TEXT("aaa.Traffic.Visual.ChaosDrive.RequireGroundedMaxSeconds"),
	30.0f,
	TEXT("Max seconds after spawn to require grounding before driving. After this, AAA Traffic will attempt to drive even if grounding is not detected.\n")
	TEXT("Default: 30.0"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveGroundedMaxGapCm(
	TEXT("aaa.Traffic.Visual.ChaosDrive.GroundedMaxGapCm"),
	75.0f,
	TEXT("Max vertical gap (cm) between pawn bounds bottom and the wheel trace hit point to consider the pawn grounded.\n")
	TEXT("Default: 75"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveGroundedTraceDepthCm(
	TEXT("aaa.Traffic.Visual.ChaosDrive.GroundedTraceDepthCm"),
	8000.0f,
	TEXT("How far (cm) to trace down from the pawn bounds bottom when detecting grounding.\n")
	TEXT("Default: 8000"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveRoadHitMaxAboveLaneCm(
	TEXT("aaa.Traffic.Visual.ChaosDrive.RoadHitMaxAboveLaneCm"),
	200.0f,
	TEXT("When searching for road collision under a lane, ignores hits whose Z is more than this many cm ABOVE the lane Z.\n")
	TEXT("Prevents snapping vehicles onto overhead collision (trees, bridges) when road collision is not ready.\n")
	TEXT("Default: 200"),
	ECVF_Default);

static ECollisionChannel ResolveChaosWheelTraceChannel(UMovementComponent* Move)
{
	const int32 Override = CVarTrafficChaosDriveWheelTraceChannelOverride.GetValueOnGameThread();
	if (Override >= 0 && Override <= 31)
	{
		return static_cast<ECollisionChannel>(Override);
	}

	if (!Move)
	{
		// Legacy default (older code assumed WorldDynamic). Prefer auto-detection when available.
		return ECC_WorldDynamic;
	}

	static const FName WheelTraceChannelPropName(TEXT("WheelTraceCollisionChannel"));
	if (FProperty* Prop = Move->GetClass()->FindPropertyByName(WheelTraceChannelPropName))
	{
		if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
		{
			if (const FNumericProperty* Underlying = EnumProp->GetUnderlyingProperty())
			{
				const int64 V = Underlying->GetSignedIntPropertyValue(Prop->ContainerPtrToValuePtr<void>(Move));
				if (V >= 0 && V <= 31)
				{
					return static_cast<ECollisionChannel>(V);
				}
			}
		}
		else if (const FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
		{
			const uint8 V = ByteProp->GetPropertyValue_InContainer(Move);
			if (V <= 31)
			{
				return static_cast<ECollisionChannel>(V);
			}
		}
		else if (const FIntProperty* IntProp = CastField<FIntProperty>(Prop))
		{
			const int32 V = IntProp->GetPropertyValue_InContainer(Move);
			if (V >= 0 && V <= 31)
			{
				return static_cast<ECollisionChannel>(V);
			}
		}
	}

	// Common Chaos default is Visibility, but we avoid hard-coding assumptions; keep legacy behavior when we can't detect.
	return ECC_WorldDynamic;
}

static bool ApplyChaosWheelTraceChannelOverride(UMovementComponent* Move)
{
	if (!Move)
	{
		return false;
	}

	const int32 Override = CVarTrafficChaosDriveWheelTraceChannelOverride.GetValueOnGameThread();
	if (Override < 0 || Override > 31)
	{
		return false;
	}

	static const FName WheelTraceChannelPropName(TEXT("WheelTraceCollisionChannel"));
	FProperty* Prop = Move->GetClass()->FindPropertyByName(WheelTraceChannelPropName);
	if (!Prop)
	{
		return false;
	}

	void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Move);
	if (!ValuePtr)
	{
		return false;
	}

	// Support the common representations used by Chaos vehicle movement components across engine versions.
	if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
	{
		if (FNumericProperty* Underlying = EnumProp->GetUnderlyingProperty())
		{
			Underlying->SetIntPropertyValue(ValuePtr, static_cast<int64>(Override));
			return true;
		}
	}
	else if (FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
	{
		ByteProp->SetPropertyValue(ValuePtr, static_cast<uint8>(Override));
		return true;
	}
	else if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
	{
		IntProp->SetPropertyValue(ValuePtr, Override);
		return true;
	}

	return false;
}

static bool SetWheelTraceCollisionChannel(UMovementComponent* Move, ECollisionChannel Channel)
{
	if (!Move)
	{
		return false;
	}

	static const FName WheelTraceChannelPropName(TEXT("WheelTraceCollisionChannel"));
	FProperty* Prop = Move->GetClass()->FindPropertyByName(WheelTraceChannelPropName);
	if (!Prop)
	{
		return false;
	}

	void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Move);
	if (!ValuePtr)
	{
		return false;
	}

	const int64 V = static_cast<int64>(Channel);
	if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
	{
		if (FNumericProperty* Underlying = EnumProp->GetUnderlyingProperty())
		{
			Underlying->SetIntPropertyValue(ValuePtr, V);
			return true;
		}
	}
	else if (FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
	{
		ByteProp->SetPropertyValue(ValuePtr, static_cast<uint8>(V));
		return true;
	}
	else if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
	{
		IntProp->SetPropertyValue(ValuePtr, static_cast<int32>(V));
		return true;
	}

	return false;
}

static bool SupportsWheelTraceChannelProperty(UMovementComponent* Move)
{
	if (!Move)
	{
		return false;
	}

	static const FName WheelTraceChannelPropName(TEXT("WheelTraceCollisionChannel"));
	return (Move->GetClass()->FindPropertyByName(WheelTraceChannelPropName) != nullptr);
}

static void AppendUniqueChannel(TArray<ECollisionChannel>& InOut, ECollisionChannel Channel)
{
	const int32 V = static_cast<int32>(Channel);
	if (V < 0 || V > 31)
	{
		return;
	}
	if (!InOut.Contains(Channel))
	{
		InOut.Add(Channel);
	}
}

static TArray<ECollisionChannel> BuildWheelTraceChannelCandidates(ECollisionChannel Preferred)
{
	TArray<ECollisionChannel> Channels;
	Channels.Reserve(32);

	AppendUniqueChannel(Channels, Preferred);
	AppendUniqueChannel(Channels, ECC_WorldStatic);
	AppendUniqueChannel(Channels, ECC_WorldDynamic);
	AppendUniqueChannel(Channels, ECC_Visibility);

	// Also try game trace channels (road kits often use a custom trace channel for wheel/suspension).
	AppendUniqueChannel(Channels, ECC_GameTraceChannel1);
	AppendUniqueChannel(Channels, ECC_GameTraceChannel2);
	AppendUniqueChannel(Channels, ECC_GameTraceChannel3);
	AppendUniqueChannel(Channels, ECC_GameTraceChannel4);
	AppendUniqueChannel(Channels, ECC_GameTraceChannel5);
	AppendUniqueChannel(Channels, ECC_GameTraceChannel6);
	AppendUniqueChannel(Channels, ECC_GameTraceChannel7);
	AppendUniqueChannel(Channels, ECC_GameTraceChannel8);
	AppendUniqueChannel(Channels, ECC_GameTraceChannel9);
	AppendUniqueChannel(Channels, ECC_GameTraceChannel10);
	AppendUniqueChannel(Channels, ECC_GameTraceChannel11);
	AppendUniqueChannel(Channels, ECC_GameTraceChannel12);
	AppendUniqueChannel(Channels, ECC_GameTraceChannel13);
	AppendUniqueChannel(Channels, ECC_GameTraceChannel14);
	AppendUniqueChannel(Channels, ECC_GameTraceChannel15);
	AppendUniqueChannel(Channels, ECC_GameTraceChannel16);
	AppendUniqueChannel(Channels, ECC_GameTraceChannel17);
	AppendUniqueChannel(Channels, ECC_GameTraceChannel18);

	return Channels;
}

static const TCHAR* CollisionChannelName(const ECollisionChannel Channel)
{
	switch (Channel)
	{
	case ECC_WorldStatic: return TEXT("WorldStatic");
	case ECC_WorldDynamic: return TEXT("WorldDynamic");
	case ECC_Pawn: return TEXT("Pawn");
	case ECC_Visibility: return TEXT("Visibility");
	case ECC_Camera: return TEXT("Camera");
	case ECC_PhysicsBody: return TEXT("PhysicsBody");
	case ECC_Vehicle: return TEXT("Vehicle");
	case ECC_Destructible: return TEXT("Destructible");
	default: return TEXT("Other");
	}
}

static TAutoConsoleVariable<float> CVarTrafficChaosDriveTurnSlowdownStartDeg(
	TEXT("aaa.Traffic.Visual.ChaosDrive.TurnSlowdownStartDeg"),
	15.0f,
	TEXT("Yaw error (degrees) at which AAA Traffic starts reducing target speed for ChaosDrive."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveTurnSlowdownFullDeg(
	TEXT("aaa.Traffic.Visual.ChaosDrive.TurnSlowdownFullDeg"),
	60.0f,
	TEXT("Yaw error (degrees) at which AAA Traffic reduces target speed to near-zero for ChaosDrive."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveSpawnSpinDampSeconds(
	TEXT("aaa.Traffic.Visual.ChaosDrive.SpawnSpinDampSeconds"),
	3.0f,
	TEXT("Seconds after spawn during which AAA Traffic will damp excessive chassis angular velocity (helps stop continuous spin)."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveSpawnSpinDampMaxAngularDegPerSec(
	TEXT("aaa.Traffic.Visual.ChaosDrive.SpawnSpinDampMaxAngularDegPerSec"),
	180.0f,
	TEXT("If chassis angular speed exceeds this (deg/sec) during SpawnSpinDampSeconds, it will be clamped."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveSpawnSpinDampMaxSpeedCmPerSec(
	TEXT("aaa.Traffic.Visual.ChaosDrive.SpawnSpinDampMaxSpeedCmPerSec"),
	1200.0f,
	TEXT("Only applies spin damping when vehicle speed is below this (cm/sec)."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficChaosDriveLowSpeedSpinClamp(
	TEXT("aaa.Traffic.Visual.ChaosDrive.LowSpeedSpinClamp"),
	1,
	TEXT("If non-zero, clamps excessive chassis angular velocity any time the vehicle is moving slowly (helps stop endless post-collision spins).\n")
	TEXT("Default: 1"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveLowSpeedSpinClampMaxAngularDegPerSec(
	TEXT("aaa.Traffic.Visual.ChaosDrive.LowSpeedSpinClampMaxAngularDegPerSec"),
	360.0f,
	TEXT("If LowSpeedSpinClamp is enabled and chassis angular speed exceeds this (deg/sec) at low speed, it will be clamped."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveLowSpeedSpinClampMaxSpeedCmPerSec(
	TEXT("aaa.Traffic.Visual.ChaosDrive.LowSpeedSpinClampMaxSpeedCmPerSec"),
	250.0f,
	TEXT("Max speed (cm/sec) at which LowSpeedSpinClamp may apply."),
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

static TAutoConsoleVariable<int32> CVarTrafficChaosDriveHoldUntilRoadCollision(
	TEXT("aaa.Traffic.Visual.ChaosDrive.HoldUntilRoadCollision"),
	1,
	TEXT("If non-zero, and no ground collision is detected under a ChaosDrive pawn, AAA Traffic temporarily disables physics and hides the pawn\n")
	TEXT("until ground collision becomes available (common when road kits build collision asynchronously at PIE start).\n")
	TEXT("Default: 1"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveHoldUntilRoadCollisionMaxSeconds(
	TEXT("aaa.Traffic.Visual.ChaosDrive.HoldUntilRoadCollisionMaxSeconds"),
	30.0f,
	TEXT("Max seconds after spawn to hold/hide a ChaosDrive pawn while waiting for road collision. Default: 30.0"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveHoldBoundsMaxExtentCm(
	TEXT("aaa.Traffic.Visual.ChaosDrive.HoldBoundsMaxExtentCm"),
	400.0f,
	TEXT("Max bounds extent (cm) considered reliable for hold/snap placement. Default: 400"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveHoldBoundsMaxOffsetFromLogicCm(
	TEXT("aaa.Traffic.Visual.ChaosDrive.HoldBoundsMaxOffsetFromLogicCm"),
	500.0f,
	TEXT("Max distance (cm) between bounds center Z and logic Z to consider bounds reliable. Default: 500"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveHoldMaxSnapCm(
	TEXT("aaa.Traffic.Visual.ChaosDrive.HoldMaxSnapCm"),
	200.0f,
	TEXT("Max Z correction (cm) allowed when snapping a held pawn to ground using bounds. Larger corrections fall back to safe placement. Default: 200"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveHoldLogicGroundMaxGapCm(
	TEXT("aaa.Traffic.Visual.ChaosDrive.HoldLogicGroundMaxGapCm"),
	50.0f,
	TEXT("Max Z gap (cm) between logic lane position and ground hit to treat as grounded when bounds are unreliable. Default: 50"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveHoldUntilRoadCollisionMinStableSeconds(
	TEXT("aaa.Traffic.Visual.ChaosDrive.HoldUntilRoadCollisionMinStableSeconds"),
	0.5f,
	TEXT("Minimum consecutive seconds of valid ground contact before releasing the road-collision hold. Default: 0.5"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficChaosDriveHoldUntilRoadCollisionAllowRehold(
	TEXT("aaa.Traffic.Visual.ChaosDrive.HoldUntilRoadCollisionAllowRehold"),
	1,
	TEXT("If non-zero, the road-collision hold can re-activate if ground contact is lost again during the hold window. Default: 1"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficChaosDriveHideWhileWaitingForRoadCollision(
	TEXT("aaa.Traffic.Visual.ChaosDrive.HideWhileWaitingForRoadCollision"),
	1,
	TEXT("If non-zero, hides ChaosDrive pawns while waiting for road collision (prevents visible floating mid-air). Default: 1"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveRoadCollisionPlaceClearanceCm(
	TEXT("aaa.Traffic.Visual.ChaosDrive.RoadCollisionPlaceClearanceCm"),
	20.0f,
	TEXT("When road collision becomes available after a hold, Z clearance (cm) above the hit point to place the pawn before re-enabling physics. Default: 20"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveForceReadySeconds(
	TEXT("aaa.Traffic.Visual.ChaosDrive.ForceReadySeconds"),
	2.0f,
	TEXT("Seconds after spawn during which AAA Traffic will aggressively re-enable physics/gravity and movement components on the Chaos pawn.\n")
	TEXT("This helps when vehicle blueprints toggle simulation/movement during BeginPlay and would otherwise remain floating/inactive.\n")
	TEXT("Default: 2.0"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveUprightFixSeconds(
	TEXT("aaa.Traffic.Visual.ChaosDrive.UprightFixSeconds"),
	10.0f,
	TEXT("Seconds after spawn during which AAA Traffic will auto-correct a Chaos vehicle that ends up upside-down."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveUprightFixMinUpZ(
	TEXT("aaa.Traffic.Visual.ChaosDrive.UprightFixMinUpZ"),
	0.2f,
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

static TAutoConsoleVariable<int32> CVarTrafficChaosDriveForceSimulatePhysics(
	TEXT("aaa.Traffic.Visual.ChaosDrive.ForceSimulatePhysics"),
	1,
	TEXT("If non-zero, and a ChaosDrive pawn has a suitable movement component but no primitive components simulating physics,\n")
	TEXT("AAA Traffic will enable SimulatePhysics on the best candidate primitive (usually the root mesh).\n")
	TEXT("This helps when a vehicle blueprint is misconfigured (physics disabled) and would otherwise float and never drive.\n")
	TEXT("Default: 1"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficChaosDriveDisableNonSimulatingCollision(
	TEXT("aaa.Traffic.Visual.ChaosDrive.DisableNonSimulatingCollision"),
	1,
	TEXT("If non-zero, AAA Traffic disables collision on non-simulating primitive components of ChaosDrive pawns.\n")
	TEXT("Many vehicle blueprints include decorative meshes (wheels, interior parts) that should not collide; if they do,\n")
	TEXT("Chaos can become unstable and vehicles can explode/launch into the air when assets stream in.\n")
	TEXT("Default: 1"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficCollisionIgnoreOtherTraffic(
	TEXT("aaa.Traffic.Collision.IgnoreOtherTraffic"),
	1,
	TEXT("If non-zero, spawned traffic vehicles ignore collisions with other traffic vehicles.\n")
	TEXT("This uses the collision profile 'AAA_TrafficVehicle' (configured via plugin Config/DefaultEngine.ini).\n")
	TEXT("Default: 1"),
	ECVF_Default);

namespace
{
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
		UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Pawn->GetRootComponent());
		if (RootPrim)
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

			// Only apply to the root/driven physics bodies. Setting collision profiles on non-simulating decorative
			// components can change collision behavior in unexpected ways and contribute to instability.
			const bool bRelevant =
				(Prim == RootPrim) ||
				Prim->IsSimulatingPhysics();
			if (!bRelevant)
			{
				continue;
			}

			Prim->SetCollisionProfileName(TrafficProfileName);
		}
	}

	static bool FindBestGroundHitNearExpectedZ(
		UWorld& World,
		const FVector& ExpectedPos,
		const FVector& Start,
		const FVector& End,
		ECollisionChannel Channel,
		const FCollisionQueryParams& Params,
		FHitResult& OutHit);

	static bool TraceDownFromPawnBounds(
		UWorld& World,
		const APawn& Pawn,
		ECollisionChannel Channel,
		float TraceDepthCm,
		FHitResult& OutHit)
	{
		const FBox Bounds = Pawn.GetComponentsBoundingBox(true);
		const FVector FallbackCenter = Pawn.GetActorLocation();
		const FVector Center = Bounds.IsValid ? Bounds.GetCenter() : FallbackCenter;
		const float StartZ = (Bounds.IsValid ? Bounds.Max.Z : FallbackCenter.Z) + 50.f;
		const float EndZ = (Bounds.IsValid ? Bounds.Min.Z : FallbackCenter.Z) - FMath::Max(0.f, TraceDepthCm);

		const FVector Start(Center.X, Center.Y, StartZ);
		const FVector End(Center.X, Center.Y, EndZ);

		FCollisionQueryParams Params(SCENE_QUERY_STAT(AAA_Traffic_GroundedTrace), /*bTraceComplex=*/false);
		Params.bReturnPhysicalMaterial = false;
		Params.AddIgnoredActor(&Pawn);

		// Prefer the hit closest to the pawn (and ignore overhead collisions) to keep wheel grounding stable.
		return FindBestGroundHitNearExpectedZ(World, Center, Start, End, Channel, Params, OutHit);
	}

	static bool FindBestGroundHitNearExpectedZ(
		UWorld& World,
		const FVector& ExpectedPos,
		const FVector& Start,
		const FVector& End,
		ECollisionChannel Channel,
		const FCollisionQueryParams& Params,
		FHitResult& OutHit)
	{
		TArray<FHitResult> Hits;
		if (!World.LineTraceMultiByChannel(Hits, Start, End, Channel, Params))
		{
			return false;
		}

		bool bFound = false;
		float BestScore = TNumericLimits<float>::Max();
		const float MaxAbove = FMath::Max(0.f, CVarTrafficChaosDriveRoadHitMaxAboveLaneCm.GetValueOnGameThread());
		for (const FHitResult& Hit : Hits)
		{
			if (!Hit.bBlockingHit)
			{
				continue;
			}

			// Ignore overhead hits far above the lane (tree canopies, bridges, signs).
			if (Hit.ImpactPoint.Z > ExpectedPos.Z + MaxAbove)
			{
				continue;
			}

			const float Dz = FMath::Abs(Hit.ImpactPoint.Z - ExpectedPos.Z);
			const float NormalZ = Hit.ImpactNormal.Z;

			// Prefer surfaces whose normal points up (roads/terrain). Strongly de-prioritize walls/trees.
			float Score = Dz;
			if (NormalZ < 0.2f)
			{
				Score += 100000.f;
			}
			else if (NormalZ < 0.6f)
			{
				Score += 1000.f;
			}

			if (Score < BestScore)
			{
				BestScore = Score;
				OutHit = Hit;
				bFound = true;
			}
		}

		return bFound;
	}

static bool IsGroundedByWheelTraceChannel(
	UWorld& World,
	const APawn& Pawn,
	ECollisionChannel WheelTraceChannel,
	float MaxGapCm,
	float TraceDepthCm,
	float& OutGapCm,
	FHitResult* OutHit)
	{
		FHitResult Hit;
		if (!TraceDownFromPawnBounds(World, Pawn, WheelTraceChannel, TraceDepthCm, Hit))
		{
			OutGapCm = TNumericLimits<float>::Max();
			if (OutHit)
			{
				*OutHit = FHitResult();
			}
			return false;
		}

		const FBox Bounds = Pawn.GetComponentsBoundingBox(true);
		float Gap = TNumericLimits<float>::Max();
		if (Bounds.IsValid)
		{
			Gap = Bounds.Min.Z - Hit.ImpactPoint.Z;
			const float MaxGap = FMath::Max(0.f, MaxGapCm);
			// Bounds can be unreliable while meshes stream; fall back to actor Z if the bounds-based gap is implausible.
			if (MaxGap > 0.f && FMath::Abs(Gap) > MaxGap * 4.0f)
			{
				Gap = Pawn.GetActorLocation().Z - Hit.ImpactPoint.Z;
			}
		}
		else
		{
			Gap = Pawn.GetActorLocation().Z - Hit.ImpactPoint.Z;
		}
		OutGapCm = Gap;
		if (OutHit)
		{
			*OutHit = Hit;
		}
		return FMath::Abs(Gap) <= FMath::Max(0.f, MaxGapCm);
	}

	static bool IsBoundsReliableForHold(const FBox& Bounds, const FVector& LogicPos)
	{
		if (!Bounds.IsValid)
		{
			return false;
		}

		const FVector Extent = Bounds.GetExtent();
		const float MaxExtent = FMath::Max(0.f, CVarTrafficChaosDriveHoldBoundsMaxExtentCm.GetValueOnGameThread());
		if (MaxExtent > 0.f &&
			(Extent.X > MaxExtent || Extent.Y > MaxExtent || Extent.Z > MaxExtent))
		{
			return false;
		}

		const float MaxOffset = FMath::Max(0.f, CVarTrafficChaosDriveHoldBoundsMaxOffsetFromLogicCm.GetValueOnGameThread());
		if (MaxOffset > 0.f && FMath::Abs(Bounds.GetCenter().Z - LogicPos.Z) > MaxOffset)
		{
			return false;
		}

		return true;
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

		// Avoid engaging handbrake during spawn settle; some vehicle setups become unstable when locked wheels are steered.
		CallSingleParam(Move, FName(TEXT("SetHandbrakeInput")), 0.f, false);
	}

	static void EnsureChaosDriveTransmissionReady(UMovementComponent* Move)
	{
		if (!Move)
		{
			return;
		}

		// Some vehicle blueprints spawn with auto-gears disabled (or expect player code to enable it).
		// Best-effort: enable auto gears when supported.
		CallSingleParam(Move, FName(TEXT("SetUseAutoGears")), 0.f, true);
		CallSingleParam(Move, FName(TEXT("SetUseAutomaticGears")), 0.f, true);
		// ChaosVehicleMovementComponent ignores inputs without a controller by default; override for traffic AI.
		CallSingleParam(Move, FName(TEXT("SetRequiresControllerForInputs")), 0.f, false);
	}

	static bool HasAnySimulatingPrimitive(APawn& Pawn)
	{
		TArray<UPrimitiveComponent*> PrimComps;
		Pawn.GetComponents(PrimComps);
		for (UPrimitiveComponent* Prim : PrimComps)
		{
			if (Prim && Prim->IsSimulatingPhysics())
			{
				return true;
			}
		}
		return false;
	}

	static UPrimitiveComponent* FindBestPhysicsPrimitive(APawn& Pawn)
	{
		// Prefer the component the movement system intends to drive, when available.
		TArray<UMovementComponent*> MoveComps;
		Pawn.GetComponents(MoveComps);
		for (UMovementComponent* Move : MoveComps)
		{
			if (!Move || !Move->UpdatedComponent)
			{
				continue;
			}
			if (UPrimitiveComponent* UpdatedPrim = Cast<UPrimitiveComponent>(Move->UpdatedComponent))
			{
				return UpdatedPrim;
			}
		}

		if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Pawn.GetRootComponent()))
		{
			// Prefer the root if it has a valid body setup (typical for Chaos vehicles).
			return RootPrim;
		}

		// Common for vehicle pawns: a skeletal mesh chassis is the primary simulated body.
		{
			TArray<USkeletalMeshComponent*> SkelComps;
			Pawn.GetComponents(SkelComps);
			for (USkeletalMeshComponent* Skel : SkelComps)
			{
				if (Skel && Skel->IsRegistered())
				{
					return Skel;
				}
			}
		}

		TArray<UPrimitiveComponent*> PrimComps;
		Pawn.GetComponents(PrimComps);
		for (UPrimitiveComponent* Prim : PrimComps)
		{
			if (Prim && Prim->IsRegistered())
			{
				return Prim;
			}
		}
		return nullptr;
	}

	static UPrimitiveComponent* FindSimulatingPhysicsPrimitive(APawn& Pawn)
	{
		// Prefer the component the movement system intends to drive, when available.
		TArray<UMovementComponent*> MoveComps;
		Pawn.GetComponents(MoveComps);
		for (UMovementComponent* Move : MoveComps)
		{
			if (!Move || !Move->UpdatedComponent)
			{
				continue;
			}
			if (UPrimitiveComponent* UpdatedPrim = Cast<UPrimitiveComponent>(Move->UpdatedComponent))
			{
				if (UpdatedPrim->IsSimulatingPhysics())
				{
					return UpdatedPrim;
				}
			}
		}

		if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Pawn.GetRootComponent()))
		{
			if (RootPrim->IsSimulatingPhysics())
			{
				return RootPrim;
			}
		}

		// Common for vehicle pawns: a skeletal mesh chassis is the primary simulated body.
		{
			TArray<USkeletalMeshComponent*> SkelComps;
			Pawn.GetComponents(SkelComps);
			for (USkeletalMeshComponent* Skel : SkelComps)
			{
				if (Skel && Skel->IsSimulatingPhysics())
				{
					return Skel;
				}
			}
		}

		TArray<UPrimitiveComponent*> PrimComps;
		Pawn.GetComponents(PrimComps);
		for (UPrimitiveComponent* Prim : PrimComps)
		{
			if (Prim && Prim->IsSimulatingPhysics())
			{
				return Prim;
			}
		}
		return nullptr;
	}

	static void EnsureChaosDrivePhysicsEnabled(APawn& Pawn)
	{
		TArray<UPrimitiveComponent*> PrimComps;
		Pawn.GetComponents(PrimComps);

		bool bHasSim = false;
		for (UPrimitiveComponent* Prim : PrimComps)
		{
			if (!Prim)
			{
				continue;
			}
			if (Prim->IsSimulatingPhysics())
			{
				bHasSim = true;
			}
		}

		if (bHasSim)
		{
			// Some vehicle blueprints may have physics enabled but gravity disabled, which looks like "floating traffic".
			// In ChaosDrive mode we want gravity/collision enabled so the vehicle can settle onto the road.
			for (UPrimitiveComponent* Prim : PrimComps)
			{
				if (!Prim)
				{
					continue;
				}
				// IMPORTANT: Only touch simulating primitives. Enabling collision on non-simulating decorative parts
				// can cause violent self-collision/road-collision explosions (flying/spinning traffic).
				if (!Prim->IsSimulatingPhysics())
				{
					continue;
				}

				if (Prim->GetCollisionEnabled() != ECollisionEnabled::QueryAndPhysics)
				{
					Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				}
				if (!Prim->IsGravityEnabled())
				{
					Prim->SetEnableGravity(true);
				}
				Prim->WakeAllRigidBodies();
			}
			return;
		}

		UPrimitiveComponent* Prim = FindBestPhysicsPrimitive(Pawn);
		if (!Prim)
		{
			return;
		}

		if (Prim->GetCollisionEnabled() != ECollisionEnabled::QueryAndPhysics)
		{
			Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
		Prim->SetEnableGravity(true);
		Prim->SetSimulatePhysics(true);
		Prim->WakeAllRigidBodies();
	}

	static void DisableNonSimulatingCollision(APawn& Pawn)
	{
		if (CVarTrafficChaosDriveDisableNonSimulatingCollision.GetValueOnGameThread() == 0)
		{
			return;
		}

		TArray<UPrimitiveComponent*> PrimComps;
		Pawn.GetComponents(PrimComps);

		for (UPrimitiveComponent* Prim : PrimComps)
		{
			if (!Prim)
			{
				continue;
			}

			if (Prim->IsSimulatingPhysics())
			{
				continue;
			}

			if (Prim->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
			{
				Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}

			if (Prim->GetGenerateOverlapEvents())
			{
				Prim->SetGenerateOverlapEvents(false);
			}
		}
	}

	static void EnsureChaosDriveMovementEnabled(APawn& Pawn)
	{
		TArray<UMovementComponent*> MoveComps;
		Pawn.GetComponents(MoveComps);
		for (UMovementComponent* Move : MoveComps)
		{
			if (!Move)
			{
				continue;
			}
			Move->Activate(true);
			Move->SetComponentTickEnabled(true);
		}
	}

	static float GetChassisAngularSpeedDegPerSec(APawn& Pawn)
	{
		UPrimitiveComponent* Prim = FindSimulatingPhysicsPrimitive(Pawn);
		if (!Prim)
		{
			return 0.f;
		}

		return Prim->GetPhysicsAngularVelocityInDegrees().Size();
	}

	static void ClampChassisAngularVelocity(APawn& Pawn, float MaxDegPerSec)
	{
		UPrimitiveComponent* Prim = FindSimulatingPhysicsPrimitive(Pawn);
		if (!Prim)
		{
			return;
		}

		const FVector W = Prim->GetPhysicsAngularVelocityInDegrees();
		const float Speed = W.Size();
		const float Max = FMath::Max(0.f, MaxDegPerSec);
		if (Speed <= Max || Speed <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		const FVector Clamped = W * (Max / Speed);
		Prim->SetPhysicsAngularVelocityInDegrees(Clamped);
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
		Prim->WakeAllRigidBodies();
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
	PrimaryActorTick.TickGroup = TG_PrePhysics;
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
	ChaosDriveRoadHoldPhysicsComps.Reset();
	ChaosDriveSpawnDampedPrim.Reset();
	ChaosDriveSavedLinearDamping = 0.f;
	ChaosDriveSavedAngularDamping = 0.f;
	bChaosDriveHasSavedDamping = false;
	bChaosDriveEverGrounded = false;
	ChaosDriveLastGroundDiagAgeSeconds = -1000.f;
	ChaosDriveLastHoldDiagAgeSeconds = -1000.f;
	ChaosDriveLastDriveDiagAgeSeconds = -1000.f;
	bChaosDriveLoggedGroundMismatch = false;
	bChaosDriveAwaitingRoadCollision = false;
	bChaosDriveWasHiddenForRoadCollision = false;
	bChaosDriveReleasedFromRoadHold = false;
	ChaosDriveRoadHoldReleaseAgeSeconds = -1.f;
	ChaosDriveHoldStableSeconds = 0.f;
	ChaosDriveHoldLastAgeSeconds = -1.f;
	PrevSteer = 0.f;
	PrevThrottle = 0.f;
	PrevBrake = 0.f;
	PrevFollowTargetTypeRaw = 0;
	PrevFollowTargetId = INDEX_NONE;
	PrevFollowTargetS = 0.f;
	bHasPrevFollowTarget = false;

	if (ChaosVehicle.IsValid())
	{
		// Traffic vehicles should be fully controlled by the traffic system (not auto-possessed by player/AI).
		ChaosVehicle->AutoPossessPlayer = EAutoReceiveInput::Disabled;
		ChaosVehicle->AutoPossessAI = EAutoPossessAI::Disabled;
		ChaosVehicle->AIControllerClass = nullptr;
		ChaosVehicle->DetachFromControllerPendingDestroy();
		ChaosVehicle->DisableInput(nullptr);

		// Many vehicle blueprints write to their movement inputs during their own tick (e.g., reading player input axes).
		// Ensure our adapter tick runs AFTER the pawn tick so we can reliably override those inputs for traffic driving.
		AddTickPrerequisiteActor(ChaosVehicle.Get());

		const ETrafficVisualMode Mode = GetTrafficVisualMode();
		const bool bDebug = (CVarTrafficChaosDriveDebug.GetValueOnGameThread() != 0);
		const int32 SampleCount = []() -> int32
		{
			static IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Debug.SampleVehicleCount"));
			return Var ? FMath::Max(0, Var->GetInt()) : 0;
		}();
		const int32 DebugIndex = LogicVehicle.IsValid() ? LogicVehicle->GetDebugSpawnIndex() : INDEX_NONE;
		const bool bSample = (SampleCount > 0) && (DebugIndex >= 0) && (DebugIndex < SampleCount);

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

			if (bSample)
			{
				UMovementComponent* Move = FindChaosMovementComponent(ChaosVehicle.Get());
				const bool bHasMove = (Move != nullptr);
				const bool bHasSim = HasAnySimulatingPrimitive(*ChaosVehicle.Get());
				const ECollisionChannel WheelCh = ResolveChaosWheelTraceChannel(Move);
				UE_LOG(LogTraffic, Log,
					TEXT("[TrafficVehicleAdapter] Sample[%d] ChaosDrive init: Pawn=%s Move=%s simulating=%d gravityRoot=%d wheelCh=%d(%s)"),
					DebugIndex,
					*ChaosVehicle->GetName(),
					bHasMove ? *Move->GetClass()->GetName() : TEXT("<none>"),
					bHasSim ? 1 : 0,
					(ChaosVehicle->GetRootComponent() && ChaosVehicle->GetRootComponent()->IsA<UPrimitiveComponent>()) ?
						(Cast<UPrimitiveComponent>(ChaosVehicle->GetRootComponent())->IsGravityEnabled() ? 1 : 0) : -1,
					static_cast<int32>(WheelCh),
					CollisionChannelName(WheelCh));
			}

			// If the pawn has a vehicle movement component but physics simulation is off, it will float and never move.
			// Fix it proactively (common when bringing in custom vehicle blueprints).
			if (CVarTrafficChaosDriveForceSimulatePhysics.GetValueOnGameThread() != 0)
			{
				// Best-effort: enable physics even if movement component discovery fails (prevents permanent "floating" visuals).
				EnsureChaosDrivePhysicsEnabled(*ChaosVehicle.Get());
			}

			// Prevent Chaos explosions caused by colliding decorative meshes (common in complex vehicle blueprints).
			DisableNonSimulatingCollision(*ChaosVehicle.Get());

			// Ensure common vehicle movement settings are in a drivable state (e.g., auto-gears enabled).
			if (UMovementComponent* Move = FindChaosMovementComponent(ChaosVehicle.Get()))
			{
				ApplyChaosWheelTraceChannelOverride(Move);
				EnsureChaosDriveTransmissionReady(Move);
			}

			// Ensure movement components are active by default (some BPs start with components deactivated).
			if (!bChaosDrivePhysicsSuspended && !bChaosDriveMovementSuspended)
			{
				EnsureChaosDriveMovementEnabled(*ChaosVehicle.Get());
			}

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
					State.CollisionEnabled = Prim->GetCollisionEnabled();
					ChaosDrivePhysicsComps.Add(State);

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
				// Avoid enabling collision on non-simulating decorative components; only ensure the driven body can collide.
				// (We still use EnsureChaosDrivePhysicsEnabled to correct misconfigured blueprints.)
				if (!bChaosDrivePhysicsSuspended && Prim->IsSimulatingPhysics() && !Prim->IsGravityEnabled())
				{
					Prim->SetEnableGravity(true);
				}
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
		const float Age = ChaosVehicle->GetGameTimeSinceCreation();
		const int32 SampleCount = []() -> int32
		{
			static IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Debug.SampleVehicleCount"));
			return Var ? FMath::Max(0, Var->GetInt()) : 0;
		}();
		const int32 DebugIndex = LogicVehicle.IsValid() ? LogicVehicle->GetDebugSpawnIndex() : INDEX_NONE;
		const bool bSample = (SampleCount > 0) && (DebugIndex >= 0) && (DebugIndex < SampleCount);

		// If road collision isn't ready yet, don't allow the pawn to fall/spin in mid-air. Hold it kinematically until
		// a ground trace starts hitting, then place it onto the road and enable physics.
		const bool bAllowRehold = CVarTrafficChaosDriveHoldUntilRoadCollisionAllowRehold.GetValueOnGameThread() != 0;
		if ((bAllowRehold || !bChaosDriveReleasedFromRoadHold) &&
			CVarTrafficChaosDriveHoldUntilRoadCollision.GetValueOnGameThread() != 0 &&
			GetWorld())
		{
			UMovementComponent* Move = FindChaosMovementComponent(ChaosVehicle.Get());
			ECollisionChannel WheelTraceChannel =
				bChaosDriveHasResolvedWheelTraceChannel ? ChaosDriveResolvedWheelTraceChannel : ResolveChaosWheelTraceChannel(Move);
			const ECollisionChannel ChaosWheelChannel = WheelTraceChannel; // Match the vehicle's wheel trace channel.

			const float MaxHoldSeconds = FMath::Max(0.f, CVarTrafficChaosDriveHoldUntilRoadCollisionMaxSeconds.GetValueOnGameThread());
			const float MinStableSeconds = FMath::Max(0.f, CVarTrafficChaosDriveHoldUntilRoadCollisionMinStableSeconds.GetValueOnGameThread());
			const float ClearanceCm = CVarTrafficChaosDriveRoadCollisionPlaceClearanceCm.GetValueOnGameThread();
			const float LogicGroundMaxGapCm = FMath::Max(0.f, CVarTrafficChaosDriveHoldLogicGroundMaxGapCm.GetValueOnGameThread());

			// Trace under the logic vehicle's XY (that's the lane-follow target), not the potentially-tumbling Chaos pawn.
			const FVector LogicPos = LogicVehicle.IsValid() ? LogicVehicle->GetActorLocation() : ChaosVehicle->GetActorLocation();
			// Trace deep enough to reach collision even when lane Z is far above the physical surface.
			const FVector Start = LogicPos + FVector(0.f, 0.f, 50000.f);
			const FVector End = LogicPos - FVector(0.f, 0.f, 100000.f);

			FCollisionQueryParams Params(SCENE_QUERY_STAT(AAA_Traffic_RoadReadyTrace), /*bTraceComplex=*/false);
			Params.bReturnPhysicalMaterial = false;
			Params.AddIgnoredActor(ChaosVehicle.Get());

			FHitResult GroundHit;
			bool bHasGroundDynamic = false;
			// Do NOT take the first hit from 50kcm above: overhead collisions (trees, signs, bridges) would be selected first
			// and cause vehicles to be snapped up into the air. Choose the hit closest to the expected lane Z instead.
			bool bHasGround = FindBestGroundHitNearExpectedZ(*GetWorld(), LogicPos, Start, End, ChaosWheelChannel, Params, GroundHit);
			bHasGroundDynamic = bHasGround;

			FHitResult StaticGroundHit;
			bool bHasGroundStatic = false;
			if (!bHasGround)
			{
				bHasGroundStatic = FindBestGroundHitNearExpectedZ(*GetWorld(), LogicPos, Start, End, ECC_WorldStatic, Params, StaticGroundHit);
				if (bHasGroundStatic)
				{
					if (UPrimitiveComponent* HitComp = StaticGroundHit.GetComponent())
					{
						if (HitComp->GetCollisionResponseToChannel(ECC_WorldDynamic) != ECR_Block)
						{
							HitComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
						}
					}
					GroundHit = StaticGroundHit;
					bHasGround = true;
				}
			}

			// If the vehicle's suspension traces are not detecting ground yet, keep holding. This is the root cause behind
			// "car flies/spins even though there's a road under it": wheels never get a stable ground hit, so Chaos goes unstable.
			const float MaxGapCm = FMath::Max(0.f, CVarTrafficChaosDriveGroundedMaxGapCm.GetValueOnGameThread());
			const float WheelTraceDepthCm = FMath::Max(0.f, CVarTrafficChaosDriveGroundedTraceDepthCm.GetValueOnGameThread());
			float WheelGapCm = 0.f;
			FHitResult WheelHit;
			bool bWheelGrounded = IsGroundedByWheelTraceChannel(*GetWorld(), *ChaosVehicle.Get(), ChaosWheelChannel, MaxGapCm, WheelTraceDepthCm, WheelGapCm, &WheelHit);

			// Auto-detect wheel trace channel when current configuration can't find ground under the lane.
			if (SupportsWheelTraceChannelProperty(Move) &&
				CVarTrafficChaosDriveAutoDetectWheelTraceChannel.GetValueOnGameThread() != 0 &&
				(!bHasGround || !bWheelGrounded) &&
				Move &&
				(Age - ChaosDriveLastWheelTraceAutoDetectAgeSeconds) >= 0.5f)
			{
				ChaosDriveLastWheelTraceAutoDetectAgeSeconds = Age;

				const ECollisionChannel PrevChannel = WheelTraceChannel;
				const TArray<ECollisionChannel> Candidates = BuildWheelTraceChannelCandidates(PrevChannel);

				ECollisionChannel BestChannel = PrevChannel;
				float BestScore = TNumericLimits<float>::Max();

				for (const ECollisionChannel Candidate : Candidates)
				{
					FHitResult CandidateHit;
					const bool bCandidateHasGround =
						FindBestGroundHitNearExpectedZ(*GetWorld(), LogicPos, Start, End, Candidate, Params, CandidateHit);
					if (!bCandidateHasGround)
					{
						continue;
					}

					float CandidateGapCm = 0.f;
					const bool bCandidateWheelGrounded =
						IsGroundedByWheelTraceChannel(*GetWorld(), *ChaosVehicle.Get(), Candidate, MaxGapCm, WheelTraceDepthCm, CandidateGapCm, nullptr);

					float Score = FMath::Abs(CandidateHit.ImpactPoint.Z - LogicPos.Z);
					if (!bCandidateWheelGrounded)
					{
						Score += 5000.f;
					}

					if (Score < BestScore)
					{
						BestScore = Score;
						BestChannel = Candidate;
					}
				}

				if (BestChannel != PrevChannel && SetWheelTraceCollisionChannel(Move, BestChannel))
				{
					bChaosDriveHasResolvedWheelTraceChannel = true;
					ChaosDriveResolvedWheelTraceChannel = BestChannel;
					WheelTraceChannel = BestChannel;

					UE_LOG(LogTraffic, Warning,
						TEXT("[TrafficVehicleAdapter] ChaosDrive: Auto-selected wheel trace channel=%d(%s) for pawn=%s (previous=%d(%s))."),
						static_cast<int32>(BestChannel),
						CollisionChannelName(BestChannel),
						*ChaosVehicle->GetName(),
						static_cast<int32>(PrevChannel),
						CollisionChannelName(PrevChannel));

					// Recompute ground checks with the new channel.
					bHasGround = FindBestGroundHitNearExpectedZ(*GetWorld(), LogicPos, Start, End, WheelTraceChannel, Params, GroundHit);
					WheelGapCm = 0.f;
					bWheelGrounded = IsGroundedByWheelTraceChannel(*GetWorld(), *ChaosVehicle.Get(), WheelTraceChannel, MaxGapCm, WheelTraceDepthCm, WheelGapCm, &WheelHit);
				}
			}

			bool bNeedsSnap = false;
			const FBox BoundsNow = ChaosVehicle->GetComponentsBoundingBox(true);
			bool bBoundsReliable = IsBoundsReliableForHold(BoundsNow, LogicPos);
			const float MaxSnapCm = FMath::Max(0.f, CVarTrafficChaosDriveHoldMaxSnapCm.GetValueOnGameThread());
			if (bHasGround && bBoundsReliable)
			{
				const float DesiredBottomZ = GroundHit.ImpactPoint.Z + ClearanceCm;
				const float CurrentBottomZ = BoundsNow.Min.Z;
				const float GapToDesiredCm = CurrentBottomZ - DesiredBottomZ; // + = pawn above desired ground placement
				bNeedsSnap = FMath::Abs(GapToDesiredCm) > FMath::Max(50.f, MaxGapCm);
				if (MaxSnapCm > 0.f && FMath::Abs(GapToDesiredCm) > MaxSnapCm)
				{
					// Bounds appear unreliable for placement (would require a huge snap); fall back to safe placement.
					bBoundsReliable = false;
					bNeedsSnap = true;
				}
			}
			else if (bHasGround && !bBoundsReliable)
			{
				// If bounds are not reliable yet (common while meshes stream in), keep holding.
				bNeedsSnap = true;
			}

			// Track consecutive stable ground time to avoid releasing on a single transient hit.
			float DeltaAge = 0.f;
			if (ChaosDriveHoldLastAgeSeconds >= 0.f)
			{
				DeltaAge = FMath::Max(0.f, Age - ChaosDriveHoldLastAgeSeconds);
			}
			ChaosDriveHoldLastAgeSeconds = Age;

			const float LogicGroundGap = bHasGround ? FMath::Abs(LogicPos.Z - GroundHit.ImpactPoint.Z) : TNumericLimits<float>::Max();
			const bool bLogicGrounded = bHasGround && (LogicGroundGap <= LogicGroundMaxGapCm);
			const bool bHoldGrounded = bWheelGrounded || (bLogicGrounded && bHasGroundDynamic);
			if (bLogicGrounded)
			{
				// If the lane target is close to the ground hit, allow release even if wheel traces are still unreliable.
				bNeedsSnap = false;
			}
			const bool bHoldStableCandidate = bHasGround && bHoldGrounded && !bNeedsSnap;
			if (bHoldStableCandidate)
			{
				ChaosDriveHoldStableSeconds += DeltaAge;
			}
			else
			{
				ChaosDriveHoldStableSeconds = 0.f;
			}
			const bool bStableEnough = (MinStableSeconds <= 0.f) || (ChaosDriveHoldStableSeconds >= MinStableSeconds);

			if (bSample && (Age - ChaosDriveLastHoldDiagAgeSeconds) >= 0.5f)
			{
				ChaosDriveLastHoldDiagAgeSeconds = Age;
				const FVector PawnPos = ChaosVehicle->GetActorLocation();
				const float GroundZ = bHasGround ? GroundHit.ImpactPoint.Z : TNumericLimits<float>::Max();
				UE_LOG(LogTraffic, Log,
					TEXT("[TrafficVehicleAdapter] Sample[%d] HoldCheck age=%.2fs enabled=1 wheelCh=%d(%s) hasGround=%d wheelGrounded=%d wheelGap=%.1f needsSnap=%d boundsValid=%d boundsReliable=%d logicGap=%.1f stable=%.2fs pawnZ=%.1f logicZ=%.1f groundZ=%.1f traceZ=[%.1f..%.1f]"),
					DebugIndex,
					Age,
					static_cast<int32>(ChaosWheelChannel),
					CollisionChannelName(ChaosWheelChannel),
					bHasGround ? 1 : 0,
					bWheelGrounded ? 1 : 0,
					WheelGapCm,
					bNeedsSnap ? 1 : 0,
					BoundsNow.IsValid ? 1 : 0,
					bBoundsReliable ? 1 : 0,
					LogicGroundGap,
					ChaosDriveHoldStableSeconds,
					PawnPos.Z,
					LogicPos.Z,
					GroundZ,
					Start.Z,
					End.Z);
			}

			if ((!bHasGround || bNeedsSnap || !bHoldGrounded || !bStableEnough) && Age <= MaxHoldSeconds)
			{
				if (!bChaosDriveAwaitingRoadCollision)
				{
					bChaosDriveAwaitingRoadCollision = true;
					bChaosDriveWasHiddenForRoadCollision = false;
					ChaosDriveRoadHoldPhysicsComps.Reset();

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
						State.CollisionEnabled = Prim->GetCollisionEnabled();
						ChaosDriveRoadHoldPhysicsComps.Add(State);

						Prim->SetEnableGravity(false);
						if (State.bWasSimulatingPhysics)
						{
							Prim->SetSimulatePhysics(false);
						}
					}

					TArray<UMovementComponent*> MoveComps;
					ChaosVehicle->GetComponents(MoveComps);
					for (UMovementComponent* MoveComp : MoveComps)
					{
						if (MoveComp)
						{
							MoveComp->Deactivate();
							MoveComp->SetComponentTickEnabled(false);
						}
					}

					if (CVarTrafficChaosDriveHideWhileWaitingForRoadCollision.GetValueOnGameThread() != 0)
					{
						ChaosVehicle->SetActorHiddenInGame(true);
						bChaosDriveWasHiddenForRoadCollision = true;
					}

					UE_LOG(LogTraffic, Warning,
						TEXT("[TrafficVehicleAdapter] Sample[%d] Holding ChaosDrive pawn until road collision is ready (age=%.2fs)."),
						DebugIndex,
						Age);
				}

				// Follow the logic transform deterministically while waiting, so the eventual physics pawn starts from the right place.
				const float Yaw = LogicVehicle.IsValid() ? LogicVehicle->GetActorRotation().Yaw : ChaosVehicle->GetActorRotation().Yaw;
				ChaosVehicle->SetActorLocationAndRotation(LogicPos, FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);

				if (bHasGround)
				{
					const FBox Bounds = ChaosVehicle->GetComponentsBoundingBox(true);
					bool bSnapBoundsOk = IsBoundsReliableForHold(Bounds, LogicPos);
					if (bSnapBoundsOk)
					{
						const float DesiredBottomZ = GroundHit.ImpactPoint.Z + ClearanceCm;
						const float DeltaZ = DesiredBottomZ - Bounds.Min.Z;
						if (MaxSnapCm <= 0.f || FMath::Abs(DeltaZ) <= MaxSnapCm)
						{
							ChaosVehicle->AddActorWorldOffset(FVector(0.f, 0.f, DeltaZ), false, nullptr, ETeleportType::TeleportPhysics);
						}
						else
						{
							bSnapBoundsOk = false;
						}
					}
					if (!bSnapBoundsOk)
					{
						// Fallback placement when bounds are not reliable: keep actor slightly above the hit point.
						const float DesiredZ = GroundHit.ImpactPoint.Z + ClearanceCm + 10.f;
						FVector P = ChaosVehicle->GetActorLocation();
						P.Z = DesiredZ;
						ChaosVehicle->SetActorLocation(P, false, nullptr, ETeleportType::TeleportPhysics);
					}
				}

				ZeroPhysicsVelocities(*ChaosVehicle.Get());
				return;
			}

			// If we were holding but exceeded the max wait time, restore physics anyway (better to drop to the ground than to stay hidden/frozen).
			if (bChaosDriveAwaitingRoadCollision && Age > MaxHoldSeconds)
			{
				const float Yaw = LogicVehicle.IsValid() ? LogicVehicle->GetActorRotation().Yaw : ChaosVehicle->GetActorRotation().Yaw;
				ChaosVehicle->SetActorLocationAndRotation(LogicPos, FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);

				if (bHasGround)
				{
					const FBox Bounds = ChaosVehicle->GetComponentsBoundingBox(true);
					bool bSnapBoundsOk = IsBoundsReliableForHold(Bounds, LogicPos);
					if (bSnapBoundsOk)
					{
						const float DesiredBottomZ = GroundHit.ImpactPoint.Z + ClearanceCm;
						const float DeltaZ = DesiredBottomZ - Bounds.Min.Z;
						if (MaxSnapCm <= 0.f || FMath::Abs(DeltaZ) <= MaxSnapCm)
						{
							ChaosVehicle->AddActorWorldOffset(FVector(0.f, 0.f, DeltaZ), false, nullptr, ETeleportType::TeleportPhysics);
						}
						else
						{
							bSnapBoundsOk = false;
						}
					}
					if (!bSnapBoundsOk)
					{
						const float DesiredZ = GroundHit.ImpactPoint.Z + ClearanceCm + 10.f;
						FVector P = ChaosVehicle->GetActorLocation();
						P.Z = DesiredZ;
						ChaosVehicle->SetActorLocation(P, false, nullptr, ETeleportType::TeleportPhysics);
					}
				}

				for (const FChaosDrivePrimWarmupState& State : ChaosDriveRoadHoldPhysicsComps)
				{
					if (UPrimitiveComponent* Prim = State.Prim.Get())
					{
						Prim->SetCollisionEnabled(State.CollisionEnabled);
						Prim->SetEnableGravity(State.bWasGravityEnabled);
						if (State.bWasSimulatingPhysics)
						{
							Prim->SetSimulatePhysics(true);
						}
					}
				}
				ChaosDriveRoadHoldPhysicsComps.Reset();

				if (bChaosDriveWasHiddenForRoadCollision)
				{
					ChaosVehicle->SetActorHiddenInGame(false);
					bChaosDriveWasHiddenForRoadCollision = false;
				}

				if (CVarTrafficChaosDriveForceSimulatePhysics.GetValueOnGameThread() != 0)
				{
					EnsureChaosDrivePhysicsEnabled(*ChaosVehicle.Get());
				}
				DisableNonSimulatingCollision(*ChaosVehicle.Get());
				if (UMovementComponent* FoundMove = FindChaosMovementComponent(ChaosVehicle.Get()))
				{
					EnsureChaosDriveTransmissionReady(FoundMove);
				}
				EnsureChaosDriveMovementEnabled(*ChaosVehicle.Get());

				ZeroPhysicsVelocities(*ChaosVehicle.Get());
				bChaosDriveAwaitingRoadCollision = false;
				bChaosDriveReleasedFromRoadHold = true;
				ChaosDriveRoadHoldReleaseAgeSeconds = Age;
			}

			if (bChaosDriveAwaitingRoadCollision && bHasGround && !bNeedsSnap && bHoldGrounded && bStableEnough)
			{
				// Place the pawn onto the newly-available road collision before enabling physics.
				const float Yaw = LogicVehicle.IsValid() ? LogicVehicle->GetActorRotation().Yaw : ChaosVehicle->GetActorRotation().Yaw;
				ChaosVehicle->SetActorLocationAndRotation(LogicPos, FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);

				// Adjust Z so the pawn's bounds bottom sits on the ground hit plus clearance.
				const FBox Bounds = ChaosVehicle->GetComponentsBoundingBox(true);
				bool bSnapBoundsOk = IsBoundsReliableForHold(Bounds, LogicPos);
				if (bSnapBoundsOk)
				{
					const float DesiredBottomZ = GroundHit.ImpactPoint.Z + ClearanceCm;
					const float DeltaZ = DesiredBottomZ - Bounds.Min.Z;
					if (MaxSnapCm <= 0.f || FMath::Abs(DeltaZ) <= MaxSnapCm)
					{
						ChaosVehicle->AddActorWorldOffset(FVector(0.f, 0.f, DeltaZ), false, nullptr, ETeleportType::TeleportPhysics);
					}
					else
					{
						bSnapBoundsOk = false;
					}
				}
				if (!bSnapBoundsOk)
				{
					const float DesiredZ = GroundHit.ImpactPoint.Z + ClearanceCm + 10.f;
					FVector P = ChaosVehicle->GetActorLocation();
					P.Z = DesiredZ;
					ChaosVehicle->SetActorLocation(P, false, nullptr, ETeleportType::TeleportPhysics);
				}

				for (const FChaosDrivePrimWarmupState& State : ChaosDriveRoadHoldPhysicsComps)
				{
					if (UPrimitiveComponent* Prim = State.Prim.Get())
					{
						Prim->SetCollisionEnabled(State.CollisionEnabled);
						Prim->SetEnableGravity(State.bWasGravityEnabled);
						if (State.bWasSimulatingPhysics)
						{
							Prim->SetSimulatePhysics(true);
						}
					}
				}
				ChaosDriveRoadHoldPhysicsComps.Reset();

				if (bChaosDriveWasHiddenForRoadCollision)
				{
					ChaosVehicle->SetActorHiddenInGame(false);
					bChaosDriveWasHiddenForRoadCollision = false;
				}

				if (CVarTrafficChaosDriveForceSimulatePhysics.GetValueOnGameThread() != 0)
				{
					EnsureChaosDrivePhysicsEnabled(*ChaosVehicle.Get());
				}
				DisableNonSimulatingCollision(*ChaosVehicle.Get());
				if (UMovementComponent* FoundMove = FindChaosMovementComponent(ChaosVehicle.Get()))
				{
					EnsureChaosDriveTransmissionReady(FoundMove);
				}
				EnsureChaosDriveMovementEnabled(*ChaosVehicle.Get());

				ZeroPhysicsVelocities(*ChaosVehicle.Get());
				bChaosDriveAwaitingRoadCollision = false;
				bChaosDriveReleasedFromRoadHold = true;
				ChaosDriveRoadHoldReleaseAgeSeconds = Age;
			}
		}

		const float ForceReadySeconds = FMath::Max(0.f, CVarTrafficChaosDriveForceReadySeconds.GetValueOnGameThread());
		// IMPORTANT: Do not "force ready" while we're explicitly holding for road collision; that defeats the hold/hide logic
		// and can lead to the exact visible flyaways you're trying to prevent.
		if (ForceReadySeconds > 0.f && GetWorld() && !bChaosDriveAwaitingRoadCollision)
		{
			if (Age <= ForceReadySeconds && !bChaosDrivePhysicsSuspended)
			{
				if (CVarTrafficChaosDriveForceSimulatePhysics.GetValueOnGameThread() != 0)
				{
					EnsureChaosDrivePhysicsEnabled(*ChaosVehicle.Get());
				}
				DisableNonSimulatingCollision(*ChaosVehicle.Get());
				if (UMovementComponent* FoundMove = FindChaosMovementComponent(ChaosVehicle.Get()))
				{
					EnsureChaosDriveTransmissionReady(FoundMove);
				}
				if (!bChaosDriveMovementSuspended)
				{
					EnsureChaosDriveMovementEnabled(*ChaosVehicle.Get());
				}
			}
		}

		// If physics is suspended, hold the pawn steady until the warmup expires.
		if (bChaosDrivePhysicsSuspended && GetWorld())
		{
			if (Age >= ChaosDrivePhysicsResumeAgeSeconds)
			{
				bChaosDrivePhysicsSuspended = false;

				for (const FChaosDrivePrimWarmupState& State : ChaosDrivePhysicsComps)
				{
					if (UPrimitiveComponent* Prim = State.Prim.Get())
					{
						Prim->SetCollisionEnabled(State.CollisionEnabled);
						Prim->SetEnableGravity(State.bWasGravityEnabled);
						if (State.bWasSimulatingPhysics)
						{
							Prim->SetSimulatePhysics(true);
						}
					}
				}
				ChaosDrivePhysicsComps.Reset();

				// Warmup restore may return to "no sim" or "no gravity" states if the pawn blueprint was authored that way.
				// In ChaosDrive mode, try to correct common misconfigurations so vehicles don't float in place forever.
				if (CVarTrafficChaosDriveForceSimulatePhysics.GetValueOnGameThread() != 0)
				{
					EnsureChaosDrivePhysicsEnabled(*ChaosVehicle.Get());
				}
				DisableNonSimulatingCollision(*ChaosVehicle.Get());

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
		if (!Move->IsActive() || !Move->IsComponentTickEnabled())
		{
			Move->SetAutoActivate(true);
			Move->Activate(true);
			Move->SetComponentTickEnabled(true);
			Move->SetActive(true, true);
			EnsureChaosDriveTransmissionReady(Move);
		}
		if (UPrimitiveComponent* UpdatedPrim = Cast<UPrimitiveComponent>(Move->UpdatedComponent))
		{
			if (!UpdatedPrim->IsSimulatingPhysics())
			{
				if (UPrimitiveComponent* SimPrim = FindSimulatingPhysicsPrimitive(*ChaosVehicle.Get()))
				{
					Move->SetUpdatedComponent(SimPrim);
				}
			}
		}
		else
		{
			if (UPrimitiveComponent* SimPrim = FindSimulatingPhysicsPrimitive(*ChaosVehicle.Get()))
			{
				Move->SetUpdatedComponent(SimPrim);
			}
		}

		// Spawn spin damping should apply even while we're holding full brake at startup.
		{
			const FVector Vel = ChaosVehicle->GetVelocity();
			const float CurrentSpeed = Vel.Size();
			const float SpinSeconds = FMath::Max(0.f, CVarTrafficChaosDriveSpawnSpinDampSeconds.GetValueOnGameThread());
			if (SpinSeconds > 0.f)
			{
				if (Age <= SpinSeconds)
				{
					if (UPrimitiveComponent* Prim = FindSimulatingPhysicsPrimitive(*ChaosVehicle.Get()))
					{
						// Temporary extra damping helps Chaos vehicles settle without violent spin at spawn.
						if (!bChaosDriveHasSavedDamping || ChaosDriveSpawnDampedPrim.Get() != Prim)
						{
							// Restore any previous component we modified.
							if (bChaosDriveHasSavedDamping)
							{
								if (UPrimitiveComponent* PrevPrim = ChaosDriveSpawnDampedPrim.Get())
								{
									PrevPrim->SetLinearDamping(ChaosDriveSavedLinearDamping);
									PrevPrim->SetAngularDamping(ChaosDriveSavedAngularDamping);
								}
							}

							ChaosDriveSpawnDampedPrim = Prim;
							ChaosDriveSavedLinearDamping = Prim->GetLinearDamping();
							ChaosDriveSavedAngularDamping = Prim->GetAngularDamping();
							bChaosDriveHasSavedDamping = true;
						}

						Prim->SetLinearDamping(2.0f);
						Prim->SetAngularDamping(80.0f);
					}

					const float MaxSpeed = FMath::Max(0.f, CVarTrafficChaosDriveSpawnSpinDampMaxSpeedCmPerSec.GetValueOnGameThread());
					if (CurrentSpeed <= MaxSpeed)
					{
						const float MaxAng = FMath::Max(0.f, CVarTrafficChaosDriveSpawnSpinDampMaxAngularDegPerSec.GetValueOnGameThread());
						if (GetChassisAngularSpeedDegPerSec(*ChaosVehicle.Get()) > MaxAng)
						{
							ClampChassisAngularVelocity(*ChaosVehicle.Get(), MaxAng);
						}
					}
				}
				else if (bChaosDriveHasSavedDamping)
				{
					// Restore damping after the spawn stabilization window.
					if (UPrimitiveComponent* PrevPrim = ChaosDriveSpawnDampedPrim.Get())
					{
						PrevPrim->SetLinearDamping(ChaosDriveSavedLinearDamping);
						PrevPrim->SetAngularDamping(ChaosDriveSavedAngularDamping);
					}
					ChaosDriveSpawnDampedPrim.Reset();
					bChaosDriveHasSavedDamping = false;
				}
			}
		}

		const float InitialBrakeSeconds = FMath::Max(0.f, CVarTrafficChaosDriveInitialBrakeSeconds.GetValueOnGameThread());
		if (InitialBrakeSeconds > 0.f && GetWorld())
		{
			if (Age < InitialBrakeSeconds)
			{
				ApplyInitialBraking(Move);
				return;
			}
		}

		const float DriveDelay = FMath::Max(0.f, CVarTrafficChaosDriveDriveDelaySeconds.GetValueOnGameThread());
		const float DriveRamp = FMath::Max(0.f, CVarTrafficChaosDriveDriveRampSeconds.GetValueOnGameThread());
		if (DriveDelay > 0.f && Age < DriveDelay)
		{
			// Don't drive while the vehicle is still settling onto the road; this avoids wheel-in-air torque and spin-outs.
			ApplyInitialBraking(Move);
			CallSingleParam(Move, FName(TEXT("SetHandbrakeInput")), 0.f, false);
			return;
		}

		bool bDriveGroundedForLog = true;
		bool bWheelGroundedForLog = true;
		bool bLogicGroundedForLog = false;
		float DebugWheelGapCm = TNumericLimits<float>::Max();
		float DebugLogicGapCm = TNumericLimits<float>::Max();
		float DebugMaxWaitSeconds = 0.f;

		// Require ground contact (by the wheel/suspension trace channel) before attempting to drive.
		if (CVarTrafficChaosDriveRequireGroundedToDrive.GetValueOnGameThread() != 0 && GetWorld())
		{
			const float MaxWaitSeconds = FMath::Max(0.f, CVarTrafficChaosDriveRequireGroundedMaxSeconds.GetValueOnGameThread());
			const float MaxGapCm = FMath::Max(0.f, CVarTrafficChaosDriveGroundedMaxGapCm.GetValueOnGameThread());
			const float TraceDepthCm = FMath::Max(0.f, CVarTrafficChaosDriveGroundedTraceDepthCm.GetValueOnGameThread());
			const float LogicGroundMaxGapCm = FMath::Max(0.f, CVarTrafficChaosDriveHoldLogicGroundMaxGapCm.GetValueOnGameThread());

			const ECollisionChannel WheelTraceChannel =
				bChaosDriveHasResolvedWheelTraceChannel ? ChaosDriveResolvedWheelTraceChannel : ResolveChaosWheelTraceChannel(Move);
			const ECollisionChannel ChaosWheelChannel = WheelTraceChannel; // Match the vehicle's wheel trace channel.

			float GapCm = 0.f;
			FHitResult DynHit;
			const bool bGrounded = IsGroundedByWheelTraceChannel(*GetWorld(), *ChaosVehicle.Get(), ChaosWheelChannel, MaxGapCm, TraceDepthCm, GapCm, &DynHit);

			FHitResult LogicHit;
			bool bLogicGrounded = false;
			float LogicGap = TNumericLimits<float>::Max();
			if (!bGrounded || bSample)
			{
				const FVector LogicPos = LogicVehicle.IsValid() ? LogicVehicle->GetActorLocation() : ChaosVehicle->GetActorLocation();
				const FVector Start = LogicPos + FVector(0.f, 0.f, 50000.f);
				const FVector End = LogicPos - FVector(0.f, 0.f, 100000.f);
				FCollisionQueryParams Params(SCENE_QUERY_STAT(AAA_Traffic_LogicGroundTrace), /*bTraceComplex=*/false);
				Params.bReturnPhysicalMaterial = false;
				Params.AddIgnoredActor(ChaosVehicle.Get());
				const bool bLogicHasGround = FindBestGroundHitNearExpectedZ(*GetWorld(), LogicPos, Start, End, WheelTraceChannel, Params, LogicHit);
				LogicGap = bLogicHasGround ? FMath::Abs(LogicPos.Z - LogicHit.ImpactPoint.Z) : TNumericLimits<float>::Max();
				bLogicGrounded = bLogicHasGround && (LogicGap <= LogicGroundMaxGapCm);
			}

			const bool bDriveGrounded = bGrounded || (bLogicGrounded && DynHit.bBlockingHit);
			bChaosDriveEverGrounded = bChaosDriveEverGrounded || bDriveGrounded;
			bDriveGroundedForLog = bDriveGrounded;
			bWheelGroundedForLog = bGrounded;
			bLogicGroundedForLog = bLogicGrounded;
			DebugWheelGapCm = GapCm;
			DebugLogicGapCm = LogicGap;
			DebugMaxWaitSeconds = MaxWaitSeconds;

			FHitResult StaticHit;
			bool bStaticHit = false;
			if (!bGrounded || bSample)
			{
				bStaticHit = TraceDownFromPawnBounds(*GetWorld(), *ChaosVehicle.Get(), ECC_WorldStatic, TraceDepthCm, StaticHit);
				if (!bGrounded && bStaticHit)
				{
					if (UPrimitiveComponent* HitComp = StaticHit.GetComponent())
					{
						if (HitComp->GetCollisionResponseToChannel(ECC_WorldDynamic) != ECR_Block)
						{
							HitComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
						}
					}
				}
			}

			// Limited, always-on diagnostics for the first few sample vehicles.
			if (bSample && (Age - ChaosDriveLastGroundDiagAgeSeconds) >= 0.5f)
			{
				ChaosDriveLastGroundDiagAgeSeconds = Age;

				const bool bDynHit = DynHit.bBlockingHit;

				if (!bDynHit && bStaticHit)
				{
					UPrimitiveComponent* HitComp = StaticHit.GetComponent();
					const ECollisionResponse RespToDyn = HitComp ? HitComp->GetCollisionResponseToChannel(ECC_WorldDynamic) : ECR_MAX;
					if (HitComp && RespToDyn != ECR_Block)
					{
						HitComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
					}

					if (!bChaosDriveLoggedGroundMismatch)
					{
						bChaosDriveLoggedGroundMismatch = true;
						const int32 ObjType = HitComp ? static_cast<int32>(HitComp->GetCollisionObjectType()) : -1;
						UE_LOG(LogTraffic, Warning,
							TEXT("[TrafficVehicleAdapter] Sample[%d] GroundTrace mismatch: ECC_WorldDynamic MISS but ECC_WorldStatic HIT (%s.%s objType=%d respToWorldDynamic=%d). Wheels may never see the road."),
							DebugIndex,
							StaticHit.GetActor() ? *StaticHit.GetActor()->GetName() : TEXT("<none>"),
							HitComp ? *HitComp->GetName() : TEXT("<none>"),
							ObjType,
							static_cast<int32>(RespToDyn));
					}
				}

				const FVector PawnPos = ChaosVehicle->GetActorLocation();
				const float DynZ = bDynHit ? DynHit.ImpactPoint.Z : TNumericLimits<float>::Max();
				const float StaticZ = bStaticHit ? StaticHit.ImpactPoint.Z : TNumericLimits<float>::Max();
				UE_LOG(LogTraffic, Log,
					TEXT("[TrafficVehicleAdapter] Sample[%d] GroundCheck age=%.2fs wheelCh=%d(%s) grounded=%d gap=%.1fcm dynHit=%d staticHit=%d pawnZ=%.1f dynZ=%.1f staticZ=%.1f logicGap=%.1f upZ=%.2f speed=%.1f"),
					DebugIndex,
					Age,
					static_cast<int32>(ChaosWheelChannel),
					CollisionChannelName(ChaosWheelChannel),
					bGrounded ? 1 : 0,
					GapCm,
					bDynHit ? 1 : 0,
					bStaticHit ? 1 : 0,
					PawnPos.Z,
					DynZ,
					StaticZ,
					LogicGap,
					ChaosVehicle->GetActorUpVector().Z,
					ChaosVehicle->GetVelocity().Size());
			}

			// During the early spawn window, do not drive until grounded.
			if (!bDriveGrounded && (Age <= MaxWaitSeconds))
			{
				if (bSample && bLogicGrounded && !bGrounded)
				{
					UE_LOG(LogTraffic, Log,
						TEXT("[TrafficVehicleAdapter] Sample[%d] Using logic-grounded fallback for drive (logicGap=%.1fcm)."),
						DebugIndex,
						LogicGap);
				}
				ApplyInitialBraking(Move);
				CallSingleParam(Move, FName(TEXT("SetHandbrakeInput")), 0.f, false);
				return;
			}
		}

		if (bSample && LogicVehicle.IsValid())
		{
			EPathFollowTargetType FollowType = EPathFollowTargetType::None;
			int32 FollowId = INDEX_NONE;
			float FollowS = 0.f;
			if (LogicVehicle->GetFollowTarget(FollowType, FollowId, FollowS))
			{
				const uint8 TypeRaw = static_cast<uint8>(FollowType);
				const bool bChanged = !bHasPrevFollowTarget || (TypeRaw != PrevFollowTargetTypeRaw) || (FollowId != PrevFollowTargetId);
				if (bChanged)
				{
					UE_LOG(LogTraffic, Log,
						TEXT("[TrafficVehicleAdapter] Sample[%d] %s FollowTarget type=%d id=%d S=%.1f"),
						DebugIndex,
						*ChaosVehicle->GetName(),
						static_cast<int32>(FollowType),
						FollowId,
						FollowS);
				}

				PrevFollowTargetTypeRaw = TypeRaw;
				PrevFollowTargetId = FollowId;
				PrevFollowTargetS = FollowS;
				bHasPrevFollowTarget = true;
			}
		}

		// Ensure any handbrake applied during spawn warmup is released before driving.
		CallSingleParam(Move, FName(TEXT("SetHandbrakeInput")), 0.f, false);

		const FVector ChaosPos = ChaosVehicle->GetActorLocation();
		const FVector ChaosFwd = ChaosVehicle->GetActorForwardVector();
		const FVector LogicPos = LogicVehicle->GetActorLocation();
		const FVector LogicFwd = LogicVehicle->GetActorForwardVector();

		const float Lookahead = FMath::Max(0.f, CVarTrafficChaosDriveLookaheadCm.GetValueOnGameThread());
		FVector Target = LogicPos + LogicFwd * Lookahead;
		FVector TargetTangent = LogicFwd;
		LogicVehicle->SampleFollowPoseAheadOf(ChaosPos, ChaosFwd, Lookahead, Target, TargetTangent);

		const FVector2D ToTarget2D(Target.X - ChaosPos.X, Target.Y - ChaosPos.Y);
		const FVector2D Fwd2D(ChaosFwd.X, ChaosFwd.Y);
		const FVector2D DesiredDir2D(TargetTangent.X, TargetTangent.Y);

		const float ToLen = ToTarget2D.Size();
		const FVector2D ToN = (ToLen > KINDA_SMALL_NUMBER) ? (ToTarget2D / ToLen) : FVector2D::ZeroVector;
		const FVector2D FwdN = Fwd2D.GetSafeNormal();

		const FVector2D DesiredN = DesiredDir2D.GetSafeNormal();
		const FVector2D AimN = (!DesiredN.IsNearlyZero()) ? DesiredN : ToN;

		const float Cross = (FwdN.X * AimN.Y) - (FwdN.Y * AimN.X);
		const float Dot = (FwdN.X * AimN.X) + (FwdN.Y * AimN.Y);
		const float YawErrorRad = FMath::Atan2(Cross, Dot);

		const float SteerGain = CVarTrafficChaosDriveSteerGain.GetValueOnGameThread();
		float Steering = FMath::Clamp(YawErrorRad * SteerGain, -1.f, 1.f);

		const float DesiredSpeed = LogicVehicle->GetPlannedSpeedCmPerSec();
		const FVector Vel = ChaosVehicle->GetVelocity();
		const float CurrentForwardSpeed = FVector::DotProduct(Vel, ChaosFwd); // signed cm/s
		const float CurrentSpeed = FMath::Abs(CurrentForwardSpeed);

		// Slow down when facing a large yaw error (prevents "full throttle + full lock" spin behavior).
		const float StartDeg = FMath::Max(0.f, CVarTrafficChaosDriveTurnSlowdownStartDeg.GetValueOnGameThread());
		const float FullDeg = FMath::Max(StartDeg + 0.1f, CVarTrafficChaosDriveTurnSlowdownFullDeg.GetValueOnGameThread());
		const float YawAbsDeg = FMath::Abs(FMath::RadiansToDegrees(YawErrorRad));
		const float TurnAlpha = FMath::Clamp((YawAbsDeg - StartDeg) / (FullDeg - StartDeg), 0.f, 1.f);
		const float TurnScale = 1.f - TurnAlpha;

		// Ramp speed up gradually after spawn.
		float DriveScale = 1.f;
		if (DriveRamp > 0.f && Age > DriveDelay)
		{
			DriveScale = FMath::Clamp((Age - DriveDelay) / DriveRamp, 0.f, 1.f);
		}

		// Ramp steering authority up gradually after spawn. This helps prevent low-speed spin-outs when yaw error is large.
		const float SteerLimit = FMath::Clamp(0.25f + 0.75f * DriveScale, 0.25f, 1.0f);
		Steering = FMath::Clamp(Steering, -SteerLimit, SteerLimit);

		const float EffectiveDesiredSpeed = DesiredSpeed * TurnScale * DriveScale;
		const float SpeedError = EffectiveDesiredSpeed - CurrentSpeed;

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

		// Spawn spin damping: clamp excessive chassis angular speed at low linear speeds for a short window.
		{
			const float SpinSeconds = FMath::Max(0.f, CVarTrafficChaosDriveSpawnSpinDampSeconds.GetValueOnGameThread());
			if (SpinSeconds > 0.f && Age <= SpinSeconds)
			{
				const float MaxSpeed = FMath::Max(0.f, CVarTrafficChaosDriveSpawnSpinDampMaxSpeedCmPerSec.GetValueOnGameThread());
				if (CurrentSpeed <= MaxSpeed)
				{
					const float MaxAng = FMath::Max(0.f, CVarTrafficChaosDriveSpawnSpinDampMaxAngularDegPerSec.GetValueOnGameThread());
					if (GetChassisAngularSpeedDegPerSec(*ChaosVehicle.Get()) > MaxAng)
					{
						ClampChassisAngularVelocity(*ChaosVehicle.Get(), MaxAng);
						// If we're spinning wildly, prefer braking over throttle this frame.
						Throttle = 0.f;
						Brake = FMath::Max(Brake, 0.5f);
					}
				}
			}
		}

		// Persistent low-speed spin clamp: prevents vehicles from endlessly pirouetting after a collision.
		if (CVarTrafficChaosDriveLowSpeedSpinClamp.GetValueOnGameThread() != 0)
		{
			const float MaxSpeed = FMath::Max(0.f, CVarTrafficChaosDriveLowSpeedSpinClampMaxSpeedCmPerSec.GetValueOnGameThread());
			if (CurrentSpeed <= MaxSpeed)
			{
				const float MaxAng = FMath::Max(0.f, CVarTrafficChaosDriveLowSpeedSpinClampMaxAngularDegPerSec.GetValueOnGameThread());
				if (GetChassisAngularSpeedDegPerSec(*ChaosVehicle.Get()) > MaxAng)
				{
					ClampChassisAngularVelocity(*ChaosVehicle.Get(), MaxAng);
					Throttle = 0.f;
					Brake = FMath::Max(Brake, 0.6f);
					Steering = 0.f;
				}
			}
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

		if (bSample && CVarTrafficChaosDriveDebug.GetValueOnGameThread() != 0 &&
			(Age - ChaosDriveLastDriveDiagAgeSeconds) >= 0.5f)
		{
			ChaosDriveLastDriveDiagAgeSeconds = Age;
			const bool bHasSimPrim = HasAnySimulatingPrimitive(*ChaosVehicle.Get());
			const bool bMoveActive = Move ? Move->IsActive() : false;
			const bool bMoveTick = Move ? Move->IsComponentTickEnabled() : false;
			const UPrimitiveComponent* MoveUpdatedPrim = Move ? Cast<UPrimitiveComponent>(Move->UpdatedComponent) : nullptr;
			const bool bMoveUpdatedSim = MoveUpdatedPrim ? MoveUpdatedPrim->IsSimulatingPhysics() : false;
			UE_LOG(LogTraffic, Log,
				TEXT("[TrafficVehicleAdapter] Sample[%d] DriveInputs age=%.2fs desired=%.1f eff=%.1f cur=%.1f yaw=%.1f turn=%.2f driveScale=%.2f steer=%.2f throttle=%.2f brake=%.2f grounded=%d/%d/%d gap=%.1f logicGap=%.1f wait=%.1f inputOk=%d/%d/%d sim=%d move=%d/%d updSim=%d"),
				DebugIndex,
				Age,
				DesiredSpeed,
				EffectiveDesiredSpeed,
				CurrentSpeed,
				YawAbsDeg,
				TurnScale,
				DriveScale,
				Steering,
				Throttle,
				Brake,
				bDriveGroundedForLog ? 1 : 0,
				bWheelGroundedForLog ? 1 : 0,
				bLogicGroundedForLog ? 1 : 0,
				DebugWheelGapCm,
				DebugLogicGapCm,
				DebugMaxWaitSeconds,
				bSteerOk ? 1 : 0,
				bThrottleOk ? 1 : 0,
				bBrakeOk ? 1 : 0,
				bHasSimPrim ? 1 : 0,
				bMoveActive ? 1 : 0,
				bMoveTick ? 1 : 0,
				bMoveUpdatedSim ? 1 : 0);
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
