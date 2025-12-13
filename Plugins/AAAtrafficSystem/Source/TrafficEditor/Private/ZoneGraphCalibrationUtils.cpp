#include "ZoneGraphCalibrationUtils.h"

#include "ZoneGraphData.h"
#include "ZoneGraphSubsystem.h"

#include <cfloat>

namespace
{
	float ComputeRoadLengthCm(const TArray<FVector>& Centerline)
	{
		float Length = 0.f;
		for (int32 i = 0; i + 1 < Centerline.Num(); ++i)
		{
			Length += (Centerline[i + 1] - Centerline[i]).Size();
		}
		return Length;
	}

	FVector ComputeRoadDirection(const TArray<FVector>& Centerline)
	{
		FVector Accum = FVector::ZeroVector;
		for (int32 i = 0; i + 1 < Centerline.Num(); ++i)
		{
			const FVector Segment = Centerline[i + 1] - Centerline[i];
			if (!Segment.IsNearlyZero())
			{
				Accum += Segment.GetSafeNormal();
			}
		}

		if (!Accum.IsNearlyZero())
		{
			return Accum.GetSafeNormal();
		}

		const FVector Fallback = Centerline.Last() - Centerline[0];
		return Fallback.IsNearlyZero() ? FVector::ForwardVector : Fallback.GetSafeNormal();
	}

	float ComputeDistanceSquaredToPolyline(const FVector& Point, const TArray<FVector>& Polyline)
	{
		float MinDistSq = FLT_MAX;
		for (int32 i = 0; i + 1 < Polyline.Num(); ++i)
		{
			MinDistSq = FMath::Min(MinDistSq, FMath::PointDistToSegmentSquared(Point, Polyline[i], Polyline[i + 1]));
		}
		return MinDistSq;
	}
}

bool ComputeCalibrationFromZoneGraph(
	UWorld* World,
	const TArray<FVector>& RoadCenterlinePoints,
	FTrafficLaneFamilyCalibration& OutCalib)
{
	if (!World || RoadCenterlinePoints.Num() < 2)
	{
		return false;
	}

	UZoneGraphSubsystem* ZGS = World->GetSubsystem<UZoneGraphSubsystem>();
	if (!ZGS)
	{
		return false;
	}

	const float RoadLengthCm = ComputeRoadLengthCm(RoadCenterlinePoints);
	if (RoadLengthCm <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FVector RoadDir = ComputeRoadDirection(RoadCenterlinePoints);
	if (RoadDir.IsNearlyZero())
	{
		return false;
	}

	FBox Bounds(EForceInit::ForceInit);
	for (const FVector& Pt : RoadCenterlinePoints)
	{
		Bounds += Pt;
	}

	// Broad-phase: keep only lanes near the road and roughly parallel to its direction.
	constexpr float ParallelThreshold = 0.7f;
	constexpr float MaxDistanceToCenterlineCm = 4000.f; // ~40m
	const float MaxDistanceToCenterlineSq = MaxDistanceToCenterlineCm * MaxDistanceToCenterlineCm;
	const float ExpandedBoundsMargin = 2000.f;
	const FBox ExpandedBounds = Bounds.ExpandBy(ExpandedBoundsMargin);

	const FZoneGraphTag VehiclesTag = ZGS->GetTagByName(FName(TEXT("Vehicles")));
	const bool bFilterToVehiclesTag = VehiclesTag.IsValid();

	int32 ForwardCount = 0;
	int32 BackwardCount = 0;
	float WidthSum = 0.f;
	int32 WidthSamples = 0;

	for (const FRegisteredZoneGraphData& Registered : ZGS->GetRegisteredZoneGraphData())
	{
		const AZoneGraphData* DataActor = Registered.ZoneGraphData;
		if (!DataActor)
		{
			continue;
		}

		const FZoneGraphStorage& Storage = DataActor->GetStorage();
		const FZoneGraphDataHandle DataHandle = Storage.DataHandle;
		if (!DataHandle.IsValid())
		{
			continue;
		}

		for (int32 LaneIndex = 0; LaneIndex < Storage.Lanes.Num(); ++LaneIndex)
		{
			const FZoneGraphLaneHandle LaneHandle(LaneIndex, DataHandle);
			if (!ZGS->IsLaneValid(LaneHandle))
			{
				continue;
			}

			if (bFilterToVehiclesTag)
			{
				FZoneGraphTagMask LaneTags;
				if (!ZGS->GetLaneTags(LaneHandle, LaneTags) || !LaneTags.Contains(VehiclesTag))
				{
					continue;
				}
			}

			float LaneLengthCm = 0.f;
			if (!ZGS->GetLaneLength(LaneHandle, LaneLengthCm))
			{
				continue;
			}

			// Avoid counting short intersection lanes when calibrating long road pieces.
			if (LaneLengthCm < RoadLengthCm * 0.3f)
			{
				continue;
			}

			FZoneGraphLaneLocation MidLoc;
			if (!ZGS->CalculateLocationAlongLane(LaneHandle, LaneLengthCm * 0.5f, MidLoc))
			{
				continue;
			}

			if (!ExpandedBounds.IsInsideOrOn(MidLoc.Position))
			{
				continue;
			}

			const FVector LaneDir = MidLoc.Direction.GetSafeNormal();
			if (LaneDir.IsNearlyZero())
			{
				continue;
			}

			const float Dot = FVector::DotProduct(LaneDir, RoadDir);
			if (FMath::Abs(Dot) < ParallelThreshold)
			{
				continue;
			}

			const float DistSq = ComputeDistanceSquaredToPolyline(MidLoc.Position, RoadCenterlinePoints);
			if (DistSq > MaxDistanceToCenterlineSq)
			{
				continue;
			}

			if (Dot >= 0.f)
			{
				++ForwardCount;
			}
			else
			{
				++BackwardCount;
			}

			const float LaneWidthCm = Storage.Lanes.IsValidIndex(LaneIndex) ? Storage.Lanes[LaneIndex].Width : 0.f;
			if (LaneWidthCm > KINDA_SMALL_NUMBER)
			{
				WidthSum += LaneWidthCm;
				++WidthSamples;
			}
		}
	}

	const int32 Total = ForwardCount + BackwardCount;
	if (Total <= 0)
	{
		return false;
	}

	const float AvgWidthCm = (WidthSamples > 0) ? (WidthSum / WidthSamples) : 350.f;

	OutCalib.NumLanesPerSideForward = FMath::Clamp(ForwardCount, 0, 5);
	OutCalib.NumLanesPerSideBackward = FMath::Clamp(BackwardCount, 0, 5);
	OutCalib.LaneWidthCm = FMath::Clamp(AvgWidthCm, 50.f, 1000.f);
	OutCalib.CenterlineOffsetCm = OutCalib.LaneWidthCm * 0.5f;

	return (OutCalib.NumLanesPerSideForward + OutCalib.NumLanesPerSideBackward) > 0;
}

