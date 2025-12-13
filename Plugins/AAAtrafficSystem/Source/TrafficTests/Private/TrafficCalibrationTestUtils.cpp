#include "TrafficCalibrationTestUtils.h"

#include "LaneCalibrationOverlayActor.h"
#include "Misc/AutomationTest.h"
#include "HAL/PlatformTime.h"
#include "RoadFamilyRegistry.h"
#include "TrafficAutomationLogger.h"
#include "TrafficLaneCalibration.h"
#include "TrafficLaneGeometry.h"
#include "TrafficNetworkAsset.h"
#include "TrafficRoadFamilySettings.h"
#include "TrafficSystemController.h"
#include "TrafficSystemEditorSubsystem.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	struct FPolylineClosestResult
	{
		FVector ClosestPoint = FVector::ZeroVector;
		FVector Tangent = FVector::ForwardVector;
		float DistSq2D = TNumericLimits<float>::Max();
		int32 SegmentIndex = INDEX_NONE;
	};

	static FPolylineClosestResult ClosestPointOnPolyline2D(const TArray<FVector>& Points, const FVector& Query)
	{
		FPolylineClosestResult Result;
		if (Points.Num() < 2)
		{
			return Result;
		}

		const FVector Query2D(Query.X, Query.Y, 0.f);
		for (int32 i = 0; i < Points.Num() - 1; ++i)
		{
			const FVector A2D(Points[i].X, Points[i].Y, 0.f);
			const FVector B2D(Points[i + 1].X, Points[i + 1].Y, 0.f);
			const FVector Seg = B2D - A2D;
			const float SegLenSq = Seg.SizeSquared();
			if (SegLenSq <= KINDA_SMALL_NUMBER)
			{
				continue;
			}

			const float T = FMath::Clamp(FVector::DotProduct(Query2D - A2D, Seg) / SegLenSq, 0.f, 1.f);
			const FVector Closest = A2D + Seg * T;
			const float D2 = FVector::DistSquared(Query2D, Closest);
			if (D2 < Result.DistSq2D)
			{
				Result.DistSq2D = D2;
				Result.ClosestPoint = FVector(Closest.X, Closest.Y, Query.Z);
				FVector Tan = Seg;
				Tan.Normalize();
				Result.Tangent = FVector(Tan.X, Tan.Y, 0.f);
				Result.SegmentIndex = i;
			}
		}

		if (Result.Tangent.IsNearlyZero())
		{
			Result.Tangent = FVector::ForwardVector;
		}
		return Result;
	}

	static bool FindClosestPointOnRoadSurface(
		UWorld* World,
		AActor* RoadActor,
		const FVector& LanePos,
		const TrafficCalibrationTestUtils::FAlignmentEvalParams& Params,
		FVector& OutSurfacePoint)
	{
		if (!World || !RoadActor)
		{
			return false;
		}

		TArray<UPrimitiveComponent*> PrimComps;
		RoadActor->GetComponents<UPrimitiveComponent>(PrimComps);

		bool bFound = false;
		float BestDist = TNumericLimits<float>::Max();
		FVector BestPoint = FVector::ZeroVector;

		for (UPrimitiveComponent* Comp : PrimComps)
		{
			if (!Comp || !Comp->IsRegistered())
			{
				continue;
			}
			if (Comp->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
			{
				continue;
			}

			FVector ClosestPoint = FVector::ZeroVector;
			const float Dist = Comp->GetClosestPointOnCollision(LanePos, ClosestPoint);
			if (Dist >= 0.f && Dist < BestDist)
			{
				BestDist = Dist;
				BestPoint = ClosestPoint;
				bFound = true;
			}
		}

		if (bFound)
		{
			OutSurfacePoint = BestPoint;
			return true;
		}

		const FVector Start = LanePos + FVector::UpVector * Params.TraceHeightAboveCm;
		const FVector End = LanePos - FVector::UpVector * Params.TraceDepthBelowCm;

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TrafficCalibrationAlign), false);
		QueryParams.bReturnPhysicalMaterial = false;

		FHitResult Hit;
		const bool bHit = World->SweepSingleByChannel(
			Hit,
			Start,
			End,
			FQuat::Identity,
			ECC_WorldStatic,
			FCollisionShape::MakeSphere(Params.SweepRadiusCm),
			QueryParams);

		if (bHit)
		{
			if (Hit.GetActor() == RoadActor || (Hit.GetComponent() && Hit.GetComponent()->GetOwner() == RoadActor))
			{
				OutSurfacePoint = Hit.ImpactPoint;
				return true;
			}
		}

		return false;
	}

	static void LogIterationMetrics(
		const FString& Prefix,
		int32 Iteration,
		const TrafficCalibrationTestUtils::FAlignmentMetrics& Metrics,
		const FTrafficLaneFamilyCalibration& Calib)
	{
		const FString Iter = FString::Printf(TEXT("%s.Iter%d"), *Prefix, Iteration);
		UTrafficAutomationLogger::LogMetricInt(Iter + TEXT(".TotalSamples"), Metrics.TotalSamples);
		UTrafficAutomationLogger::LogMetricInt(Iter + TEXT(".SamplesWithSurface"), Metrics.SamplesWithSurface);
		UTrafficAutomationLogger::LogMetricInt(Iter + TEXT(".SamplesWithinTol"), Metrics.SamplesWithinTolerance);
		UTrafficAutomationLogger::LogMetricFloat(Iter + TEXT(".MeanDevCm"), Metrics.MeanLateralDeviationCm, 2);
		UTrafficAutomationLogger::LogMetricFloat(Iter + TEXT(".MaxDevCm"), Metrics.MaxLateralDeviationCm, 2);
		UTrafficAutomationLogger::LogMetricFloat(Iter + TEXT(".MaxHeadingDeg"), Metrics.MaxHeadingErrorDeg, 2);
		UTrafficAutomationLogger::LogMetricFloat(Iter + TEXT(".CoveragePercent"), Metrics.CoveragePercent, 2);
		UTrafficAutomationLogger::LogMetricFloat(Iter + TEXT(".MeanOutwardErrCm"), Metrics.MeanOutwardSignedErrorCm, 2);
		UTrafficAutomationLogger::LogMetricInt(Iter + TEXT(".Calib.NumFwd"), Calib.NumLanesPerSideForward);
		UTrafficAutomationLogger::LogMetricInt(Iter + TEXT(".Calib.NumBack"), Calib.NumLanesPerSideBackward);
		UTrafficAutomationLogger::LogMetricFloat(Iter + TEXT(".Calib.LaneWidthCm"), Calib.LaneWidthCm, 2);
		UTrafficAutomationLogger::LogMetricFloat(Iter + TEXT(".Calib.CenterOffsetCm"), Calib.CenterlineOffsetCm, 2);
	}
}

