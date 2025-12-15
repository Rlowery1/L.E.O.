#include "TrafficNetworkBuilder.h"
#include "TrafficGeometryProvider.h"
#include "TrafficLaneGeometry.h"
#include "TrafficMovementGeometry.h"
#include "TrafficRuntimeModule.h"
#include "HAL/IConsoleManager.h"

namespace
{
	static TAutoConsoleVariable<float> CVarTrafficIntersectionSnapToLaneRadiusCm(
		TEXT("aaa.Traffic.Intersections.SnapToLaneRadiusCm"),
		350.f,
		TEXT("If >0, unconnected lane endpoints within this radius (cm) of another lane's interior will snap by splitting that lane to form a merge/diverge intersection. Default: 350."),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarTrafficIntersectionSnapMinDistanceFromLaneEndsCm(
		TEXT("aaa.Traffic.Intersections.SnapMinDistanceFromLaneEndsCm"),
		300.f,
		TEXT("Minimum distance (cm) from a lane start/end for an interior snap split point. Default: 300."),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarTrafficIntersectionSnapMinAlignmentDot(
		TEXT("aaa.Traffic.Intersections.SnapMinAlignmentDot"),
		0.35f,
		TEXT("Minimum dot product between endpoint direction and target lane tangent when snapping endpoints to lane interiors. Default: 0.35."),
		ECVF_Default);

	struct FPolylineClosestPoint
	{
		bool bValid = false;
		float DistanceSq = TNumericLimits<float>::Max();
		float SAlongCm = 0.f;
		FVector ClosestPoint = FVector::ZeroVector;
		FVector Tangent = FVector::ForwardVector;
	};

	static FPolylineClosestPoint FindClosestPointOnPolyline(const TArray<FVector>& Points, const FVector& Query)
	{
		FPolylineClosestPoint Result;
		if (Points.Num() < 2)
		{
			return Result;
		}

		float AccumulatedS = 0.f;
		for (int32 i = 0; i < Points.Num() - 1; ++i)
		{
			const FVector A = Points[i];
			const FVector B = Points[i + 1];
			const FVector AB = B - A;
			const float SegLen = AB.Size();
			if (SegLen <= KINDA_SMALL_NUMBER)
			{
				continue;
			}

			const float T = FMath::Clamp(FVector::DotProduct(Query - A, AB) / FMath::Square(SegLen), 0.f, 1.f);
			const FVector Closest = A + AB * T;
			const float DistSq = FVector::DistSquared(Query, Closest);
			if (DistSq < Result.DistanceSq)
			{
				Result.bValid = true;
				Result.DistanceSq = DistSq;
				Result.ClosestPoint = Closest;
				Result.SAlongCm = AccumulatedS + (SegLen * T);
				Result.Tangent = AB.GetSafeNormal();
			}

			AccumulatedS += SegLen;
		}

		return Result;
	}

	static float ComputePolylineLengthCm(const TArray<FVector>& Points)
	{
		if (Points.Num() < 2)
		{
			return 0.f;
		}

		float Len = 0.f;
		for (int32 i = 0; i < Points.Num() - 1; ++i)
		{
			Len += FVector::Dist(Points[i], Points[i + 1]);
		}
		return Len;
	}

	static bool SplitPolylineAtS(const TArray<FVector>& Points, float SplitS, TArray<FVector>& OutA, TArray<FVector>& OutB)
	{
		OutA.Reset();
		OutB.Reset();

		if (Points.Num() < 2)
		{
			return false;
		}

		const float TotalLen = ComputePolylineLengthCm(Points);
		if (TotalLen <= KINDA_SMALL_NUMBER)
		{
			return false;
		}

		float RemainingS = FMath::Clamp(SplitS, 0.f, TotalLen);
		float Accumulated = 0.f;

		for (int32 i = 0; i < Points.Num() - 1; ++i)
		{
			const FVector A = Points[i];
			const FVector B = Points[i + 1];
			const float SegLen = FVector::Dist(A, B);
			if (SegLen <= KINDA_SMALL_NUMBER)
			{
				continue;
			}

			if (Accumulated + SegLen >= RemainingS)
			{
				const float T = (RemainingS - Accumulated) / SegLen;
				const FVector SplitPoint = FMath::Lerp(A, B, T);

				OutA.Append(Points.GetData(), i + 1);
				OutA.Add(SplitPoint);

				OutB.Add(SplitPoint);
				for (int32 j = i + 1; j < Points.Num(); ++j)
				{
					OutB.Add(Points[j]);
				}

				return OutA.Num() >= 2 && OutB.Num() >= 2;
			}

			Accumulated += SegLen;
		}

		return false;
	}

	struct FLaneSplitRequest
	{
		int32 LaneId = INDEX_NONE;
		float SplitSAlongCm = 0.f;
	};

	static void UniqueSortSplits(TArray<float>& SplitsCm, float ToleranceCm)
	{
		SplitsCm.Sort();
		TArray<float> Unique;
		for (float Value : SplitsCm)
		{
			if (Unique.Num() == 0 || FMath::Abs(Value - Unique.Last()) > ToleranceCm)
			{
				Unique.Add(Value);
			}
		}
		SplitsCm = MoveTemp(Unique);
	}
}

void FTrafficNetworkBuilder::BuildNetworkFromRoads(
	const TArray<FTrafficRoad>& InputRoads,
	const UTrafficRoadFamilySettings* FamilySettings,
	FTrafficNetwork& OutNetwork)
{
	OutNetwork = FTrafficNetwork();

	if (!FamilySettings)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[FTrafficNetworkBuilder] No UTrafficRoadFamilySettings provided."));
		return;
	}

	const TArray<FRoadFamilyDefinition>& Families = FamilySettings->Families;
	if (Families.Num() == 0)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[FTrafficNetworkBuilder] No road families configured."));
		return;
	}

