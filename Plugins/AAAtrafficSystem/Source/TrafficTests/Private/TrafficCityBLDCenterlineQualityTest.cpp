#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

#include "TrafficAutomationLogger.h"
#include "TrafficGeometryProvider.h"
#include "TrafficGeometryProviderFactory.h"
#include "TrafficRuntimeModule.h"

#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SplineComponent.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"

namespace
{
	struct FCenterlineQualityMetrics
	{
		int32 NumPoints = 0;
		float LengthCm = 0.f;
		float MaxHeadingStepDeg = 0.f;
		int32 SpikeCount = 0;
	};

	static FCenterlineQualityMetrics EvaluateCenterline(const TArray<FVector>& Points, const float SpikeThresholdDeg)
	{
		FCenterlineQualityMetrics Metrics;
		Metrics.NumPoints = Points.Num();

		if (Points.Num() < 2)
		{
			return Metrics;
		}

		for (int32 i = 0; i + 1 < Points.Num(); ++i)
		{
			Metrics.LengthCm += FVector::Dist(Points[i], Points[i + 1]);
		}

		for (int32 i = 0; i + 2 < Points.Num(); ++i)
		{
			const FVector Dir0 = (Points[i + 1] - Points[i]).GetSafeNormal2D();
			const FVector Dir1 = (Points[i + 2] - Points[i + 1]).GetSafeNormal2D();
			if (Dir0.IsNearlyZero() || Dir1.IsNearlyZero())
			{
				continue;
			}

			const float Dot = FMath::Clamp(FVector::DotProduct(Dir0, Dir1), -1.f, 1.f);
			const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));
			Metrics.MaxHeadingStepDeg = FMath::Max(Metrics.MaxHeadingStepDeg, AngleDeg);
			if (AngleDeg > SpikeThresholdDeg)
			{
				++Metrics.SpikeCount;
			}
		}

		return Metrics;
	}

	static void CollectCityBLDRoadActors(UWorld* World, TArray<AActor*>& OutActors, int32 MaxActors)
	{
		OutActors.Reset();
		if (!World || MaxActors <= 0)
		{
			return;
		}

		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor)
			{
				continue;
			}

			if (Actor->GetClass()->GetName().Contains(TEXT("BP_MeshRoad"), ESearchCase::IgnoreCase))
			{
				OutActors.Add(Actor);
				if (OutActors.Num() >= MaxActors)
				{
					return;
				}
			}
		}
	}

	static void EnableCollisionForActor(AActor* Actor)
	{
		if (!Actor)
		{
			return;
		}

		TArray<UPrimitiveComponent*> PrimComps;
		Actor->GetComponents<UPrimitiveComponent>(PrimComps);
		for (UPrimitiveComponent* Prim : PrimComps)
		{
			if (!Prim)
			{
				continue;
			}

			Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Prim->SetCollisionObjectType(ECC_WorldStatic);
			Prim->SetCollisionResponseToAllChannels(ECR_Block);
		}
	}

	static bool ConfigureControlSplineWorld(AActor* RoadActor, const TArray<FVector>& WorldPoints)
	{
		if (!RoadActor || WorldPoints.Num() < 2)
		{
			return false;
		}

		const USplineComponent* ControlSplineConst = nullptr;
		TArray<USplineComponent*> Splines;
		RoadActor->GetComponents<USplineComponent>(Splines);
		for (USplineComponent* Spline : Splines)
		{
			if (Spline && Spline->GetName().Contains(TEXT("Control"), ESearchCase::IgnoreCase))
			{
				ControlSplineConst = Spline;
				break;
			}
		}
		if (!ControlSplineConst && Splines.Num() > 0)
		{
			ControlSplineConst = Splines[0];
		}

		USplineComponent* ControlSpline = const_cast<USplineComponent*>(ControlSplineConst);
		if (!ControlSpline)
		{
			return false;
		}

		ControlSpline->ClearSplinePoints(false);
		for (const FVector& P : WorldPoints)
		{
			ControlSpline->AddSplinePoint(P, ESplineCoordinateSpace::World, false);
			const int32 NewIndex = ControlSpline->GetNumberOfSplinePoints() - 1;
			ControlSpline->SetSplinePointType(NewIndex, ESplinePointType::Linear, false);
		}
		ControlSpline->UpdateSpline();
		RoadActor->RerunConstructionScripts();
		return true;
	}

	static UClass* LoadCityBLDMeshRoadClass(FString& OutUsedPath)
	{
		OutUsedPath.Reset();

		static const TCHAR* CandidatePaths[] =
		{
			TEXT("/CityBLD/Blueprints/Roads/BP_MeshRoad.BP_MeshRoad_C"),
			TEXT("/CityBLD/CityBLD/Blueprints/Roads/BP_MeshRoad.BP_MeshRoad_C"),
		};

		for (const TCHAR* Path : CandidatePaths)
		{
			if (UClass* Loaded = LoadClass<AActor>(nullptr, Path))
			{
				OutUsedPath = Path;
				return Loaded;
			}
		}

		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficCityBLDCenterlineQualityTest,
	"Traffic.CityBLD.CenterlineQuality",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficCityBLDCenterlineQualityTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	const FString LocalTestName = TEXT("Traffic.CityBLD.CenterlineQuality");
	UTrafficAutomationLogger::BeginTestLog(LocalTestName);
	UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION)));

	constexpr int32 MaxRoadsToCheck = 10;
	constexpr float SpikeThresholdDeg = 25.f;
	constexpr float MaxAllowedHeadingStepDeg = 25.f;
	constexpr int32 MaxAllowedSpikeCount = 0;
	constexpr float MinCenterlineLengthCm = 500.f;

	if (!GEditor)
	{
		AddError(TEXT("GEditor is null."));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	// Prefer the RoadLab map (real-world content), otherwise fall back to a deterministic synthetic setup.
	const FString RoadLabMapPath = TEXT("/Game/Maps/Test_Maps/RoadLab/Traffic_RoadLab");
	const bool bUseRoadLabMap = FParse::Param(FCommandLine::Get(), TEXT("TrafficUseRoadLab"));
	bool bLoadedRoadLab = false;
	if (bUseRoadLabMap && !IsRunningCommandlet())
	{
		bLoadedRoadLab = AutomationOpenMap(RoadLabMapPath);
		if (!bLoadedRoadLab && GEditor && !GEditor->PlayWorld && !GIsPlayInEditorWorld)
		{
			FAutomationEditorCommonUtils::CreateNewMap();
		}
	}
	UTrafficAutomationLogger::LogMetricInt(TEXT("RoadLabMapLoaded"), bLoadedRoadLab ? 1 : 0);

	UWorld* World = nullptr;
	if (bLoadedRoadLab && GEditor)
	{
		World = GEditor->GetEditorWorldContext().World();
	}
	else
	{
		World = AutomationCommon::GetAnyGameWorld();
		if (!World && GEditor)
		{
			World = GEditor->GetEditorWorldContext().World();
		}
	}
	if (!World)
	{
		AddError(TEXT("World is null."));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	TArray<AActor*> Roads;
	CollectCityBLDRoadActors(World, Roads, MaxRoadsToCheck);

	bool bUsedSyntheticRoads = false;

	// If the map doesn't contain any CityBLD roads (streaming unloaded, missing content, etc),
	// fall back to spawning a few deterministic BP_MeshRoad instances and forcing a kinked control spline.
	if (Roads.Num() == 0)
	{
		FString MeshRoadClassPath;
		UClass* MeshRoadClass = LoadCityBLDMeshRoadClass(MeshRoadClassPath);
		if (!MeshRoadClass)
		{
			AddError(TEXT("Failed to load CityBLD BP_MeshRoad class (tried /CityBLD/Blueprints/... and /CityBLD/CityBLD/Blueprints/...)."));
			UTrafficAutomationLogger::EndTestLog();
			return false;
		}
		UTrafficAutomationLogger::LogMetric(TEXT("CityBLD.MeshRoadClassPath"), MeshRoadClassPath);

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AActor* RoadA = World->SpawnActor<AActor>(MeshRoadClass, FTransform::Identity, Params);
		AActor* RoadB = World->SpawnActor<AActor>(MeshRoadClass, FTransform(FRotator(0.f, 90.f, 0.f), FVector(0.f, 0.f, 0.f)), Params);
		AActor* RoadC = World->SpawnActor<AActor>(MeshRoadClass, FTransform(FRotator(0.f, -45.f, 0.f), FVector(500.f, 500.f, 0.f)), Params);

		EnableCollisionForActor(RoadA);
		EnableCollisionForActor(RoadB);
		EnableCollisionForActor(RoadC);

		const FVector Base = FVector::ZeroVector;
		ConfigureControlSplineWorld(RoadA, { Base, Base + FVector(3000.f, 0.f, 0.f), Base + FVector(3000.f, 3000.f, 0.f) });
		ConfigureControlSplineWorld(RoadB, { Base + FVector(0.f, 5000.f, 0.f), Base + FVector(3000.f, 5000.f, 0.f), Base + FVector(3000.f, 8000.f, 0.f) });
		ConfigureControlSplineWorld(RoadC, { Base + FVector(5000.f, 0.f, 0.f), Base + FVector(8000.f, 0.f, 0.f), Base + FVector(8000.f, 3000.f, 0.f) });

		CollectCityBLDRoadActors(World, Roads, MaxRoadsToCheck);
		bUsedSyntheticRoads = true;
	}

	UTrafficAutomationLogger::LogMetricInt(TEXT("UsedSyntheticRoads"), bUsedSyntheticRoads ? 1 : 0);

	if (Roads.Num() == 0)
	{
		AddError(TEXT("No CityBLD BP_MeshRoad actors found to evaluate."));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	TArray<TObjectPtr<UObject>> ProviderObjects;
	TArray<ITrafficRoadGeometryProvider*> Providers;
	UTrafficGeometryProviderFactory::CreateProviderChainForEditorWorld(World, ProviderObjects, Providers);

	int32 NumFailed = 0;
	int32 NumEvaluated = 0;

	for (int32 RoadIdx = 0; RoadIdx < Roads.Num(); ++RoadIdx)
	{
		AActor* RoadActor = Roads[RoadIdx];
		if (!RoadActor)
		{
			continue;
		}

		TArray<FVector> CenterlinePoints;
		bool bGotCenterline = false;
		FString ProviderUsed(TEXT("None"));

		for (int32 ProviderIdx = 0; ProviderIdx < Providers.Num(); ++ProviderIdx)
		{
			ITrafficRoadGeometryProvider* Provider = Providers[ProviderIdx];
			if (!Provider)
			{
				continue;
			}

			if (Provider->GetDisplayCenterlineForActor(RoadActor, CenterlinePoints) && CenterlinePoints.Num() >= 2)
			{
				bGotCenterline = true;
				if (ProviderObjects.IsValidIndex(ProviderIdx) && ProviderObjects[ProviderIdx])
				{
					ProviderUsed = ProviderObjects[ProviderIdx]->GetClass()->GetName();
				}
				break;
			}
		}

		if (!bGotCenterline)
		{
			++NumFailed;
			AddError(FString::Printf(TEXT("No provider could supply a display centerline for '%s'."), *GetNameSafe(RoadActor)));
			continue;
		}

		const FCenterlineQualityMetrics Metrics = EvaluateCenterline(CenterlinePoints, SpikeThresholdDeg);

		if (Metrics.LengthCm < MinCenterlineLengthCm)
		{
			UTrafficAutomationLogger::LogLine(FString::Printf(
				TEXT("Skip=%s Reason=DegenerateCenterline LengthCm=%.3f"),
				*GetNameSafe(RoadActor),
				Metrics.LengthCm));
			continue;
		}

		++NumEvaluated;

		const FString Prefix = FString::Printf(TEXT("Road%d"), RoadIdx);
		UTrafficAutomationLogger::LogMetric(Prefix + TEXT(".Actor"), GetNameSafe(RoadActor));
		UTrafficAutomationLogger::LogMetric(Prefix + TEXT(".Provider"), ProviderUsed);
		UTrafficAutomationLogger::LogMetricInt(Prefix + TEXT(".NumPoints"), Metrics.NumPoints);
		UTrafficAutomationLogger::LogMetricFloat(Prefix + TEXT(".LengthCm"), Metrics.LengthCm, 1);
		UTrafficAutomationLogger::LogMetricFloat(Prefix + TEXT(".MaxHeadingStepDeg"), Metrics.MaxHeadingStepDeg, 2);
		UTrafficAutomationLogger::LogMetricInt(Prefix + TEXT(".SpikeCount"), Metrics.SpikeCount);

		if (Metrics.MaxHeadingStepDeg > MaxAllowedHeadingStepDeg || Metrics.SpikeCount > MaxAllowedSpikeCount)
		{
			++NumFailed;
			AddError(FString::Printf(
				TEXT("Centerline quality thresholds failed for '%s' (Provider=%s MaxHeadingStepDeg=%.2f SpikeCount=%d)."),
				*GetNameSafe(RoadActor),
				*ProviderUsed,
				Metrics.MaxHeadingStepDeg,
				Metrics.SpikeCount));
		}
	}

	UTrafficAutomationLogger::LogMetricInt(TEXT("RoadsChecked"), Roads.Num());
	UTrafficAutomationLogger::LogMetricInt(TEXT("RoadsEvaluated"), NumEvaluated);
	UTrafficAutomationLogger::LogMetricInt(TEXT("Failures"), NumFailed);
	if (NumEvaluated <= 0)
	{
		AddError(TEXT("No non-degenerate CityBLD centerlines were evaluated."));
	}
	UTrafficAutomationLogger::EndTestLog();
	return !HasAnyErrors();
#else
	return true;
#endif
}
