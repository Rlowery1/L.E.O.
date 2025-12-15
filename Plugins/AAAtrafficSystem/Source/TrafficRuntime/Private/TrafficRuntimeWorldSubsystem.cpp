#include "TrafficRuntimeWorldSubsystem.h"

#include "TrafficRuntimeSettings.h"
#include "TrafficSystemController.h"
#include "TrafficRuntimeModule.h"

#include "Engine/World.h"
#include "EngineUtils.h"

void UTrafficRuntimeWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	const EWorldType::Type WorldType = InWorld.WorldType;
	if (WorldType == EWorldType::Editor)
	{
		return;
	}

	const UTrafficRuntimeSettings* Settings = GetDefault<UTrafficRuntimeSettings>();
	if (!Settings || !Settings->bAutoSpawnTrafficOnBeginPlay)
	{
		return;
	}

	if (Settings->bAutoSpawnOnlyInPIE && WorldType != EWorldType::PIE)
	{
		return;
	}

	// Respect any user-placed controller.
	for (TActorIterator<ATrafficSystemController> It(&InWorld); It; ++It)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ATrafficSystemController* Controller = InWorld.SpawnActor<ATrafficSystemController>(ATrafficSystemController::StaticClass(), FTransform::Identity, Params);
	if (!Controller)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficRuntimeWorldSubsystem] Failed to spawn TrafficSystemController."));
		return;
	}

	Controller->SetRuntimeConfigFromProjectSettings();

	if (Settings->bAutoBuildNetwork)
	{
		Controller->Runtime_BuildTrafficNetwork();
	}

	if (Settings->bAutoSpawnVehicles)
	{
		Controller->Runtime_SpawnTraffic();
	}
}

