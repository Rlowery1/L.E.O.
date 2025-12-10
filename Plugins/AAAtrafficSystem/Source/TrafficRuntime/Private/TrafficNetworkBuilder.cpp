#include "TrafficNetworkBuilder.h"
#include "TrafficGeometryProvider.h"
#include "TrafficMovementGeometry.h"
#include "TrafficRuntimeModule.h"

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
		const float DirectionSign = bForwardDirection ? 1.f : -1.f;

		for (int32 LaneIndex = 0; LaneIndex < Side.NumLanes; ++LaneIndex)
		{
			const float CenterOffset = InnerOffset + LaneIndex * LaneWidth;
			const float LateralOffset = DirectionSign * CenterOffset;

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
				const FVector Pos = Centerline[i];

				FVector Tangent;
				if (i < NumPoints - 1)
				{
					Tangent = (Centerline[i + 1] - Centerline[i]).GetSafeNormal();
				}
				else
				{
					Tangent = (Centerline[i] - Centerline[i - 1]).GetSafeNormal();
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

	TArray<FLaneEndpoint> AllEndpoints;

	for (const FTrafficLane& Lane : OutNetwork.Lanes)
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
		AllEndpoints.Add(StartEp);

		FLaneEndpoint EndEp;
		EndEp.LaneId = Lane.LaneId;
		EndEp.Position = Lane.CenterlinePoints[NumPts - 1];
		EndEp.Direction = (Lane.CenterlinePoints[NumPts - 1] - Lane.CenterlinePoints[NumPts - 2]).GetSafeNormal();
		EndEp.bIsStart = false;
		AllEndpoints.Add(EndEp);
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

			bool bIsIncoming = false;
			if (Lane->Direction == ELaneDirection::Forward)
			{
				bIsIncoming = !Ep.bIsStart;
			}
			else if (Lane->Direction == ELaneDirection::Backward)
			{
				bIsIncoming = Ep.bIsStart;
			}

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

		for (const FLaneEndpoint* InEp : IncomingEndpoints)
		{
			for (const FLaneEndpoint* OutEp : OutgoingEndpoints)
			{
				if (InEp->LaneId == OutEp->LaneId)
				{
					continue;
				}

				FTrafficMovement Movement;
				Movement.MovementId = NextMovementId++;
				Movement.IntersectionId = Intersection.IntersectionId;
				Movement.IncomingLaneId = InEp->LaneId;
				Movement.OutgoingLaneId = OutEp->LaneId;

				const FVector InDir = InEp->Direction.GetSafeNormal();
				const FVector OutDir = OutEp->Direction.GetSafeNormal();
				const float DotProduct = FVector::DotProduct(InDir, OutDir);
				const FVector CrossProduct = FVector::CrossProduct(InDir, OutDir);

				if (DotProduct > 0.7f)
				{
					Movement.TurnType = ETrafficTurnType::Through;
				}
				else if (DotProduct < -0.7f)
				{
					Movement.TurnType = ETrafficTurnType::UTurn;
				}
				else if (CrossProduct.Z > 0.0f)
				{
					Movement.TurnType = ETrafficTurnType::Left;
				}
				else
				{
					Movement.TurnType = ETrafficTurnType::Right;
				}

				const FVector StartPos = InEp->Position;
				const FVector EndPos = OutEp->Position;
				const FVector InDirNorm = (-InEp->Direction).GetSafeNormal();
				const FVector OutDirNorm = OutEp->Direction.GetSafeNormal();

				TrafficMovementGeometry::BuildSmoothMovementPath(
					StartPos,
					InDirNorm,
					EndPos,
					OutDirNorm,
					24,
					Movement);

				OutNetwork.Movements.Add(Movement);
			}
		}
	}

	UE_LOG(LogTraffic, Log,
		TEXT("[FTrafficNetworkBuilder] BuildIntersectionsAndMovements: %d intersections, %d movements."),
		OutNetwork.Intersections.Num(),
		OutNetwork.Movements.Num());
}
