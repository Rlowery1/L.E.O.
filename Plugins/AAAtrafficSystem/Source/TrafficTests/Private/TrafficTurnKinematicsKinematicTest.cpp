#include "Misc/AutomationTest.h"
#include "TrafficRoadTypes.h"
#include "TrafficRoadFamilySettings.h"
#include "TrafficNetworkBuilder.h"
#include "TrafficMovementGeometry.h"
#include "TrafficKinematicFollower.h"
#include "TrafficAutomationLogger.h"
#include "Modules/ModuleManager.h"
#include "TrafficRuntimeModule.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficTurnKinematicsKinematicSimpleCrossTest,
	"Traffic.Motion.TurnKinematics.KinematicSimpleCross",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FTrafficTurnKinematicsKinematicSimpleCrossTest::RunTest(const FString& Parameters)
{
	const FString LocalTestName = TEXT("Traffic.Motion.TurnKinematics.KinematicSimpleCross");
	UTrafficAutomationLogger::BeginTestLog(LocalTestName);
	UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION)));

	if (!FModuleManager::Get().IsModuleLoaded("TrafficRuntime"))
	{
		AddError(TEXT("TrafficRuntime module is not loaded."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=TrafficRuntime module not loaded"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	const UTrafficRoadFamilySettings* FamSettings = GetDefault<UTrafficRoadFamilySettings>();
	if (!FamSettings || FamSettings->Families.Num() == 0)
	{
		AddError(TEXT("No road families configured."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=No road families configured"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	TArray<FTrafficRoad> Roads;
	const int32 NumPoints = 5;
	const float SegmentLength = 1000.f;

	{
		FTrafficRoad Road;
		Road.RoadId = 0;
		Road.FamilyId = 0;
		for (int32 i = 0; i < NumPoints; ++i)
		{
			const float X = (i - 2) * SegmentLength;
			Road.CenterlinePoints.Add(FVector(X, 0.f, 0.f));
		}
		Roads.Add(Road);
	}

	{
		FTrafficRoad Road;
		Road.RoadId = 1;
		Road.FamilyId = 0;
		for (int32 i = 0; i < NumPoints; ++i)
		{
			const float Y = (i - 2) * SegmentLength;
			Road.CenterlinePoints.Add(FVector(0.f, Y, 0.f));
		}
		Roads.Add(Road);
	}

	FTrafficNetwork Network;
	FTrafficNetworkBuilder::BuildNetworkFromRoads(Roads, FamSettings, Network);

	if (Network.Movements.Num() == 0)
	{
		AddError(TEXT("No movements generated for synthetic cross."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=No movements generated"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	UTrafficAutomationLogger::LogMetric(TEXT("MovementCount"), FString::FromInt(Network.Movements.Num()));

	const FTrafficMovement& Movement = Network.Movements[0];

	TArray<FMovementSample> Samples;
	TrafficMovementGeometry::AnalyzeMovementPath(Movement, Samples);

	if (Samples.Num() < 2)
	{
		AddError(TEXT("Movement path has too few samples."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=MovementSamplesTooFew"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	const float MovementLength = Samples.Last().S;
	UTrafficAutomationLogger::LogMetricFloat(TEXT("MovementLengthCm"), MovementLength, 1);

	UTrafficKinematicFollower* Follower = NewObject<UTrafficKinematicFollower>();
	const float SpeedCmPerSec = 800.f;
	Follower->InitForMovement(&Movement, 0.0f, SpeedCmPerSec);

	const int32 NumSteps = 80;
	const float TotalTime = MovementLength / SpeedCmPerSec;
	const float DeltaTime = (NumSteps > 0) ? (TotalTime / NumSteps) : 0.0f;

	UTrafficAutomationLogger::LogMetric(TEXT("NumSteps"), FString::FromInt(NumSteps));
	UTrafficAutomationLogger::LogMetricFloat(TEXT("TotalTime"), TotalTime, 3);

	float MaxHeadingStepDeg = 0.0f;
	float MaxCurvature = 0.0f;

	FVector PrevTangent = FVector::ZeroVector;

	for (int32 StepIndex = 0; StepIndex < NumSteps; ++StepIndex)
	{
		Follower->Step(DeltaTime);

		FVector Pos, Tangent;
		const bool bPoseOk = Follower->GetCurrentPose(Pos, Tangent);
		if (!bPoseOk)
		{
			AddError(TEXT("UTrafficKinematicFollower::GetCurrentPose failed for movement."));
			UTrafficAutomationLogger::LogLine(TEXT("Error=GetCurrentPose failed"));
			UTrafficAutomationLogger::EndTestLog();
			return false;
		}

		float ClosestS = Follower->GetState().S;
		float ClosestDist = TNumericLimits<float>::Max();
		float LocalCurvature = 0.0f;

		for (const FMovementSample& Sample : Samples)
		{
			const float Dist = FMath::Abs(Sample.S - ClosestS);
			if (Dist < ClosestDist)
			{
				ClosestDist = Dist;
				LocalCurvature = Sample.Curvature;
			}
		}

		MaxCurvature = FMath::Max(MaxCurvature, FMath::Abs(LocalCurvature));

		if (!PrevTangent.IsNearlyZero())
		{
			const float Dot = FMath::Clamp(FVector::DotProduct(PrevTangent, Tangent), -1.0f, 1.0f);
			const float AngleRad = FMath::Acos(Dot);
			const float AngleDeg = FMath::RadiansToDegrees(AngleRad);
			MaxHeadingStepDeg = FMath::Max(MaxHeadingStepDeg, AngleDeg);
		}

		PrevTangent = Tangent;
	}

	UTrafficAutomationLogger::LogMetricFloat(TEXT("MaxCurvature"), MaxCurvature, 6);
	UTrafficAutomationLogger::LogMetricFloat(TEXT("MaxHeadingStepDeg"), MaxHeadingStepDeg, 3);

	const float MaxHeadingStepToleranceDeg = 10.0f;
	const bool bHeadingSmooth = (MaxHeadingStepDeg <= MaxHeadingStepToleranceDeg);

	UTrafficAutomationLogger::LogMetric(TEXT("HeadingSmooth"), bHeadingSmooth ? TEXT("true") : TEXT("false"));

	if (!bHeadingSmooth)
	{
		AddError(FString::Printf(TEXT("MaxHeadingStepDeg too large: %f"), MaxHeadingStepDeg));
	}

	UTrafficAutomationLogger::EndTestLog();

	return bHeadingSmooth;
}

