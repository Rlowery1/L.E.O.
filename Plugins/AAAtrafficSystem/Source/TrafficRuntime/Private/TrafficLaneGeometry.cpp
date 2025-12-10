#include "TrafficLaneGeometry.h"
#include "TrafficRuntimeModule.h"

namespace
{
	FVector ComputeLaneTangentAtIndex(const FTrafficLane& Lane, int32 Index)
	{
		const int32 NumPts = Lane.CenterlinePoints.Num();
		if (NumPts < 2)
		{
			return FVector::ForwardVector;
		}

		if (Index <= 0)
		{
			return (Lane.CenterlinePoints[1] - Lane.CenterlinePoints[0]).GetSafeNormal();
		}

		if (Index >= NumPts - 1)
		{
			return (Lane.CenterlinePoints[NumPts - 1] - Lane.CenterlinePoints[NumPts - 2]).GetSafeNormal();
		}

		const FVector Prev = Lane.CenterlinePoints[Index - 1];
		const FVector Next = Lane.CenterlinePoints[Index + 1];
		return (Next - Prev).GetSafeNormal();
	}
}

float TrafficLaneGeometry::ComputeLaneLengthCm(const FTrafficLane& Lane)
{
	const int32 NumPts = Lane.CenterlinePoints.Num();
	if (NumPts < 2)
	{
		return 0.0f;
	}

	float Length = 0.0f;
	for (int32 i = 0; i < NumPts - 1; ++i)
	{
		Length += FVector::Dist(Lane.CenterlinePoints[i], Lane.CenterlinePoints[i + 1]);
	}
	return Length;
}

void TrafficLaneGeometry::BuildCumulativeDistances(
	const FTrafficLane& Lane,
	TArray<float>& OutCumulative)
{
	const int32 NumPts = Lane.CenterlinePoints.Num();
	OutCumulative.Reset();
	if (NumPts == 0)
	{
		return;
	}

	OutCumulative.SetNum(NumPts);
	OutCumulative[0] = 0.0f;

	float Accum = 0.0f;
	for (int32 i = 1; i < NumPts; ++i)
	{
		const float SegLen = FVector::Dist(Lane.CenterlinePoints[i - 1], Lane.CenterlinePoints[i]);
		Accum += SegLen;
		OutCumulative[i] = Accum;
	}
}

bool TrafficLaneGeometry::ProjectPointOntoLane(
	const FTrafficLane& Lane,
	const FVector& WorldLocation,
	const FVector& VehicleForward,
	FLaneProjectionResult& OutResult,
	int32 SearchStartIndex)
{
	const int32 NumPts = Lane.CenterlinePoints.Num();
	if (NumPts < 2)
	{
		return false;
	}

	const int32 NumSegments = NumPts - 1;

	float BestDistSq = TNumericLimits<float>::Max();
	int32 BestSegmentIndex = INDEX_NONE;
	float BestTOnSegment = 0.0f;
	FVector BestPoint = FVector::ZeroVector;

	auto EvaluateSegment = [&](int32 SegIndex)
	{
		const FVector A = Lane.CenterlinePoints[SegIndex];
		const FVector B = Lane.CenterlinePoints[SegIndex + 1];
		const FVector AB = B - A;
		const float ABLenSq = AB.SizeSquared();

		if (ABLenSq < KINDA_SMALL_NUMBER)
		{
			return;
		}

		const float T = FMath::Clamp(
			FVector::DotProduct(WorldLocation - A, AB) / ABLenSq,
			0.0f,
			1.0f);

		const FVector Closest = A + AB * T;
		const float DistSq = FVector::DistSquared(WorldLocation, Closest);

		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestSegmentIndex = SegIndex;
			BestTOnSegment = T;
			BestPoint = Closest;
		}
	};

	for (int32 i = 0; i < NumSegments; ++i)
	{
		EvaluateSegment(i);
	}

	if (BestSegmentIndex == INDEX_NONE)
	{
		return false;
	}

	TArray<float> Cumulative;
	BuildCumulativeDistances(Lane, Cumulative);

	float S = 0.0f;
	if (BestSegmentIndex >= 0 && BestSegmentIndex < Cumulative.Num())
	{
		S = Cumulative[BestSegmentIndex];
		const FVector A = Lane.CenterlinePoints[BestSegmentIndex];
		const FVector B = Lane.CenterlinePoints[BestSegmentIndex + 1];
		const float SegLen = FVector::Dist(A, B);
		S += SegLen * BestTOnSegment;
	}

	const FVector Tangent = ComputeLaneTangentAtIndex(Lane, BestSegmentIndex).GetSafeNormal();
	const FVector Up = FVector::UpVector;
	const FVector Right = FVector::CrossProduct(Up, Tangent).GetSafeNormal();

	const FVector Delta = WorldLocation - BestPoint;
	const float LateralOffset = FVector::DotProduct(Delta, Right);

	FVector VehicleFwd = VehicleForward.GetSafeNormal();
	if (VehicleFwd.IsNearlyZero())
	{
		VehicleFwd = Tangent;
	}

	const float Dot = FMath::Clamp(FVector::DotProduct(VehicleFwd, Tangent), -1.0f, 1.0f);
	const float AngleRad = FMath::Acos(Dot);
	const float HeadingErrorDeg = FMath::RadiansToDegrees(AngleRad);

	OutResult.LaneId = Lane.LaneId;
	OutResult.S = S;
	OutResult.ClosestPoint = BestPoint;
	OutResult.LateralOffsetCm = LateralOffset;
	OutResult.HeadingErrorDeg = HeadingErrorDeg;
	OutResult.SegmentIndex = BestSegmentIndex;

	return true;
}

bool TrafficLaneGeometry::SamplePoseAtS(
	const FTrafficLane& Lane,
	float S,
	FVector& OutPosition,
	FVector& OutTangent)
{
	const int32 NumPts = Lane.CenterlinePoints.Num();
	if (NumPts < 2)
	{
		return false;
	}

	TArray<float> Cumulative;
	BuildCumulativeDistances(Lane, Cumulative);

	const float TotalLen = Cumulative.Last();
	if (TotalLen <= KINDA_SMALL_NUMBER)
	{
		OutPosition = Lane.CenterlinePoints[0];
		OutTangent = FVector::ForwardVector;
		return true;
	}

	const float ClampedS = FMath::Clamp(S, 0.0f, TotalLen);

	int32 SegmentIdx = 0;
	for (int32 i = 0; i < Cumulative.Num() - 1; ++i)
	{
		if (ClampedS >= Cumulative[i] && ClampedS <= Cumulative[i + 1])
		{
			SegmentIdx = i;
			break;
		}
	}

	const FVector A = Lane.CenterlinePoints[SegmentIdx];
	const FVector B = Lane.CenterlinePoints[SegmentIdx + 1];
	const float SegStartS = Cumulative[SegmentIdx];
	const float SegLen = FVector::Dist(A, B);
	float T = 0.0f;
	if (SegLen > KINDA_SMALL_NUMBER)
	{
		T = (ClampedS - SegStartS) / SegLen;
	}

	OutPosition = FMath::Lerp(A, B, T);
	OutTangent = (B - A).GetSafeNormal();
	if (OutTangent.IsNearlyZero())
	{
		OutTangent = FVector::ForwardVector;
	}

	return true;
}