UTrafficNetworkAsset* TrafficCalibrationTestUtils::FindBuiltNetworkAsset(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ATrafficSystemController> It(World); It; ++It)
	{
		if (UTrafficNetworkAsset* Net = It->GetBuiltNetworkAsset())
		{
			return Net;
		}
	}
	return nullptr;
}

bool TrafficCalibrationTestUtils::EvaluateNetworkLaneAlignment(
	UWorld* World,
	const FTrafficNetwork& Network,
	const FAlignmentEvalParams& Params,
	FAlignmentMetrics& OutMetrics)
{
	OutMetrics = FAlignmentMetrics();

	if (!World)
	{
		return false;
	}

	TMap<int32, const FTrafficRoad*> RoadById;
	for (const FTrafficRoad& Road : Network.Roads)
	{
		RoadById.Add(Road.RoadId, &Road);
	}

	double SumDev = 0.0;
	double SumOutward = 0.0;
	float MaxDev = 0.f;
	float MaxHeading = 0.f;

	int32 TotalSamples = 0;
	int32 WithSurface = 0;
	int32 WithinTol = 0;

	for (const FTrafficLane& Lane : Network.Lanes)
	{
		const FTrafficRoad* Road = RoadById.FindRef(Lane.RoadId);
		if (!Road)
		{
			continue;
		}

		AActor* RoadActor = Road->SourceActor.Get();
		if (!RoadActor)
		{
			continue;
		}

		const float LaneLength = TrafficLaneGeometry::ComputeLaneLengthCm(Lane);
		if (LaneLength <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const int32 NumSamples = FMath::Clamp(Params.SamplesPerLane, 8, 256);
		const float StepS = LaneLength / static_cast<float>(NumSamples - 1);

		for (int32 i = 0; i < NumSamples; ++i)
		{
			const float S = StepS * i;

			FVector LanePos, LaneTangent;
			if (!TrafficLaneGeometry::SamplePoseAtS(Lane, S, LanePos, LaneTangent))
			{
				continue;
			}

			FPolylineClosestResult RoadClosest = ClosestPointOnPolyline2D(Road->CenterlinePoints, LanePos);
			FVector RoadTan = RoadClosest.Tangent;
			RoadTan.Z = 0.f;
			if (!RoadTan.Normalize())
			{
				RoadTan = FVector::ForwardVector;
			}

			const FVector RoadRight = FVector::CrossProduct(FVector::UpVector, RoadTan).GetSafeNormal();

			const FVector LaneTan2D = FVector(LaneTangent.X, LaneTangent.Y, 0.f).GetSafeNormal();
			const float HeadingDot = FMath::Abs(FVector::DotProduct(LaneTan2D, RoadTan));
			const float HeadingDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(HeadingDot, -1.f, 1.f)));
			MaxHeading = FMath::Max(MaxHeading, HeadingDeg);

			FVector SurfacePoint = FVector::ZeroVector;
			const bool bHasSurface = FindClosestPointOnRoadSurface(World, RoadActor, LanePos, Params, SurfacePoint);

			++TotalSamples;

			if (!bHasSurface)
			{
				const float FallbackDev = Params.SweepRadiusCm;
				SumDev += FallbackDev;
				MaxDev = FMath::Max(MaxDev, FallbackDev);
				continue;
			}

			++WithSurface;

			const float Dev2D = FVector::Dist2D(LanePos, SurfacePoint);
			SumDev += Dev2D;
			MaxDev = FMath::Max(MaxDev, Dev2D);
			if (Dev2D <= Params.LateralToleranceCm)
			{
				++WithinTol;
			}

			const float SignedLaneOffsetFromCenter = FVector::DotProduct(LanePos - RoadClosest.ClosestPoint, RoadRight);
			const float SideSign = (SignedLaneOffsetFromCenter >= 0.f) ? 1.f : -1.f;
			const float SignedSurfaceError = FVector::DotProduct(LanePos - SurfacePoint, RoadRight);
			const float OutwardSignedError = SignedSurfaceError * SideSign;
			SumOutward += static_cast<double>(OutwardSignedError);
		}
	}

	OutMetrics.TotalSamples = TotalSamples;
	OutMetrics.SamplesWithSurface = WithSurface;
	OutMetrics.SamplesWithinTolerance = WithinTol;
	OutMetrics.MaxHeadingErrorDeg = MaxHeading;
	OutMetrics.MaxLateralDeviationCm = MaxDev;

	if (TotalSamples > 0)
	{
		OutMetrics.MeanLateralDeviationCm = static_cast<float>(SumDev / static_cast<double>(TotalSamples));
		OutMetrics.CoveragePercent = 100.0f * static_cast<float>(WithinTol) / static_cast<float>(TotalSamples);
		OutMetrics.MeanOutwardSignedErrorCm = static_cast<float>(SumOutward / static_cast<double>(TotalSamples));
	}

	return TotalSamples > 0;
}

