#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "TrafficSystemEditorSubsystem.h"
#include "RoadFamilyRegistry.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "Editor.h"

#if WITH_EDITOR

// Simple spline-based dummy road actor for calibration safety checks.
UCLASS()
class ATrafficCalibrationDummyRoad : public AActor
{
	GENERATED_BODY()
public:
	ATrafficCalibrationDummyRoad()
	{
		USplineComponent* Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
		SetRootComponent(Spline);
		Spline->SetClosedLoop(false);
		Spline->ClearSplinePoints(false);
		Spline->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::Local, false);
		Spline->AddSplinePoint(FVector(1000.f, 0.f, 0.f), ESplineCoordinateSpace::Local, false);
		Spline->AddSplinePoint(FVector(2000.f, 500.f, 0.f), ESplineCoordinateSpace::Local, false);
		Spline->UpdateSpline();
	}
};

#endif // WITH_EDITOR

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

	FRoadFamilyInfo* FamilyInfo = Registry->FindOrCreateFamilyForClass(ATrafficCalibrationDummyRoad::StaticClass());
	if (!FamilyInfo)
	{
		AddError(TEXT("Failed to register dummy road family."));
		return false;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	World->SpawnActor<ATrafficCalibrationDummyRoad>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	World->SpawnActor<ATrafficCalibrationDummyRoad>(FVector(3000.f, 0.f, 0.f), FRotator::ZeroRotator, Params);

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
