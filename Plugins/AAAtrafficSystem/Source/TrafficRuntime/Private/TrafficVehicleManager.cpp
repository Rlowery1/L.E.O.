#include "TrafficVehicleManager.h"
#include "TrafficVehicleBase.h"
#include "TrafficNetworkAsset.h"
#include "TrafficRoadTypes.h"
#include "TrafficSystemController.h"
#include "TrafficRuntimeModule.h"
#include "TrafficVehicleSettings.h"
#include "TrafficVehicleAdapter.h"
#include "TrafficVehicleProfile.h"
#include "TrafficLaneGeometry.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/MovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "UObject/SoftObjectPath.h"
#include "Misc/AutomationTest.h"
#include "ZoneGraphSubsystem.h"
#include "CollisionQueryParams.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/BodySetup.h"

static TAutoConsoleVariable<int32> CVarShowLogicDebugMesh(
	TEXT("aaa.Traffic.ShowLogicDebugMesh"),
	0,
	TEXT("If non-zero, show the TrafficVehicleBase debug cube even when Chaos visuals are present.\n")
	TEXT("Default: 0 (hide cube when Chaos pawn is spawned)."),
	ECVF_Default);

static int32 GetTrafficVisualModeForManager()
{
	IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Visual.Mode"));
	return Var ? Var->GetInt() : 1;
}

