#include "Misc/AutomationTest.h"
#include "TrafficAutomationLogger.h"
#include "Modules/ModuleManager.h"
#include "TrafficRuntimeModule.h"
#include "TrafficSystemEditorSubsystem.h"
#include "RoadFamilyRegistry.h"
#include "TrafficRoadMetadataComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficRoadFamiliesTest,
	"Traffic.Logic.RoadFamilies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FTrafficRoadFamiliesTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	const FString LocalTestName = TEXT("Traffic.Logic.RoadFamilies");
	UTrafficAutomationLogger::BeginTestLog(LocalTestName);
	UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION)));

	if (!FModuleManager::Get().IsModuleLoaded("TrafficRuntime"))
	{
		AddError(TEXT("TrafficRuntime module is not loaded."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=TrafficRuntime module not loaded"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	FAutomationEditorCommonUtils::CreateNewMap();
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		AddError(TEXT("No editor world available for test."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=NoEditorWorld"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	UTrafficSystemEditorSubsystem* Subsys = GEditor->GetEditorSubsystem<UTrafficSystemEditorSubsystem>();
	if (!Subsys)
	{
		AddError(TEXT("UTrafficSystemEditorSubsystem not available."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=SubsystemMissing"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get();
	if (!Registry)
	{
		AddError(TEXT("URoadFamilyRegistry not available."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=RegistryMissing"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	auto SpawnRoad = [&](UClass* ActorClass, const FVector& Offset) -> AActor*
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* RoadActor = World->SpawnActor<AActor>(ActorClass, Offset, FRotator::ZeroRotator, Params);
		if (!RoadActor)
		{
			return nullptr;
		}

		USplineComponent* Spline = NewObject<USplineComponent>(RoadActor);
		Spline->RegisterComponent();
		RoadActor->AddInstanceComponent(Spline);
		RoadActor->SetRootComponent(Spline);
		Spline->ClearSplinePoints(false);
		Spline->AddSplinePoint(Offset + FVector(-500.f, 0.f, 200.f), ESplineCoordinateSpace::World, false);
		Spline->AddSplinePoint(Offset + FVector(500.f, 0.f, 200.f), ESplineCoordinateSpace::World, false);
		Spline->UpdateSpline();
		return RoadActor;
	};

	AActor* RoadA = SpawnRoad(AActor::StaticClass(), FVector(0.f, 0.f, 0.f));
	AActor* RoadB = SpawnRoad(AStaticMeshActor::StaticClass(), FVector(0.f, 600.f, 0.f));
	if (!RoadA || !RoadB)
	{
		AddError(TEXT("Failed to spawn synthetic road actors for registry test."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=RoadSpawnFailed"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	Subsys->Editor_PrepareMapForTraffic();
	Registry->RefreshCache();

	const TArray<FRoadFamilyInfo>& Families = Registry->GetAllFamilies();
	TestTrue(TEXT("At least one family detected"), Families.Num() > 0);

	auto ValidateMeta = [&](AActor* Actor, const TCHAR* Label)
	{
		UTrafficRoadMetadataComponent* Meta = Actor ? Actor->FindComponentByClass<UTrafficRoadMetadataComponent>() : nullptr;
		TestNotNull(FString::Printf(TEXT("%s has metadata"), Label), Meta);
		if (Meta)
		{
			TestTrue(FString::Printf(TEXT("%s metadata has family id"), Label), Meta->RoadFamilyId.IsValid());
			const FRoadFamilyInfo* Info = Registry->FindFamilyByClass(Actor->GetClass());
			TestNotNull(FString::Printf(TEXT("%s registry entry exists"), Label), Info);
			if (Info)
			{
				TestTrue(FString::Printf(TEXT("%s metadata matches registry id"), Label), Meta->RoadFamilyId == Info->FamilyId);
			}
		}
	};

	ValidateMeta(RoadA, TEXT("RoadA"));
	ValidateMeta(RoadB, TEXT("RoadB"));

	UTrafficAutomationLogger::EndTestLog();
	return true;
#else
	return false;
#endif
}

