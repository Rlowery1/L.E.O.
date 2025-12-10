#include "TrafficMovementGeometry.h"
#include "TrafficRuntimeModule.h"

namespace
{
	FVector EvalQuadraticBezier(const FVector& P0, const FVector& P1, const FVector& P2, float T)
	{
		const float OneMinusT = 1.0f - T;
		const float OneMinusT2 = OneMinusT * OneMinusT;
		const float T2 = T * T;

		const float X = OneMinusT2 * P0.X + 2.0f * OneMinusT * T * P1.X + T2 * P2.X;
		const float Y = OneMinusT2 * P0.Y + 2.0f * OneMinusT * T * P1.Y + T2 * P2.Y;
		const float Z = OneMinusT2 * P0.Z + 2.0f * OneMinusT * T * P1.Z + T2 * P2.Z;

		return FVector(X, Y, Z);
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
	const FVector P2 = InEndPos;

	const float ChordLen = FVector::Dist(P0, P2);
	const float ControlDist = 0.5f * ChordLen;

	const FVector P1 = 0.5f * (P0 + InStartDir * ControlDist + P2 - InEndDir * ControlDist);

	InOutMovement.PathPoints.Reset();
	InOutMovement.PathPoints.Reserve(NumSamples);

	for (int32 i = 0; i < NumSamples; ++i)
	{
		const float T = (NumSamples > 1) ? static_cast<float>(i) / static_cast<float>(NumSamples - 1) : 0.0f;
		const FVector Pos = EvalQuadraticBezier(P0, P1, P2, T);
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
	OutTangent = (B - A).GetSafeNormal();
	if (OutTangent.IsNearlyZero())
	{
		OutTangent = FVector::ForwardVector;
	}

	return true;
}

