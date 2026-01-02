#include "TrafficRuntimeWorldSubsystem.h"

#include "CityBLDRoadGeometryProvider.h"
#include "Components/SplineComponent.h"
#include "TrafficRuntimeSettings.h"
#include "TrafficCityBLDAdapterSettings.h"
#include "TrafficRoadFamilySettings.h"
#include "TrafficRoadMetadataComponent.h"
#include "TrafficSystemController.h"
#include "TrafficVehicleManager.h"
#include "TrafficVisualMode.h"
#include "TrafficRuntimeModule.h"
#include "TrafficNetworkAsset.h"

#include "Components/DynamicMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "VectorTypes.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Algo/Reverse.h"
#include "TimerManager.h"
#include "CollisionQueryParams.h"
#include "UObject/UnrealType.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"

namespace
{
	static bool IsCityBLDRoadCandidate_Runtime(const AActor* Actor, const UTrafficCityBLDAdapterSettings* AdapterSettings);

	static TAutoConsoleVariable<int32> CVarTrafficOnScreenBuildStamp(
		TEXT("aaa.Traffic.Debug.OnScreenBuildStamp"),
		1,
		TEXT("If non-zero, prints the AAA Traffic runtime build stamp on-screen at PIE start.\n")
		TEXT("This provides an unmissable confirmation that the latest plugin binaries are loaded.\n")
		TEXT("Default: 1"),
		ECVF_Default);

	static TAutoConsoleVariable<int32> CVarTrafficCityBLDCalibrateLaneWidthFromMesh(
		TEXT("aaa.Traffic.CityBLD.CalibrateLaneWidthFromMesh"),
		1,
		TEXT("If non-zero, estimates lane width from CityBLD dynamic mesh geometry at runtime and overrides RoadFamily lane widths.\n")
		TEXT("Disable this when meshes include sidewalks/shoulders that inflate the computed width and cause traffic to drift out of lane.\n")
		TEXT("Default: 1"),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarTrafficStopLineCaptureAutoQuitSeconds(
		TEXT("aaa.Traffic.Runtime.StopLineCaptureAutoQuitSeconds"),
		15.f,
		TEXT("If > 0, automatically exits the process this many seconds after running aaa.Traffic.Runtime.StartStopLineCapture.\n")
		TEXT("This is intended for headless CLI captures (e.g. -nullrhi + -ExecCmds). Default: 15"),
		ECVF_Default);

	static bool IsCVarExplicitlySet(const IConsoleVariable* Var)
	{
		if (!Var)
		{
			return false;
		}

		const uint32 SetBy = (Var->GetFlags() & ECVF_SetByMask);
		return
			(SetBy == ECVF_SetByConsole) ||
			(SetBy == ECVF_SetByCommandline) ||
			(SetBy == ECVF_SetByCode);
	}

	static const TCHAR* GetCVarSetByString(const IConsoleVariable* Var)
	{
		if (!Var)
		{
			return TEXT("None");
		}

		switch (Var->GetFlags() & ECVF_SetByMask)
		{
		case ECVF_SetByConstructor: return TEXT("Constructor");
		case ECVF_SetByScalability: return TEXT("Scalability");
		case ECVF_SetByGameSetting: return TEXT("GameSetting");
		case ECVF_SetByProjectSetting: return TEXT("ProjectSetting");
		case ECVF_SetBySystemSettingsIni: return TEXT("SystemSettingsIni");
		case ECVF_SetByDeviceProfile: return TEXT("DeviceProfile");
		case ECVF_SetByConsole: return TEXT("Console");
		case ECVF_SetByCommandline: return TEXT("Commandline");
		case ECVF_SetByCode: return TEXT("Code");
		default: break;
		}

		return TEXT("Unknown");
	}

	static float ReadFloatCVar(const TCHAR* Name, float DefaultValue)
	{
		if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			return Var->GetFloat();
		}
		return DefaultValue;
	}

