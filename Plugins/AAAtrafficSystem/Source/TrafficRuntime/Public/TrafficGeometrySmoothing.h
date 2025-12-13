#pragma once

#include "CoreMinimal.h"

namespace TrafficGeometrySmoothing
{
	struct FBezierSegment
	{
		FVector P0 = FVector::ZeroVector;
		FVector P1 = FVector::ZeroVector;
		FVector P2 = FVector::ZeroVector;
		FVector P3 = FVector::ZeroVector;
	};

	inline FVector BezierInterpolate(
		const FVector& B0,
		const FVector& B1,
		const FVector& B2,
		const FVector& B3,
		float T)
	{
		const float U = 1.0f - T;
		const float TT = T * T;
		const float UU = U * U;
		const float UUU = UU * U;
		const float TTT = TT * T;

		return (UUU * B0) +
			(3.0f * UU * T * B1) +
			(3.0f * U * TT * B2) +
			(TTT * B3);
	}

	// Compute turn angles (curvature) at each interior point of a polyline in XY.
	// The returned array has the same length as the input; the first and last angles are zero.
	inline void ComputeCurvatureAngles(const TArray<FVector>& Points, TArray<float>& OutAngles)
	{
		const int32 Count = Points.Num();
		OutAngles.SetNumZeroed(Count);
		if (Count < 3)
		{
			return;
		}

		for (int32 i = 1; i < Count - 1; ++i)
		{
			FVector2D V0(Points[i].X - Points[i - 1].X, Points[i].Y - Points[i - 1].Y);
			FVector2D V1(Points[i + 1].X - Points[i].X, Points[i + 1].Y - Points[i].Y);

			if (!V0.Normalize() || !V1.Normalize())
			{
				OutAngles[i] = 0.f;
				continue;
			}

			const float Dot = FMath::Clamp(FVector2D::DotProduct(V0, V1), -1.f, 1.f);
			OutAngles[i] = FMath::Acos(Dot);
		}
	}

	// Detect intervals where curvature exceeds a threshold. Each interval is an inclusive (start,end) index pair.
	// Adjacent high-curvature points are merged into a single interval.
	inline void DetectCurvatureSpikes(
		const TArray<FVector>& Points,
		float ThresholdRadians,
		TArray<FIntPoint>& OutIntervals)
	{
		OutIntervals.Reset();
		if (Points.Num() < 3)
		{
			return;
		}

		TArray<float> Angles;
		ComputeCurvatureAngles(Points, Angles);

		bool bInInterval = false;
		int32 StartIdx = 0;

		for (int32 i = 1; i < Angles.Num() - 1; ++i)
		{
			if (Angles[i] > ThresholdRadians)
			{
				if (!bInInterval)
				{
					bInInterval = true;
					StartIdx = FMath::Max(0, i - 1);
				}
			}
			else if (bInInterval)
			{
				const int32 EndIdx = FMath::Min(Angles.Num() - 1, i + 1);
				OutIntervals.Add(FIntPoint(StartIdx, EndIdx));
				bInInterval = false;
			}
		}

		if (bInInterval)
		{
			OutIntervals.Add(FIntPoint(StartIdx, Angles.Num() - 1));
		}
	}

	// Replace each high-curvature interval with a cubic Bézier segment that smoothly connects its endpoints.
	// The number of points produced matches the number of points in the replaced interval.
	inline void ReplaceSpikeRegions(
		const TArray<FVector>& InPoints,
		const TArray<FIntPoint>& Intervals,
		TArray<FVector>& OutPoints)
	{
		OutPoints.Reset();
		if (Intervals.Num() == 0)
		{
			OutPoints = InPoints;
			return;
		}

		int32 PrevEnd = 0;
		for (const FIntPoint& Interval : Intervals)
		{
			const int32 Start = FMath::Clamp(Interval.X, 0, InPoints.Num() - 1);
			const int32 End = FMath::Clamp(Interval.Y, Start, InPoints.Num() - 1);

			for (int32 i = PrevEnd; i < Start; ++i)
			{
				OutPoints.Add(InPoints[i]);
			}

			const FVector& P0 = InPoints[Start];
			const FVector& P3 = InPoints[End];

			FVector T0 = FVector::ZeroVector;
			FVector T3 = FVector::ZeroVector;
			if (Start > 0)
			{
				T0 = InPoints[Start] - InPoints[Start - 1];
			}
			else if (Start + 1 < InPoints.Num())
			{
				T0 = InPoints[Start + 1] - InPoints[Start];
			}

			if (End + 1 < InPoints.Num())
			{
				T3 = InPoints[End + 1] - InPoints[End];
			}
			else if (End > 0)
			{
				T3 = InPoints[End] - InPoints[End - 1];
			}

			const float Dist = FVector::Dist(P0, P3);
			const float CtrlDist = Dist * 0.25f;
			if (!T0.IsNearlyZero())
			{
				T0 = T0.GetSafeNormal() * CtrlDist;
			}
			if (!T3.IsNearlyZero())
			{
				T3 = T3.GetSafeNormal() * CtrlDist;
			}

			const FVector B0 = P0;
			const FVector B1 = P0 + T0;
			const FVector B2 = P3 - T3;
			const FVector B3 = P3;

			const int32 ReplaceCount = End - Start;
			if (ReplaceCount <= 0)
			{
				OutPoints.Add(B3);
			}
			else
			{
				for (int32 j = 0; j < ReplaceCount; ++j)
				{
					const float T = static_cast<float>(j) / static_cast<float>(ReplaceCount);
					OutPoints.Add(BezierInterpolate(B0, B1, B2, B3, T));
				}
				OutPoints.Add(B3);
			}

			PrevEnd = End + 1;
		}

		for (int32 i = PrevEnd; i < InPoints.Num(); ++i)
		{
			OutPoints.Add(InPoints[i]);
		}
	}

