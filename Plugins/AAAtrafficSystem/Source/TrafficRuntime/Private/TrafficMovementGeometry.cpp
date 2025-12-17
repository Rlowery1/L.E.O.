#include "TrafficMovementGeometry.h"
#include "TrafficRuntimeModule.h"

namespace
{
	static TAutoConsoleVariable<float> CVarTrafficMovementControlDistFraction(
		TEXT("aaa.Traffic.Movement.ControlDistFraction"),
		0.5f,
		TEXT("Fraction of chord length used for cubic Bezier control point distance when building intersection movements.\n")
		TEXT("Higher = wider turns, lower = tighter turns. Default: 0.5"),
		ECVF_Default);

	static TAutoConsoleVariable<float> CVarTrafficMovementMaxControlDistCm(
		TEXT("aaa.Traffic.Movement.MaxControlDistCm"),
		5000.f,
		TEXT("Max control point distance (cm) used when building intersection movements (prevents extreme overshoot on long chords). Default: 5000"),
		ECVF_Default);

	FVector EvalCubicBezier(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float T)
	{
		const float OneMinusT = 1.0f - T;
		const float OneMinusT2 = OneMinusT * OneMinusT;
		const float OneMinusT3 = OneMinusT2 * OneMinusT;
		const float T2 = T * T;
		const float T3 = T2 * T;

		return
			(OneMinusT3 * P0) +
			(3.0f * OneMinusT2 * T * P1) +
			(3.0f * OneMinusT * T2 * P2) +
			(T3 * P3);
	}
}

void TrafficMovementGeometry::BuildSmoothMovementPath(
	const FVector& InStartPos,
	const FVector& InStartDir,
	const FVector& InEndPos,
	const FVector& InEndDir,
	int32 NumSamples,
	FTrafficMovement& InOutMovement)
{
	NumSamples = FMath::Clamp(NumSamples, 3, 64);

	const FVector P0 = InStartPos;
	const FVector P3 = InEndPos;

	const FVector FallbackDir = (P3 - P0).GetSafeNormal();
	const FVector StartDir = InStartDir.GetSafeNormal(UE_SMALL_NUMBER, FallbackDir);
	const FVector EndDir = InEndDir.GetSafeNormal(UE_SMALL_NUMBER, FallbackDir);

	const float ChordLen = FVector::Dist(P0, P3);
	const float ControlFraction = FMath::Clamp(CVarTrafficMovementControlDistFraction.GetValueOnAnyThread(), 0.f, 2.f);
	const float MaxControlDist = FMath::Max(0.f, CVarTrafficMovementMaxControlDistCm.GetValueOnAnyThread());
	const float ControlDist = FMath::Min(ChordLen * ControlFraction, MaxControlDist);

	// Use a cubic Bezier so we can match both the incoming and outgoing lane directions at the endpoints.
	const FVector C1 = P0 + StartDir * ControlDist;
	const FVector C2 = P3 - EndDir * ControlDist;

	InOutMovement.PathPoints.Reset();
	InOutMovement.PathPoints.Reserve(NumSamples);

	for (int32 i = 0; i < NumSamples; ++i)
	{
		const float T = (NumSamples > 1) ? static_cast<float>(i) / static_cast<float>(NumSamples - 1) : 0.0f;
		const FVector Pos = EvalCubicBezier(P0, C1, C2, P3, T);
		InOutMovement.PathPoints.Add(Pos);
	}
}

void TrafficMovementGeometry::AnalyzeMovementPath(
	const FTrafficMovement& Movement,
	TArray<FMovementSample>& OutSamples)
{
	OutSamples.Reset();

	const int32 NumPts = Movement.PathPoints.Num();
	if (NumPts < 2)
	{
		return;
	}

	TArray<float> Cumulative;
	Cumulative.SetNum(NumPts);
	Cumulative[0] = 0.0f;

	float Accum = 0.0f;
	for (int32 i = 1; i < NumPts; ++i)
	{
		const float SegLen = FVector::Dist(Movement.PathPoints[i - 1], Movement.PathPoints[i]);
		Accum += SegLen;
		Cumulative[i] = Accum;
	}

	OutSamples.SetNum(NumPts);

	for (int32 i = 0; i < NumPts; ++i)
	{
		FMovementSample& Sample = OutSamples[i];
		Sample.S = Cumulative[i];
		Sample.Position = Movement.PathPoints[i];

		FVector Tangent;
		if (i == 0)
		{
			Tangent = (Movement.PathPoints[1] - Movement.PathPoints[0]).GetSafeNormal();
		}
		else if (i == NumPts - 1)
		{
			Tangent = (Movement.PathPoints[NumPts - 1] - Movement.PathPoints[NumPts - 2]).GetSafeNormal();
		}
		else
		{
			Tangent = (Movement.PathPoints[i + 1] - Movement.PathPoints[i - 1]).GetSafeNormal();
		}

		if (!Tangent.IsNearlyZero())
		{
			Tangent.Normalize();
		}
		else
		{
			Tangent = FVector::ForwardVector;
		}

		Sample.Tangent = Tangent;

		float Curvature = 0.0f;
		if (i > 0 && i < NumPts - 1)
		{
			const FVector PPrev = Movement.PathPoints[i - 1];
			const FVector PNext = Movement.PathPoints[i + 1];
			const float DistPrev = FVector::Dist(PPrev, Sample.Position);
			const float DistNext = FVector::Dist(Sample.Position, PNext);
			const float DistChord = FVector::Dist(PPrev, PNext);

			const float Denom = DistPrev * DistNext * DistChord;
			if (Denom > KINDA_SMALL_NUMBER)
			{
				const FVector AB = PPrev - Sample.Position;
				const FVector AC = PNext - Sample.Position;
				const float Area = 0.5f * FVector::CrossProduct(AB, AC).Size();
				Curvature = 4.0f * Area / Denom;
			}
		}

		Sample.Curvature = Curvature;
	}
}