	OutNetwork.Roads = InputRoads;
	OutNetwork.Lanes.Reset();

	int32 NextLaneId = 0;

	for (FTrafficRoad& Road : OutNetwork.Roads)
	{
		const int32 FamilyIndex = (Road.FamilyId >= 0 && Road.FamilyId < Families.Num())
			? Road.FamilyId
			: 0;
		const FRoadFamilyDefinition& Family = Families[FamilyIndex];

		TArray<FTrafficLane> RoadLanes;
		GenerateLanesForRoad(Road, Family, NextLaneId, RoadLanes);

		OutNetwork.Lanes.Append(RoadLanes);
		Road.Lanes = RoadLanes;

		UE_LOG(LogTraffic, Log,
			TEXT("[FTrafficNetworkBuilder] RoadId=%d Family=%s Lanes=%d"),
			Road.RoadId,
			*Family.FamilyName.ToString(),
			RoadLanes.Num());
	}

	BuildIntersectionsAndMovements(OutNetwork, 500.0f);
}

void FTrafficNetworkBuilder::BuildNetworkFromWorld(
	UWorld* World,
	ITrafficRoadGeometryProvider& GeometryProvider,
	const UTrafficRoadFamilySettings* FamilySettings,
	FTrafficNetwork& OutNetwork)
{
	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[FTrafficNetworkBuilder] BuildNetworkFromWorld: World is null."));
		OutNetwork = FTrafficNetwork();
		return;
	}

	TArray<FTrafficRoad> Roads;
	GeometryProvider.CollectRoads(World, Roads);
	UE_LOG(LogTraffic, Log, TEXT("[FTrafficNetworkBuilder] BuildNetworkFromWorld: Collected %d roads."), Roads.Num());

	BuildNetworkFromRoads(Roads, FamilySettings, OutNetwork);
}

