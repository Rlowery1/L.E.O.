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
#include "HAL/IConsoleManager.h"
#include "UObject/SoftObjectPath.h"
#include "Misc/AutomationTest.h"
#include "ZoneGraphSubsystem.h"

static TAutoConsoleVariable<int32> CVarShowLogicDebugMesh(
	TEXT("aaa.Traffic.ShowLogicDebugMesh"),
	0,
	TEXT("If non-zero, show the TrafficVehicleBase debug cube even when Chaos visuals are present.\n")
	TEXT("Default: 0 (hide cube when Chaos pawn is spawned)."),
	ECVF_Default);

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
	if (!bForceLogicOnlyForTests && Profile && Profile->VehicleClass.IsValid())
	{
		VisualClass = Profile->VehicleClass.LoadSynchronous();
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

	if (!bForceLogicOnlyForTests && !VisualClass && !GIsAutomationTesting)
	{
		UE_LOG(LogTraffic, Error,
			TEXT("[VehicleManager] No Chaos vehicle configured. Set a DefaultVehicleProfile with a valid VehicleClass in Project Settings -> AAA Traffic Vehicle Settings."));
		return;
	}

	const TArray<FTrafficLane>& Lanes = NetworkAsset->Network.Lanes;
	LastLaneSpawnTimes.Empty();
	TMap<int32, int32> SpawnedPerLane;

	FActorSpawnParameters VehicleSpawnParams;
	VehicleSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	VehicleSpawnParams.Owner = this;

	for (int32 LaneIndex = 0; LaneIndex < Lanes.Num(); ++LaneIndex)
	{
		const FTrafficLane& Lane = Lanes[LaneIndex];

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
			Vehicles.Add(Vehicle);
			SpawnedPerLane.FindOrAdd(LaneKey) += 1;
			if (ActiveMetrics)
			{
				ActiveMetrics->VehiclesSpawned++;
			}

			if (VisualClass)
			{
				FActorSpawnParameters Params;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				Params.Owner = this;
				APawn* VisualPawn = World->SpawnActor<APawn>(VisualClass, SpawnXform, Params);
				if (VisualPawn)
				{
					VisualPawn->SetActorEnableCollision(false);
					ATrafficVehicleAdapter* Adapter = World->SpawnActor<ATrafficVehicleAdapter>(Params);
					if (Adapter)
					{
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

				Vehicle->InitializeOnZoneGraphLane(ZGS, LaneHandle, Dist, SpeedCmPerSec);
				Vehicles.Add(Vehicle);

				if (ActiveMetrics)
				{
					ActiveMetrics->VehiclesSpawned++;
				}

				if (VisualClass && !bForceLogicOnlyForTests)
				{
					FActorSpawnParameters Params;
					Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					Params.Owner = this;

					APawn* VisualPawn = World->SpawnActor<APawn>(VisualClass, SpawnXform, Params);
					if (VisualPawn)
					{
						VisualPawn->SetActorEnableCollision(false);

						ATrafficVehicleAdapter* Adapter = World->SpawnActor<ATrafficVehicleAdapter>(Params);
						if (Adapter)
						{
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