bool TrafficCalibrationTestUtils::AlignmentMeetsThresholds(
	const FAlignmentMetrics& Metrics,
	const FAlignmentThresholds& Thresholds,
	FString* OutFailureReason)
{
	auto Fail = [&](const TCHAR* Reason)
	{
		if (OutFailureReason)
		{
			*OutFailureReason = Reason;
		}
		return false;
	};

	if (Metrics.TotalSamples <= 0)
	{
		return Fail(TEXT("NoSamples"));
	}
	if (Metrics.MeanLateralDeviationCm > Thresholds.MaxMeanDeviationCm)
	{
		return Fail(TEXT("MeanDeviationTooHigh"));
	}
	if (Metrics.MaxLateralDeviationCm > Thresholds.MaxDeviationCm)
	{
		return Fail(TEXT("MaxDeviationTooHigh"));
	}
	if (Metrics.MaxHeadingErrorDeg > Thresholds.MaxHeadingErrorDeg)
	{
		return Fail(TEXT("MaxHeadingTooHigh"));
	}
	if (Metrics.CoveragePercent < Thresholds.MinCoveragePercent)
	{
		return Fail(TEXT("CoverageTooLow"));
	}
	return true;
}

bool TrafficCalibrationTestUtils::ApplyCalibrationToRoadFamilySettings(const FTrafficLaneFamilyCalibration& Calib, int32 FamilyIndex)
{
	UTrafficRoadFamilySettings* Settings = GetMutableDefault<UTrafficRoadFamilySettings>();
	if (!Settings || !Settings->Families.IsValidIndex(FamilyIndex))
	{
		return false;
	}

	FRoadFamilyDefinition& Fam = Settings->Families[FamilyIndex];
	Fam.Forward.NumLanes = Calib.NumLanesPerSideForward;
	Fam.Backward.NumLanes = Calib.NumLanesPerSideBackward;
	Fam.Forward.LaneWidthCm = Calib.LaneWidthCm;
	Fam.Backward.LaneWidthCm = Calib.LaneWidthCm;
	Fam.Forward.InnerLaneCenterOffsetCm = Calib.CenterlineOffsetCm;
	Fam.Backward.InnerLaneCenterOffsetCm = Calib.CenterlineOffsetCm;
	return true;
}

