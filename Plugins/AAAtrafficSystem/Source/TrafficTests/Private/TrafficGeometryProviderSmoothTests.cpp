#include "Misc/AutomationTest.h"

#include "TrafficGeometrySmoothing.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficSpikeReplacementTest,
	"Traffic.GeometryProvider.Math.SpikeReplacement",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FTrafficSpikeReplacementTest::RunTest(const FString& Parameters)
{
	TArray<FVector> Raw;
	Raw.Add(FVector(0.f, 0.f, 0.f));
	Raw.Add(FVector(1000.f, 0.f, 0.f));
	Raw.Add(FVector(1000.f, 1000.f, 0.f));
	Raw.Add(FVector(2000.f, 1000.f, 0.f));

	TArray<FIntPoint> Intervals;
	TrafficGeometrySmoothing::DetectCurvatureSpikes(Raw, FMath::DegreesToRadians(35.f), Intervals);

	TArray<FVector> Replaced;
	TrafficGeometrySmoothing::ReplaceSpikeRegions(Raw, Intervals, Replaced);

	auto ComputeMaxAngle = [](const TArray<FVector>& Points) -> float
	{
		TArray<float> Angles;
		TrafficGeometrySmoothing::ComputeCurvatureAngles(Points, Angles);

		float MaxAngle = 0.f;
		for (const float Angle : Angles)
		{
			MaxAngle = FMath::Max(MaxAngle, Angle);
		}
		return MaxAngle;
	};

	const float RawMax = ComputeMaxAngle(Raw);
	const float ReplacedMax = ComputeMaxAngle(Replaced);

	TestTrue(TEXT("Spike replacement reduces maximum turn angle"), ReplacedMax < RawMax);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficAdaptiveBlendTest,
	"Traffic.GeometryProvider.Math.AdaptiveBlending",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FTrafficAdaptiveBlendTest::RunTest(const FString& Parameters)
{
	TArray<FVector> Guide;
	Guide.Add(FVector(0.f, 0.f, 0.f));
	Guide.Add(FVector(1000.f, 0.f, 0.f));
	Guide.Add(FVector(1000.f, 1000.f, 0.f));
	Guide.Add(FVector(2000.f, 1000.f, 0.f));

	TArray<FVector> Mesh;
	Mesh.Add(FVector(0.f, 0.f, 0.f));
	Mesh.Add(FVector(1000.f, 0.f, 0.f));
	Mesh.Add(FVector(1500.f, 500.f, 0.f));
	Mesh.Add(FVector(2000.f, 1000.f, 0.f));

	TArray<float> Weights;
	TrafficGeometrySmoothing::ComputeBlendWeights(Guide, Mesh, FMath::DegreesToRadians(35.f), 200.f, FMath::DegreesToRadians(20.f), Weights);

	TArray<FVector> Blended;
	TrafficGeometrySmoothing::BlendPolylinesWeighted(Guide, Mesh, Weights, Blended);

	TestTrue(TEXT("Blended point count matches input"), Blended.Num() == Guide.Num());

	const float DistGuide = FVector::Dist2D(Blended[2], Guide[2]);
	const float DistMesh = FVector::Dist2D(Blended[2], Mesh[2]);
	TestTrue(TEXT("Adaptive blending favours mesh on sharp turn"), DistMesh < DistGuide);

	TestTrue(TEXT("Blending preserves endpoints (start)"), FVector::DistSquared(Blended[0], Guide[0]) < 1e-2f);
	TestTrue(TEXT("Blending preserves endpoints (end)"), FVector::DistSquared(Blended.Last(), Guide.Last()) < 1e-2f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficTangentAlignmentTest,
	"Traffic.GeometryProvider.Math.TangentAlignment",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FTrafficTangentAlignmentTest::RunTest(const FString& Parameters)
{
	// Guide: straight line along X axis.
	TArray<FVector> Guide;
	Guide.Add(FVector(0.f, 0.f, 0.f));
	Guide.Add(FVector(1000.f, 0.f, 0.f));
	Guide.Add(FVector(2000.f, 0.f, 0.f));
	Guide.Add(FVector(3000.f, 0.f, 0.f));

	// Mesh: deviates into Y direction (misaligned tangents).
	TArray<FVector> Mesh;
	Mesh.Add(FVector(0.f, 0.f, 0.f));
	Mesh.Add(FVector(1000.f, 500.f, 0.f));
	Mesh.Add(FVector(2000.f, 1000.f, 0.f));
	Mesh.Add(FVector(3000.f, 1500.f, 0.f));

	TArray<float> Weights;
	TrafficGeometrySmoothing::ComputeBlendWeights(
		Guide,
		Mesh,
		FMath::DegreesToRadians(35.f),
		200.f,
		FMath::DegreesToRadians(20.f),
		Weights);

	bool bAllZero = true;
	for (int32 i = 1; i < Weights.Num() - 1; ++i)
	{
		if (Weights[i] > 0.f)
		{
			bAllZero = false;
			break;
		}
	}

	TestTrue(TEXT("Tangent alignment: all interior weights are zero"), bAllZero);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficCurvatureDistanceTest,
	"Traffic.GeometryProvider.Math.CurvatureDistance",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FTrafficCurvatureDistanceTest::RunTest(const FString& Parameters)
{
	// Guide: sharp 90-degree corner.
	TArray<FVector> Guide;
	Guide.Add(FVector(0.f, 0.f, 0.f));
	Guide.Add(FVector(1000.f, 0.f, 0.f));
	Guide.Add(FVector(1000.f, 1000.f, 0.f));
	Guide.Add(FVector(2000.f, 1000.f, 0.f));

	// Mesh: smooths the corner.
	TArray<FVector> Mesh;
	Mesh.Add(FVector(0.f, 0.f, 0.f));
	Mesh.Add(FVector(1000.f, 0.f, 0.f));
	Mesh.Add(FVector(1500.f, 500.f, 0.f));
	Mesh.Add(FVector(2000.f, 1000.f, 0.f));

	TArray<float> Weights;
	TrafficGeometrySmoothing::ComputeBlendWeights(
		Guide,
		Mesh,
		FMath::DegreesToRadians(35.f),
		200.f,
		FMath::DegreesToRadians(20.f),
		Weights);

	TestTrue(TEXT("Corner weight > 0"), Weights.IsValidIndex(2) && Weights[2] > 0.f);
	TestTrue(TEXT("Endpoints have zero weight (start)"), Weights.IsValidIndex(0) && Weights[0] == 0.f);
	TestTrue(TEXT("Endpoints have zero weight (end)"), Weights.Num() > 0 && Weights.Last() == 0.f);
	return true;
}