bool TrafficMovementGeometry::ProjectPointOntoMovement(
	const FTrafficMovement& Movement,
	const FVector& WorldLocation,
	float& OutS)
{
	const int32 NumPts = Movement.PathPoints.Num();
	if (NumPts < 2)
	{
		return false;
	}

	float BestDistSq = TNumericLimits<float>::Max();
	float BestS = 0.0f;

	float AccumS = 0.0f;
	for (int32 i = 0; i < NumPts - 1; ++i)
	{
		const FVector A = Movement.PathPoints[i];
		const FVector B = Movement.PathPoints[i + 1];
		const FVector AB = B - A;
		const float SegLenSq = AB.SizeSquared();
		const float SegLen = FMath::Sqrt(SegLenSq);
		if (SegLen <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const float T = FMath::Clamp(FVector::DotProduct(WorldLocation - A, AB) / SegLenSq, 0.0f, 1.0f);
		const FVector Closest = A + T * AB;
		const float DistSq = FVector::DistSquared(Closest, WorldLocation);
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestS = AccumS + (T * SegLen);
		}

		AccumS += SegLen;
	}

	if (BestDistSq == TNumericLimits<float>::Max())
	{
		return false;
	}

	OutS = BestS;
	return true;
}

bool TrafficMovementGeometry::SamplePoseAtS(
	const FTrafficMovement& Movement,
	float S,
	FVector& OutPosition,
	FVector& OutTangent)
{
	const int32 NumPts = Movement.PathPoints.Num();
	if (NumPts < 2)
	{
		return false;
	}

	TArray<float> Cumulative;
	Cumulative.SetNum(NumPts);
	Cumulative[0] = 0.0f;

	float Accum = 0.0f;
	for (int32 i = 1; i < NumPts; ++i)
	{
		const float SegLen = FVector::Dist(Movement.PathPoints[i - 1], Movement.PathPoints[i]);
		Accum += SegLen;
		Cumulative[i] = Accum;
	}

	const float TotalLen = Cumulative.Last();
	if (TotalLen <= KINDA_SMALL_NUMBER)
	{
		OutPosition = Movement.PathPoints[0];
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

	const FVector A = Movement.PathPoints[SegmentIdx];
	const FVector B = Movement.PathPoints[SegmentIdx + 1];
	const float SegStartS = Cumulative[SegmentIdx];
	const float SegLen = FVector::Dist(A, B);
	float T = 0.0f;
	if (SegLen > KINDA_SMALL_NUMBER)
	{
		T = (ClampedS - SegStartS) / SegLen;
	}

	OutPosition = FMath::Lerp(A, B, T);

	// Smooth tangent by interpolating central-difference tangents at the segment endpoints.
	auto TangentAtPoint = [&](int32 PointIdx) -> FVector
	{
		PointIdx = FMath::Clamp(PointIdx, 0, NumPts - 1);
		if (NumPts < 2)
		{
			return FVector::ForwardVector;
		}

		FVector Tangent;
		if (PointIdx == 0)
		{
			Tangent = (Movement.PathPoints[1] - Movement.PathPoints[0]);
		}
		else if (PointIdx == NumPts - 1)
		{
			Tangent = (Movement.PathPoints[NumPts - 1] - Movement.PathPoints[NumPts - 2]);
		}
		else
		{
			Tangent = (Movement.PathPoints[PointIdx + 1] - Movement.PathPoints[PointIdx - 1]);
		}

		Tangent = Tangent.GetSafeNormal();
		return Tangent.IsNearlyZero() ? FVector::ForwardVector : Tangent;
	};

	FVector TanA = TangentAtPoint(SegmentIdx);
	FVector TanB = TangentAtPoint(SegmentIdx + 1);
	if (FVector::DotProduct(TanA, TanB) < 0.f)
	{
		TanB = -TanB;
	}

	OutTangent = FMath::Lerp(TanA, TanB, T).GetSafeNormal();
	if (OutTangent.IsNearlyZero())
	{
		OutTangent = (B - A).GetSafeNormal();
		if (OutTangent.IsNearlyZero())
		{
			OutTangent = FVector::ForwardVector;
		}
	}

	return true;
}