	static void LogIntersectionCVarSnapshot(const TCHAR* Context)
	{
		auto LogInt = [](const TCHAR* Name, int32 Fallback)
		{
			IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name);
			UE_LOG(LogTraffic, Log, TEXT("[TrafficRuntimeWorldSubsystem] CVar %s=%d (setBy=%s)"), Name, Var ? Var->GetInt() : Fallback, GetCVarSetByString(Var));
		};
		auto LogFloat = [](const TCHAR* Name, float Fallback)
		{
			IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name);
			UE_LOG(LogTraffic, Log, TEXT("[TrafficRuntimeWorldSubsystem] CVar %s=%.3f (setBy=%s)"), Name, Var ? Var->GetFloat() : Fallback, GetCVarSetByString(Var));
		};
		auto LogString = [](const TCHAR* Name, const TCHAR* Fallback)
		{
			IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name);
			const FString Value = Var ? Var->GetString() : FString(Fallback ? Fallback : TEXT(""));
			UE_LOG(LogTraffic, Log, TEXT("[TrafficRuntimeWorldSubsystem] CVar %s=\"%s\" (setBy=%s)"), Name, *Value, GetCVarSetByString(Var));
		};

		UE_LOG(LogTraffic, Log, TEXT("[TrafficRuntimeWorldSubsystem] Intersection CVar snapshot (%s):"), Context ? Context : TEXT("Unknown"));

		LogFloat(TEXT("aaa.Traffic.Intersections.StopLineOffsetCm"), 300.f);
		LogInt(TEXT("aaa.Traffic.Intersections.StopLineOffsetAuto"), 1);
		LogFloat(TEXT("aaa.Traffic.Intersections.ClusterRadiusCm"), 500.f);
		LogInt(TEXT("aaa.Traffic.Intersections.AnchorFromActors"), 1);
		LogString(TEXT("aaa.Traffic.Intersections.AnchorActorNameContains"), TEXT("MeshIntersection"));
		LogString(TEXT("aaa.Traffic.Intersections.AnchorActorTag"), TEXT(""));
		LogFloat(TEXT("aaa.Traffic.Intersections.AnchorRadiusLaneWidthScale"), 2.0f);
		LogFloat(TEXT("aaa.Traffic.Intersections.RadiusLaneWidthScale"), 1.25f);
		LogInt(TEXT("aaa.Traffic.Intersections.AnchorAllowRadialFallback"), 0);
		LogFloat(TEXT("aaa.Traffic.Intersections.AnchorMinAlignmentDot"), 0.1f);
		LogInt(TEXT("aaa.Traffic.Intersections.CrossSplitEnabled"), 1);
		LogInt(TEXT("aaa.Traffic.Intersections.StopLinePreferBoundary"), 0);

		// Cross-split tuning (lane-end placement).
		LogFloat(TEXT("aaa.Traffic.Intersections.CrossSplitMinDistanceFromLaneEndsCm"), 300.f);
		LogFloat(TEXT("aaa.Traffic.Intersections.CrossSplitDistanceLaneWidthScale"), 4.0f);
		LogFloat(TEXT("aaa.Traffic.Intersections.CrossSplitMaxParallelDot"), 0.98f);
		LogFloat(TEXT("aaa.Traffic.Intersections.CrossSplitToleranceCm"), 50.f);

		// Movement geometry and ChaosDrive tracking (useful when investigating post-turn drift).
		LogFloat(TEXT("aaa.Traffic.Movement.ControlDistFraction"), 0.5f);
		LogFloat(TEXT("aaa.Traffic.Visual.ChaosDrive.LookaheadCm"), 1200.f);
		LogFloat(TEXT("aaa.Traffic.Visual.ChaosDrive.SteerGain"), 1.0f);
		LogFloat(TEXT("aaa.Traffic.Visual.ChaosDrive.TurnSlowdownStartDeg"), 15.0f);
		LogFloat(TEXT("aaa.Traffic.Visual.ChaosDrive.TurnSlowdownFullDeg"), 60.0f);
		LogInt(TEXT("aaa.Traffic.Collision.IgnoreOtherTraffic"), 1);
		LogInt(TEXT("aaa.Traffic.CityBLD.CalibrateLaneWidthFromMesh"), 1);

		// Extra context for stop-line auto behavior.
		LogFloat(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoWidthScale"), 0.25f);
		LogFloat(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoMinCm"), 40.f);
		LogFloat(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoMaxCm"), 120.f);
		LogFloat(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoNominalLaneWidthCm"), 350.f);
		LogFloat(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoBufferCm"), 50.f);
	}

	static void DumpIntersectionCVarSnapshot_Console()
	{
		LogIntersectionCVarSnapshot(TEXT("Console"));
	}

	static FAutoConsoleCommand CCmdDumpIntersectionCVars(
		TEXT("aaa.Traffic.Debug.DumpIntersectionCVars"),
		TEXT("Logs key intersection-related console variables (stop line offset, clustering radius, anchoring, radius clamps)."),
		FConsoleCommandDelegate::CreateStatic(&DumpIntersectionCVarSnapshot_Console));

	static FAutoConsoleCommandWithWorld CCmdTrafficRuntimeBuildTrafficNetwork(
		TEXT("aaa.Traffic.Runtime.BuildTrafficNetwork"),
		TEXT("Calls Runtime_BuildTrafficNetwork() on all TrafficSystemController actors in the current world."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			int32 ControllerCount = 0;
			if (World)
			{
				for (TActorIterator<ATrafficSystemController> It(World); It; ++It)
				{
					if (ATrafficSystemController* Controller = *It)
					{
						++ControllerCount;
						Controller->Runtime_BuildTrafficNetwork();
					}
				}
			}
			UE_LOG(LogTraffic, Log, TEXT("[TrafficRuntimeWorldSubsystem] aaa.Traffic.Runtime.BuildTrafficNetwork executed (controllers=%d)."), ControllerCount);
		}));

	static FAutoConsoleCommandWithWorld CCmdTrafficRuntimeSpawnTraffic(
		TEXT("aaa.Traffic.Runtime.SpawnTraffic"),
		TEXT("Calls Runtime_SpawnTraffic() on all TrafficSystemController actors in the current world."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			int32 ControllerCount = 0;
			if (World)
			{
				for (TActorIterator<ATrafficSystemController> It(World); It; ++It)
				{
					if (ATrafficSystemController* Controller = *It)
					{
						++ControllerCount;
						Controller->Runtime_SpawnTraffic();
					}
				}
			}
			UE_LOG(LogTraffic, Log, TEXT("[TrafficRuntimeWorldSubsystem] aaa.Traffic.Runtime.SpawnTraffic executed (controllers=%d)."), ControllerCount);
		}));

	static FAutoConsoleCommandWithWorld CCmdTrafficRuntimeStartStopLineCapture(
		TEXT("aaa.Traffic.Runtime.StartStopLineCapture"),
		TEXT("Enables aaa.Traffic.Intersections.DebugStopLine=1, builds the traffic network, and spawns logic test vehicles (single-command helper for -ExecCmds)."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			if (GEngine && World)
			{
				GEngine->Exec(World, TEXT("aaa.Traffic.Intersections.DebugStopLine 1"));
			}

			int32 ControllerCount = 0;
			if (World)
			{
				for (TActorIterator<ATrafficSystemController> It(World); It; ++It)
				{
					if (ATrafficSystemController* Controller = *It)
					{
						++ControllerCount;
						Controller->Runtime_BuildTrafficNetwork();
					}
				}

				// Spawn logic-only test vehicles so `[TrafficVehicle] StopLine ...` logs are emitted from ATrafficVehicleBase::Tick,
				// regardless of ZoneGraph/Chaos vehicle setup.
				ATrafficVehicleManager* Manager = nullptr;
				for (TActorIterator<ATrafficVehicleManager> It(World); It; ++It)
				{
					Manager = *It;
					break;
				}
				if (!Manager)
				{
					FActorSpawnParameters Params;
					Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					Manager = World->SpawnActor<ATrafficVehicleManager>(ATrafficVehicleManager::StaticClass(), FTransform::Identity, Params);
				}

				if (Manager)
				{
					Manager->ClearVehicles();
					Manager->SetForceLogicOnlyForTests(true);
					Manager->SpawnTestVehicles(/*VehiclesPerLane*/ 2, /*SpeedCmPerSec*/ 800.f);
				}

				const float QuitSeconds = CVarTrafficStopLineCaptureAutoQuitSeconds.GetValueOnGameThread();
				if (QuitSeconds > 0.f)
				{
					FTimerHandle QuitTimer;
					World->GetTimerManager().SetTimer(
						QuitTimer,
						FTimerDelegate::CreateLambda([]
						{
							FPlatformMisc::RequestExit(false);
						}),
						QuitSeconds,
						false);
				}
			}

			UE_LOG(LogTraffic, Log, TEXT("[TrafficRuntimeWorldSubsystem] aaa.Traffic.Runtime.StartStopLineCapture executed (controllers=%d)."), ControllerCount);
		}));

	static FAutoConsoleCommandWithWorld CCmdTrafficRuntimeStartStopLineCaptureNoCrossSplitPreferBoundary(
		TEXT("aaa.Traffic.Runtime.StartStopLineCaptureNoCrossSplitPreferBoundary"),
		TEXT("Sets CrossSplitEnabled=0 and StopLinePreferBoundary=1, then runs aaa.Traffic.Runtime.StartStopLineCapture (debug helper)."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			if (GEngine && World)
			{
				GEngine->Exec(World, TEXT("aaa.Traffic.Intersections.CrossSplitEnabled 0"));
				GEngine->Exec(World, TEXT("aaa.Traffic.Intersections.StopLinePreferBoundary 1"));
				GEngine->Exec(World, TEXT("aaa.Traffic.Runtime.StartStopLineCapture"));
			}
		}));

	static FAutoConsoleCommandWithWorld CCmdTrafficRuntimeStartStopLineCaptureNoCrossSplitPreferBoundaryBigBoundary(
		TEXT("aaa.Traffic.Runtime.StartStopLineCaptureNoCrossSplitPreferBoundaryBigBoundary"),
		TEXT("Also relaxes stop-boundary clamp CVars (LaneWidthScale=10, Min=0, Max=2000) before running the capture helpers."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			if (GEngine && World)
			{
				GEngine->Exec(World, TEXT("aaa.Traffic.Intersections.StopLineBoundaryRadiusLaneWidthScale 10"));
				GEngine->Exec(World, TEXT("aaa.Traffic.Intersections.StopLineBoundaryRadiusMinCm 0"));
				GEngine->Exec(World, TEXT("aaa.Traffic.Intersections.StopLineBoundaryRadiusMaxCm 2000"));
				GEngine->Exec(World, TEXT("aaa.Traffic.Runtime.StartStopLineCaptureNoCrossSplitPreferBoundary"));
			}
		}));

	static FAutoConsoleCommandWithWorld CCmdTrafficRuntimeStartStopLineCaptureNoCrossSplitBigBoundaryDefaultPrefer(
		TEXT("aaa.Traffic.Runtime.StartStopLineCaptureNoCrossSplitBigBoundaryDefaultPrefer"),
		TEXT("Disables cross-splitting and relaxes stop-boundary clamp CVars (LaneWidthScale=10, Min=0, Max=2000), but keeps StopLinePreferBoundary=0 (default behavior)."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			if (GEngine && World)
			{
				GEngine->Exec(World, TEXT("aaa.Traffic.Intersections.StopLineBoundaryRadiusLaneWidthScale 10"));
				GEngine->Exec(World, TEXT("aaa.Traffic.Intersections.StopLineBoundaryRadiusMinCm 0"));
				GEngine->Exec(World, TEXT("aaa.Traffic.Intersections.StopLineBoundaryRadiusMaxCm 2000"));
				GEngine->Exec(World, TEXT("aaa.Traffic.Intersections.CrossSplitEnabled 0"));
				GEngine->Exec(World, TEXT("aaa.Traffic.Intersections.StopLinePreferBoundary 0"));
				GEngine->Exec(World, TEXT("aaa.Traffic.Runtime.StartStopLineCapture"));
			}
		}));

	static void ApplyIntersectionStopLineDefaultsForWorld(UWorld& World)
	{
		IConsoleVariable* StopLineVar = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.StopLineOffsetCm"));
		if (!StopLineVar)
		{
			return;
		}

		if (IConsoleVariable* AutoVar = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.StopLineOffsetAuto")))
		{
			const bool bAutoExplicit = IsCVarExplicitlySet(AutoVar);
			const int32 AutoValue = AutoVar->GetInt();
			const float AutoScale = ReadFloatCVar(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoWidthScale"), 0.25f);
			const float AutoMinCm = ReadFloatCVar(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoMinCm"), 40.f);
			const float AutoMaxCm = ReadFloatCVar(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoMaxCm"), 120.f);
			const float AutoNominalCm = ReadFloatCVar(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoNominalLaneWidthCm"), 350.f);
			const float AutoBufferCm = ReadFloatCVar(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoBufferCm"), 50.f);

			UE_LOG(LogTraffic, Log, TEXT("[TrafficRuntimeWorldSubsystem] StopLineAuto=%d (setBy=%s) scale=%.2f min=%.1f max=%.1f buffer=%.1f nominal=%.1f"),
				AutoValue,
				GetCVarSetByString(AutoVar),
				AutoScale,
				AutoMinCm,
				AutoMaxCm,
				AutoBufferCm,
				AutoNominalCm);

			if (AutoVar->GetInt() != 0)
			{
				return;
			}

			// If auto is disabled explicitly, we keep using StopLineOffsetCm.
			if (bAutoExplicit)
			{
				return;
			}
		}

		// Respect explicit user overrides (console/commandline/code). Project settings are intended as defaults.
		if (IsCVarExplicitlySet(StopLineVar))
		{
			return;
		}

		const UTrafficRuntimeSettings* RuntimeSettings = GetDefault<UTrafficRuntimeSettings>();
		const UTrafficCityBLDAdapterSettings* CitySettings = GetDefault<UTrafficCityBLDAdapterSettings>();

		float DesiredStopLineOffsetCm = StopLineVar->GetFloat();
		bool bShouldApply = false;

		if (RuntimeSettings && RuntimeSettings->bOverrideIntersectionStopLineOffset)
		{
			DesiredStopLineOffsetCm = FMath::Max(0.f, RuntimeSettings->IntersectionStopLineOffsetCm);
			bShouldApply = true;
		}
		else if (RuntimeSettings && RuntimeSettings->bEnableCityBLDAdapter && CitySettings && CitySettings->bApplyRecommendedIntersectionStopLineOffset)
		{
			const float CityValue = FMath::Max(0.f, CitySettings->RecommendedIntersectionStopLineOffsetCm);

			// Only apply the CityBLD-tuned default when this world actually contains CityBLD-style roads.
			bool bHasCityBLDRoads = false;
			for (TActorIterator<AActor> It(&World); It; ++It)
			{
				if (IsCityBLDRoadCandidate_Runtime(*It, CitySettings))
				{
					bHasCityBLDRoads = true;
					break;
				}
			}

			if (bHasCityBLDRoads)
			{
				DesiredStopLineOffsetCm = CityValue;
				bShouldApply = true;
			}
		}

		if (bShouldApply)
		{
			StopLineVar->Set(DesiredStopLineOffsetCm, ECVF_SetByProjectSetting);
			UE_LOG(LogTraffic, Log, TEXT("[TrafficRuntimeWorldSubsystem] Stop line offset default: %.1fcm (source=%s)"),
				DesiredStopLineOffsetCm,
				(RuntimeSettings && RuntimeSettings->bOverrideIntersectionStopLineOffset) ? TEXT("ProjectOverride") : TEXT("CityBLDRecommended"));
		}
	}

	static void ApplyIntersectionControlDefaultsForWorld(UWorld& World)
	{
		const UTrafficRuntimeSettings* RuntimeSettings = GetDefault<UTrafficRuntimeSettings>();
		if (!RuntimeSettings)
		{
			return;
		}

		IConsoleVariable* ControlModeVar = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.ControlMode"));
		IConsoleVariable* RequireFullStopVar = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.RequireFullStop"));
		IConsoleVariable* GreenVar = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.GreenSeconds"));
		IConsoleVariable* YellowVar = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.YellowSeconds"));
		IConsoleVariable* AllRedVar = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.AllRedSeconds"));
		IConsoleVariable* CoordEnabledVar = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.CoordinationEnabled"));
		IConsoleVariable* CoordSpeedVar = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.CoordinationSpeedCmPerSec"));
		IConsoleVariable* CoordAxisVar = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.CoordinationAxisMode"));
		IConsoleVariable* PermittedLeftVar = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.PermittedLeftYield"));
		IConsoleVariable* PermittedLeftDistVar = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.TrafficLight.PermittedLeftApproachDistanceCm"));

		if (ControlModeVar && !IsCVarExplicitlySet(ControlModeVar))
		{
			ControlModeVar->Set(static_cast<int32>(RuntimeSettings->IntersectionControlMode), ECVF_SetByProjectSetting);
		}

		// 4-way-stop should default to a full stop at the stop line (unless the user explicitly overrides the CVar).
		if (RequireFullStopVar && !IsCVarExplicitlySet(RequireFullStopVar))
		{
			const int32 Mode = static_cast<int32>(RuntimeSettings->IntersectionControlMode);
			const int32 RequireStop = (Mode == static_cast<int32>(ETrafficIntersectionControlMode::FourWayStop)) ? 1 : 0;
			RequireFullStopVar->Set(RequireStop, ECVF_SetByProjectSetting);
		}
		if (GreenVar && !IsCVarExplicitlySet(GreenVar))
		{
			GreenVar->Set(FMath::Max(0.1f, RuntimeSettings->TrafficLightGreenSeconds), ECVF_SetByProjectSetting);
		}
		if (YellowVar && !IsCVarExplicitlySet(YellowVar))
		{
			YellowVar->Set(FMath::Max(0.f, RuntimeSettings->TrafficLightYellowSeconds), ECVF_SetByProjectSetting);
		}
		if (AllRedVar && !IsCVarExplicitlySet(AllRedVar))
		{
			AllRedVar->Set(FMath::Max(0.f, RuntimeSettings->TrafficLightAllRedSeconds), ECVF_SetByProjectSetting);
		}
		if (CoordEnabledVar && !IsCVarExplicitlySet(CoordEnabledVar))
		{
			CoordEnabledVar->Set(RuntimeSettings->bTrafficLightCoordinationEnabled ? 1 : 0, ECVF_SetByProjectSetting);
		}
		if (CoordSpeedVar && !IsCVarExplicitlySet(CoordSpeedVar))
		{
			CoordSpeedVar->Set(FMath::Max(1.f, RuntimeSettings->TrafficLightCoordinationSpeedCmPerSec), ECVF_SetByProjectSetting);
		}
		if (CoordAxisVar && !IsCVarExplicitlySet(CoordAxisVar))
		{
			CoordAxisVar->Set(static_cast<int32>(RuntimeSettings->TrafficLightCoordinationAxis), ECVF_SetByProjectSetting);
		}
		if (PermittedLeftVar && !IsCVarExplicitlySet(PermittedLeftVar))
		{
			PermittedLeftVar->Set(RuntimeSettings->bTrafficLightPermittedLeftYield ? 1 : 0, ECVF_SetByProjectSetting);
		}
		if (PermittedLeftDistVar && !IsCVarExplicitlySet(PermittedLeftDistVar))
		{
			PermittedLeftDistVar->Set(FMath::Max(0.f, RuntimeSettings->TrafficLightPermittedLeftApproachDistanceCm), ECVF_SetByProjectSetting);
		}
	}

	static void ApplyRoutingDefaultsForWorld(UWorld& World)
	{
		const UTrafficRuntimeSettings* RuntimeSettings = GetDefault<UTrafficRuntimeSettings>();
		if (!RuntimeSettings)
		{
			return;
		}

		IConsoleVariable* TurnPolicyVar = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Routing.TurnPolicy"));
		if (TurnPolicyVar && !IsCVarExplicitlySet(TurnPolicyVar))
		{
			TurnPolicyVar->Set(static_cast<int32>(RuntimeSettings->TurnPolicy), ECVF_SetByProjectSetting);
		}
	}

	static bool IsCityBLDRoadCandidate_Runtime(const AActor* Actor, const UTrafficCityBLDAdapterSettings* AdapterSettings)
	{
		if (!Actor)
		{
			return false;
		}

		if (AdapterSettings && !AdapterSettings->RoadActorTag.IsNone() && Actor->ActorHasTag(AdapterSettings->RoadActorTag))
		{
			return true;
		}

		const FString ClassName = Actor->GetClass()->GetName();
		if (ClassName.Contains(TEXT("BP_MeshRoad"), ESearchCase::IgnoreCase) ||
			ClassName.Contains(TEXT("BP_ModularRoad"), ESearchCase::IgnoreCase))
		{
			return true;
		}

		if (AdapterSettings)
		{
			for (const FString& Contains : AdapterSettings->RoadClassNameContains)
			{
				if (!Contains.IsEmpty() && ClassName.Contains(Contains, ESearchCase::IgnoreCase))
				{
					return true;
				}
			}
		}

		return false;
	}

	static bool TryReadStringLikePropertyValue_Runtime(const FProperty* Prop, const void* ValuePtr, FString& OutValue)
	{
		if (!Prop || !ValuePtr)
		{
			return false;
		}

		if (const FStrProperty* StrProp = CastField<const FStrProperty>(Prop))
		{
			OutValue = StrProp->GetPropertyValue(ValuePtr);
			return true;
		}

		if (const FNameProperty* NameProp = CastField<const FNameProperty>(Prop))
		{
			OutValue = NameProp->GetPropertyValue(ValuePtr).ToString();
			return true;
		}

		if (const FTextProperty* TextProp = CastField<const FTextProperty>(Prop))
		{
			OutValue = TextProp->GetPropertyValue(ValuePtr).ToString();
			return true;
		}

		return false;
	}

	static FString FindBestStyleOrPresetNameOnObject_Runtime(UObject* Obj)
	{
		if (!Obj)
		{
			return FString();
		}

		static const TArray<FName> PreferredPropertyNames = {
			FName(TEXT("StyleName")),
			FName(TEXT("StreetStyleName")),
			FName(TEXT("RoadStyleName")),
			FName(TEXT("CityBLDPresetName")),
		};

		FString Best;

		for (TFieldIterator<FProperty> It(Obj->GetClass()); It; ++It)
		{
			const FProperty* Prop = *It;
			if (!Prop)
			{
				continue;
			}

			const FName PropName = Prop->GetFName();
			const FString PropNameStr = PropName.ToString();
			const FString DisplayName = Prop->GetMetaData(TEXT("DisplayName"));

			const bool bLooksLikeStyle =
				PreferredPropertyNames.Contains(PropName) ||
				PropNameStr.Contains(TEXT("Style"), ESearchCase::IgnoreCase) ||
				PropNameStr.Contains(TEXT("Preset"), ESearchCase::IgnoreCase) ||
				DisplayName.Contains(TEXT("Style"), ESearchCase::IgnoreCase) ||
				DisplayName.Contains(TEXT("Preset"), ESearchCase::IgnoreCase);

			if (!bLooksLikeStyle)
			{
				continue;
			}

			const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Obj);
			FString Value;

			if (TryReadStringLikePropertyValue_Runtime(Prop, ValuePtr, Value))
			{
				Value.TrimStartAndEndInline();
				if (Value.Len() > Best.Len())
				{
					Best = Value;
				}
				continue;
			}

			// One hop into object references (CityBLD commonly stores style data in nested objects/data assets).
			if (const FObjectPropertyBase* ObjProp = CastField<const FObjectPropertyBase>(Prop))
			{
				UObject* Ref = ObjProp->GetObjectPropertyValue(ValuePtr);
				if (!Ref)
				{
					continue;
				}

				const FString Nested = FindBestStyleOrPresetNameOnObject_Runtime(Ref);
				if (Nested.Len() > Best.Len())
				{
					Best = Nested;
				}
				continue;
			}

			// Struct leaf scan for common "StyleName" fields.
			if (const FStructProperty* StructProp = CastField<const FStructProperty>(Prop))
			{
				const void* StructPtr = StructProp->ContainerPtrToValuePtr<void>(Obj);
				if (!StructPtr)
				{
					continue;
				}

				for (TFieldIterator<FProperty> InnerIt(StructProp->Struct); InnerIt; ++InnerIt)
				{
					const FProperty* Inner = *InnerIt;
					if (!Inner)
					{
						continue;
					}

					const FString InnerNameStr = Inner->GetName();
					const FString InnerDisplayName = Inner->GetMetaData(TEXT("DisplayName"));
					const bool bLooksLikeStyleLeaf =
						InnerNameStr.Equals(TEXT("StyleName"), ESearchCase::IgnoreCase) ||
						InnerNameStr.Equals(TEXT("PresetName"), ESearchCase::IgnoreCase) ||
						InnerDisplayName.Equals(TEXT("Style Name"), ESearchCase::IgnoreCase) ||
						InnerDisplayName.Equals(TEXT("Preset Name"), ESearchCase::IgnoreCase);

					if (!bLooksLikeStyleLeaf)
					{
						continue;
					}

					const void* InnerPtr = Inner->ContainerPtrToValuePtr<void>(StructPtr);
					FString InnerValue;
					if (TryReadStringLikePropertyValue_Runtime(Inner, InnerPtr, InnerValue))
					{
						InnerValue.TrimStartAndEndInline();
						if (InnerValue.Len() > Best.Len())
						{
							Best = InnerValue;
						}
					}
				}
			}
		}

		return Best;
	}

	static FString TryGetCityBLDVariantKeyFromActor_Runtime(const AActor* RoadActor)
	{
		if (!RoadActor)
		{
			return FString();
		}

		FString Best;
		TArray<UActorComponent*> Comps;
		RoadActor->GetComponents(Comps);
		for (UActorComponent* C : Comps)
		{
			if (!C)
			{
				continue;
			}

			const FString Candidate = FindBestStyleOrPresetNameOnObject_Runtime(C);
			if (Candidate.Len() > Best.Len())
			{
				Best = Candidate;
			}
		}

		// Also scan the actor itself.
		{
			const FString Candidate = FindBestStyleOrPresetNameOnObject_Runtime(const_cast<AActor*>(RoadActor));
			if (Candidate.Len() > Best.Len())
			{
				Best = Candidate;
			}
		}

		Best.TrimStartAndEndInline();
		return Best;
	}

	static void EnsureFamilyDefinitionExists(UTrafficRoadFamilySettings* RoadSettings, const FName FamilyName, const FString& FamilyNameStr)
	{
		if (!RoadSettings || FamilyName.IsNone())
		{
			return;
		}

		if (RoadSettings->FindFamilyByName(FamilyName))
		{
			return;
		}

		FRoadFamilyDefinition NewDef;
		NewDef.FamilyName = FamilyName;
		NewDef.Forward.NumLanes = 1;
		NewDef.Backward.NumLanes = 1;

		// Best-effort heuristic seeding when the project settings haven't been saved yet.
		if (FamilyNameStr.Contains(TEXT("Four Lane"), ESearchCase::IgnoreCase))
		{
			NewDef.Forward.NumLanes = 2;
			NewDef.Backward.NumLanes = 2;
		}
		else if (FamilyNameStr.Contains(TEXT("Two Lane"), ESearchCase::IgnoreCase))
		{
			NewDef.Forward.NumLanes = 1;
			NewDef.Backward.NumLanes = 1;
		}
		else if (FamilyNameStr.Contains(TEXT("3 Lane"), ESearchCase::IgnoreCase) &&
				 (FamilyNameStr.Contains(TEXT("One-Way"), ESearchCase::IgnoreCase) ||
				  FamilyNameStr.Contains(TEXT("One Way"), ESearchCase::IgnoreCase) ||
				  FamilyNameStr.Contains(TEXT("1 Way"), ESearchCase::IgnoreCase)))
		{
			NewDef.Forward.NumLanes = 3;
			NewDef.Backward.NumLanes = 0;
		}
		else if (FamilyNameStr.Contains(TEXT("Ramp"), ESearchCase::IgnoreCase))
		{
			NewDef.Forward.NumLanes = 1;
			NewDef.Backward.NumLanes = 0;
		}

		NewDef.Forward.LaneWidthCm = 350.f;
		NewDef.Backward.LaneWidthCm = 350.f;

		// Basic offset defaults: center the first lane at half a lane width.
		NewDef.Forward.InnerLaneCenterOffsetCm = NewDef.Forward.LaneWidthCm * 0.5f;
		NewDef.Backward.InnerLaneCenterOffsetCm = NewDef.Backward.LaneWidthCm * 0.5f;

		// Default ZoneGraph lane profile paths (plugin mount point). Optional; can be overridden by calibration.
		NewDef.VehicleLaneProfile = FSoftObjectPath(TEXT("/AAAtrafficSystem/ZoneProfiles/CityBLDUrbanTwoLane.CityBLDUrbanTwoLane"));
		NewDef.FootpathLaneProfile = FSoftObjectPath(TEXT("/AAAtrafficSystem/ZoneProfiles/CityBLDFootpath.CityBLDFootpath"));

		RoadSettings->Families.Add(NewDef);
	}

	static bool CollectCityBLDDynamicMeshVertices_Runtime(const AActor* Actor, TArray<FVector>& OutVertices)
	{
		OutVertices.Reset();
		if (!Actor)
		{
			return false;
		}

		TArray<UDynamicMeshComponent*> DynMeshComps;
		Actor->GetComponents<UDynamicMeshComponent>(DynMeshComps);
		if (DynMeshComps.Num() == 0)
		{
			return false;
		}

		for (UDynamicMeshComponent* Comp : DynMeshComps)
		{
			if (!Comp || !Comp->GetDynamicMesh())
			{
				continue;
			}

			const FTransform Xform = Comp->GetComponentTransform();
			const UDynamicMesh* DynMesh = Comp->GetDynamicMesh();
			const UE::Geometry::FDynamicMesh3* Mesh = DynMesh ? DynMesh->GetMeshPtr() : nullptr;
			if (!Mesh)
			{
				continue;
			}

			for (int32 Vid : Mesh->VertexIndicesItr())
			{
				const FVector3d PosD = Mesh->GetVertex(Vid);
				OutVertices.Add(Xform.TransformPosition(static_cast<FVector>(PosD)));
			}
		}

		return OutVertices.Num() >= 16;
	}

	static bool EstimateLaneWidthFromCityBLDMesh_Runtime(
		const TArray<FVector>& CenterlinePoints,
		const TArray<FVector>& Vertices,
		int32 TotalLanes,
		float& OutLaneWidthCm)
	{
		OutLaneWidthCm = 0.f;

		if (CenterlinePoints.Num() < 2 || Vertices.Num() < 16 || TotalLanes <= 0)
		{
			return false;
		}

		float TotalLength = 0.f;
		for (int32 i = 1; i < CenterlinePoints.Num(); ++i)
		{
			TotalLength += FVector::Dist(CenterlinePoints[i - 1], CenterlinePoints[i]);
		}

		const int32 SampleCount = FMath::Clamp(static_cast<int32>(TotalLength / 300.f), 8, 40);
		const float StepLength = (SampleCount > 1) ? (TotalLength / static_cast<float>(SampleCount - 1)) : 300.f;
		const float SliceHalfLength = FMath::Clamp(StepLength * 1.5f, 100.f, 600.f);
		const FVector Up(0.f, 0.f, 1.f);

		TArray<float> WidthSamples;
		WidthSamples.Reserve(SampleCount);

		const float MinRoadWidth = 150.f * static_cast<float>(TotalLanes);
		const float MaxRoadWidth = 800.f * static_cast<float>(TotalLanes);

		for (int32 SampleIdx = 0; SampleIdx < SampleCount; ++SampleIdx)
		{
			const float Alpha = (SampleCount > 1) ? (static_cast<float>(SampleIdx) / static_cast<float>(SampleCount - 1)) : 0.f;
			const int32 IndexF = FMath::Clamp(static_cast<int32>(Alpha * static_cast<float>(CenterlinePoints.Num() - 1)), 0, CenterlinePoints.Num() - 1);
			const int32 PrevIdx = FMath::Max(0, IndexF - 1);
			const int32 NextIdx = FMath::Min(CenterlinePoints.Num() - 1, IndexF + 1);

			const FVector Pos = CenterlinePoints[IndexF];
			FVector Tangent = CenterlinePoints[NextIdx] - CenterlinePoints[PrevIdx];
			Tangent.Z = 0.f;
			if (!Tangent.Normalize())
			{
				continue;
			}

			FVector Right = FVector::CrossProduct(Tangent, Up);
			if (!Right.Normalize())
			{
				continue;
			}

			float MinDist = FLT_MAX;
			float MaxDist = -FLT_MAX;
			int32 InSlice = 0;

			for (const FVector& V : Vertices)
			{
				const FVector Delta = V - Pos;
				const float Along = FVector::DotProduct(Delta, Tangent);
				if (FMath::Abs(Along) > SliceHalfLength)
				{
					continue;
				}

				const float D = FVector::DotProduct(Delta, Right);
				MinDist = FMath::Min(MinDist, D);
				MaxDist = FMath::Max(MaxDist, D);
				++InSlice;
			}

			if (InSlice < 16 || !(MinDist < MaxDist))
			{
				continue;
			}

			const float Width = MaxDist - MinDist;
			if (Width < MinRoadWidth || Width > MaxRoadWidth)
			{
				continue;
			}

			WidthSamples.Add(Width);
		}

		if (WidthSamples.Num() < 3)
		{
			return false;
		}

		WidthSamples.Sort();
		const int32 KeepCount = FMath::Max(1, FMath::FloorToInt(static_cast<float>(WidthSamples.Num()) * 0.6f));
		float SumWidth = 0.f;
		for (int32 i = 0; i < KeepCount; ++i)
		{
			SumWidth += WidthSamples[i];
		}

		const float RoadWidth = SumWidth / static_cast<float>(KeepCount);
		OutLaneWidthCm = RoadWidth / static_cast<float>(TotalLanes);
		return OutLaneWidthCm > 1.f;
	}

	static void Runtime_PrepareCityBLDRoadMetadata(UWorld& World)
	{
		const UTrafficRuntimeSettings* RuntimeSettings = GetDefault<UTrafficRuntimeSettings>();
		if (!RuntimeSettings || !RuntimeSettings->bEnableCityBLDAdapter)
		{
			return;
		}

		const UTrafficCityBLDAdapterSettings* AdapterSettings = GetDefault<UTrafficCityBLDAdapterSettings>();
		UTrafficRoadFamilySettings* RoadSettings = GetMutableDefault<UTrafficRoadFamilySettings>();

		int32 CityBLDActorsProcessed = 0;
		int32 MetadataCreated = 0;
		int32 FamilyAssignedFromVariant = 0;
		int32 MissingFamiliesAdded = 0;
		int32 RampRoleRemapped = 0;
		int32 CalibratedActors = 0;
		int32 CalibratedFamilies = 0;

		// Collect highway polylines for ramp role classification (best-effort; only for CityBLD modular highway setups).
		UCityBLDRoadGeometryProvider* Provider = NewObject<UCityBLDRoadGeometryProvider>();
		TArray<AActor*> HighwayActors;
		TArray<AActor*> RampActors;
		TArray<AActor*> RoadActorsForCalibration;
		TMap<FName, TArray<float>> FamilyLaneWidthSamples;

		for (TActorIterator<AActor> It(&World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor)
			{
				continue;
			}

			if (!IsCityBLDRoadCandidate_Runtime(Actor, AdapterSettings))
			{
				continue;
			}

			if (!Actor->FindComponentByClass<USplineComponent>())
			{
				continue;
			}

			++CityBLDActorsProcessed;
			RoadActorsForCalibration.Add(Actor);

			const FString VariantKey = TryGetCityBLDVariantKeyFromActor_Runtime(Actor);
			const bool bLooksLikeRamp = VariantKey.Contains(TEXT("Ramp"), ESearchCase::IgnoreCase);
			const bool bLooksLikeHighway = !bLooksLikeRamp &&
				(VariantKey.Contains(TEXT("Freeway"), ESearchCase::IgnoreCase) ||
				 VariantKey.Contains(TEXT("Highway"), ESearchCase::IgnoreCase));

			if (bLooksLikeRamp)
			{
				RampActors.Add(Actor);
			}
			else if (bLooksLikeHighway)
			{
				HighwayActors.Add(Actor);
			}

			UTrafficRoadMetadataComponent* Meta = Actor->FindComponentByClass<UTrafficRoadMetadataComponent>();
			if (!Meta)
			{
				Meta = NewObject<UTrafficRoadMetadataComponent>(Actor);
				Meta->RegisterComponent();
				Actor->AddInstanceComponent(Meta);
				++MetadataCreated;
			}
			else if (!Meta->IsRegistered())
			{
				Meta->RegisterComponent();
			}

			// If this CityBLD actor ended up with a generic default family, prefer its CityBLD preset/style name instead.
			if (Meta->FamilyName.IsNone() || Meta->FamilyName == FName(TEXT("Urban_2x2")))
			{
				if (!VariantKey.IsEmpty())
				{
					Meta->FamilyName = FName(*VariantKey);
					++FamilyAssignedFromVariant;
				}
			}

			Meta->bIncludeInTraffic = true;

			const FString FamilyNameStr = Meta->FamilyName.ToString();
			const bool bHadFamily = (RoadSettings && RoadSettings->FindFamilyByName(Meta->FamilyName) != nullptr);
			EnsureFamilyDefinitionExists(RoadSettings, Meta->FamilyName, FamilyNameStr);
			if (!bHadFamily && RoadSettings && RoadSettings->FindFamilyByName(Meta->FamilyName))
			{
				++MissingFamiliesAdded;
			}
		}

		// Best-effort ramp role split (On/Off) based on current reverse direction and which endpoint is nearest a highway.
		if (RampActors.Num() > 0 && HighwayActors.Num() > 0)
		{
			TArray<TArray<FVector>> HighwayPolylines;
			for (AActor* Highway : HighwayActors)
			{
				TArray<FVector> Pts;
				if (Provider && Provider->GetDisplayCenterlineForActor(Highway, Pts) && Pts.Num() >= 2)
				{
					HighwayPolylines.Add(MoveTemp(Pts));
				}
			}

			auto ClosestPointDistanceSqToPolyline = [](const TArray<FVector>& Poly, const FVector& Pt) -> float
			{
				if (Poly.Num() < 2)
				{
					return TNumericLimits<float>::Max();
				}

				float BestSq = TNumericLimits<float>::Max();
				for (int32 i = 0; i + 1 < Poly.Num(); ++i)
				{
					const FVector A = Poly[i];
					const FVector B = Poly[i + 1];
					const FVector Closest = FMath::ClosestPointOnSegment(Pt, A, B);
					BestSq = FMath::Min(BestSq, FVector::DistSquared(Pt, Closest));
				}
				return BestSq;
			};

			for (AActor* Ramp : RampActors)
			{
				if (!Ramp)
				{
					continue;
				}

				UTrafficRoadMetadataComponent* Meta = Ramp->FindComponentByClass<UTrafficRoadMetadataComponent>();
				if (!Meta || !Meta->bIncludeInTraffic || Meta->FamilyName.IsNone())
				{
					continue;
				}

				TArray<FVector> RampCenterline;
				if (!Provider || !Provider->GetDisplayCenterlineForActor(Ramp, RampCenterline) || RampCenterline.Num() < 2)
				{
					continue;
				}

				// Provider returns driving-direction centerline (may already be reversed). We want raw endpoints for distance checks.
				TArray<FVector> RampRaw = RampCenterline;
				if (Meta->bReverseCenterlineDirection)
				{
					Algo::Reverse(RampRaw);
				}

				const FVector StartRaw = RampRaw[0];
				const FVector EndRaw = RampRaw.Last();

				float BestStartSq = TNumericLimits<float>::Max();
				float BestEndSq = TNumericLimits<float>::Max();
				for (const TArray<FVector>& Hwy : HighwayPolylines)
				{
					BestStartSq = FMath::Min(BestStartSq, ClosestPointDistanceSqToPolyline(Hwy, StartRaw));
					BestEndSq = FMath::Min(BestEndSq, ClosestPointDistanceSqToPolyline(Hwy, EndRaw));
				}

				const bool bHighwayAtStart = (BestStartSq <= BestEndSq);

				// Physical highway endpoint.
				const FVector HighwayEndpoint = bHighwayAtStart ? StartRaw : EndRaw;

				// Physical driving endpoints (based on reverse flag).
				const FVector DrivingStart = Meta->bReverseCenterlineDirection ? EndRaw : StartRaw;
				const FVector DrivingEnd = Meta->bReverseCenterlineDirection ? StartRaw : EndRaw;

				enum class ERampRole : uint8 { Unknown, On, Off };
				ERampRole Role = ERampRole::Unknown;
				if (DrivingEnd.Equals(HighwayEndpoint, 1.f))
				{
					Role = ERampRole::On;
				}
				else if (DrivingStart.Equals(HighwayEndpoint, 1.f))
				{
					Role = ERampRole::Off;
				}

				if (Role == ERampRole::Unknown)
				{
					continue;
				}

				FString BaseFamilyName = Meta->FamilyName.ToString();
				BaseFamilyName.TrimStartAndEndInline();
				BaseFamilyName = BaseFamilyName.LeftChop(BaseFamilyName.EndsWith(TEXT(" (On)")) ? 5 : 0);
				BaseFamilyName = BaseFamilyName.LeftChop(BaseFamilyName.EndsWith(TEXT(" (Off)")) ? 6 : 0);
				BaseFamilyName.TrimEndInline();

				const FString RoleSuffix = (Role == ERampRole::On) ? TEXT(" (On)") : TEXT(" (Off)");
				const FString VariantKey = BaseFamilyName + RoleSuffix;
				Meta->FamilyName = FName(*VariantKey);

				++RampRoleRemapped;
				EnsureFamilyDefinitionExists(RoadSettings, Meta->FamilyName, VariantKey);
			}
		}

		// Calibrate CityBLD lane widths from dynamic mesh geometry (best-effort, runtime only).
		if (CVarTrafficCityBLDCalibrateLaneWidthFromMesh.GetValueOnGameThread() != 0 && RoadActorsForCalibration.Num() > 0 && RoadSettings && Provider)
		{
			for (AActor* RoadActor : RoadActorsForCalibration)
			{
				if (!RoadActor)
				{
					continue;
				}

				UTrafficRoadMetadataComponent* Meta = RoadActor->FindComponentByClass<UTrafficRoadMetadataComponent>();
				if (!Meta || !Meta->bIncludeInTraffic || Meta->FamilyName.IsNone())
				{
					continue;
				}

				FRoadFamilyDefinition* FamilyDef = RoadSettings->FindFamilyByNameMutable(Meta->FamilyName);
				if (!FamilyDef)
				{
					continue;
				}

				const int32 TotalLanes = FMath::Max(1, FamilyDef->Forward.NumLanes + FamilyDef->Backward.NumLanes);

				TArray<FVector> Centerline;
				if (!Provider->GetDisplayCenterlineForActor(RoadActor, Centerline) || Centerline.Num() < 2)
				{
					continue;
				}

				TArray<FVector> MeshVertices;
				if (!CollectCityBLDDynamicMeshVertices_Runtime(RoadActor, MeshVertices))
				{
					continue;
				}

				float LaneWidthCm = 0.f;
				if (!EstimateLaneWidthFromCityBLDMesh_Runtime(Centerline, MeshVertices, TotalLanes, LaneWidthCm))
				{
					continue;
				}

				FamilyLaneWidthSamples.FindOrAdd(Meta->FamilyName).Add(LaneWidthCm);
				++CalibratedActors;
			}

			for (auto& Pair : FamilyLaneWidthSamples)
			{
				TArray<float>& Samples = Pair.Value;
				if (Samples.Num() == 0)
				{
					continue;
				}

				Samples.Sort();
				const int32 KeepCount = FMath::Max(1, FMath::FloorToInt(static_cast<float>(Samples.Num()) * 0.6f));
				float Sum = 0.f;
				for (int32 i = 0; i < KeepCount; ++i)
				{
					Sum += Samples[i];
				}

				const float Avg = Sum / static_cast<float>(KeepCount);
				const float LaneWidthCm = FMath::Clamp(Avg, 200.f, 500.f);

				FRoadFamilyDefinition* FamilyDef = RoadSettings->FindFamilyByNameMutable(Pair.Key);
				if (!FamilyDef)
				{
					continue;
				}

				if (FamilyDef->Forward.NumLanes > 0)
				{
					FamilyDef->Forward.LaneWidthCm = LaneWidthCm;
					FamilyDef->Forward.InnerLaneCenterOffsetCm = LaneWidthCm * 0.5f;
				}
				if (FamilyDef->Backward.NumLanes > 0)
				{
					FamilyDef->Backward.LaneWidthCm = LaneWidthCm;
					FamilyDef->Backward.InnerLaneCenterOffsetCm = LaneWidthCm * 0.5f;
				}

				++CalibratedFamilies;
				UE_LOG(LogTraffic, Log,
					TEXT("[TrafficRuntimeWorldSubsystem] CityBLD lane width calibrated: Family=%s Lanes=%d Width=%.1fcm Samples=%d"),
					*Pair.Key.ToString(),
					FamilyDef->Forward.NumLanes + FamilyDef->Backward.NumLanes,
					LaneWidthCm,
					Samples.Num());
			}
		}

		if (CityBLDActorsProcessed > 0)
		{
			UE_LOG(LogTraffic, Log,
				TEXT("[TrafficRuntimeWorldSubsystem] CityBLD runtime prepare: Actors=%d MetaCreated=%d FamilyFromVariant=%d FamiliesAdded=%d RampRoleRemapped=%d CalibratedActors=%d CalibratedFamilies=%d"),
				CityBLDActorsProcessed,
				MetadataCreated,
				FamilyAssignedFromVariant,
				MissingFamiliesAdded,
				RampRoleRemapped,
				CalibratedActors,
				CalibratedFamilies);
		}
	}
}

void UTrafficRuntimeWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	const EWorldType::Type WorldType = InWorld.WorldType;
	if (WorldType == EWorldType::Editor)
	{
		return;
	}

	// Apply kit-tuned traffic rule defaults (unless the user explicitly overrode the CVars).
	// This is intentionally independent of auto-spawn so that Project Settings act as defaults
	// for any runtime workflow (manual controller placement, Blueprints, etc).
	ApplyIntersectionStopLineDefaultsForWorld(InWorld);
	ApplyIntersectionControlDefaultsForWorld(InWorld);
	ApplyRoutingDefaultsForWorld(InWorld);
	LogIntersectionCVarSnapshot(TEXT("BeginPlay"));

	// Always log the effective runtime defaults once per PIE/Game session to make debugging "press play and send logs" easy.
	auto ReadIntCVar = [](const TCHAR* Name, int32 Fallback) -> int32
	{
		if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			return Var->GetInt();
		}
		return Fallback;
	};
	auto ReadFloatCVar = [](const TCHAR* Name, float Fallback) -> float
	{
		if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			return Var->GetFloat();
		}
		return Fallback;
	};

	const int32 VisualMode = GetTrafficVisualModeRaw();
	const int32 IntersectionMode = ReadIntCVar(TEXT("aaa.Traffic.Intersections.ControlMode"), 0);
	const int32 RequireFullStop = ReadIntCVar(TEXT("aaa.Traffic.Intersections.RequireFullStop"), 0);
	const int32 TurnPolicy = ReadIntCVar(TEXT("aaa.Traffic.Routing.TurnPolicy"), 0);
	const float StopLineOffsetCm = ReadFloatCVar(TEXT("aaa.Traffic.Intersections.StopLineOffsetCm"), 300.f);
	const int32 ReservationEnabled = ReadIntCVar(TEXT("aaa.Traffic.Intersections.ReservationEnabled"), 1);
	const float ReservationHoldSeconds = ReadFloatCVar(TEXT("aaa.Traffic.Intersections.ReservationHoldSeconds"), 3.f);
	const float TLGreen = ReadFloatCVar(TEXT("aaa.Traffic.Intersections.TrafficLight.GreenSeconds"), 10.f);
	const float TLYellow = ReadFloatCVar(TEXT("aaa.Traffic.Intersections.TrafficLight.YellowSeconds"), 2.f);
	const float TLAllRed = ReadFloatCVar(TEXT("aaa.Traffic.Intersections.TrafficLight.AllRedSeconds"), 1.f);
	const float ChaosWarmup = ReadFloatCVar(TEXT("aaa.Traffic.Visual.ChaosDrive.PhysicsWarmupSeconds"), 0.5f);
	const int32 ChaosForceSim = ReadIntCVar(TEXT("aaa.Traffic.Visual.ChaosDrive.ForceSimulatePhysics"), 1);

	UE_LOG(LogTraffic, Log,
		TEXT("[TrafficRuntimeWorldSubsystem] Defaults: VisualMode=%d IntersectionMode=%d RequireFullStop=%d TurnPolicy=%d StopLineOffsetCm=%.1f ReservationEnabled=%d Hold=%.2fs TL(g=%.1f y=%.1f r=%.1f) Chaos(warmup=%.2fs forceSim=%d) Build=%s %s"),
		VisualMode,
		IntersectionMode,
		RequireFullStop,
		TurnPolicy,
		StopLineOffsetCm,
		ReservationEnabled,
		ReservationHoldSeconds,
		TLGreen,
		TLYellow,
		TLAllRed,
		ChaosWarmup,
		ChaosForceSim,
		TEXT(__DATE__),
		TEXT(__TIME__));

	if (GEngine && (CVarTrafficOnScreenBuildStamp.GetValueOnGameThread() != 0))
	{
		const FString Msg = FString::Printf(TEXT("AAA Traffic Build: %s %s"), TEXT(__DATE__), TEXT(__TIME__));
		static const uint64 BuildStampKey = 0xAAA747AFFFu;
		GEngine->AddOnScreenDebugMessage(
			/*Key=*/BuildStampKey,
			/*TimeToDisplay=*/10.f,
			/*DisplayColor=*/FColor::Cyan,
			Msg);
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

	UE_LOG(LogTraffic, Log,
		TEXT("[TrafficRuntimeWorldSubsystem] OnWorldBeginPlay: World=%s Type=%d AutoBuild=%s AutoSpawn=%s VehiclesPerLane=%d Speed=%.1f ZoneGraph=%s"),
		*InWorld.GetName(),
		static_cast<int32>(WorldType),
		Settings->bAutoBuildNetwork ? TEXT("true") : TEXT("false"),
		Settings->bAutoSpawnVehicles ? TEXT("true") : TEXT("false"),
		Settings->VehiclesPerLaneRuntime,
		Settings->RuntimeSpeedCmPerSec,
		Settings->bGenerateZoneGraph ? TEXT("true") : TEXT("false"));

	// If the user ran the Editor Test harness before PIE, those vehicles/controllers are duplicated into the PIE world.
	// Clear any previously spawned traffic actors so we always have a clean, predictable runtime start.
	static const FName TrafficSpawnedVehicleTag(TEXT("AAA_TrafficVehicle"));
	int32 ClearedCount = 0;
	{
		TArray<AActor*> ToDestroy;
		for (TActorIterator<AActor> It(&InWorld); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor && Actor->Tags.Contains(TrafficSpawnedVehicleTag))
			{
				ToDestroy.Add(Actor);
			}
		}

		for (AActor* Actor : ToDestroy)
		{
			if (Actor && !Actor->IsActorBeingDestroyed())
			{
				InWorld.DestroyActor(Actor);
				++ClearedCount;
			}
		}
	}

	if (ClearedCount > 0)
	{
		UE_LOG(LogTraffic, Log, TEXT("[TrafficRuntimeWorldSubsystem] Cleared %d pre-existing traffic actors in PIE/Game world."), ClearedCount);
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ATrafficSystemController* Controller = nullptr;
	for (TActorIterator<ATrafficSystemController> It(&InWorld); It; ++It)
	{
		Controller = *It;
		break;
	}

	if (!Controller)
	{
		Controller = InWorld.SpawnActor<ATrafficSystemController>(ATrafficSystemController::StaticClass(), FTransform::Identity, Params);
	}

	if (!Controller)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[TrafficRuntimeWorldSubsystem] Failed to spawn TrafficSystemController."));
		return;
	}

	Controller->SetRuntimeConfigFromProjectSettings();

	// Runtime one-button safety: ensure CityBLD roads have metadata + family names before we build.
	// This prevents "everything becomes Urban_2x2" in PIE when the editor-prepared settings aren't present yet.
	Runtime_PrepareCityBLDRoadMetadata(InWorld);

	if (Settings->bAutoBuildNetwork)
	{
		Controller->Runtime_BuildTrafficNetwork();
	}

	if (Settings->bAutoSpawnVehicles)
	{
		DeferredSpawnController = Controller;
		ScheduleDeferredTrafficSpawn();
	}
}

namespace
{
	static TAutoConsoleVariable<int32> CVarTrafficRuntimeDeferSpawnUntilCollision(
		TEXT("aaa.Traffic.Runtime.DeferSpawnUntilRoadCollision"),
		1,
		TEXT("If non-zero, delays runtime vehicle spawning until road collision is detected under the traffic network (prevents Chaos vehicles falling from mid-air on PIE start).\n")
		TEXT("Default: 1"),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarTrafficRuntimeDeferSpawnPollSeconds(
		TEXT("aaa.Traffic.Runtime.DeferSpawnPollSeconds"),
		0.25f,
		TEXT("Polling interval (seconds) while waiting for road collision before spawning traffic. Default: 0.25"),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarTrafficRuntimeDeferSpawnMaxWaitSeconds(
		TEXT("aaa.Traffic.Runtime.DeferSpawnMaxWaitSeconds"),
		6.0f,
		TEXT("Max seconds to wait for road collision before spawning anyway. Default: 6.0"),
		ECVF_Default);

	static TAutoConsoleVariable<int32> CVarTrafficRuntimeDeferSpawnRequiredHits(
		TEXT("aaa.Traffic.Runtime.DeferSpawnRequiredHits"),
		3,
		TEXT("How many ground hits (sampled from the built traffic network) are required to consider road collision ready. Default: 3"),
		ECVF_Default);
}

void UTrafficRuntimeWorldSubsystem::ScheduleDeferredTrafficSpawn()
{
	UWorld* World = GetWorld();
	if (!World || !DeferredSpawnController.IsValid())
	{
		return;
	}

	if (CVarTrafficRuntimeDeferSpawnUntilCollision.GetValueOnGameThread() == 0)
	{
		DeferredSpawnController->Runtime_SpawnTraffic();
		return;
	}

	// Only defer when ChaosDrive visuals are active (kinematic/logic-only doesn't need road collision).
	if (GetTrafficVisualMode() != ETrafficVisualMode::ChaosDrive)
	{
		DeferredSpawnController->Runtime_SpawnTraffic();
		return;
	}

	DeferredSpawnAttempts = 0;
	DeferredSpawnStartWallSeconds = FPlatformTime::Seconds();

	// If collision is already ready, spawn immediately.
	if (IsRoadCollisionReadyForTraffic())
	{
		DeferredSpawnController->Runtime_SpawnTraffic();
		return;
	}

	const float Poll = FMath::Max(0.05f, CVarTrafficRuntimeDeferSpawnPollSeconds.GetValueOnGameThread());
	World->GetTimerManager().SetTimer(
		DeferredSpawnTimerHandle,
		this,
		&UTrafficRuntimeWorldSubsystem::TryDeferredTrafficSpawn,
		Poll,
		/*bLoop=*/true);
}

void UTrafficRuntimeWorldSubsystem::TryDeferredTrafficSpawn()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!DeferredSpawnController.IsValid())
	{
		World->GetTimerManager().ClearTimer(DeferredSpawnTimerHandle);
		return;
	}

	const float MaxWait = FMath::Max(0.f, CVarTrafficRuntimeDeferSpawnMaxWaitSeconds.GetValueOnGameThread());
	const double Elapsed = FPlatformTime::Seconds() - DeferredSpawnStartWallSeconds;

	++DeferredSpawnAttempts;

	if (IsRoadCollisionReadyForTraffic() || (Elapsed >= MaxWait))
	{
		World->GetTimerManager().ClearTimer(DeferredSpawnTimerHandle);

		if (Elapsed >= MaxWait)
		{
			UE_LOG(LogTraffic, Warning,
				TEXT("[TrafficRuntimeWorldSubsystem] Road collision not detected after %.2fs (%d polls); spawning traffic anyway."),
				Elapsed,
				DeferredSpawnAttempts);
		}

		DeferredSpawnController->Runtime_SpawnTraffic();
	}
}

bool UTrafficRuntimeWorldSubsystem::IsRoadCollisionReadyForTraffic() const
{
	const UWorld* World = GetWorld();
	if (!World || !DeferredSpawnController.IsValid())
	{
		return true;
	}

	UTrafficNetworkAsset* Net = DeferredSpawnController->GetBuiltNetworkAsset();
	if (!Net || Net->Network.Roads.Num() == 0)
	{
		// Nothing to collide with, nothing to wait for.
		return true;
	}

	const int32 Required = FMath::Max(1, CVarTrafficRuntimeDeferSpawnRequiredHits.GetValueOnGameThread());
	int32 Hits = 0;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(AAA_Traffic_RoadCollisionReady), /*bTraceComplex=*/false);
	Params.bReturnPhysicalMaterial = false;

	// Sample a few points from the built road centerlines; these should be over drivable surface.
	for (const FTrafficRoad& Road : Net->Network.Roads)
	{
		AActor* RoadActor = Road.SourceActor.Get();
		if (!RoadActor || Road.CenterlinePoints.Num() < 2)
		{
			continue;
		}

		const FVector P = Road.CenterlinePoints[Road.CenterlinePoints.Num() / 2];
		const FVector Start = P + FVector(0.f, 0.f, 5000.f);
		const FVector End = P - FVector(0.f, 0.f, 10000.f);

		TArray<FHitResult> HitResults;
		const bool bAny =
			World->LineTraceMultiByChannel(HitResults, Start, End, ECC_WorldStatic, Params) ||
			World->LineTraceMultiByChannel(HitResults, Start, End, ECC_WorldDynamic, Params) ||
			World->LineTraceMultiByChannel(HitResults, Start, End, ECC_Visibility, Params);

		if (!bAny)
		{
			continue;
		}

		// Prefer hits on the road actor itself; if its collision isn't ready yet, we'd typically get zero hits.
		bool bCounted = false;
		for (const FHitResult& HR : HitResults)
		{
			if (HR.bBlockingHit && HR.GetActor() == RoadActor)
			{
				++Hits;
				bCounted = true;
				break;
			}
		}

		// Fallback: accept any blocking hit (some kits use separate collision actors/components).
		if (!bCounted)
		{
			for (const FHitResult& HR : HitResults)
			{
				if (HR.bBlockingHit)
				{
					++Hits;
					break;
				}
			}
		}

		if (Hits >= Required)
		{
			return true;
		}
	}

	return false;
}