static TAutoConsoleVariable<float> CVarTrafficChaosDriveSpawnZOffsetCm(
	TEXT("aaa.Traffic.Visual.ChaosDrive.SpawnZOffsetCm"),
	20.f,
	TEXT("Extra Z clearance (cm) above ground when spawning ChaosDrive visual pawns."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveSpawnTraceMaxDeltaZCm(
	TEXT("aaa.Traffic.Visual.ChaosDrive.SpawnTraceMaxDeltaZCm"),
	100.f,
	TEXT("If the ground trace hit is farther than this (cm) from the lane's desired Z, ignore the trace and use lane Z instead."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveSpawnInitialLiftCm(
	TEXT("aaa.Traffic.Visual.ChaosDrive.SpawnInitialLiftCm"),
	200.f,
	TEXT("Extra Z (cm) to spawn the Chaos pawn above its final position, then sweep down into place (helps avoid initial penetration impulses)."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficSpawnMaxVehicles(
	TEXT("aaa.Traffic.Spawn.MaxVehicles"),
	0,
	TEXT("If >0, limits total spawned traffic vehicles (useful for Chaos stability debugging). Default: 0 (no limit)."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficChaosDriveSpawnDebug(
	TEXT("aaa.Traffic.Visual.ChaosDrive.SpawnDebug"),
	0,
	TEXT("If non-zero, logs ChaosDrive spawn placement traces and penetration resolution."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarTrafficChaosDriveSpawnResolvePenetration(
	TEXT("aaa.Traffic.Visual.ChaosDrive.SpawnResolvePenetration"),
	1,
	TEXT("If non-zero, attempts to resolve spawn overlap by lifting the vehicle upward."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveSpawnResolveStepCm(
	TEXT("aaa.Traffic.Visual.ChaosDrive.SpawnResolveStepCm"),
	50.f,
	TEXT("Step (cm) per iteration when resolving spawn overlap by lifting."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTrafficChaosDriveSpawnResolveMaxLiftCm(
	TEXT("aaa.Traffic.Visual.ChaosDrive.SpawnResolveMaxLiftCm"),
	2500.f,
	TEXT("Max total lift (cm) when resolving spawn overlap by lifting."),
	ECVF_Default);

static const FName TrafficSpawnedVehicleTag(TEXT("AAA_TrafficVehicle"));

static int32 IsComplexAsSimpleCollision(const FHitResult& Hit)
{
	const UPrimitiveComponent* Prim = Hit.GetComponent();
	const UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Prim);
	if (!SMC)
	{
		return 0;
	}

	const UStaticMesh* Mesh = SMC->GetStaticMesh();
	const UBodySetup* BodySetup = Mesh ? Mesh->GetBodySetup() : nullptr;
	if (!BodySetup)
	{
		return 0;
	}

	return (BodySetup->CollisionTraceFlag == CTF_UseComplexAsSimple) ? 1 : 0;
}

static float EstimatePawnHalfHeightCm(const TSubclassOf<APawn>& PawnClass)
{
	if (!PawnClass)
	{
		return 100.f;
	}

	const APawn* CDO = PawnClass->GetDefaultObject<APawn>();
	if (!CDO)
	{
		return 100.f;
	}

	const FBox Box = CDO->GetComponentsBoundingBox(true);
	if (!Box.IsValid)
	{
		return 100.f;
	}

	return FMath::Max(50.f, Box.GetExtent().Z);
}

static float EstimatePawnBottomOffsetCm(const TSubclassOf<APawn>& PawnClass)
{
	if (!PawnClass)
	{
		return 100.f;
	}

	const APawn* CDO = PawnClass->GetDefaultObject<APawn>();
	if (!CDO)
	{
		return 100.f;
	}

	const FBox Box = CDO->GetComponentsBoundingBox(true);
	if (!Box.IsValid)
	{
		return 100.f;
	}

	// If the box min is negative, the actor origin is above the lowest point of its bounds.
	// Spawn so that the lowest point sits on the ground plus a small clearance.
	return FMath::Clamp(-Box.Min.Z, 0.f, 500.f);
}

static void LogPawnBoundsForDebug(const TSubclassOf<APawn>& PawnClass)
{
	if (CVarTrafficChaosDriveSpawnDebug.GetValueOnGameThread() == 0 || !PawnClass)
	{
		return;
	}

	const APawn* CDO = PawnClass->GetDefaultObject<APawn>();
	if (!CDO)
	{
		return;
	}

	const FBox Box = CDO->GetComponentsBoundingBox(true);
	if (!Box.IsValid)
	{
		return;
	}

	UE_LOG(LogTraffic, Log,
		TEXT("[VehicleManager] VisualCDO bounds min=(%.1f %.1f %.1f) max=(%.1f %.1f %.1f) extent=(%.1f %.1f %.1f)"),
		Box.Min.X, Box.Min.Y, Box.Min.Z,
		Box.Max.X, Box.Max.Y, Box.Max.Z,
		Box.GetExtent().X, Box.GetExtent().Y, Box.GetExtent().Z);
}

static FVector EstimatePawnBoxExtentCm(const TSubclassOf<APawn>& PawnClass)
{
	if (!PawnClass)
	{
		return FVector(200.f, 100.f, 100.f);
	}

	const APawn* CDO = PawnClass->GetDefaultObject<APawn>();
	if (!CDO)
	{
		return FVector(200.f, 100.f, 100.f);
	}

	FBox Box = CDO->GetComponentsBoundingBox(true);
	if (!Box.IsValid)
	{
		return FVector(200.f, 100.f, 100.f);
	}

	const FVector Ext = Box.GetExtent();
	return FVector(FMath::Max(50.f, Ext.X), FMath::Max(50.f, Ext.Y), FMath::Max(50.f, Ext.Z));
}

static FVector ComputeChaosDriveSpawnLocation(
	UWorld& World,
	const FVector& DesiredLocation,
	float BottomOffsetCm,
	float ExtraZOffsetCm,
	const FCollisionQueryParams& QueryParams,
	bool& bOutUsedTraceZ,
	FHitResult* OutHit)
{
	const FVector Start = DesiredLocation + FVector(0.f, 0.f, 3000.f);
	const FVector End = DesiredLocation - FVector(0.f, 0.f, 5000.f);

	FHitResult Hit;
	bool bHit =
		World.LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, QueryParams) ||
		World.LineTraceSingleByChannel(Hit, Start, End, ECC_WorldDynamic, QueryParams) ||
		World.LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams);

	if (!bHit)
	{
		// Fallback: object-type query that can catch custom collision setups.
		FCollisionObjectQueryParams ObjParams;
		ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
		ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);
		ObjParams.AddObjectTypesToQuery(ECC_PhysicsBody);
		ObjParams.AddObjectTypesToQuery(ECC_Vehicle);
		ObjParams.AddObjectTypesToQuery(ECC_Pawn);
		bHit = World.LineTraceSingleByObjectType(Hit, Start, End, ObjParams, QueryParams);
	}

	const float Lift = FMath::Max(0.f, BottomOffsetCm) + FMath::Max(0.f, ExtraZOffsetCm);
	const float MaxDeltaZ = FMath::Max(0.f, CVarTrafficChaosDriveSpawnTraceMaxDeltaZCm.GetValueOnGameThread());
	const bool bTraceZTrusted = bHit && (FMath::Abs(Hit.ImpactPoint.Z - DesiredLocation.Z) <= MaxDeltaZ);

	bOutUsedTraceZ = bTraceZTrusted;

	const float BaseZ = bTraceZTrusted ? Hit.ImpactPoint.Z : DesiredLocation.Z;
	if (bHit)
	{
		if (OutHit)
		{
			*OutHit = Hit;
		}
		return FVector(DesiredLocation.X, DesiredLocation.Y, BaseZ + Lift);
	}

	if (OutHit)
	{
		*OutHit = FHitResult();
	}
	bOutUsedTraceZ = false;
	return DesiredLocation + FVector(0.f, 0.f, Lift);
}

static bool IsChaosDriveSpawnLocationFree(
	UWorld& World,
	const FVector& Location,
	const FQuat& Rotation,
	const FVector& BoxExtentCm,
	const FCollisionQueryParams& QueryParams)
{
	FCollisionShape Shape = FCollisionShape::MakeBox(BoxExtentCm);

	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjParams.AddObjectTypesToQuery(ECC_Vehicle);
	ObjParams.AddObjectTypesToQuery(ECC_Pawn);

	// UE5.7 provides OverlapAnyTestByObjectType (blocking vs overlap is controlled by collision settings).
	return !World.OverlapAnyTestByObjectType(Location, Rotation, ObjParams, Shape, QueryParams);
}

static FVector ResolveChaosDriveSpawnPenetrationByLifting(
	UWorld& World,
	const FVector& InitialLocation,
	const FQuat& Rotation,
	const FVector& BoxExtentCm,
	const FCollisionQueryParams& QueryParams)
{
	if (CVarTrafficChaosDriveSpawnResolvePenetration.GetValueOnGameThread() == 0)
	{
		return InitialLocation;
	}

	const float Step = FMath::Max(1.f, CVarTrafficChaosDriveSpawnResolveStepCm.GetValueOnGameThread());
	const float MaxLift = FMath::Max(0.f, CVarTrafficChaosDriveSpawnResolveMaxLiftCm.GetValueOnGameThread());

	FVector Loc = InitialLocation;
	for (float Lift = 0.f; Lift <= MaxLift; Lift += Step)
	{
		if (IsChaosDriveSpawnLocationFree(World, Loc, Rotation, BoxExtentCm, QueryParams))
		{
			return Loc;
		}
		Loc.Z += Step;
	}

	return InitialLocation;
}

static FQuat MakeSpawnRotationFromGroundHit(const FTransform& DesiredXform, const FHitResult& Hit)
{
	FVector Up = Hit.ImpactNormal.GetSafeNormal();
	if (Up.IsNearlyZero())
	{
		return DesiredXform.GetRotation();
	}

	// Some road meshes (especially thin/double-sided collision) can report a downward normal even when tracing from above.
	// Never align a vehicle to a downward "up" vector.
	if (Up.Z < 0.f)
	{
		Up *= -1.f;
	}

	// If the "ground" is too steep to be drivable, keep the desired rotation (prevents snapping onto walls/undersides).
	if (Up.Z < 0.25f)
	{
		return DesiredXform.GetRotation();
	}

	FVector Forward = DesiredXform.GetRotation().GetForwardVector();
	Forward = (Forward - Up * FVector::DotProduct(Forward, Up));
	if (!Forward.Normalize())
	{
		// Fallback: pick an arbitrary forward perpendicular to Up.
		Forward = FVector::CrossProduct(Up, FVector::RightVector);
		if (!Forward.Normalize())
		{
			Forward = FVector::CrossProduct(Up, FVector::ForwardVector);
			Forward.Normalize();
		}
	}

	const FMatrix RotM = FRotationMatrix::MakeFromXZ(Forward, Up);
	return RotM.ToQuat();
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

static void DisableAutoPossess(APawn& Pawn)
{
	Pawn.AutoPossessPlayer = EAutoReceiveInput::Disabled;
	Pawn.AutoPossessAI = EAutoPossessAI::Disabled;
	Pawn.AIControllerClass = nullptr;
	Pawn.DetachFromControllerPendingDestroy();
	Pawn.DisableInput(nullptr);
}

static void TemporarilyDisablePhysics(APawn& Pawn, TArray<UPrimitiveComponent*>& OutSimulatingComps)
{
	OutSimulatingComps.Reset();

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
			OutSimulatingComps.Add(Prim);
			Prim->SetSimulatePhysics(false);
		}
	}

	TArray<UMovementComponent*> MoveComps;
	Pawn.GetComponents(MoveComps);
	for (UMovementComponent* Move : MoveComps)
	{
		if (Move)
		{
			Move->Deactivate();
			Move->SetComponentTickEnabled(false);
		}
	}
}

static void RestorePhysicsAndOptionallyMovement(APawn& Pawn, const TArray<UPrimitiveComponent*>& SimulatingComps, bool bRestoreMovement)
{
	for (UPrimitiveComponent* Prim : SimulatingComps)
	{
		if (Prim)
		{
			Prim->SetSimulatePhysics(true);
		}
	}

	if (!bRestoreMovement)
	{
		return;
	}

	TArray<UMovementComponent*> MoveComps;
	Pawn.GetComponents(MoveComps);
	for (UMovementComponent* Move : MoveComps)
	{
		if (Move)
		{
			Move->Activate(true);
			Move->SetComponentTickEnabled(true);
		}
	}
}

// Helper to compute safe spawn positions along a lane.
static void ComputeSpawnPositionsForLane(
	const FTrafficLane& Lane,
	int32 DesiredVehicles,
	float VehicleLengthCm,
	float MinSpawnSpacingMultiplier,
	float MinUsableLaneLengthCm,
	TArray<float>& OutSParams)
{
	OutSParams.Reset();

	const float LaneLength = TrafficLaneGeometry::ComputeLaneLengthCm(Lane);

	if (LaneLength <= 0.f || LaneLength < MinUsableLaneLengthCm)
	{
		// Lane is too short for multiple vehicles, maybe too short for even one. Let caller decide.
		return;
	}

	const float MinSpacing = VehicleLengthCm * FMath::Max(MinSpawnSpacingMultiplier, 1.0f);
	const int32 MaxVehiclesByLength = FMath::FloorToInt(LaneLength / MinSpacing);

	const int32 NumToSpawn = FMath::Clamp(DesiredVehicles, 0, MaxVehiclesByLength);

	if (NumToSpawn <= 0)
	{
		return;
	}

	// Evenly distribute along the lane, leaving some margin at start/end.
	const float Segment = LaneLength / (NumToSpawn + 1);
	for (int32 i = 0; i < NumToSpawn; ++i)
	{
		const float S = Segment * (i + 1); // in cm along lane
		OutSParams.Add(S);
	}
}

ATrafficVehicleManager::ATrafficVehicleManager()
{
	PrimaryActorTick.bCanEverTick = false;
	NetworkAsset = nullptr;
	Tags.AddUnique(TrafficSpawnedVehicleTag);
}

void ATrafficVehicleManager::BeginPlay()
{
	Super::BeginPlay();
}

UTrafficNetworkAsset* ATrafficVehicleManager::FindNetworkAsset() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ATrafficSystemController> It(World); It; ++It)
	{
		if (ATrafficSystemController* Controller = *It)
		{
			return Controller->GetBuiltNetworkAsset();
		}
	}

	return nullptr;
}

bool ATrafficVehicleManager::LoadNetwork()
{
	NetworkAsset = FindNetworkAsset();
	if (!NetworkAsset)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[VehicleManager] No network asset available."));
		return false;
	}
	return true;
}

void ATrafficVehicleManager::ClearVehicles()
{
	DestroyAdaptersAndVisuals();

	for (ATrafficVehicleBase* Vehicle : Vehicles)
	{
		if (Vehicle)
		{
			Vehicle->Destroy();
		}
	}
	Vehicles.Empty();
	LastLaneSpawnTimes.Empty();
}

void ATrafficVehicleManager::DestroyAdaptersAndVisuals()
{
	for (TWeakObjectPtr<ATrafficVehicleAdapter>& Adapter : Adapters)
	{
		if (Adapter.IsValid())
		{
			Adapter->Destroy();
		}
	}
	Adapters.Empty();

	for (TWeakObjectPtr<APawn>& Visual : VisualVehicles)
	{
		if (Visual.IsValid())
		{
			Visual->Destroy();
		}
	}
	VisualVehicles.Empty();
}

void ATrafficVehicleManager::SetActiveRunMetrics(FTrafficRunMetrics* InMetrics)
{
	ActiveMetrics = InMetrics;
}

void ATrafficVehicleManager::SetForceLogicOnlyForTests(bool bInForce)
{
	bForceLogicOnlyForTests = bInForce;
}

const UTrafficVehicleProfile* ATrafficVehicleManager::ResolveDefaultVehicleProfile() const
{
	const UTrafficVehicleSettings* Settings = UTrafficVehicleSettings::Get();
	if (!Settings)
	{
		return nullptr;
	}

	const UTrafficVehicleProfile* Profile = Settings->GetDefaultVehicleProfile();
	if (!Profile)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[VehicleManager] DefaultVehicleProfile could not be resolved."));
	}
	return Profile;
}

void ATrafficVehicleManager::SpawnTestVehicles(int32 VehiclesPerLane, float SpeedCmPerSec)
{
	if (!LoadNetwork())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ATrafficSystemController* Controller = nullptr;
	for (TActorIterator<ATrafficSystemController> It(World); It; ++It)
	{
		Controller = *It;
		break;
	}

	if (NetworkAsset)
	{
		UE_LOG(LogTraffic, Log, TEXT("[VehicleManager] Spawning test vehicles on %d lanes."), NetworkAsset->Network.Lanes.Num());
	}

	ClearVehicles();

	const UTrafficVehicleProfile* Profile = ResolveDefaultVehicleProfile();
	if (Profile && !bForceLogicOnlyForTests)
	{
		UE_LOG(LogTraffic, Log, TEXT("[VehicleManager] Using vehicle profile '%s' (%s)."),
			*Profile->GetName(),
			Profile->VehicleClass.IsNull() ? TEXT("<no class>") : *Profile->VehicleClass.ToString());
	}

	const UTrafficVehicleSettings* VehicleSettings = UTrafficVehicleSettings::Get();
	const float MinSpacingMultiplier = VehicleSettings ? VehicleSettings->MinSpawnSpacingMultiplier : 2.0f;
	const float MinUsableLaneLengthCm = VehicleSettings ? VehicleSettings->MinUsableLaneLengthCm : 2000.f;

	// Use default vehicle profile to estimate length; fall back to a conservative guess if needed.
	float ApproxVehicleLengthCm = 450.f;
	if (Profile && Profile->LengthCm > 0.f)
	{
		ApproxVehicleLengthCm = Profile->LengthCm;
	}

	TSubclassOf<ATrafficVehicleBase> LogicClass = ATrafficVehicleBase::StaticClass();
	TSubclassOf<APawn> VisualClass = nullptr;
	if (!bForceLogicOnlyForTests && Profile && !Profile->VehicleClass.IsNull())
	{
		VisualClass = Profile->VehicleClass.LoadSynchronous();
		if (!VisualClass)
		{
			UE_LOG(LogTraffic, Warning,
				TEXT("[VehicleManager] DefaultVehicleProfile VehicleClass failed to load: %s"),
				*Profile->VehicleClass.ToString());
		}
	}
	else if (!bForceLogicOnlyForTests && !VisualClass && GIsAutomationTesting)
	{
		static const TCHAR* DevChaosClassPath = TEXT("/Game/CitySampleVehicles/vehicle07_Car/BP_vehicle07_Car.BP_vehicle07_Car_C");
		UE_LOG(LogTraffic, Warning,
			TEXT("[VehicleManager] Automation: DefaultVehicleProfile invalid. Attempting to load dev Chaos class at %s"),
			DevChaosClassPath);

		FSoftObjectPath DevPath(DevChaosClassPath);
		UObject* LoadedObj = DevPath.TryLoad();
		UClass* DevClass = Cast<UClass>(LoadedObj);
		if (!DevClass)
		{
			DevClass = LoadClass<APawn>(nullptr, DevChaosClassPath);
		}

		if (DevClass)
		{
			VisualClass = DevClass;
			UE_LOG(LogTraffic, Warning,
				TEXT("[VehicleManager] Automation: Using dev Chaos class %s"),
				DevChaosClassPath);
		}
		else
		{
			UE_LOG(LogTraffic, Error,
				TEXT("[VehicleManager] Automation: Failed to load dev Chaos class at %s"),
				DevChaosClassPath);
		}
	}

	// Allow forcing logic-only spawn via CVar (useful for debugging and CI).
	if (GetTrafficVisualModeForManager() <= 0)
	{
		VisualClass = nullptr;
	}

	if (!bForceLogicOnlyForTests && !VisualClass && !GIsAutomationTesting)
	{
		UE_LOG(LogTraffic, Warning,
			TEXT("[VehicleManager] No Chaos vehicle configured. Spawning logic-only vehicles (debug cubes).\n")
			TEXT("Set a DefaultVehicleProfile with a valid VehicleClass in Project Settings -> AAA Traffic Vehicle Settings for full Chaos visuals."));
	}

	const TArray<FTrafficLane>& Lanes = NetworkAsset->Network.Lanes;
	LastLaneSpawnTimes.Empty();
	TMap<int32, int32> SpawnedPerLane;
	const int32 MaxVehicles = CVarTrafficSpawnMaxVehicles.GetValueOnGameThread();

	if (VisualClass)
	{
		LogPawnBoundsForDebug(VisualClass);
	}

	FActorSpawnParameters VehicleSpawnParams;
	VehicleSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	VehicleSpawnParams.Owner = this;

	for (int32 LaneIndex = 0; LaneIndex < Lanes.Num(); ++LaneIndex)
	{
		if (MaxVehicles > 0 && Vehicles.Num() >= MaxVehicles)
		{
			break;
		}

		const FTrafficLane& Lane = Lanes[LaneIndex];

		// When the network builder splits lanes to support mid-spline merges/diverges, we only want to spawn on the root segment.
		// Vehicles will transition across split segments via generated movements.
		if (Lane.PrevLaneId != INDEX_NONE)
		{
			continue;
		}

		if (Lane.CenterlinePoints.Num() < 2)
		{
			UE_LOG(LogTraffic, Verbose, TEXT("[VehicleManager] Lane %d has insufficient points; skipping spawn."), LaneIndex);
			continue;
		}

		const float LaneLength = TrafficLaneGeometry::ComputeLaneLengthCm(Lane);

		TArray<float> SpawnS;
		ComputeSpawnPositionsForLane(Lane, VehiclesPerLane, ApproxVehicleLengthCm, MinSpacingMultiplier, MinUsableLaneLengthCm, SpawnS);

		if (SpawnS.Num() == 0)
		{
			UE_LOG(LogTraffic, Verbose,
				TEXT("[VehicleManager] Lane %d (length=%.1fcm) too short for %d vehicles; skipping spawn."),
				LaneIndex,
				LaneLength,
				VehiclesPerLane);
			continue;
		}

		for (float S : SpawnS)
		{
			FVector SpawnPos = FVector::ZeroVector;
			FVector SpawnTangent = FVector::ForwardVector;
			if (!TrafficLaneGeometry::SamplePoseAtS(Lane, S, SpawnPos, SpawnTangent))
			{
				UE_LOG(LogTraffic, Warning, TEXT("[VehicleManager] Failed to sample lane %d at S=%.1f; skipping vehicle."), LaneIndex, S);
				continue;
			}

			const FTransform SpawnXform(SpawnTangent.Rotation(), SpawnPos);

			ATrafficVehicleBase* Vehicle = World->SpawnActor<ATrafficVehicleBase>(LogicClass, SpawnXform, VehicleSpawnParams);
			if (!Vehicle)
			{
				UE_LOG(LogTraffic, Warning, TEXT("[VehicleManager] Failed to spawn vehicle for lane %d (class %s)."), LaneIndex, *LogicClass->GetName());
				continue;
			}
			Vehicle->Tags.AddUnique(TrafficSpawnedVehicleTag);
			Vehicle->SetApproxVehicleLengthCm(ApproxVehicleLengthCm);

			const int32 LaneKey = (Lane.LaneId >= 0) ? Lane.LaneId : LaneIndex;
			const float Now = World->GetTimeSeconds();
			if (ActiveMetrics)
			{
				if (const float* LastTime = LastLaneSpawnTimes.Find(LaneKey))
				{
					ActiveMetrics->AccumulateHeadway(Now - *LastTime);
				}
			}
			LastLaneSpawnTimes.Add(LaneKey, Now);

			Vehicle->InitializeOnLane(&Lane, S, SpeedCmPerSec);
			Vehicle->SetNetworkAsset(NetworkAsset);
			Vehicle->SetTrafficSystemController(Controller);
			Vehicles.Add(Vehicle);
			SpawnedPerLane.FindOrAdd(LaneKey) += 1;
			if (ActiveMetrics)
			{
				ActiveMetrics->VehiclesSpawned++;
			}

			if (VisualClass)
			{
				FActorSpawnParameters Params;
				const int32 VisualMode = GetTrafficVisualModeForManager();
				Params.SpawnCollisionHandlingOverride =
					(VisualMode == 2) ? ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn : ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				Params.Owner = this;

				FTransform VisualXform = SpawnXform;
				if (VisualMode == 2)
				{
					// Disable logic collision before doing spawn queries so we don't trace/hit our own proxy vehicles.
					if (Vehicle)
					{
					Vehicle->SetActorEnableCollision(false);
					}

					FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AAA_Traffic_ChaosDriveSpawn), /*bTraceComplex=*/false);
					QueryParams.bReturnPhysicalMaterial = false;
					QueryParams.bFindInitialOverlaps = true;
					QueryParams.AddIgnoredActor(this);
					if (Vehicle)
					{
						QueryParams.AddIgnoredActor(Vehicle);
					}
					for (ATrafficVehicleBase* ExistingLogic : Vehicles)
					{
						if (ExistingLogic)
						{
							QueryParams.AddIgnoredActor(ExistingLogic);
						}
					}
					for (const TWeakObjectPtr<APawn>& ExistingVisual : VisualVehicles)
					{
						if (ExistingVisual.IsValid())
						{
							QueryParams.AddIgnoredActor(ExistingVisual.Get());
						}
					}

					const float BottomOffset = EstimatePawnBottomOffsetCm(VisualClass);
					const float ExtraZ = CVarTrafficChaosDriveSpawnZOffsetCm.GetValueOnGameThread();
					const FVector Desired = VisualXform.GetLocation();
					const FQuat FlatYaw = FQuat(FRotator(0.f, VisualXform.GetRotation().Rotator().Yaw, 0.f));

					FHitResult Hit;
					bool bUsedTraceZ = false;
					FVector Adjusted = ComputeChaosDriveSpawnLocation(*World, Desired, BottomOffset, ExtraZ, QueryParams, bUsedTraceZ, &Hit);
					const FVector BoxExtent = EstimatePawnBoxExtentCm(VisualClass) + FVector(25.f, 25.f, 10.f);
					const FVector Resolved = ResolveChaosDriveSpawnPenetrationByLifting(*World, Adjusted, FlatYaw, BoxExtent, QueryParams);

					if (Hit.bBlockingHit && bUsedTraceZ)
					{
						VisualXform.SetRotation(MakeSpawnRotationFromGroundHit(VisualXform, Hit));
					}

					if (CVarTrafficChaosDriveSpawnDebug.GetValueOnGameThread() != 0)
					{
						UE_LOG(LogTraffic, Log,
							TEXT("[VehicleManager] ChaosDriveSpawn lane=%d desired=(%.1f %.1f %.1f) traceHit=%d traceUsed=%d hit=%s comp=%s complexAsSimple=%d n=(%.2f %.2f %.2f) impactZ=%.1f spawnZ=%.1f resolvedZ=%.1f bottomOffset=%.1f clearance=%.1f extent=(%.1f %.1f %.1f)"),
							Lane.LaneId,
							Desired.X, Desired.Y, Desired.Z,
							Hit.bBlockingHit ? 1 : 0,
							bUsedTraceZ ? 1 : 0,
							Hit.bBlockingHit && Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("<none>"),
							Hit.bBlockingHit && Hit.GetComponent() ? *Hit.GetComponent()->GetName() : TEXT("<none>"),
							Hit.bBlockingHit ? IsComplexAsSimpleCollision(Hit) : 0,
							Hit.bBlockingHit ? Hit.ImpactNormal.X : 0.f,
							Hit.bBlockingHit ? Hit.ImpactNormal.Y : 0.f,
							Hit.bBlockingHit ? Hit.ImpactNormal.Z : 0.f,
							Hit.bBlockingHit ? Hit.ImpactPoint.Z : 0.f,
							Adjusted.Z,
							Resolved.Z,
							BottomOffset,
							ExtraZ,
							BoxExtent.X, BoxExtent.Y, BoxExtent.Z);
					}

					VisualXform.SetLocation(Resolved);
					if (!Hit.bBlockingHit || !bUsedTraceZ)
					{
						VisualXform.SetRotation(FlatYaw);
					}
				}

				APawn* VisualPawn = nullptr;
				if (VisualMode == 2)
				{
					const float InitialLift = FMath::Max(0.f, CVarTrafficChaosDriveSpawnInitialLiftCm.GetValueOnGameThread());
					FTransform SpawnXformHigh = VisualXform;
					SpawnXformHigh.AddToTranslation(FVector(0.f, 0.f, InitialLift));

					VisualPawn = World->SpawnActorDeferred<APawn>(VisualClass, SpawnXformHigh, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
					if (VisualPawn)
					{
						DisableAutoPossess(*VisualPawn);
						VisualPawn->FinishSpawning(SpawnXformHigh, /*bIsDefaultTransform=*/true);

						TArray<UPrimitiveComponent*> DisabledSim;
						TemporarilyDisablePhysics(*VisualPawn, DisabledSim);

						// Sweep down into place with real pawn collision (more accurate than our approximate spawn box).
						FHitResult SweepHit;
						const float Step = FMath::Max(1.f, CVarTrafficChaosDriveSpawnResolveStepCm.GetValueOnGameThread());
						const float MaxLift = FMath::Max(0.f, CVarTrafficChaosDriveSpawnResolveMaxLiftCm.GetValueOnGameThread());
						bool bPlaced = false;
						FVector Candidate = VisualXform.GetLocation();
						for (float Lift = 0.f; Lift <= MaxLift; Lift += Step)
						{
							Candidate.Z = VisualXform.GetLocation().Z + Lift;
							// Use a real sweep (TeleportPhysics can bypass sweeps). Physics is currently disabled, so this is safe.
							if (VisualPawn->SetActorLocationAndRotation(Candidate, VisualXform.GetRotation(), /*bSweep=*/true, &SweepHit, ETeleportType::None))
							{
								bPlaced = true;
								break;
							}
						}
						if (!bPlaced)
						{
							// Fallback: teleport to the requested transform.
							VisualPawn->SetActorLocationAndRotation(VisualXform.GetLocation(), VisualXform.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
						}

						if (CVarTrafficChaosDriveSpawnDebug.GetValueOnGameThread() != 0)
						{
							UE_LOG(LogTraffic, Log,
								TEXT("[VehicleManager] ChaosDriveSpawnPlace placed=%d final=(%.1f %.1f %.1f) sweepHit=%d hit=%s"),
								bPlaced ? 1 : 0,
								VisualPawn->GetActorLocation().X,
								VisualPawn->GetActorLocation().Y,
								VisualPawn->GetActorLocation().Z,
								SweepHit.bBlockingHit ? 1 : 0,
								SweepHit.bBlockingHit && SweepHit.GetActor() ? *SweepHit.GetActor()->GetName() : TEXT("<none>"));
						}

						// Keep movement disabled here; the TrafficVehicleAdapter will enable it after it applies initial braking
						// (and respects aaa.Traffic.Visual.ChaosDrive.DisableMovementSeconds).
						RestorePhysicsAndOptionallyMovement(*VisualPawn, DisabledSim, /*bRestoreMovement=*/false);
					}
				}
				else
				{
					VisualPawn = World->SpawnActor<APawn>(VisualClass, VisualXform, Params);
				}
				if (VisualPawn)
				{
					VisualPawn->Tags.AddUnique(TrafficSpawnedVehicleTag);

					// Spawn stabilization: clear any initial velocities caused by spawn adjustment/penetration resolution.
					if (VisualMode == 2)
					{
						ZeroPhysicsVelocities(*VisualPawn);
					}

					ATrafficVehicleAdapter* Adapter = World->SpawnActor<ATrafficVehicleAdapter>(Params);
					if (Adapter)
					{
						Adapter->Tags.AddUnique(TrafficSpawnedVehicleTag);
						Adapter->Initialize(Vehicle, VisualPawn);
						Adapters.Add(Adapter);
					}
					VisualVehicles.Add(VisualPawn);

					// Hide the logic debug cube when Chaos visuals are active, unless dev CVar overrides it.
					const int32 bShowDebugMesh = CVarShowLogicDebugMesh.GetValueOnGameThread();
					if (!bForceLogicOnlyForTests && bShowDebugMesh == 0 && Vehicle)
					{
						Vehicle->SetDebugBodyVisible(false);
					}
				}
			}
		}
	}

	if (!bForceLogicOnlyForTests && (!Profile || !VisualClass))
	{
		UE_LOG(LogTraffic, Warning,
			TEXT("[VehicleManager] No valid Chaos vehicle class configured. "
				 "Spawning logic-only TrafficVehicleBase pawns (debug cubes) without visual Chaos vehicles.\n"
				 "Configure a DefaultVehicleProfile and VehicleClass in Project Settings -> AAA Traffic Vehicle Settings for full Chaos visuals."));
	}

	for (const TPair<int32, int32>& Pair : SpawnedPerLane)
	{
		UE_LOG(LogTraffic, Log, TEXT("[VehicleManager] Lane %d spawned %d vehicles."), Pair.Key, Pair.Value);
	}

	UE_LOG(LogTraffic, Log, TEXT("[VehicleManager] Spawned %d vehicles."), Vehicles.Num());
}

void ATrafficVehicleManager::SpawnZoneGraphVehicles(int32 VehiclesPerLane, float SpeedCmPerSec, FName RequiredLaneTag)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UZoneGraphSubsystem* ZGS = World->GetSubsystem<UZoneGraphSubsystem>();
	if (!ZGS)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[VehicleManager] ZoneGraphSubsystem unavailable; cannot spawn ZoneGraph vehicles."));
		return;
	}

	ClearVehicles();
	const int32 MaxVehicles = CVarTrafficSpawnMaxVehicles.GetValueOnGameThread();

	ATrafficSystemController* Controller = nullptr;
	for (TActorIterator<ATrafficSystemController> It(World); It; ++It)
	{
		Controller = *It;
		break;
	}

	const UTrafficVehicleProfile* Profile = ResolveDefaultVehicleProfile();
	if (Profile && !bForceLogicOnlyForTests)
	{
		UE_LOG(LogTraffic, Log, TEXT("[VehicleManager] ZoneGraph: Using vehicle profile '%s' (%s)."),
			*Profile->GetName(),
			Profile->VehicleClass.IsNull() ? TEXT("<no class>") : *Profile->VehicleClass.ToString());
	}

	const UTrafficVehicleSettings* VehicleSettings = UTrafficVehicleSettings::Get();
	const float MinSpacingMultiplier = VehicleSettings ? VehicleSettings->MinSpawnSpacingMultiplier : 2.0f;
	const float MinUsableLaneLengthCm = VehicleSettings ? VehicleSettings->MinUsableLaneLengthCm : 2000.f;

	// Default sizes if no profile is configured.
	const float ApproxVehicleLengthCm = Profile ? Profile->LengthCm : 450.f;

	// Resolve visual class if allowed.
	UClass* VisualClass = nullptr;
	if (Profile && !bForceLogicOnlyForTests)
	{
		VisualClass = Profile->VehicleClass.LoadSynchronous();
	}

	if (VisualClass)
	{
		LogPawnBoundsForDebug(VisualClass);
	}

	// Allow forcing logic-only spawn via CVar (useful for debugging and CI).
	if (GetTrafficVisualModeForManager() <= 0)
	{
		VisualClass = nullptr;
	}

	// In automation, allow a dev fallback class if the default profile is missing.
	if (GIsAutomationTesting && !VisualClass && !bForceLogicOnlyForTests)
	{
		const TCHAR* DevChaosClassPath = TEXT("/Game/Traffic/Dev/BP_TrafficChaosCar.BP_TrafficChaosCar_C");
		FSoftObjectPath DevPath(DevChaosClassPath);
		UObject* LoadedObj = DevPath.TryLoad();
		VisualClass = Cast<UClass>(LoadedObj);
		if (!VisualClass)
		{
			VisualClass = LoadClass<APawn>(nullptr, DevChaosClassPath);
		}
		if (VisualClass)
		{
			UE_LOG(LogTraffic, Warning, TEXT("[VehicleManager] ZoneGraph Automation: Using dev Chaos class %s"), DevChaosClassPath);
		}
	}

	// Note: vehicles spawned here are also tagged with AAA_TrafficVehicle for editor cleanup tooling.

	if (!bForceLogicOnlyForTests && !VisualClass && !GIsAutomationTesting)
	{
		UE_LOG(LogTraffic, Error,
			TEXT("[VehicleManager] ZoneGraph: No Chaos vehicle configured. Set a DefaultVehicleProfile with a valid VehicleClass in Project Settings -> AAA Traffic Vehicle Settings."));
		// Still allow logic-only spawn to proceed (debug cubes).
	}

	FZoneGraphTag RequiredTag = FZoneGraphTag::None;
	if (!RequiredLaneTag.IsNone())
	{
		RequiredTag = ZGS->GetTagByName(RequiredLaneTag);
		if (!RequiredTag.IsValid())
		{
			UE_LOG(LogTraffic, Warning,
				TEXT("[VehicleManager] ZoneGraph: Required tag '%s' is not defined in ZoneGraph settings; spawning on all lanes."),
				*RequiredLaneTag.ToString());
		}
	}

	const float MinSpacing = ApproxVehicleLengthCm * FMath::Max(MinSpacingMultiplier, 1.0f);

	FActorSpawnParameters VehicleSpawnParams;
	VehicleSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	VehicleSpawnParams.Owner = this;

	int32 TotalLaneCount = 0;
	int32 EligibleLaneCount = 0;

	for (const FRegisteredZoneGraphData& Registered : ZGS->GetRegisteredZoneGraphData())
	{
		const AZoneGraphData* DataActor = Registered.ZoneGraphData;
		if (!DataActor)
		{
			continue;
		}

		const FZoneGraphStorage& Storage = DataActor->GetStorage();
		const FZoneGraphDataHandle DataHandle = Storage.DataHandle;
		if (!DataHandle.IsValid())
		{
			continue;
		}

		for (int32 LaneIndex = 0; LaneIndex < Storage.Lanes.Num(); ++LaneIndex)
		{
			++TotalLaneCount;

			const FZoneGraphLaneHandle LaneHandle(LaneIndex, DataHandle);
			if (!ZGS->IsLaneValid(LaneHandle))
			{
				continue;
			}

			if (RequiredTag.IsValid())
			{
				FZoneGraphTagMask LaneTags;
				if (!ZGS->GetLaneTags(LaneHandle, LaneTags) || !LaneTags.Contains(RequiredTag))
				{
					continue;
				}
			}

			float LaneLength = 0.f;
			if (!ZGS->GetLaneLength(LaneHandle, LaneLength))
			{
				continue;
			}

			if (LaneLength < MinUsableLaneLengthCm || LaneLength <= KINDA_SMALL_NUMBER)
			{
				continue;
			}

			const int32 MaxVehiclesByLength = FMath::FloorToInt(LaneLength / MinSpacing);
			const int32 NumToSpawn = FMath::Clamp(VehiclesPerLane, 0, MaxVehiclesByLength);
			if (NumToSpawn <= 0)
			{
				continue;
			}

			++EligibleLaneCount;

			const float Segment = LaneLength / (NumToSpawn + 1);
			for (int32 i = 0; i < NumToSpawn; ++i)
			{
				if (MaxVehicles > 0 && Vehicles.Num() >= MaxVehicles)
				{
					return;
				}

				float Dist = Segment * (i + 1);
				// Small jitter to avoid perfect alignment stacks.
				Dist += FMath::FRandRange(-0.1f * Segment, 0.1f * Segment);
				Dist = FMath::Clamp(Dist, 0.f, LaneLength);

				FZoneGraphLaneLocation LaneLoc;
				if (!ZGS->CalculateLocationAlongLane(LaneHandle, Dist, LaneLoc))
				{
					continue;
				}

				const FTransform SpawnXform(LaneLoc.Direction.Rotation(), LaneLoc.Position);

				ATrafficVehicleBase* Vehicle = World->SpawnActor<ATrafficVehicleBase>(ATrafficVehicleBase::StaticClass(), SpawnXform, VehicleSpawnParams);
				if (!Vehicle)
				{
					continue;
				}
				Vehicle->Tags.AddUnique(TrafficSpawnedVehicleTag);
				Vehicle->SetTrafficSystemController(Controller);

				Vehicle->InitializeOnZoneGraphLane(ZGS, LaneHandle, Dist, SpeedCmPerSec);
				Vehicles.Add(Vehicle);

				if (ActiveMetrics)
				{
					ActiveMetrics->VehiclesSpawned++;
				}

				if (VisualClass && !bForceLogicOnlyForTests)
				{
					FActorSpawnParameters Params;
					const int32 VisualMode = GetTrafficVisualModeForManager();
					Params.SpawnCollisionHandlingOverride =
						(VisualMode == 2) ? ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn : ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					Params.Owner = this;

					FTransform VisualXform = SpawnXform;
				if (VisualMode == 2)
				{
					// Disable logic collision before doing spawn queries so we don't trace/hit our own proxy vehicles.
					if (Vehicle)
					{
						Vehicle->SetActorEnableCollision(false);
					}

					FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AAA_Traffic_ChaosDriveSpawn), /*bTraceComplex=*/false);
					QueryParams.bReturnPhysicalMaterial = false;
					QueryParams.bFindInitialOverlaps = true;
					QueryParams.AddIgnoredActor(this);
					if (Vehicle)
					{
						QueryParams.AddIgnoredActor(Vehicle);
					}
					for (ATrafficVehicleBase* ExistingLogic : Vehicles)
					{
						if (ExistingLogic)
						{
							QueryParams.AddIgnoredActor(ExistingLogic);
						}
					}
					for (const TWeakObjectPtr<APawn>& ExistingVisual : VisualVehicles)
					{
						if (ExistingVisual.IsValid())
						{
							QueryParams.AddIgnoredActor(ExistingVisual.Get());
						}
					}

					const float BottomOffset = EstimatePawnBottomOffsetCm(VisualClass);
					const float ExtraZ = CVarTrafficChaosDriveSpawnZOffsetCm.GetValueOnGameThread();
					const FVector Desired = VisualXform.GetLocation();
					const FQuat FlatYaw = FQuat(FRotator(0.f, VisualXform.GetRotation().Rotator().Yaw, 0.f));

						FHitResult Hit;
						bool bUsedTraceZ = false;
						FVector Adjusted = ComputeChaosDriveSpawnLocation(*World, Desired, BottomOffset, ExtraZ, QueryParams, bUsedTraceZ, &Hit);
						const FVector BoxExtent = EstimatePawnBoxExtentCm(VisualClass) + FVector(25.f, 25.f, 10.f);
						const FVector Resolved = ResolveChaosDriveSpawnPenetrationByLifting(*World, Adjusted, FlatYaw, BoxExtent, QueryParams);

						if (Hit.bBlockingHit && bUsedTraceZ)
						{
							VisualXform.SetRotation(MakeSpawnRotationFromGroundHit(VisualXform, Hit));
						}

						if (CVarTrafficChaosDriveSpawnDebug.GetValueOnGameThread() != 0)
						{
							UE_LOG(LogTraffic, Log,
								TEXT("[VehicleManager] ChaosDriveSpawn(ZG) desired=(%.1f %.1f %.1f) traceHit=%d traceUsed=%d hit=%s comp=%s complexAsSimple=%d n=(%.2f %.2f %.2f) impactZ=%.1f spawnZ=%.1f resolvedZ=%.1f bottomOffset=%.1f clearance=%.1f extent=(%.1f %.1f %.1f)"),
								Desired.X, Desired.Y, Desired.Z,
								Hit.bBlockingHit ? 1 : 0,
								bUsedTraceZ ? 1 : 0,
								Hit.bBlockingHit && Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("<none>"),
								Hit.bBlockingHit && Hit.GetComponent() ? *Hit.GetComponent()->GetName() : TEXT("<none>"),
								Hit.bBlockingHit ? IsComplexAsSimpleCollision(Hit) : 0,
								Hit.bBlockingHit ? Hit.ImpactNormal.X : 0.f,
								Hit.bBlockingHit ? Hit.ImpactNormal.Y : 0.f,
								Hit.bBlockingHit ? Hit.ImpactNormal.Z : 0.f,
								Hit.bBlockingHit ? Hit.ImpactPoint.Z : 0.f,
								Adjusted.Z,
								Resolved.Z,
								BottomOffset,
								ExtraZ,
								BoxExtent.X, BoxExtent.Y, BoxExtent.Z);
						}

						VisualXform.SetLocation(Resolved);
						if (!Hit.bBlockingHit || !bUsedTraceZ)
						{
							VisualXform.SetRotation(FlatYaw);
						}
					}

				APawn* VisualPawn = nullptr;
				if (VisualMode == 2)
				{
					const float InitialLift = FMath::Max(0.f, CVarTrafficChaosDriveSpawnInitialLiftCm.GetValueOnGameThread());
					FTransform SpawnXformHigh = VisualXform;
					SpawnXformHigh.AddToTranslation(FVector(0.f, 0.f, InitialLift));

					VisualPawn = World->SpawnActorDeferred<APawn>(VisualClass, SpawnXformHigh, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
					if (VisualPawn)
					{
						DisableAutoPossess(*VisualPawn);
						VisualPawn->FinishSpawning(SpawnXformHigh, /*bIsDefaultTransform=*/true);

						TArray<UPrimitiveComponent*> DisabledSim;
						TemporarilyDisablePhysics(*VisualPawn, DisabledSim);

						FHitResult SweepHit;
						const float Step = FMath::Max(1.f, CVarTrafficChaosDriveSpawnResolveStepCm.GetValueOnGameThread());
						const float MaxLift = FMath::Max(0.f, CVarTrafficChaosDriveSpawnResolveMaxLiftCm.GetValueOnGameThread());
						bool bPlaced = false;
						FVector Candidate = VisualXform.GetLocation();
						for (float Lift = 0.f; Lift <= MaxLift; Lift += Step)
						{
							Candidate.Z = VisualXform.GetLocation().Z + Lift;
							// Use a real sweep (TeleportPhysics can bypass sweeps). Physics is currently disabled, so this is safe.
							if (VisualPawn->SetActorLocationAndRotation(Candidate, VisualXform.GetRotation(), /*bSweep=*/true, &SweepHit, ETeleportType::None))
							{
								bPlaced = true;
								break;
							}
						}
						if (!bPlaced)
						{
							VisualPawn->SetActorLocationAndRotation(VisualXform.GetLocation(), VisualXform.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
						}

						if (CVarTrafficChaosDriveSpawnDebug.GetValueOnGameThread() != 0)
						{
							UE_LOG(LogTraffic, Log,
								TEXT("[VehicleManager] ChaosDriveSpawnPlace(ZG) placed=%d final=(%.1f %.1f %.1f) sweepHit=%d hit=%s"),
								bPlaced ? 1 : 0,
								VisualPawn->GetActorLocation().X,
								VisualPawn->GetActorLocation().Y,
								VisualPawn->GetActorLocation().Z,
								SweepHit.bBlockingHit ? 1 : 0,
								SweepHit.bBlockingHit && SweepHit.GetActor() ? *SweepHit.GetActor()->GetName() : TEXT("<none>"));
						}

						// Keep movement disabled here; the TrafficVehicleAdapter will enable it after it applies initial braking
						// (and respects aaa.Traffic.Visual.ChaosDrive.DisableMovementSeconds).
						RestorePhysicsAndOptionallyMovement(*VisualPawn, DisabledSim, /*bRestoreMovement=*/false);
					}
				}
				else
				{
					VisualPawn = World->SpawnActor<APawn>(VisualClass, VisualXform, Params);
				}
				if (VisualPawn)
				{
					VisualPawn->Tags.AddUnique(TrafficSpawnedVehicleTag);

					// Spawn stabilization: clear any initial velocities caused by spawn adjustment/penetration resolution.
					if (VisualMode == 2)
					{
						ZeroPhysicsVelocities(*VisualPawn);
					}

					ATrafficVehicleAdapter* Adapter = World->SpawnActor<ATrafficVehicleAdapter>(Params);
					if (Adapter)
					{
							Adapter->Tags.AddUnique(TrafficSpawnedVehicleTag);
							Adapter->Initialize(Vehicle, VisualPawn);
							Adapters.Add(Adapter);
						}

						VisualVehicles.Add(VisualPawn);

						const int32 bShowDebugMesh = CVarShowLogicDebugMesh.GetValueOnGameThread();
						if (!bForceLogicOnlyForTests && bShowDebugMesh == 0 && Vehicle)
						{
							Vehicle->SetDebugBodyVisible(false);
						}
					}
				}
			}
		}
	}

	UE_LOG(LogTraffic, Log,
		TEXT("[VehicleManager] ZoneGraph: EligibleLanes=%d/%d SpawnedVehicles=%d RequiredTag=%s"),
		EligibleLaneCount,
		TotalLaneCount,
		Vehicles.Num(),
		*RequiredLaneTag.ToString());
}
