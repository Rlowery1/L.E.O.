#include "Misc/AutomationTest.h"
#include "TrafficRoadTypes.h"
#include "TrafficLaneGeometry.h"
#include "TrafficLaneCalibration.h"
#include "TrafficAutomationLogger.h"
#include "Modules/ModuleManager.h"
#include "TrafficRuntimeModule.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Tests/AutomationEditorCommon.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficLaneCalibrationFlatPlaneTest,
	"Traffic.Logic.LaneCalibration.FlatPlane",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FTrafficLaneCalibrationFlatPlaneTest::RunTest(const FString& Parameters)
{
	const FString LocalTestName = TEXT("Traffic.Logic.LaneCalibration.FlatPlane");
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
	FAutomationEditorCommonUtils::CreateNewMap();

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		AddError(TEXT("No editor world available."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=NoEditorWorld"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AStaticMeshActor* PlaneActor = World->SpawnActor<AStaticMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!PlaneActor)
	{
		AddError(TEXT("Failed to spawn AStaticMeshActor for flat plane."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=SpawnPlaneFailed"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	UStaticMeshComponent* MeshComp = PlaneActor->GetStaticMeshComponent();
	if (!MeshComp)
	{
		AddError(TEXT("PlaneActor has no StaticMeshComponent."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=NoStaticMeshComponent"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (!PlaneMesh)
	{
		AddError(TEXT("Failed to load /Engine/BasicShapes/Plane.Plane static mesh."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=LoadPlaneMeshFailed"));
		PlaneActor->Destroy();
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	MeshComp->SetStaticMesh(PlaneMesh);
	// Make the plane large enough to fully cover the synthetic lane extents.
	MeshComp->SetWorldScale3D(FVector(120.f, 120.f, 1.f));
	MeshComp->SetMobility(EComponentMobility::Static);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComp->SetCollisionObjectType(ECC_WorldStatic);
	MeshComp->SetCollisionResponseToAllChannels(ECR_Block);

	FTrafficLane Lane;
	Lane.LaneId = 0;
	Lane.RoadId = 0;
	Lane.SideIndex = 0;
	Lane.Width = 350.f;
	Lane.Direction = ELaneDirection::Forward;
	Lane.SpeedLimitKmh = 50.f;

	const int32 NumPoints = 11;
	const float SegmentLength = 1000.f;

	for (int32 i = 0; i < NumPoints; ++i)
	{
		const float X = (i - 5) * SegmentLength;
		Lane.CenterlinePoints.Add(FVector(X, 0.f, 0.f));
	}

	FLaneSurfaceCoverageMetrics Metrics;
	const int32 NumSamplesAlongLane = 50;
	const float TraceHeightAboveCm = 200.f;
	const float TraceDepthBelowCm = 300.f;

	const bool bEvalOk = TrafficLaneCalibration::EvaluateLaneSurfaceCoverage(
		World,
		Lane,
		NumSamplesAlongLane,
		TraceHeightAboveCm,
		TraceDepthBelowCm,
		Metrics);

	if (!bEvalOk)
	{
		AddError(TEXT("EvaluateLaneSurfaceCoverage returned false."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=EvaluateLaneSurfaceCoverageFailed"));
		PlaneActor->Destroy();
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	UTrafficAutomationLogger::LogMetric(TEXT("LaneId"), FString::FromInt(Metrics.LaneId));
	UTrafficAutomationLogger::LogMetric(TEXT("RoadId"), FString::FromInt(Metrics.RoadId));
	UTrafficAutomationLogger::LogMetric(TEXT("NumSamples"), FString::FromInt(Metrics.NumSamples));
	UTrafficAutomationLogger::LogMetric(TEXT("NumSamplesOnRoad"), FString::FromInt(Metrics.NumSamplesOnRoad));
	UTrafficAutomationLogger::LogMetricFloat(TEXT("CoveragePercent"), Metrics.CoveragePercent, 2);
	UTrafficAutomationLogger::LogMetricFloat(TEXT("MaxVerticalGapCm"), Metrics.MaxVerticalGapCm, 2);

	const float ExpectedCoverage = 100.0f;
	const float CoverageTolerance = 0.5f;
	const float ExpectedGap = 0.0f;
	const float GapTolerance = 1.0f;

	const bool bCoverageOk = FMath::Abs(Metrics.CoveragePercent - ExpectedCoverage) <= CoverageTolerance;
	const bool bGapOk = FMath::Abs(Metrics.MaxVerticalGapCm - ExpectedGap) <= GapTolerance;

	UTrafficAutomationLogger::LogMetric(TEXT("CoverageOk"), bCoverageOk ? TEXT("true") : TEXT("false"));
	UTrafficAutomationLogger::LogMetric(TEXT("GapOk"), bGapOk ? TEXT("true") : TEXT("false"));

	if (!bCoverageOk)
	{
		AddError(FString::Printf(TEXT("CoveragePercent mismatch: %f"), Metrics.CoveragePercent));
	}

	if (!bGapOk)
	{
		AddError(FString::Printf(TEXT("MaxVerticalGapCm mismatch: %f"), Metrics.MaxVerticalGapCm));
	}

	PlaneActor->Destroy();

	UTrafficAutomationLogger::EndTestLog();

	return bCoverageOk && bGapOk;
#endif
}