void FTrafficNetworkBuilder::GenerateLanesForRoad(
	const FTrafficRoad& SourceRoad,
	const FRoadFamilyDefinition& Family,
	int32& InOutNextLaneId,
	TArray<FTrafficLane>& OutLanes)
{
	OutLanes.Reset();

	const TArray<FVector>& Centerline = SourceRoad.CenterlinePoints;
	const int32 NumPoints = Centerline.Num();
	if (NumPoints < 2)
	{
		return;
	}

	auto GenerateSide = [&](
		const FTrafficLaneLayoutSide& Side,
		bool bForwardDirection,
		ELaneDirection LaneDirectionTag)
	{
		if (Side.NumLanes <= 0)
		{
			return;
		}

		const float InnerOffset = Side.InnerLaneCenterOffsetCm;
		const float LaneWidth = Side.LaneWidthCm;

		// Keep lane points ordered in the lane's travel direction.
		// Forward lanes travel along the road's centerline order; backward lanes travel in reverse.
		const bool bReverseAlongCenterline = (LaneDirectionTag == ELaneDirection::Backward);

		for (int32 LaneIndex = 0; LaneIndex < Side.NumLanes; ++LaneIndex)
		{
			const float Adjustment = Side.LaneCenterOffsetAdjustmentsCm.IsValidIndex(LaneIndex)
				? Side.LaneCenterOffsetAdjustmentsCm[LaneIndex]
				: 0.f;
			const float CenterOffset = InnerOffset + LaneIndex * LaneWidth + Adjustment;
			const float LateralOffset = CenterOffset;

			FTrafficLane Lane;
			Lane.LaneId = InOutNextLaneId++;
			Lane.RoadId = SourceRoad.RoadId;
			Lane.SideIndex = bForwardDirection ? 0 : 1;
			Lane.Width = LaneWidth;
			Lane.Direction = LaneDirectionTag;
			Lane.SpeedLimitKmh = Family.DefaultSpeedLimitKmh;
			Lane.CenterlinePoints.Reserve(NumPoints);

			for (int32 i = 0; i < NumPoints; ++i)
			{
				const int32 SrcIndex = bReverseAlongCenterline ? (NumPoints - 1 - i) : i;
				const FVector Pos = Centerline[SrcIndex];

				FVector Tangent;
				if (i < NumPoints - 1)
				{
					const int32 SrcNextIndex = bReverseAlongCenterline ? (NumPoints - 1 - (i + 1)) : (i + 1);
					Tangent = (Centerline[SrcNextIndex] - Centerline[SrcIndex]).GetSafeNormal();
				}
				else
				{
					const int32 SrcPrevIndex = bReverseAlongCenterline ? (NumPoints - 1 - (i - 1)) : (i - 1);
					Tangent = (Centerline[SrcIndex] - Centerline[SrcPrevIndex]).GetSafeNormal();
				}

				const FVector UpVector = FVector::UpVector;
				const FVector RightVector = FVector::CrossProduct(UpVector, Tangent).GetSafeNormal();
				const FVector OffsetPos = Pos + RightVector * LateralOffset;

				Lane.CenterlinePoints.Add(OffsetPos);
			}

			OutLanes.Add(Lane);
		}
	};

	GenerateSide(Family.Forward, true, ELaneDirection::Forward);
	GenerateSide(Family.Backward, false, ELaneDirection::Backward);
}