FTrafficLaneFamilyCalibration TrafficCalibrationTestUtils::ComputeNextCalibration(
	const FTrafficLaneFamilyCalibration& Current,
	const FAlignmentMetrics& Metrics,
	float MaxStepOffsetCm,
	float MaxStepWidthCm)
{
	FTrafficLaneFamilyCalibration Next = Current;

	const float OffsetStep = FMath::Clamp(Metrics.MeanOutwardSignedErrorCm * 0.5f, -MaxStepOffsetCm, MaxStepOffsetCm);
	const float WidthStep = FMath::Clamp(Metrics.MeanOutwardSignedErrorCm * 0.25f, -MaxStepWidthCm, MaxStepWidthCm);

	Next.CenterlineOffsetCm = FMath::Clamp(Current.CenterlineOffsetCm - OffsetStep, 0.f, 2000.f);
	Next.LaneWidthCm = FMath::Clamp(Current.LaneWidthCm - WidthStep, 50.f, 1000.f);

	return Next;
}

bool TrafficCalibrationTestUtils::RunEditorCalibrationLoop(
	FAutomationTestBase* Test,
	UWorld* World,
	UTrafficSystemEditorSubsystem* Subsys,
	const FString& MetricPrefix,
        const FGuid& FamilyId,
        int32 MaxIterations,
        const FAlignmentEvalParams& EvalParams,
        const FAlignmentThresholds& Thresholds,
        FAlignmentMetrics& OutFinalMetrics,
        FTrafficLaneFamilyCalibration& OutFinalCalibration,
        double MaxWallSeconds)
{
        OutFinalMetrics = FAlignmentMetrics();
        OutFinalCalibration = FTrafficLaneFamilyCalibration();

        if (!World || !Subsys)
	{
		return false;
	}

	const UTrafficRoadFamilySettings* RoadSettings = GetDefault<UTrafficRoadFamilySettings>();
	if (!RoadSettings || RoadSettings->Families.Num() == 0)
	{
		if (Test)
		{
			Test->AddError(TEXT("UTrafficRoadFamilySettings has no families configured."));
		}
		return false;
	}

	FTrafficLaneFamilyCalibration Current;
	Current.NumLanesPerSideForward = RoadSettings->Families[0].Forward.NumLanes;
	Current.NumLanesPerSideBackward = RoadSettings->Families[0].Backward.NumLanes;
	Current.LaneWidthCm = RoadSettings->Families[0].Forward.LaneWidthCm;
	Current.CenterlineOffsetCm = RoadSettings->Families[0].Forward.InnerLaneCenterOffsetCm;

        MaxIterations = FMath::Clamp(MaxIterations, 1, 10);

        const double StartTime = FPlatformTime::Seconds();
        const double Deadline = StartTime + FMath::Max(1.0, MaxWallSeconds);

        for (int32 Iter = 1; Iter <= MaxIterations; ++Iter)
        {
                UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("%s.Iteration=%d"), *MetricPrefix, Iter));

                if (FPlatformTime::Seconds() > Deadline)
                {
                        if (Test)
                        {
                                Test->AddError(TEXT("Calibration loop exceeded allotted wall time."));
                        }
                        UTrafficAutomationLogger::LogLine(TEXT("Error=CalibrationTimeout"));
                        return false;
                }

                Subsys->Editor_BeginCalibrationForFamily(FamilyId);

		ALaneCalibrationOverlayActor* Overlay = nullptr;
		for (TActorIterator<ALaneCalibrationOverlayActor> It(World); It; ++It)
		{
			Overlay = *It;
			break;
		}

		if (Overlay)
		{
			Overlay->NumLanesPerSideForward = Current.NumLanesPerSideForward;
			Overlay->NumLanesPerSideBackward = Current.NumLanesPerSideBackward;
			Overlay->LaneWidthCm = Current.LaneWidthCm;
			Overlay->CenterlineOffsetCm = Current.CenterlineOffsetCm;
			Overlay->Editor_RebuildFromCachedCenterline();
		}

		Subsys->Editor_BakeCalibrationForActiveFamily();
		Subsys->ResetRoadLab();

		(void)ApplyCalibrationToRoadFamilySettings(Current, /*FamilyIndex=*/0);

		Subsys->DoBuild();

		UTrafficNetworkAsset* Net = FindBuiltNetworkAsset(World);
		if (!Net || Net->Network.Lanes.Num() == 0)
		{
			if (Test)
			{
				Test->AddError(TEXT("No built traffic network available for alignment evaluation."));
			}
			return false;
		}

		FAlignmentMetrics Metrics;
		if (!EvaluateNetworkLaneAlignment(World, Net->Network, EvalParams, Metrics))
		{
			if (Test)
			{
				Test->AddError(TEXT("EvaluateNetworkLaneAlignment failed."));
			}
			return false;
		}

		LogIterationMetrics(MetricPrefix, Iter, Metrics, Current);

		FString FailureReason;
		if (AlignmentMeetsThresholds(Metrics, Thresholds, &FailureReason))
		{
			UTrafficAutomationLogger::LogMetric(TEXT("Calibration.Pass"), TEXT("true"));
			OutFinalMetrics = Metrics;
			OutFinalCalibration = Current;
			return true;
		}

		UTrafficAutomationLogger::LogMetric(TEXT("Calibration.Pass"), TEXT("false"));
		UTrafficAutomationLogger::LogMetric(TEXT("Calibration.FailReason"), FailureReason);

		if (Iter < MaxIterations)
		{
			Current = ComputeNextCalibration(Current, Metrics);
		}
		else
		{
			OutFinalMetrics = Metrics;
			OutFinalCalibration = Current;
		}
	}

	return false;
}