	// Compute blending weights between guide and mesh candidates based on curvature and positional differences.
	// Higher weights favour the mesh candidate. Endpoints are always assigned zero weight.
	inline void ComputeBlendWeights(
		const TArray<FVector>& Guide,
		const TArray<FVector>& Mesh,
		float CurvThresholdRad,
		float DistThreshold,
		TArray<float>& OutWeights)
	{
		const int32 Count = FMath::Min(Guide.Num(), Mesh.Num());
		OutWeights.SetNumZeroed(Count);
		if (Count < 3)
		{
			return;
		}

		TArray<float> GuideCurv;
		ComputeCurvatureAngles(Guide, GuideCurv);
		TArray<float> MeshCurv;
		ComputeCurvatureAngles(Mesh, MeshCurv);

		for (int32 i = 0; i < Count; ++i)
		{
			float Weight = 0.f;

			if (GuideCurv.IsValidIndex(i) && MeshCurv.IsValidIndex(i) &&
				GuideCurv[i] > CurvThresholdRad && MeshCurv[i] < GuideCurv[i])
			{
				Weight += 0.5f;
			}

			const float Dist2D = FVector::Dist2D(Guide[i], Mesh[i]);
			if (Dist2D > DistThreshold)
			{
				Weight += 0.5f;
			}

			OutWeights[i] = FMath::Clamp(Weight, 0.f, 1.f);
		}

		OutWeights[0] = 0.f;
		OutWeights[Count - 1] = 0.f;
	}

	// Blend two polylines per-sample according to weights (0 = guide, 1 = mesh).
	inline void BlendPolylinesWeighted(
		const TArray<FVector>& Guide,
		const TArray<FVector>& Mesh,
		const TArray<float>& Weights,
		TArray<FVector>& OutPoints)
	{
		const int32 Count = FMath::Min3(Guide.Num(), Mesh.Num(), Weights.Num());
		OutPoints.SetNumUninitialized(Count);

		for (int32 i = 0; i < Count; ++i)
		{
			const float W = Weights[i];
			OutPoints[i] = Guide[i] * (1.f - W) + Mesh[i] * W;
		}
	}

	// Convert a polyline to uniform Catmull-Rom and then to cubic Bézier segments.
	inline void CatmullRomToBezier(const TArray<FVector>& Points, TArray<FBezierSegment>& OutSegments)
	{
		OutSegments.Reset();
		const int32 Count = Points.Num();
		if (Count < 2)
		{
			return;
		}

		OutSegments.Reserve(Count - 1);

		for (int32 i = 0; i < Count - 1; ++i)
		{
			const FVector& P1 = Points[i];
			const FVector& P2 = Points[i + 1];

			const FVector P0 = (i > 0) ? Points[i - 1] : (P1 + (P1 - P2));
			const FVector P3 = (i + 2 < Count) ? Points[i + 2] : (P2 + (P2 - P1));

			FBezierSegment Segment;
			Segment.P0 = P1;
			Segment.P1 = P1 + (P2 - P0) / 6.0f;
			Segment.P2 = P2 - (P3 - P1) / 6.0f;
			Segment.P3 = P2;
			OutSegments.Add(Segment);
		}
	}

	// Sample Bézier segments uniformly. SamplesPerSegment controls how many sub-steps are generated per segment.
	// Output point count will be Segments.Num() * SamplesPerSegment + 1.
	inline void SampleBezierSegments(
		const TArray<FBezierSegment>& Segments,
		int32 SamplesPerSegment,
		TArray<FVector>& OutPoints)
	{
		OutPoints.Reset();
		if (Segments.Num() == 0)
		{
			return;
		}

		SamplesPerSegment = FMath::Max(1, SamplesPerSegment);
		OutPoints.Reserve(Segments.Num() * SamplesPerSegment + 1);

		OutPoints.Add(Segments[0].P0);
		for (const FBezierSegment& Segment : Segments)
		{
			for (int32 i = 1; i <= SamplesPerSegment; ++i)
			{
				const float T = static_cast<float>(i) / static_cast<float>(SamplesPerSegment);
				OutPoints.Add(BezierInterpolate(Segment.P0, Segment.P1, Segment.P2, Segment.P3, T));
			}
		}
	}
}

