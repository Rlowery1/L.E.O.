#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "TrafficSystemEditorSubsystem.h"
#include "RoadFamilyRegistry.h"
#include "Components/SplineComponent.h"
#include "TrafficRoadMetadataComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Editor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficCalibrationSafetyTest,
	"Traffic.Editor.Calibration.MultipleRoads_NoDestroy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficCalibrationSafetyTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	FAutomationEditorCommonUtils::CreateNewMap();

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		AddError(TEXT("No editor world available for calibration safety test."));
		return false;
	}

	UTrafficSystemEditorSubsystem* Subsys = GEditor->GetEditorSubsystem<UTrafficSystemEditorSubsystem>();
	if (!Subsys)
	{
		AddError(TEXT("TrafficSystemEditorSubsystem unavailable."));
		return false;
	}

	URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get();
	if (!Registry)
	{
		AddError(TEXT("RoadFamilyRegistry unavailable."));
		return false;
	}

	FRoadFamilyInfo* FamilyInfo = Registry->FindOrCreateFamilyForClass(APawn::StaticClass());
	if (!FamilyInfo)
	{
		AddError(TEXT("Failed to register dummy road family."));
		return false;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	auto SpawnDummyRoad = [&](const FVector& Location)
	{
		APawn* RoadActor = World->SpawnActor<APawn>(APawn::StaticClass(), Location, FRotator::ZeroRotator, Params);
		if (!RoadActor)
		{
			return;
		}

		USplineComponent* Spline = NewObject<USplineComponent>(RoadActor, TEXT("Spline"));
		if (Spline)
		{
			Spline->RegisterComponent();
			Spline->SetMobility(EComponentMobility::Static);
			Spline->SetClosedLoop(false);
			Spline->ClearSplinePoints(false);
			Spline->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::Local, false);
			Spline->AddSplinePoint(FVector(1000.f, 0.f, 0.f), ESplineCoordinateSpace::Local, true);
			RoadActor->AddInstanceComponent(Spline);
			RoadActor->SetRootComponent(Spline);
		}

		UTrafficRoadMetadataComponent* Meta = NewObject<UTrafficRoadMetadataComponent>(RoadActor, TEXT("TrafficMeta"));
		if (Meta)
		{
			Meta->RoadFamilyId = FamilyInfo->FamilyId;
			Meta->bIncludeInTraffic = true;
			Meta->RegisterComponent();
			RoadActor->AddInstanceComponent(Meta);
		}
	};

	SpawnDummyRoad(FVector::ZeroVector);
	SpawnDummyRoad(FVector(3000.f, 0.f, 0.f));

	Subsys->Editor_PrepareMapForTraffic();

	TArray<AActor*> ActorsBefore;
	Subsys->GetActorsForFamily(FamilyInfo->FamilyId, ActorsBefore);

	Subsys->Editor_BeginCalibrationForFamily(FamilyInfo->FamilyId);
	Subsys->Editor_BakeCalibrationForActiveFamily();

	TArray<AActor*> ActorsAfter;
	Subsys->GetActorsForFamily(FamilyInfo->FamilyId, ActorsAfter);

	TestEqual(TEXT("Road actors preserved through calibration workflow"), ActorsAfter.Num(), ActorsBefore.Num());
	return !HasAnyErrors();
#else
	return false;
#endif
}