void FTrafficNetworkBuilder::BuildIntersectionsAndMovements(
	FTrafficNetwork& OutNetwork,
	float ClusterRadiusCm)
{
	OutNetwork.Intersections.Reset();
	OutNetwork.Movements.Reset();

	auto BuildEndpoints = [](const TArray<FTrafficLane>& Lanes, TArray<FLaneEndpoint>& OutEndpoints)
	{
		OutEndpoints.Reset();
		for (const FTrafficLane& Lane : Lanes)
		{
			const int32 NumPts = Lane.CenterlinePoints.Num();
			if (NumPts < 2)
			{
				continue;
			}

			FLaneEndpoint StartEp;
			StartEp.LaneId = Lane.LaneId;
			StartEp.Position = Lane.CenterlinePoints[0];
			StartEp.Direction = (Lane.CenterlinePoints[1] - Lane.CenterlinePoints[0]).GetSafeNormal();
			StartEp.bIsStart = true;
			OutEndpoints.Add(StartEp);

			FLaneEndpoint EndEp;
			EndEp.LaneId = Lane.LaneId;
			EndEp.Position = Lane.CenterlinePoints[NumPts - 1];
			EndEp.Direction = (Lane.CenterlinePoints[NumPts - 1] - Lane.CenterlinePoints[NumPts - 2]).GetSafeNormal();
			EndEp.bIsStart = false;
			OutEndpoints.Add(EndEp);
		}
	};

	TArray<FLaneEndpoint> AllEndpoints;
	BuildEndpoints(OutNetwork.Lanes, AllEndpoints);

	// CityBLD modular roads can "merge" ramps into the middle of a highway spline (no matching endpoints),
	// which looks fine visually but results in no intersection in our endpoint-only graph.
	// To support this, snap unconnected endpoints to the interior of the closest aligned lane by splitting that lane.
	const float SnapRadius = CVarTrafficIntersectionSnapToLaneRadiusCm.GetValueOnGameThread();
	if (SnapRadius > KINDA_SMALL_NUMBER)
	{
		TArray<bool> HasNeighbor;
		HasNeighbor.Init(false, AllEndpoints.Num());
		for (int32 i = 0; i < AllEndpoints.Num(); ++i)
		{
			for (int32 j = i + 1; j < AllEndpoints.Num(); ++j)
			{
				if (FVector::Dist(AllEndpoints[i].Position, AllEndpoints[j].Position) <= ClusterRadiusCm)
				{
					HasNeighbor[i] = true;
					HasNeighbor[j] = true;
				}
			}
		}

		TMap<int32, TArray<float>> SplitsByLaneId;
		const float MinFromEnds = FMath::Max(0.f, CVarTrafficIntersectionSnapMinDistanceFromLaneEndsCm.GetValueOnGameThread());
		const float MinDot = FMath::Clamp(CVarTrafficIntersectionSnapMinAlignmentDot.GetValueOnGameThread(), -1.f, 1.f);
		const float SnapRadiusSq = FMath::Square(SnapRadius);

		for (int32 EndpointIndex = 0; EndpointIndex < AllEndpoints.Num(); ++EndpointIndex)
		{
			if (HasNeighbor[EndpointIndex])
			{
				continue;
			}

			const FLaneEndpoint& Ep = AllEndpoints[EndpointIndex];
			const FTrafficLane* SourceLane = nullptr;
			for (const FTrafficLane& L : OutNetwork.Lanes)
			{
				if (L.LaneId == Ep.LaneId)
				{
					SourceLane = &L;
					break;
				}
			}

			FPolylineClosestPoint Best;
			int32 BestLaneId = INDEX_NONE;

			for (const FTrafficLane& Candidate : OutNetwork.Lanes)
			{
				if (Candidate.LaneId == Ep.LaneId)
				{
					continue;
				}
				if (SourceLane && Candidate.RoadId == SourceLane->RoadId)
				{
					continue;
				}

				const FPolylineClosestPoint Closest = FindClosestPointOnPolyline(Candidate.CenterlinePoints, Ep.Position);
				if (!Closest.bValid || Closest.DistanceSq > SnapRadiusSq)
				{
					continue;
				}

				const float LaneLen = TrafficLaneGeometry::ComputeLaneLengthCm(Candidate);
				if (Closest.SAlongCm < MinFromEnds || Closest.SAlongCm > (LaneLen - MinFromEnds))
				{
					continue;
				}

				const float Dot = FVector::DotProduct(Ep.Direction.GetSafeNormal(), Closest.Tangent.GetSafeNormal());
				if (Dot < MinDot)
				{
					continue;
				}

				// Prefer closest distance, then better alignment.
				const float Score = Closest.DistanceSq - (Dot * 10000.f);
				const float BestScore = Best.bValid ? (Best.DistanceSq - (FVector::DotProduct(Ep.Direction.GetSafeNormal(), Best.Tangent.GetSafeNormal()) * 10000.f)) : TNumericLimits<float>::Max();
				if (!Best.bValid || Score < BestScore)
				{
					Best = Closest;
					BestLaneId = Candidate.LaneId;
				}
			}

			if (BestLaneId != INDEX_NONE && Best.bValid)
			{
				SplitsByLaneId.FindOrAdd(BestLaneId).Add(Best.SAlongCm);
			}
		}

		int32 NextLaneId = 0;
		for (const FTrafficLane& L : OutNetwork.Lanes)
		{
			NextLaneId = FMath::Max(NextLaneId, (L.LaneId >= 0) ? (L.LaneId + 1) : NextLaneId);
		}

		int32 NumSplitsApplied = 0;
		if (SplitsByLaneId.Num() > 0)
		{
			TArray<FTrafficLane> NewLanes;
			NewLanes.Reserve(OutNetwork.Lanes.Num() + SplitsByLaneId.Num());

			for (const FTrafficLane& Lane : OutNetwork.Lanes)
			{
				const TArray<float>* SplitsPtr = SplitsByLaneId.Find(Lane.LaneId);
				if (!SplitsPtr || SplitsPtr->Num() == 0)
				{
					NewLanes.Add(Lane);
					continue;
				}

				TArray<float> Splits = *SplitsPtr;
				UniqueSortSplits(Splits, /*ToleranceCm=*/75.f);

				TArray<FVector> CurrentPoints = Lane.CenterlinePoints;
				float ConsumedS = 0.f;
				int32 SegmentId = Lane.LaneId;
				int32 PrevId = Lane.PrevLaneId;

				for (int32 SplitIndex = 0; SplitIndex < Splits.Num(); ++SplitIndex)
				{
					const float SplitS = Splits[SplitIndex];
					const float LocalS = SplitS - ConsumedS;
					TArray<FVector> A, B;
					if (!SplitPolylineAtS(CurrentPoints, LocalS, A, B))
					{
						continue;
					}

					const int32 NextId = NextLaneId++;

					FTrafficLane Segment = Lane;
					Segment.LaneId = SegmentId;
					Segment.CenterlinePoints = MoveTemp(A);
					Segment.PrevLaneId = PrevId;
					Segment.NextLaneId = NextId;
					NewLanes.Add(MoveTemp(Segment));

					PrevId = SegmentId;
					SegmentId = NextId;
					CurrentPoints = MoveTemp(B);
					ConsumedS = SplitS;
					++NumSplitsApplied;
				}

				FTrafficLane Tail = Lane;
				Tail.LaneId = SegmentId;
				Tail.CenterlinePoints = MoveTemp(CurrentPoints);
				Tail.PrevLaneId = PrevId;
				Tail.NextLaneId = Lane.NextLaneId;
				NewLanes.Add(MoveTemp(Tail));
			}

			if (NumSplitsApplied > 0)
			{
				OutNetwork.Lanes = MoveTemp(NewLanes);
				for (FTrafficRoad& Road : OutNetwork.Roads)
				{
					Road.Lanes.Reset();
					for (const FTrafficLane& Lane : OutNetwork.Lanes)
					{
						if (Lane.RoadId == Road.RoadId)
						{
							Road.Lanes.Add(Lane);
						}
					}
				}

				BuildEndpoints(OutNetwork.Lanes, AllEndpoints);
				UE_LOG(LogTraffic, Log,
					TEXT("[FTrafficNetworkBuilder] Snap-to-lane: applied %d lane split(s) to support mid-spline merges/diverges."),
					NumSplitsApplied);
			}
		}
	}

	TArray<FEndpointCluster> Clusters;
	TArray<bool> Assigned;
	Assigned.SetNumZeroed(AllEndpoints.Num());

	for (int32 i = 0; i < AllEndpoints.Num(); ++i)
	{
		if (Assigned[i])
		{
			continue;
		}

		FEndpointCluster Cluster;
		Cluster.Center = AllEndpoints[i].Position;
		Cluster.Endpoints.Add(AllEndpoints[i]);
		Assigned[i] = true;

		for (int32 j = i + 1; j < AllEndpoints.Num(); ++j)
		{
			if (Assigned[j])
			{
				continue;
			}

			const float Dist = FVector::Dist(Cluster.Center, AllEndpoints[j].Position);
			if (Dist <= ClusterRadiusCm)
			{
				Cluster.Endpoints.Add(AllEndpoints[j]);
				Assigned[j] = true;
			}
		}

		if (Cluster.Endpoints.Num() >= 2)
		{
			FVector Sum = FVector::ZeroVector;
			for (const FLaneEndpoint& Ep : Cluster.Endpoints)
			{
				Sum += Ep.Position;
			}
			Cluster.Center = Sum / static_cast<float>(Cluster.Endpoints.Num());
			Clusters.Add(Cluster);
		}
	}

	int32 NextIntersectionId = 0;
	int32 NextMovementId = 0;

	for (const FEndpointCluster& Cluster : Clusters)
	{
		FTrafficIntersection Intersection;
		Intersection.IntersectionId = NextIntersectionId++;
		Intersection.Center = Cluster.Center;

		float MaxDist = 0.0f;
		for (const FLaneEndpoint& Ep : Cluster.Endpoints)
		{
			MaxDist = FMath::Max(MaxDist, FVector::Dist(Cluster.Center, Ep.Position));
		}
		Intersection.Radius = MaxDist;

		TArray<const FLaneEndpoint*> IncomingEndpoints;
		TArray<const FLaneEndpoint*> OutgoingEndpoints;

		for (const FLaneEndpoint& Ep : Cluster.Endpoints)
		{
			const FTrafficLane* Lane = nullptr;
			for (const FTrafficLane& L : OutNetwork.Lanes)
			{
				if (L.LaneId == Ep.LaneId)
				{
					Lane = &L;
					break;
				}
			}

			if (!Lane)
			{
				continue;
			}

			// Lane centerline points are stored in travel order, so an endpoint is incoming when it is the travel end.
			const bool bIsIncoming = !Ep.bIsStart;

			if (bIsIncoming)
			{
				IncomingEndpoints.Add(&Ep);
				Intersection.IncomingLaneIds.Add(Ep.LaneId);
			}
			else
			{
				OutgoingEndpoints.Add(&Ep);
				Intersection.OutgoingLaneIds.Add(Ep.LaneId);
			}
		}

		OutNetwork.Intersections.Add(Intersection);

		auto ComputeTurnType = [](const FVector& InDir, const FVector& OutDir) -> ETrafficTurnType
		{
			const FVector InDirN = InDir.GetSafeNormal();
			const FVector OutDirN = OutDir.GetSafeNormal();
			const float DotProduct = FVector::DotProduct(InDirN, OutDirN);
			const FVector CrossProduct = FVector::CrossProduct(InDirN, OutDirN);

			if (DotProduct > 0.7f)
			{
				return ETrafficTurnType::Through;
			}
			if (DotProduct < -0.7f)
			{
				return ETrafficTurnType::UTurn;
			}
			return (CrossProduct.Z > 0.0f) ? ETrafficTurnType::Left : ETrafficTurnType::Right;
		};

		auto ScoreCandidate = [&](const FLaneEndpoint* InEp, const FLaneEndpoint* OutEp) -> float
		{
			const float DistScore = FVector::Dist(InEp->Position, OutEp->Position) / 100.0f; // meters-ish

			// InEp is an incoming endpoint at the lane's travel end. Its direction already points into the intersection.
			const FVector InDir = InEp->Direction.GetSafeNormal();
			const FVector OutDir = OutEp->Direction.GetSafeNormal();
			const float Dot = FMath::Clamp(FVector::DotProduct(InDir, OutDir), -1.f, 1.f);
			const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));

			const ETrafficTurnType TurnType = ComputeTurnType(InDir, OutDir);
			const float IdealDeg =
				(TurnType == ETrafficTurnType::Through) ? 0.f :
				(TurnType == ETrafficTurnType::Left) ? 90.f :
				(TurnType == ETrafficTurnType::Right) ? 90.f :
				180.f;

			const float AngleScore = FMath::Abs(AngleDeg - IdealDeg) / 15.0f;
			const float UTurnPenalty = (TurnType == ETrafficTurnType::UTurn) ? 50.f : 0.f;
			return DistScore + AngleScore + UTurnPenalty;
		};

		auto AddMovement = [&](const FLaneEndpoint* InEp, const FLaneEndpoint* OutEp, ETrafficTurnType TurnType)
		{
			if (!InEp || !OutEp)
			{
				return;
			}

			FTrafficMovement Movement;
			Movement.MovementId = NextMovementId++;
			Movement.IntersectionId = Intersection.IntersectionId;
			Movement.IncomingLaneId = InEp->LaneId;
			Movement.OutgoingLaneId = OutEp->LaneId;
			Movement.TurnType = TurnType;

			const FVector StartPos = InEp->Position;
			const FVector EndPos = OutEp->Position;
			const FVector InDirNorm = InEp->Direction.GetSafeNormal();
			const FVector OutDirNorm = OutEp->Direction.GetSafeNormal();

			TrafficMovementGeometry::BuildSmoothMovementPath(
				StartPos,
				InDirNorm,
				EndPos,
				OutDirNorm,
				24,
				Movement);

			OutNetwork.Movements.Add(Movement);
		};

		for (const FLaneEndpoint* InEp : IncomingEndpoints)
		{
			const FLaneEndpoint* BestThrough = nullptr;
			const FLaneEndpoint* BestLeft = nullptr;
			const FLaneEndpoint* BestRight = nullptr;

			float BestThroughScore = TNumericLimits<float>::Max();
			float BestLeftScore = TNumericLimits<float>::Max();
			float BestRightScore = TNumericLimits<float>::Max();

			for (const FLaneEndpoint* OutEp : OutgoingEndpoints)
			{
				if (InEp->LaneId == OutEp->LaneId)
				{
					continue;
				}

				const FVector InDirNorm = InEp->Direction.GetSafeNormal();
				const FVector OutDirNorm = OutEp->Direction.GetSafeNormal();
				const ETrafficTurnType TurnType = ComputeTurnType(InDirNorm, OutDirNorm);
				if (TurnType == ETrafficTurnType::UTurn)
				{
					continue;
				}

				const float Score = ScoreCandidate(InEp, OutEp);
				if (TurnType == ETrafficTurnType::Through && Score < BestThroughScore)
				{
					BestThroughScore = Score;
					BestThrough = OutEp;
				}
				else if (TurnType == ETrafficTurnType::Left && Score < BestLeftScore)
				{
					BestLeftScore = Score;
					BestLeft = OutEp;
				}
				else if (TurnType == ETrafficTurnType::Right && Score < BestRightScore)
				{
					BestRightScore = Score;
					BestRight = OutEp;
				}
			}

			AddMovement(InEp, BestThrough, ETrafficTurnType::Through);
			AddMovement(InEp, BestLeft, ETrafficTurnType::Left);
			AddMovement(InEp, BestRight, ETrafficTurnType::Right);
		}
	}

	UE_LOG(LogTraffic, Log,
		TEXT("[FTrafficNetworkBuilder] BuildIntersectionsAndMovements: %d intersections, %d movements."),
		OutNetwork.Intersections.Num(),
		OutNetwork.Movements.Num());
}
