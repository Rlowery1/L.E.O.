#include "TrafficNetworkBuilder.h"
#include "TrafficGeometryProvider.h"
#include "TrafficLaneGeometry.h"
#include "TrafficMovementGeometry.h"
#include "TrafficRuntimeModule.h"
#include "HAL/IConsoleManager.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

namespace
{
	static const TCHAR* GetCVarSetByString(const IConsoleVariable* Var)
	{
		if (!Var)
		{
			return TEXT("None");
		}

		switch (Var->GetFlags() & ECVF_SetByMask)
		{
		case ECVF_SetByConstructor: return TEXT("Constructor");
		case ECVF_SetByScalability: return TEXT("Scalability");
		case ECVF_SetByGameSetting: return TEXT("GameSetting");
		case ECVF_SetByProjectSetting: return TEXT("ProjectSetting");
		case ECVF_SetBySystemSettingsIni: return TEXT("SystemSettingsIni");
		case ECVF_SetByDeviceProfile: return TEXT("DeviceProfile");
		case ECVF_SetByConsole: return TEXT("Console");
		case ECVF_SetByCommandline: return TEXT("Commandline");
		case ECVF_SetByCode: return TEXT("Code");
		default: break;
		}

		return TEXT("Unknown");
	}

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

	static TAutoConsoleVariable<float> CVarTrafficIntersectionClusterRadiusCm(
		TEXT("aaa.Traffic.Intersections.ClusterRadiusCm"),
		500.f,
		TEXT("Endpoint clustering radius (cm) for building intersections. Increase if roads stop short of the intersection mesh. Default: 500."),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarTrafficIntersectionCrossSplitMinDistanceFromLaneEndsCm(
		TEXT("aaa.Traffic.Intersections.CrossSplitMinDistanceFromLaneEndsCm"),
		300.f,
		TEXT("Minimum distance (cm) from lane ends when splitting lanes at road centerline crossings. Default: 300."),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarTrafficIntersectionCrossSplitMaxParallelDot(
		TEXT("aaa.Traffic.Intersections.CrossSplitMaxParallelDot"),
		0.98f,
		TEXT("Skip cross splits when abs(dot) between crossing segments exceeds this (nearly parallel). Default: 0.98."),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarTrafficIntersectionCrossSplitToleranceCm(
		TEXT("aaa.Traffic.Intersections.CrossSplitToleranceCm"),
		75.f,
		TEXT("Tolerance (cm) for deduplicating cross split positions on the same lane. Default: 75."),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarTrafficIntersectionCrossSplitDistanceLaneWidthScale(
		TEXT("aaa.Traffic.Intersections.CrossSplitDistanceLaneWidthScale"),
		4.0f,
		TEXT("Max lateral distance from a road crossing to consider a lane for split, as a multiple of lane width. Default: 4.0."),
		ECVF_Default);

	static TAutoConsoleVariable<int32> CVarTrafficIntersectionAnchorFromActors(
		TEXT("aaa.Traffic.Intersections.AnchorFromActors"),
		1,
		TEXT("If non-zero, attempt to anchor intersection centers/radii and lane endpoints using intersection mesh actors. Default: 1."),
		ECVF_Default);

	static TAutoConsoleVariable<FString> CVarTrafficIntersectionAnchorActorNameContains(
		TEXT("aaa.Traffic.Intersections.AnchorActorNameContains"),
		TEXT("MeshIntersection"),
		TEXT("Case-insensitive substring to match intersection anchor actors by name. Default: MeshIntersection."),
		ECVF_Default);

	static TAutoConsoleVariable<FString> CVarTrafficIntersectionAnchorActorTag(
		TEXT("aaa.Traffic.Intersections.AnchorActorTag"),
		TEXT(""),
		TEXT("Optional actor tag to match intersection anchors (empty = ignore). Default: empty."),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarTrafficIntersectionAnchorMaxDistanceCm(
		TEXT("aaa.Traffic.Intersections.AnchorMaxDistanceCm"),
		5000.f,
		TEXT("Maximum distance (cm) between an intersection cluster and an anchor actor to apply anchoring. Default: 5000."),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarTrafficIntersectionAnchorMinAlignmentDot(
		TEXT("aaa.Traffic.Intersections.AnchorMinAlignmentDot"),
		0.1f,
		TEXT("Minimum dot between lane end direction and vector toward anchor center to extend endpoints. Default: 0.1."),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarTrafficIntersectionAnchorRadiusScale(
		TEXT("aaa.Traffic.Intersections.AnchorRadiusScale"),
		1.0f,
		TEXT("Scale applied to anchor radius derived from actor bounds. Default: 1.0."),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarTrafficIntersectionAnchorRadiusLaneWidthScale(
		TEXT("aaa.Traffic.Intersections.AnchorRadiusLaneWidthScale"),
		2.0f,
		TEXT("Clamp anchor radius to (average lane width * scale). <=0 disables the clamp. Default: 2.0."),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarTrafficIntersectionRadiusLaneWidthScale(
		TEXT("aaa.Traffic.Intersections.RadiusLaneWidthScale"),
		1.25f,
		TEXT("Clamp intersection radius to (average lane width * scale). <=0 disables the clamp. Default: 1.25."),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarTrafficIntersectionAnchorRadiusBiasCm(
		TEXT("aaa.Traffic.Intersections.AnchorRadiusBiasCm"),
		0.0f,
		TEXT("Bias (cm) added to anchor radius derived from actor bounds (can be negative). Default: 0."),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarTrafficIntersectionAnchorExtendMaxCm(
		TEXT("aaa.Traffic.Intersections.AnchorExtendMaxCm"),
		5000.f,
		TEXT("Maximum distance (cm) to extend a lane endpoint toward an anchor. Default: 5000."),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarTrafficIntersectionAnchorEndpointToleranceCm(
		TEXT("aaa.Traffic.Intersections.AnchorEndpointToleranceCm"),
		150.f,
		TEXT("Tolerance (cm) used when snapping endpoints to anchor boundaries. Default: 150."),
		ECVF_Default);

	static TAutoConsoleVariable<int32> CVarTrafficIntersectionAnchorDebug(
		TEXT("aaa.Traffic.Intersections.AnchorDebug"),
		0,
		TEXT("If non-zero, log anchor radius clamp diagnostics per intersection. Default: 0."),
		ECVF_Default);

	struct FIntersectionAnchor
	{
		FVector Center = FVector::ZeroVector;
		float Radius = 0.f;
		FString DebugName;
	};

	static bool IntersectRayCircle2D(
		const FVector& RayOrigin,
		const FVector& RayDir,
		const FVector& Center,
		float Radius,
		float& OutT)
	{
		FVector2D O(RayOrigin.X, RayOrigin.Y);
		FVector2D D(RayDir.X, RayDir.Y);
		const float DirLenSq = D.SizeSquared();
		if (DirLenSq <= KINDA_SMALL_NUMBER)
		{
			return false;
		}
		D /= FMath::Sqrt(DirLenSq);

		const FVector2D C(Center.X, Center.Y);
		const FVector2D OC = O - C;
		const float A = FVector2D::DotProduct(D, D);
		const float B = 2.f * FVector2D::DotProduct(OC, D);
		const float CCoef = FVector2D::DotProduct(OC, OC) - Radius * Radius;
		const float Disc = (B * B) - (4.f * A * CCoef);
		if (Disc < 0.f || A <= KINDA_SMALL_NUMBER)
		{
			return false;
		}

		const float SqrtDisc = FMath::Sqrt(Disc);
		const float T0 = (-B - SqrtDisc) / (2.f * A);
		const float T1 = (-B + SqrtDisc) / (2.f * A);

		float BestT = TNumericLimits<float>::Max();
		if (T0 >= 0.f)
		{
			BestT = T0;
		}
		if (T1 >= 0.f)
		{
			BestT = FMath::Min(BestT, T1);
		}
		if (!FMath::IsFinite(BestT) || BestT == TNumericLimits<float>::Max())
		{
			return false;
		}

		OutT = BestT;
		return true;
	}

	static float Cross2D(const FVector2D& A, const FVector2D& B)
	{
		return A.X * B.Y - A.Y * B.X;
	}

	static bool IntersectSegments2D(
		const FVector& A0,
		const FVector& A1,
		const FVector& B0,
		const FVector& B1,
		float& OutTA,
		float& OutTB,
		FVector& OutPoint)
	{
		const FVector2D P(A0.X, A0.Y);
		const FVector2D R(A1.X - A0.X, A1.Y - A0.Y);
		const FVector2D Q(B0.X, B0.Y);
		const FVector2D S(B1.X - B0.X, B1.Y - B0.Y);

		const float Rxs = Cross2D(R, S);
		const FVector2D QMinusP = Q - P;

		if (FMath::Abs(Rxs) <= KINDA_SMALL_NUMBER)
		{
			return false;
		}

		const float T = Cross2D(QMinusP, S) / Rxs;
		const float U = Cross2D(QMinusP, R) / Rxs;

		if (T < 0.f || T > 1.f || U < 0.f || U > 1.f)
		{
			return false;
		}

		OutTA = T;
		OutTB = U;
		OutPoint = A0 + (A1 - A0) * T;
		return true;
	}

	static void RefreshRoadLaneCopies(FTrafficNetwork& OutNetwork)
	{
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
	}

	static void CollectIntersectionAnchors(UWorld* World, TArray<FIntersectionAnchor>& OutAnchors)
	{
		OutAnchors.Reset();
		if (!World)
		{
			return;
		}

		const FString NameContains = CVarTrafficIntersectionAnchorActorNameContains.GetValueOnGameThread();
		const FString TagString = CVarTrafficIntersectionAnchorActorTag.GetValueOnGameThread();
		const bool bUseTag = !TagString.IsEmpty();
		const FName TagName(*TagString);
		const float RadiusScale = FMath::Max(0.f, CVarTrafficIntersectionAnchorRadiusScale.GetValueOnGameThread());
		const float RadiusBias = CVarTrafficIntersectionAnchorRadiusBiasCm.GetValueOnGameThread();

		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor)
			{
				continue;
			}

			const bool bTagMatch = bUseTag && Actor->Tags.Contains(TagName);
			bool bNameMatch = !NameContains.IsEmpty() && Actor->GetName().Contains(NameContains, ESearchCase::IgnoreCase);
			if (!bNameMatch && !NameContains.IsEmpty())
			{
				const UClass* ActorClass = Actor->GetClass();
				const FString ClassName = ActorClass ? ActorClass->GetName() : FString();
				bNameMatch = !ClassName.IsEmpty() && ClassName.Contains(NameContains, ESearchCase::IgnoreCase);
			}
#if WITH_EDITOR
			if (!bNameMatch && !NameContains.IsEmpty())
			{
				const FString Label = Actor->GetActorLabel();
				bNameMatch = !Label.IsEmpty() && Label.Contains(NameContains, ESearchCase::IgnoreCase);
			}
#endif
			if (!bTagMatch && !bNameMatch)
			{
				continue;
			}

			FBox Bounds = Actor->GetComponentsBoundingBox(false);
			if (!Bounds.IsValid)
			{
				Bounds = Actor->GetComponentsBoundingBox(true);
			}
			if (!Bounds.IsValid)
			{
				continue;
			}

			const FVector Center = Bounds.GetCenter();
			const FVector Extent = Bounds.GetExtent();
			const float RawRadius = FMath::Max(Extent.X, Extent.Y);
			const float Radius = FMath::Max(0.f, RawRadius * RadiusScale + RadiusBias);
			if (Radius <= KINDA_SMALL_NUMBER)
			{
				continue;
			}

			FIntersectionAnchor Anchor;
			Anchor.Center = Center;
			Anchor.Radius = Radius;
			Anchor.DebugName = Actor->GetName();
			OutAnchors.Add(Anchor);
		}
	}

	static int32 FindNearestAnchorIndex(
		const TArray<FIntersectionAnchor>& Anchors,
		const FVector& Query,
		float MaxDistanceCm)
	{
		int32 BestIndex = INDEX_NONE;
		float BestDistSq = FMath::Square(MaxDistanceCm);
		for (int32 Index = 0; Index < Anchors.Num(); ++Index)
		{
			const FIntersectionAnchor& Anchor = Anchors[Index];
			const float DistSq = FVector::DistSquared2D(Anchor.Center, Query);
			if (DistSq <= BestDistSq)
			{
				BestDistSq = DistSq;
				BestIndex = Index;
			}
		}
		return BestIndex;
	}

	static const FIntersectionAnchor* FindNearestAnchor(
		const TArray<FIntersectionAnchor>& Anchors,
		const FVector& Query,
		float MaxDistanceCm)
	{
		const int32 BestIndex = FindNearestAnchorIndex(Anchors, Query, MaxDistanceCm);
		return Anchors.IsValidIndex(BestIndex) ? &Anchors[BestIndex] : nullptr;
	}

	static const FTrafficLane* FindLaneById(const FTrafficNetwork& Network, int32 LaneId)
	{
		for (const FTrafficLane& Lane : Network.Lanes)
		{
			if (Lane.LaneId == LaneId)
			{
				return &Lane;
			}
		}
		return nullptr;
	}

	static float ComputeAverageLaneWidthCm(const FTrafficNetwork& Network, const FTrafficIntersection& Intersection)
	{
		double Sum = 0.0;
		int32 Count = 0;
		for (int32 LaneId : Intersection.IncomingLaneIds)
		{
			if (const FTrafficLane* Lane = FindLaneById(Network, LaneId))
			{
				if (Lane->Width > KINDA_SMALL_NUMBER)
				{
					Sum += Lane->Width;
					++Count;
				}
			}
		}
		for (int32 LaneId : Intersection.OutgoingLaneIds)
		{
			if (const FTrafficLane* Lane = FindLaneById(Network, LaneId))
			{
				if (Lane->Width > KINDA_SMALL_NUMBER)
				{
					Sum += Lane->Width;
					++Count;
				}
			}
		}
		if (Count <= 0)
		{
			return 350.f;
		}
		return static_cast<float>(Sum / static_cast<double>(Count));
	}

	static float ApplyAnchorRadiusClamp(float AnchorRadius, float AverageLaneWidthCm)
	{
		const float Scale = CVarTrafficIntersectionAnchorRadiusLaneWidthScale.GetValueOnGameThread();
		if (Scale <= 0.f || AverageLaneWidthCm <= KINDA_SMALL_NUMBER)
		{
			return AnchorRadius;
		}
		const float MaxRadius = AverageLaneWidthCm * Scale;
		return FMath::Min(AnchorRadius, MaxRadius);
	}

	static float ComputeAverageLaneWidthNearAnchor(
		const FTrafficNetwork& Network,
		const FVector& AnchorCenter,
		float MaxDistanceCm)
	{
		double Sum = 0.0;
		int32 Count = 0;
		const bool bUseDistance = MaxDistanceCm > 0.f;
		const float MaxDistSq = bUseDistance ? FMath::Square(MaxDistanceCm) : 0.f;
		const FVector2D Anchor2D(AnchorCenter.X, AnchorCenter.Y);

		for (const FTrafficLane& Lane : Network.Lanes)
		{
			if (Lane.Width <= KINDA_SMALL_NUMBER || Lane.CenterlinePoints.Num() < 2)
			{
				continue;
			}

			if (bUseDistance)
			{
				float MinDistSq = TNumericLimits<float>::Max();
				for (int32 i = 0; i < Lane.CenterlinePoints.Num() - 1; ++i)
				{
					const FVector2D A(Lane.CenterlinePoints[i].X, Lane.CenterlinePoints[i].Y);
					const FVector2D B(Lane.CenterlinePoints[i + 1].X, Lane.CenterlinePoints[i + 1].Y);
					const FVector2D AB = B - A;
					const float LenSq = AB.SizeSquared();
					const float T = (LenSq > KINDA_SMALL_NUMBER)
						? FMath::Clamp(FVector2D::DotProduct(Anchor2D - A, AB) / LenSq, 0.f, 1.f)
						: 0.f;
					const FVector2D Closest = A + AB * T;
					MinDistSq = FMath::Min(MinDistSq, FVector2D::DistSquared(Anchor2D, Closest));
					if (MinDistSq <= MaxDistSq)
					{
						break;
					}
				}

				if (MinDistSq > MaxDistSq)
				{
					continue;
				}
			}

			Sum += Lane.Width;
			++Count;
		}

		if (Count <= 0)
		{
			return 0.f;
		}
		return static_cast<float>(Sum / static_cast<double>(Count));
	}

	static void ComputeAnchorEffectiveRadii(
		const FTrafficNetwork& Network,
		const TArray<FIntersectionAnchor>& Anchors,
		TArray<float>& OutRadii)
	{
		OutRadii.Reset(Anchors.Num());
		OutRadii.Reserve(Anchors.Num());

		const float MaxDistance = FMath::Max(0.f, CVarTrafficIntersectionAnchorMaxDistanceCm.GetValueOnGameThread());

		for (const FIntersectionAnchor& Anchor : Anchors)
		{
			const float AvgLaneWidth = ComputeAverageLaneWidthNearAnchor(Network, Anchor.Center, MaxDistance);
			const float EffectiveRadius = ApplyAnchorRadiusClamp(Anchor.Radius, AvgLaneWidth);
			OutRadii.Add(EffectiveRadius);
		}
	}

	static void ClampIntersectionRadiiByLaneWidth(FTrafficNetwork& OutNetwork)
	{
		const float Scale = CVarTrafficIntersectionRadiusLaneWidthScale.GetValueOnGameThread();
		if (Scale <= 0.f || OutNetwork.Intersections.Num() == 0)
		{
			return;
		}

		for (FTrafficIntersection& Intersection : OutNetwork.Intersections)
		{
			const float AvgLaneWidth = ComputeAverageLaneWidthCm(OutNetwork, Intersection);
			if (AvgLaneWidth <= KINDA_SMALL_NUMBER)
			{
				continue;
			}

			const float MaxRadius = AvgLaneWidth * Scale;
			if (Intersection.Radius > MaxRadius)
			{
				Intersection.Radius = MaxRadius;
			}
		}
	}

	static int32 AdjustLaneEndpointsToAnchors(
		FTrafficNetwork& OutNetwork,
		const TArray<FIntersectionAnchor>& Anchors,
		const TArray<float>& AnchorEffectiveRadii)
	{
		if (Anchors.Num() == 0 || OutNetwork.Lanes.Num() == 0 || Anchors.Num() != AnchorEffectiveRadii.Num())
		{
			return 0;
		}

		const float MaxDistance = FMath::Max(0.f, CVarTrafficIntersectionAnchorMaxDistanceCm.GetValueOnGameThread());
		const float MinAlignDot = FMath::Clamp(CVarTrafficIntersectionAnchorMinAlignmentDot.GetValueOnGameThread(), -1.f, 1.f);
		const float ExtendMax = FMath::Max(0.f, CVarTrafficIntersectionAnchorExtendMaxCm.GetValueOnGameThread());
		const float EndpointTolerance = FMath::Max(0.f, CVarTrafficIntersectionAnchorEndpointToleranceCm.GetValueOnGameThread());
		const bool bDebug = CVarTrafficIntersectionAnchorDebug.GetValueOnGameThread() != 0;

		int32 AdjustedLanes = 0;
		TSet<int32> AdjustedLaneIds;
		TSet<int32> AdjustedEndpointKeys;

		auto AdjustLaneEndpoint = [&](FTrafficLane& Lane, bool bIsEnd)
		{
			if (Lane.CenterlinePoints.Num() < 2)
			{
				return false;
			}

			const int32 EndpointKey = (Lane.LaneId << 1) | (bIsEnd ? 1 : 0);
			if (AdjustedEndpointKeys.Contains(EndpointKey))
			{
				return false;
			}

			const int32 Index = bIsEnd ? (Lane.CenterlinePoints.Num() - 1) : 0;
			const int32 Adjacent = bIsEnd ? (Lane.CenterlinePoints.Num() - 2) : 1;
			const FVector End = Lane.CenterlinePoints[Index];
			const FVector Prev = Lane.CenterlinePoints[Adjacent];
			const FVector Dir = (End - Prev).GetSafeNormal();
			if (Dir.IsNearlyZero())
			{
				return false;
			}

			const int32 AnchorIndex = FindNearestAnchorIndex(Anchors, End, MaxDistance);
			if (!Anchors.IsValidIndex(AnchorIndex))
			{
				return false;
			}

			const FIntersectionAnchor& Anchor = Anchors[AnchorIndex];
			const float EffectiveRadius = AnchorEffectiveRadii[AnchorIndex];

			const float DistToCenter = FVector::Dist2D(End, Anchor.Center);
			if (EndpointTolerance > 0.f && FMath::Abs(DistToCenter - EffectiveRadius) <= EndpointTolerance)
			{
				return false;
			}

			auto TryApplyNewEnd = [&](const FVector& Candidate, const TCHAR* Reason)
			{
				const float Delta2D = FVector::Dist2D(End, Candidate);
				if (ExtendMax > 0.f && Delta2D > ExtendMax)
				{
					return false;
				}

				const float DistToBoundary = FMath::Abs(FVector::Dist2D(Candidate, Anchor.Center) - EffectiveRadius);
				if (EndpointTolerance > 0.f && DistToBoundary > EndpointTolerance)
				{
					return false;
				}

				Lane.CenterlinePoints[Index] = Candidate;
				AdjustedLaneIds.Add(Lane.LaneId);
				AdjustedEndpointKeys.Add(EndpointKey);

				if (bDebug)
				{
					UE_LOG(LogTraffic, Log, TEXT("[FTrafficNetworkBuilder] AnchorAdjust lane=%d reason=%s delta=%.1fcm distToBoundary=%.1fcm anchor=%s"),
						Lane.LaneId, Reason, Delta2D, DistToBoundary, *Anchor.DebugName);
				}

				return true;
			};

			FVector ToCenter = Anchor.Center - End;
			FVector ToCenter2D(ToCenter.X, ToCenter.Y, 0.f);
			if (ToCenter2D.IsNearlyZero())
			{
				return false;
			}

			const FVector ToCenterDir = ToCenter2D.GetSafeNormal();
			float AlignDot = FVector::DotProduct(Dir, ToCenterDir);
			FVector RayDir = Dir;
			if (AlignDot < -MinAlignDot)
			{
				RayDir = -Dir;
				AlignDot = -AlignDot;
			}

			auto TryRadialCandidate = [&]() -> bool
			{
				const FVector Candidate = Anchor.Center - (ToCenterDir * EffectiveRadius);
				return TryApplyNewEnd(Candidate, TEXT("radial"));
			};

			if (AlignDot < MinAlignDot)
			{
				return TryRadialCandidate();
			}

			float T = 0.f;
			if (!IntersectRayCircle2D(End, RayDir, Anchor.Center, EffectiveRadius, T))
			{
				return TryRadialCandidate();
			}

			const FVector NewEnd = End + RayDir * T;
			return TryApplyNewEnd(NewEnd, TEXT("ray"));
		};

		for (FTrafficLane& Lane : OutNetwork.Lanes)
		{
			AdjustLaneEndpoint(Lane, /*bIsEnd=*/false);
			AdjustLaneEndpoint(Lane, /*bIsEnd=*/true);
		}

		AdjustedLanes = AdjustedLaneIds.Num();
		if (AdjustedLanes > 0)
		{
			RefreshRoadLaneCopies(OutNetwork);
		}

		return AdjustedLanes;
	}

	static int32 ApplyAnchorCentersToIntersections(
		FTrafficNetwork& OutNetwork,
		const TArray<FIntersectionAnchor>& Anchors,
		const TArray<float>& AnchorEffectiveRadii)
	{
		if (Anchors.Num() == 0 || OutNetwork.Intersections.Num() == 0 || Anchors.Num() != AnchorEffectiveRadii.Num())
		{
			return 0;
		}

		const float MaxDistance = FMath::Max(0.f, CVarTrafficIntersectionAnchorMaxDistanceCm.GetValueOnGameThread());
		const bool bDebug = CVarTrafficIntersectionAnchorDebug.GetValueOnGameThread() != 0;
		int32 AnchoredCount = 0;

		for (FTrafficIntersection& Intersection : OutNetwork.Intersections)
		{
			const int32 AnchorIndex = FindNearestAnchorIndex(Anchors, Intersection.Center, MaxDistance);
			if (!Anchors.IsValidIndex(AnchorIndex))
			{
				continue;
			}

			const FIntersectionAnchor& Anchor = Anchors[AnchorIndex];
			const float EffectiveRadius = AnchorEffectiveRadii[AnchorIndex];
			if (bDebug)
			{
				const float AvgLaneWidth = ComputeAverageLaneWidthCm(OutNetwork, Intersection);
				UE_LOG(LogTraffic, Log, TEXT("[FTrafficNetworkBuilder] AnchorApply int=%d anchor=%s rawRadius=%.1f effRadius=%.1f avgLaneWidth=%.1f"),
					Intersection.IntersectionId, *Anchor.DebugName, Anchor.Radius, EffectiveRadius, AvgLaneWidth);
			}

			Intersection.Center = Anchor.Center;
			Intersection.Radius = EffectiveRadius;
			++AnchoredCount;
		}

		return AnchoredCount;
	}

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

	struct FLaneSegmentData
	{
		TArray<float> StartS;
		TArray<float> Lengths;
		float TotalLength = 0.f;
	};

	static void BuildLaneSegmentData(const TArray<FVector>& Points, FLaneSegmentData& OutData)
	{
		OutData.StartS.Reset();
		OutData.Lengths.Reset();
		OutData.TotalLength = 0.f;

		if (Points.Num() < 2)
		{
			return;
		}

		const int32 NumSegments = Points.Num() - 1;
		OutData.StartS.SetNumZeroed(NumSegments);
		OutData.Lengths.SetNumZeroed(NumSegments);

		float Accumulated = 0.f;
		for (int32 i = 0; i < NumSegments; ++i)
		{
			OutData.StartS[i] = Accumulated;
			const float SegLen = FVector::Dist(Points[i], Points[i + 1]);
			OutData.Lengths[i] = SegLen;
			Accumulated += SegLen;
		}
		OutData.TotalLength = Accumulated;
	}

	static int32 ApplyLaneSplits(
		FTrafficNetwork& OutNetwork,
		const TMap<int32, TArray<float>>& SplitsByLaneId,
		float SplitToleranceCm,
		const TCHAR* DebugLabel)
	{
		if (SplitsByLaneId.Num() == 0)
		{
			return 0;
		}

		int32 NextLaneId = 0;
		for (const FTrafficLane& L : OutNetwork.Lanes)
		{
			NextLaneId = FMath::Max(NextLaneId, (L.LaneId >= 0) ? (L.LaneId + 1) : NextLaneId);
		}

		int32 NumSplitsApplied = 0;
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
			UniqueSortSplits(Splits, SplitToleranceCm);

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
			RefreshRoadLaneCopies(OutNetwork);
			if (DebugLabel)
			{
				UE_LOG(LogTraffic, Log,
					TEXT("[FTrafficNetworkBuilder] %s: applied %d lane split(s)."),
					DebugLabel,
					NumSplitsApplied);
			}
		}

		return NumSplitsApplied;
	}

	struct FRoadCrossing
	{
		int32 RoadAId = INDEX_NONE;
		int32 RoadBId = INDEX_NONE;
		FVector Point = FVector::ZeroVector;
	};

	static void CollectRoadCrossings(
		const TArray<FTrafficRoad>& Roads,
		TArray<FRoadCrossing>& OutCrossings)
	{
		OutCrossings.Reset();

		for (int32 i = 0; i < Roads.Num(); ++i)
		{
			const FTrafficRoad& RoadA = Roads[i];
			if (RoadA.CenterlinePoints.Num() < 2)
			{
				continue;
			}

			for (int32 j = i + 1; j < Roads.Num(); ++j)
			{
				const FTrafficRoad& RoadB = Roads[j];
				if (RoadB.CenterlinePoints.Num() < 2)
				{
					continue;
				}

				for (int32 a = 0; a < RoadA.CenterlinePoints.Num() - 1; ++a)
				{
					const FVector A0 = RoadA.CenterlinePoints[a];
					const FVector A1 = RoadA.CenterlinePoints[a + 1];

					for (int32 b = 0; b < RoadB.CenterlinePoints.Num() - 1; ++b)
					{
						const FVector B0 = RoadB.CenterlinePoints[b];
						const FVector B1 = RoadB.CenterlinePoints[b + 1];

						float TA = 0.f;
						float TB = 0.f;
						FVector Point = FVector::ZeroVector;
						if (!IntersectSegments2D(A0, A1, B0, B1, TA, TB, Point))
						{
							continue;
						}

						FRoadCrossing Crossing;
						Crossing.RoadAId = RoadA.RoadId;
						Crossing.RoadBId = RoadB.RoadId;
						Crossing.Point = Point;
						OutCrossings.Add(Crossing);
					}
				}
			}
		}
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

	const float ClusterRadiusCm = FMath::Max(0.f, CVarTrafficIntersectionClusterRadiusCm.GetValueOnGameThread());
	BuildIntersectionsAndMovements(OutNetwork, ClusterRadiusCm);
	ClampIntersectionRadiiByLaneWidth(OutNetwork);
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

	const int32 AnchorFromActors = CVarTrafficIntersectionAnchorFromActors.GetValueOnGameThread();
	IConsoleVariable* AnchorVar = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.AnchorFromActors"));
	UE_LOG(LogTraffic, Log,
		TEXT("[FTrafficNetworkBuilder] AnchorFromActors=%d (setBy=%s)"),
		AnchorFromActors,
		GetCVarSetByString(AnchorVar));

	if (AnchorFromActors != 0)
	{
		TArray<FIntersectionAnchor> Anchors;
		CollectIntersectionAnchors(World, Anchors);
		const FString NameContains = CVarTrafficIntersectionAnchorActorNameContains.GetValueOnGameThread();
		const FString TagString = CVarTrafficIntersectionAnchorActorTag.GetValueOnGameThread();

		UE_LOG(LogTraffic, Log,
			TEXT("[FTrafficNetworkBuilder] Intersection anchors: Found=%d NameContains='%s' Tag='%s'"),
			Anchors.Num(),
			*NameContains,
			*TagString);

		if (Anchors.Num() > 0)
		{
			TArray<float> AnchorEffectiveRadii;
			ComputeAnchorEffectiveRadii(OutNetwork, Anchors, AnchorEffectiveRadii);

			const int32 Adjusted = AdjustLaneEndpointsToAnchors(OutNetwork, Anchors, AnchorEffectiveRadii);
			float ClusterRadiusCm = FMath::Max(0.f, CVarTrafficIntersectionClusterRadiusCm.GetValueOnGameThread());
			float MaxAnchorRadius = 0.f;
			for (float Radius : AnchorEffectiveRadii)
			{
				MaxAnchorRadius = FMath::Max(MaxAnchorRadius, Radius);
			}
			const float EndpointTolerance = FMath::Max(0.f, CVarTrafficIntersectionAnchorEndpointToleranceCm.GetValueOnGameThread());
			const float AnchorClusterRadius = FMath::Max(ClusterRadiusCm, MaxAnchorRadius + EndpointTolerance);

			if (Adjusted > 0 || AnchorClusterRadius > ClusterRadiusCm + KINDA_SMALL_NUMBER)
			{
				BuildIntersectionsAndMovements(OutNetwork, AnchorClusterRadius);
			}

			const int32 Anchored = ApplyAnchorCentersToIntersections(OutNetwork, Anchors, AnchorEffectiveRadii);
			ClampIntersectionRadiiByLaneWidth(OutNetwork);
			UE_LOG(LogTraffic, Log,
				TEXT("[FTrafficNetworkBuilder] Intersection anchors: Found=%d Adjusted=%d Anchored=%d ClusterRadius=%.1f"),
				Anchors.Num(),
				Adjusted,
				Anchored,
				AnchorClusterRadius);
		}
	}
	else
	{
		UE_LOG(LogTraffic, Log,
			TEXT("[FTrafficNetworkBuilder] Intersection anchoring disabled; lane endpoints may remain at road mesh ends."));
	}
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

	// Split lanes at road centerline crossings so mid-spline intersections are represented in the endpoint graph.
	TArray<FRoadCrossing> RoadCrossings;
	CollectRoadCrossings(OutNetwork.Roads, RoadCrossings);

	if (RoadCrossings.Num() > 0)
	{
		TMap<int32, TArray<float>> CrossSplitsByLaneId;
		const float MinFromEnds = FMath::Max(0.f, CVarTrafficIntersectionCrossSplitMinDistanceFromLaneEndsCm.GetValueOnGameThread());
		const float MaxParallelDot = FMath::Clamp(CVarTrafficIntersectionCrossSplitMaxParallelDot.GetValueOnGameThread(), 0.f, 1.f);
		const float LaneWidthScale = FMath::Max(0.f, CVarTrafficIntersectionCrossSplitDistanceLaneWidthScale.GetValueOnGameThread());
		const float SplitToleranceCm = FMath::Max(0.f, CVarTrafficIntersectionCrossSplitToleranceCm.GetValueOnGameThread());

		for (const FRoadCrossing& Crossing : RoadCrossings)
		{
			for (const FTrafficLane& Lane : OutNetwork.Lanes)
			{
				if (Lane.RoadId != Crossing.RoadAId && Lane.RoadId != Crossing.RoadBId)
				{
					continue;
				}

				const FPolylineClosestPoint Closest = FindClosestPointOnPolyline(Lane.CenterlinePoints, Crossing.Point);
				if (!Closest.bValid)
				{
					continue;
				}

				const float LaneLen = TrafficLaneGeometry::ComputeLaneLengthCm(Lane);
				if (LaneLen <= KINDA_SMALL_NUMBER)
				{
					continue;
				}

				if (MinFromEnds > 0.f && (Closest.SAlongCm < MinFromEnds || Closest.SAlongCm > (LaneLen - MinFromEnds)))
				{
					continue;
				}

				const float MaxDistance = FMath::Max(200.f, Lane.Width * LaneWidthScale);
				if (Closest.DistanceSq > FMath::Square(MaxDistance))
				{
					continue;
				}

				CrossSplitsByLaneId.FindOrAdd(Lane.LaneId).Add(Closest.SAlongCm);
			}
		}

		if (CrossSplitsByLaneId.Num() > 0)
		{
			ApplyLaneSplits(OutNetwork, CrossSplitsByLaneId, SplitToleranceCm, TEXT("Cross-split"));
		}
	}

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

		if (SplitsByLaneId.Num() > 0)
		{
			const int32 NumSplitsApplied = ApplyLaneSplits(
				OutNetwork,
				SplitsByLaneId,
				/*SplitToleranceCm=*/75.f,
				TEXT("Snap-to-lane"));

			if (NumSplitsApplied > 0)
			{
				BuildEndpoints(OutNetwork.Lanes, AllEndpoints);
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
