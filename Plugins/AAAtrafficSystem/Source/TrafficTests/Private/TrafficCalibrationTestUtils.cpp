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
#include "Components/DynamicMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "VectorTypes.h"
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

	struct FPolylineSampleCache
	{
		TArray<float> CumulativeS;
		float TotalLengthCm = 0.f;
	};

	static void BuildPolylineCumulativeDistances(const TArray<FVector>& Points, FPolylineSampleCache& OutCache)
	{
		OutCache.CumulativeS.Reset();
		OutCache.TotalLengthCm = 0.f;

		const int32 NumPts = Points.Num();
		if (NumPts < 2)
		{
			return;
		}

		OutCache.CumulativeS.SetNum(NumPts);
		OutCache.CumulativeS[0] = 0.f;

		float Accum = 0.f;
		for (int32 i = 1; i < NumPts; ++i)
		{
			Accum += FVector::Dist(Points[i - 1], Points[i]);
			OutCache.CumulativeS[i] = Accum;
		}
		OutCache.TotalLengthCm = Accum;
	}

	static bool SamplePolylineAtS(
		const TArray<FVector>& Points,
		const FPolylineSampleCache& Cache,
		float S,
		FVector& OutPos,
		FVector& OutTan)
	{
		const int32 NumPts = Points.Num();
		if (NumPts < 2 || Cache.CumulativeS.Num() != NumPts)
		{
			return false;
		}

		if (Cache.TotalLengthCm <= KINDA_SMALL_NUMBER)
		{
			OutPos = Points[0];
			OutTan = (Points[1] - Points[0]).GetSafeNormal2D();
			if (OutTan.IsNearlyZero())
			{
				OutTan = FVector::ForwardVector;
			}
			return true;
		}

		const float ClampedS = FMath::Clamp(S, 0.f, Cache.TotalLengthCm);

		int32 SegmentIdx = 0;
		for (int32 i = 0; i + 1 < Cache.CumulativeS.Num(); ++i)
		{
			if (ClampedS >= Cache.CumulativeS[i] && ClampedS <= Cache.CumulativeS[i + 1])
			{
				SegmentIdx = i;
				break;
			}
		}

		const FVector A = Points[SegmentIdx];
		const FVector B = Points[SegmentIdx + 1];
		const float SegStartS = Cache.CumulativeS[SegmentIdx];
		const float SegLen = FVector::Dist(A, B);
		const float T = (SegLen > KINDA_SMALL_NUMBER) ? (ClampedS - SegStartS) / SegLen : 0.f;

		OutPos = FMath::Lerp(A, B, T);
		OutTan = (B - A).GetSafeNormal2D();
		if (OutTan.IsNearlyZero())
		{
			OutTan = FVector::ForwardVector;
		}
		return true;
	}

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

	static bool CollectDynamicMeshVertices(AActor* RoadActor, TArray<FVector>& OutVerts)
	{
		OutVerts.Reset();
		if (!RoadActor)
		{
			return false;
		}

		TArray<UDynamicMeshComponent*> DynMeshComps;
		RoadActor->GetComponents<UDynamicMeshComponent>(DynMeshComps);
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

			OutVerts.Reserve(OutVerts.Num() + Mesh->VertexCount());
			for (int32 Vid : Mesh->VertexIndicesItr())
			{
				const FVector3d PosD = Mesh->GetVertex(Vid);
				OutVerts.Add(Xform.TransformPosition(static_cast<FVector>(PosD)));
			}
		}

		return OutVerts.Num() > 0;
	}

	static bool ComputeLateralBoundsFromVertices(
		const TArray<FVector>& RoadVerts,
		const FVector& RoadPos,
		const FVector& RoadTangent,
		const FVector& RoadRight,
		float HalfLength,
		int32 MinVerts,
		float& OutMinLateral,
		float& OutMaxLateral)
	{
		OutMinLateral = FLT_MAX;
		OutMaxLateral = -FLT_MAX;

		int32 InSlice = 0;
		for (const FVector& V : RoadVerts)
		{
			const FVector Delta = V - RoadPos;
			const float Along = FVector::DotProduct(Delta, RoadTangent);
			if (FMath::Abs(Along) > HalfLength)
			{
				continue;
			}

			const float Lateral = FVector::DotProduct(Delta, RoadRight);
			OutMinLateral = FMath::Min(OutMinLateral, Lateral);
			OutMaxLateral = FMath::Max(OutMaxLateral, Lateral);
			++InSlice;
		}

		return (InSlice >= MinVerts) && (OutMinLateral < OutMaxLateral);
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

	struct FSampleDebug
	{
		int32 RoadId = INDEX_NONE;
		int32 LaneId = INDEX_NONE;
		ELaneDirection LaneDirection = ELaneDirection::Forward;
		int32 SideIndex = 0;
		FString ActorName;

		int32 SampleIndex = INDEX_NONE;
		float LaneS = 0.f;
		float LaneLengthCm = 0.f;
		float RoadS = 0.f;
		float RoadLengthCm = 0.f;

		FVector LanePos = FVector::ZeroVector;
		FVector LaneTan = FVector::ForwardVector;
		FVector RoadPos = FVector::ZeroVector;
		FVector RoadTan = FVector::ForwardVector;

		bool bHasSurface = false;
		bool bUsedVertexBounds = false;
		float MinLat = 0.f;
		float MaxLat = 0.f;
		float LaneLat = 0.f;

		float DevCm = 0.f;
		float HeadingDeg = 0.f;

		int32 LaneNumPoints = 0;
		int32 RoadNumPoints = 0;
		float LaneMaxSegLenCm = 0.f;
		float RoadMaxSegLenCm = 0.f;
	};

	auto LaneDirToString = [](ELaneDirection Dir) -> const TCHAR*
	{
		switch (Dir)
		{
		case ELaneDirection::Forward:
			return TEXT("Forward");
		case ELaneDirection::Backward:
			return TEXT("Backward");
		case ELaneDirection::Bidirectional:
			return TEXT("Bidirectional");
		default:
			return TEXT("Unknown");
		}
	};

	auto ComputeMaxSegLen2D = [](const TArray<FVector>& Points) -> float
	{
		float MaxLen = 0.f;
		for (int32 i = 1; i < Points.Num(); ++i)
		{
			MaxLen = FMath::Max(MaxLen, FVector::Dist2D(Points[i - 1], Points[i]));
		}
		return MaxLen;
	};

	TMap<int32, const FTrafficRoad*> RoadById;
	TMap<int32, FPolylineSampleCache> RoadSampleCacheById;
	for (const FTrafficRoad& Road : Network.Roads)
	{
		RoadById.Add(Road.RoadId, &Road);

		FPolylineSampleCache Cache;
		BuildPolylineCumulativeDistances(Road.CenterlinePoints, Cache);
		RoadSampleCacheById.Add(Road.RoadId, MoveTemp(Cache));
	}

	TMap<AActor*, TArray<FVector>> DynamicVertsByActor;

	double SumDev = 0.0;
	double SumOutward = 0.0;
	float MaxDev = 0.f;
	float MaxHeading = 0.f;
	FSampleDebug WorstDevSample;
	FSampleDebug WorstHeadingSample;
	WorstDevSample.DevCm = -1.f;
	WorstHeadingSample.HeadingDeg = -1.f;

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

		const FPolylineSampleCache* RoadCache = RoadSampleCacheById.Find(Lane.RoadId);
		if (!RoadCache || RoadCache->TotalLengthCm <= KINDA_SMALL_NUMBER)
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

			const float LaneT = (LaneLength > KINDA_SMALL_NUMBER) ? (S / LaneLength) : 0.f;
			const float RoadS = LaneT * RoadCache->TotalLengthCm;

			FVector RoadPos = FVector::ZeroVector;
			FVector RoadTan = FVector::ForwardVector;
			if (!SamplePolylineAtS(Road->CenterlinePoints, *RoadCache, RoadS, RoadPos, RoadTan))
			{
				continue;
			}

			const FVector RoadRight = FVector::CrossProduct(FVector::UpVector, RoadTan).GetSafeNormal();

			const FVector LaneTan2D = FVector(LaneTangent.X, LaneTangent.Y, 0.f).GetSafeNormal();
			const float HeadingDot = FMath::Abs(FVector::DotProduct(LaneTan2D, RoadTan));
			const float HeadingDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(HeadingDot, -1.f, 1.f)));
			MaxHeading = FMath::Max(MaxHeading, HeadingDeg);

			++TotalSamples;

			float MinLat = 0.f;
			float MaxLat = 0.f;
			bool bHasSurface = false;
			bool bUsedVertexBounds = false;
			float DevCm = 0.f;
			float OutwardErrCm = 0.f;
			float LaneLat = 0.f;

			if (TArray<FVector>* CachedVerts = DynamicVertsByActor.Find(RoadActor))
			{
				bHasSurface = ComputeLateralBoundsFromVertices(
					*CachedVerts,
					RoadPos,
					RoadTan,
					RoadRight,
					Params.CrossSectionHalfLengthCm,
					Params.MinVertsPerCrossSection,
					MinLat,
					MaxLat);
				bUsedVertexBounds = bHasSurface;
			}
			else
			{
				TArray<FVector> Verts;
				CollectDynamicMeshVertices(RoadActor, Verts);
				DynamicVertsByActor.Add(RoadActor, MoveTemp(Verts));
				if (TArray<FVector>* NewVerts = DynamicVertsByActor.Find(RoadActor))
				{
					bHasSurface = ComputeLateralBoundsFromVertices(
						*NewVerts,
						RoadPos,
						RoadTan,
						RoadRight,
						Params.CrossSectionHalfLengthCm,
						Params.MinVertsPerCrossSection,
						MinLat,
						MaxLat);
					bUsedVertexBounds = bHasSurface;
				}
			}

			if (bHasSurface)
			{
				++WithSurface;

				LaneLat = FVector::DotProduct(LanePos - RoadPos, RoadRight);
				const float ClampedLat = FMath::Clamp(LaneLat, MinLat, MaxLat);
				DevCm = FMath::Abs(LaneLat - ClampedLat);

				if (LaneLat > MaxLat)
				{
					OutwardErrCm = LaneLat - MaxLat;
				}
				else if (LaneLat < MinLat)
				{
					OutwardErrCm = MinLat - LaneLat;
				}
			}
			else
			{
				FVector SurfacePoint = FVector::ZeroVector;
				bHasSurface = FindClosestPointOnRoadSurface(World, RoadActor, LanePos, Params, SurfacePoint);
				if (bHasSurface)
				{
					++WithSurface;
					DevCm = FVector::Dist2D(LanePos, SurfacePoint);

					const float SignedLaneOffsetFromCenter = FVector::DotProduct(LanePos - RoadPos, RoadRight);
					const float SideSign = (SignedLaneOffsetFromCenter >= 0.f) ? 1.f : -1.f;
					const float SignedSurfaceError = FVector::DotProduct(LanePos - SurfacePoint, RoadRight);
					OutwardErrCm = SignedSurfaceError * SideSign;
					if (OutwardErrCm < 0.f)
					{
						OutwardErrCm = 0.f;
					}
				}
			}

			if (!bHasSurface)
			{
				continue;
			}

			SumDev += DevCm;
			MaxDev = FMath::Max(MaxDev, DevCm);
			if (DevCm > WorstDevSample.DevCm)
			{
				WorstDevSample.RoadId = Road->RoadId;
				WorstDevSample.LaneId = Lane.LaneId;
				WorstDevSample.LaneDirection = Lane.Direction;
				WorstDevSample.SideIndex = Lane.SideIndex;
				WorstDevSample.ActorName = RoadActor ? RoadActor->GetName() : TEXT("None");
				WorstDevSample.SampleIndex = i;
				WorstDevSample.LaneS = S;
				WorstDevSample.LaneLengthCm = LaneLength;
				WorstDevSample.RoadS = RoadS;
				WorstDevSample.RoadLengthCm = RoadCache->TotalLengthCm;
				WorstDevSample.LanePos = LanePos;
				WorstDevSample.LaneTan = LaneTangent;
				WorstDevSample.RoadPos = RoadPos;
				WorstDevSample.RoadTan = RoadTan;
				WorstDevSample.bHasSurface = bHasSurface;
				WorstDevSample.bUsedVertexBounds = bUsedVertexBounds;
				WorstDevSample.MinLat = MinLat;
				WorstDevSample.MaxLat = MaxLat;
				WorstDevSample.LaneLat = LaneLat;
				WorstDevSample.DevCm = DevCm;
				WorstDevSample.HeadingDeg = HeadingDeg;
				WorstDevSample.LaneNumPoints = Lane.CenterlinePoints.Num();
				WorstDevSample.RoadNumPoints = Road->CenterlinePoints.Num();
				WorstDevSample.LaneMaxSegLenCm = ComputeMaxSegLen2D(Lane.CenterlinePoints);
				WorstDevSample.RoadMaxSegLenCm = ComputeMaxSegLen2D(Road->CenterlinePoints);
			}

			if (HeadingDeg > WorstHeadingSample.HeadingDeg)
			{
				WorstHeadingSample.RoadId = Road->RoadId;
				WorstHeadingSample.LaneId = Lane.LaneId;
				WorstHeadingSample.LaneDirection = Lane.Direction;
				WorstHeadingSample.SideIndex = Lane.SideIndex;
				WorstHeadingSample.ActorName = RoadActor ? RoadActor->GetName() : TEXT("None");
				WorstHeadingSample.SampleIndex = i;
				WorstHeadingSample.LaneS = S;
				WorstHeadingSample.LaneLengthCm = LaneLength;
				WorstHeadingSample.RoadS = RoadS;
				WorstHeadingSample.RoadLengthCm = RoadCache->TotalLengthCm;
				WorstHeadingSample.LanePos = LanePos;
				WorstHeadingSample.LaneTan = LaneTangent;
				WorstHeadingSample.RoadPos = RoadPos;
				WorstHeadingSample.RoadTan = RoadTan;
				WorstHeadingSample.bHasSurface = bHasSurface;
				WorstHeadingSample.bUsedVertexBounds = bUsedVertexBounds;
				WorstHeadingSample.MinLat = MinLat;
				WorstHeadingSample.MaxLat = MaxLat;
				WorstHeadingSample.LaneLat = LaneLat;
				WorstHeadingSample.DevCm = DevCm;
				WorstHeadingSample.HeadingDeg = HeadingDeg;
				WorstHeadingSample.LaneNumPoints = Lane.CenterlinePoints.Num();
				WorstHeadingSample.RoadNumPoints = Road->CenterlinePoints.Num();
				WorstHeadingSample.LaneMaxSegLenCm = ComputeMaxSegLen2D(Lane.CenterlinePoints);
				WorstHeadingSample.RoadMaxSegLenCm = ComputeMaxSegLen2D(Road->CenterlinePoints);
			}
			if (DevCm <= Params.LateralToleranceCm)
			{
				++WithinTol;
			}

			SumOutward += static_cast<double>(OutwardErrCm);

		}
	}

	OutMetrics.TotalSamples = TotalSamples;
	OutMetrics.SamplesWithSurface = WithSurface;
	OutMetrics.SamplesWithinTolerance = WithinTol;
	OutMetrics.MaxHeadingErrorDeg = MaxHeading;
	OutMetrics.MaxLateralDeviationCm = MaxDev;

	if (WithSurface > 0)
	{
		OutMetrics.MeanLateralDeviationCm = static_cast<float>(SumDev / static_cast<double>(WithSurface));
	}

	if (TotalSamples > 0)
	{
		OutMetrics.CoveragePercent = 100.0f * static_cast<float>(WithinTol) / static_cast<float>(TotalSamples);
	}

	if (WithSurface > 0)
	{
		OutMetrics.MeanOutwardSignedErrorCm = static_cast<float>(SumOutward / static_cast<double>(WithSurface));
	}

	if (WorstDevSample.DevCm >= 0.f)
	{
		UTrafficAutomationLogger::LogLine(FString::Printf(
			TEXT("Alignment.WorstDev=DevCm=%.2f HeadingDeg=%.2f RoadId=%d LaneId=%d LaneDir=%s SideIndex=%d Actor=%s Sample=%d LaneS=%.1f/%.1f RoadS=%.1f/%.1f UsedVertexBounds=%d MinLat=%.1f MaxLat=%.1f LaneLat=%.1f LanePts=%d RoadPts=%d LaneMaxSegCm=%.1f RoadMaxSegCm=%.1f LanePos=%.0f,%.0f RoadPos=%.0f,%.0f"),
			WorstDevSample.DevCm,
			WorstDevSample.HeadingDeg,
			WorstDevSample.RoadId,
			WorstDevSample.LaneId,
			LaneDirToString(WorstDevSample.LaneDirection),
			WorstDevSample.SideIndex,
			*WorstDevSample.ActorName,
			WorstDevSample.SampleIndex,
			WorstDevSample.LaneS,
			WorstDevSample.LaneLengthCm,
			WorstDevSample.RoadS,
			WorstDevSample.RoadLengthCm,
			WorstDevSample.bUsedVertexBounds ? 1 : 0,
			WorstDevSample.MinLat,
			WorstDevSample.MaxLat,
			WorstDevSample.LaneLat,
			WorstDevSample.LaneNumPoints,
			WorstDevSample.RoadNumPoints,
			WorstDevSample.LaneMaxSegLenCm,
			WorstDevSample.RoadMaxSegLenCm,
			WorstDevSample.LanePos.X,
			WorstDevSample.LanePos.Y,
			WorstDevSample.RoadPos.X,
			WorstDevSample.RoadPos.Y));
	}

	if (WorstHeadingSample.HeadingDeg >= 0.f)
	{
		UTrafficAutomationLogger::LogLine(FString::Printf(
			TEXT("Alignment.WorstHeading=HeadingDeg=%.2f DevCm=%.2f RoadId=%d LaneId=%d LaneDir=%s SideIndex=%d Actor=%s Sample=%d LaneS=%.1f/%.1f RoadS=%.1f/%.1f UsedVertexBounds=%d MinLat=%.1f MaxLat=%.1f LaneLat=%.1f LanePts=%d RoadPts=%d LaneMaxSegCm=%.1f RoadMaxSegCm=%.1f LaneTan=%.2f,%.2f RoadTan=%.2f,%.2f LanePos=%.0f,%.0f RoadPos=%.0f,%.0f"),
			WorstHeadingSample.HeadingDeg,
			WorstHeadingSample.DevCm,
			WorstHeadingSample.RoadId,
			WorstHeadingSample.LaneId,
			LaneDirToString(WorstHeadingSample.LaneDirection),
			WorstHeadingSample.SideIndex,
			*WorstHeadingSample.ActorName,
			WorstHeadingSample.SampleIndex,
			WorstHeadingSample.LaneS,
			WorstHeadingSample.LaneLengthCm,
			WorstHeadingSample.RoadS,
			WorstHeadingSample.RoadLengthCm,
			WorstHeadingSample.bUsedVertexBounds ? 1 : 0,
			WorstHeadingSample.MinLat,
			WorstHeadingSample.MaxLat,
			WorstHeadingSample.LaneLat,
			WorstHeadingSample.LaneNumPoints,
			WorstHeadingSample.RoadNumPoints,
			WorstHeadingSample.LaneMaxSegLenCm,
			WorstHeadingSample.RoadMaxSegLenCm,
			WorstHeadingSample.LaneTan.X,
			WorstHeadingSample.LaneTan.Y,
			WorstHeadingSample.RoadTan.X,
			WorstHeadingSample.RoadTan.Y,
			WorstHeadingSample.LanePos.X,
			WorstHeadingSample.LanePos.Y,
			WorstHeadingSample.RoadPos.X,
			WorstHeadingSample.RoadPos.Y));
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
	if (Metrics.SamplesWithSurface <= 0)
	{
		return Fail(TEXT("NoSurfaceSamples"));
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

	const float TargetMeanDevCm = 10.0f;
	const float TargetMaxDevCm = 20.0f;

	const float ExcessMean = FMath::Max(0.f, Metrics.MeanLateralDeviationCm - TargetMeanDevCm);
	const float ExcessMax = FMath::Max(0.f, Metrics.MaxLateralDeviationCm - TargetMaxDevCm);

	// Drive updates using the worst-case deviation to avoid "good mean but bad outliers" situations.
	const float Drive = FMath::Max3(Metrics.MeanOutwardSignedErrorCm, ExcessMean, ExcessMax);

	// Shrink lane envelope more aggressively when outliers exceed tolerance.
	const float OffsetStep = FMath::Clamp(Drive * 0.25f, -MaxStepOffsetCm, MaxStepOffsetCm);
	const float WidthStep = FMath::Clamp(Drive * 0.10f, -MaxStepWidthCm, MaxStepWidthCm);

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
