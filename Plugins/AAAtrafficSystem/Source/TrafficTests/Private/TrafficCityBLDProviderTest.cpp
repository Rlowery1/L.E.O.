#include "Misc/AutomationTest.h"
#include "CityBLDRoadGeometryProvider.h"
#include "TrafficRoadTypes.h"
#include "TrafficCityBLDAdapterSettings.h"
#include "TrafficRoadFamilySettings.h"
#include "TrafficAutomationLogger.h"
#include "Modules/ModuleManager.h"
#include "TrafficRuntimeModule.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Actor.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficCityBLDProviderSyntheticTest,
	"Traffic.Integration.CityBLD.ProviderSynthetic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
);

bool FTrafficCityBLDProviderSyntheticTest::RunTest(const FString& Parameters)
{
	const FString LocalTestName = TEXT("Traffic.Integration.CityBLD.ProviderSynthetic");
	UTrafficAutomationLogger::BeginTestLog(LocalTestName);
	UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION)));

	if (!FModuleManager::Get().IsModuleLoaded("TrafficRuntime"))
	{
		AddError(TEXT("TrafficRuntime module is not loaded."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=TrafficRuntime module not loaded"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

#if !WITH_EDITOR
	AddError(TEXT("This test requires editor (WITH_EDITOR)."));
	UTrafficAutomationLogger::LogLine(TEXT("Error=NotInEditor"));
	UTrafficAutomationLogger::EndTestLog();
	return false;
#else
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		AddError(TEXT("No editor world available."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=NoEditorWorld"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	const UTrafficCityBLDAdapterSettings* AdapterSettings = GetDefault<UTrafficCityBLDAdapterSettings>();
	const UTrafficRoadFamilySettings* FamSettings = GetDefault<UTrafficRoadFamilySettings>();

	if (!AdapterSettings || !FamSettings || FamSettings->Families.Num() == 0)
	{
		AddError(TEXT("Adapter settings or road families not available."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=SettingsOrFamiliesMissing"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* RoadActor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!RoadActor)
	{
		AddError(TEXT("Failed to spawn synthetic road actor."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=SpawnFailed"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	if (!AdapterSettings->RoadActorTag.IsNone())
	{
		RoadActor->Tags.Add(AdapterSettings->RoadActorTag);
	}

	USplineComponent* Spline = NewObject<USplineComponent>(RoadActor);
	Spline->SetupAttachment(RoadActor->GetRootComponent());

	if (!AdapterSettings->RoadSplineTag.IsNone())
	{
		Spline->ComponentTags.Add(AdapterSettings->RoadSplineTag);
	}

	Spline->RegisterComponent();

	const int32 NumPoints = 4;
	const float SegmentLength = 1000.f;

	Spline->ClearSplinePoints(false);
	for (int32 i = 0; i < NumPoints; ++i)
	{
		const float X = i * SegmentLength;
		Spline->AddSplinePoint(FVector(X, 0.f, 0.f), ESplineCoordinateSpace::World, false);
	}
	Spline->UpdateSpline();

	TArray<FTrafficRoad> Roads;
	UCityBLDRoadGeometryProvider* Provider = NewObject<UCityBLDRoadGeometryProvider>();
	Provider->CollectRoads(World, Roads);

	const int32 RoadCount = Roads.Num();
	UTrafficAutomationLogger::LogMetric(TEXT("CollectedRoadCount"), FString::FromInt(RoadCount));

	TestTrue(TEXT("At least one CityBLD road detected"), RoadCount >= 1);

	const FTrafficRoad* TargetRoad = nullptr;
	for (const FTrafficRoad& Road : Roads)
	{
		if (Road.SourceActor == RoadActor)
		{
			TargetRoad = &Road;
			break;
		}
	}

	if (TargetRoad)
	{
		const FTrafficRoad& R = *TargetRoad;
		const int32 CenterlinePointCount = R.CenterlinePoints.Num();
		UTrafficAutomationLogger::LogMetric(TEXT("CenterlinePointCount"), FString::FromInt(CenterlinePointCount));
		TestTrue(TEXT("Centerline has at least 4 samples"), CenterlinePointCount >= NumPoints);

		if (CenterlinePointCount >= 2)
		{
			const FVector ExpectedStart(0.f, 0.f, 0.f);
			const float ExpectedLength = (NumPoints - 1) * SegmentLength;

			const FVector Start = R.CenterlinePoints[0];
			const FVector End = R.CenterlinePoints.Last();

			TestTrue(TEXT("Start near expected"), Start.Equals(ExpectedStart, 1.0f));
			const float ActualLength = FVector::Dist(Start, End);
			TestTrue(TEXT("End extends forward"), ActualLength >= ExpectedLength * 0.5f);

			float PrevDist = 0.f;
			const FVector StartRef = R.CenterlinePoints[0];
			for (int32 i = 1; i < CenterlinePointCount; ++i)
			{
				const float Dist = FVector::Dist(StartRef, R.CenterlinePoints[i]);
				TestTrue(TEXT("Centerline distance is non-decreasing"), Dist >= PrevDist - 1e-3f);
				PrevDist = Dist;
			}
		}
	}
	else
	{
		AddError(TEXT("Synthetic CityBLD road not found in collected roads."));
	}

	RoadActor->Destroy();

	UTrafficAutomationLogger::EndTestLog();
	return RoadCount >= 1;
#endif
}
