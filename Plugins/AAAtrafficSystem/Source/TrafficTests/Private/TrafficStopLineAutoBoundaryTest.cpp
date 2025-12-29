#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TrafficSystemController.h"
#include "TrafficNetworkAsset.h"
#include "TrafficRoadTypes.h"
#include "TrafficLaneGeometry.h"
#include "TrafficKinematicFollower.h"
#include "TrafficRouting.h"
#include "TrafficVehicleAdapter.h"
#include "TrafficVehicleBase.h"
#include "TrafficChaosTestUtils.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"

#if WITH_EDITOR

namespace
{
	static const TCHAR* StopLineMapPackage = TEXT("/AAAtrafficSystem/Maps/Traffic_BaselineCurve");
	static const float StopLineMaxChaosPathErrorCm = 200.0f;

	struct FTrafficStopLineAutoState
	{
		TWeakObjectPtr<UWorld> PIEWorld;
		bool bSavedCVars = false;
		bool bSavedVisualMode = false;
		int32 PrevStopLineAuto = 0;
		int32 PrevVisualMode = 0;
		bool bFailed = false;
	};

	static float ReadFloatCVar_StopLine(const TCHAR* Name, float DefaultValue)
	{
		if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			return Var->GetFloat();
		}
		return DefaultValue;
	}

	static int32 ReadIntCVar_StopLine(const TCHAR* Name, int32 DefaultValue)
	{
		if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			return Var->GetInt();
		}
		return DefaultValue;
	}

	static float ComputeAutoStopLineOffsetCm_Test(float LaneWidthCm)
	{
		const float Width = (LaneWidthCm > KINDA_SMALL_NUMBER)
			? LaneWidthCm
			: FMath::Max(0.f, ReadFloatCVar_StopLine(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoNominalLaneWidthCm"), 350.f));
		const float Scale = FMath::Max(0.f, ReadFloatCVar_StopLine(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoWidthScale"), 0.25f));
		const float MinCm = FMath::Max(0.f, ReadFloatCVar_StopLine(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoMinCm"), 40.f));
		const float MaxCm = FMath::Max(MinCm, ReadFloatCVar_StopLine(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoMaxCm"), 120.f));
		const float BufferCm = FMath::Max(0.f, ReadFloatCVar_StopLine(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoBufferCm"), 50.f));
		return FMath::Clamp(Width * Scale, MinCm, MaxCm) + BufferCm;
	}

	static float GetEffectiveStopLineOffsetCm_Test(const FTrafficLane* Lane)
	{
		const float Base = FMath::Max(0.f, ReadFloatCVar_StopLine(TEXT("aaa.Traffic.Intersections.StopLineOffsetCm"), 0.f));
		if (ReadIntCVar_StopLine(TEXT("aaa.Traffic.Intersections.StopLineOffsetAuto"), 0) == 0)
		{
			return Base;
		}

		const float LaneWidth = Lane ? Lane->Width : 0.f;
		return ComputeAutoStopLineOffsetCm_Test(LaneWidth);
	}

	static float ComputeStopLineBoundaryRadiusCm_Test(const FTrafficLane* Lane, float IntersectionRadius)
	{
		const float LaneWidth = (Lane && Lane->Width > KINDA_SMALL_NUMBER)
			? Lane->Width
			: FMath::Max(0.f, ReadFloatCVar_StopLine(TEXT("aaa.Traffic.Intersections.StopLineOffsetAutoNominalLaneWidthCm"), 350.f));
		const float LaneWidthScale = FMath::Max(0.f, ReadFloatCVar_StopLine(TEXT("aaa.Traffic.Intersections.StopLineBoundaryRadiusLaneWidthScale"), 1.2f));
		float Radius = FMath::Min(IntersectionRadius, LaneWidth * LaneWidthScale);

		const float MinCm = FMath::Max(0.f, ReadFloatCVar_StopLine(TEXT("aaa.Traffic.Intersections.StopLineBoundaryRadiusMinCm"), 60.f));
		const float MaxCm = FMath::Max(MinCm, ReadFloatCVar_StopLine(TEXT("aaa.Traffic.Intersections.StopLineBoundaryRadiusMaxCm"), 220.f));
		Radius = FMath::Clamp(Radius, MinCm, MaxCm);
		Radius += ReadFloatCVar_StopLine(TEXT("aaa.Traffic.Intersections.StopLineBoundaryRadiusBiasCm"), 20.f);
		return FMath::Max(0.f, Radius);
	}

	static bool IntersectRayCircle2D_StopLineTest(
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

	static bool TryComputeStopSFromIntersectionBoundary_Test(
		const FTrafficNetwork& Net,
		const FTrafficLane& Lane,
		const int32 IntersectionId,
		const float StopLineOffsetCm,
		float& OutStopS)
	{
		const FTrafficIntersection* Intersection = Net.Intersections.FindByPredicate(
			[&](const FTrafficIntersection& I) { return I.IntersectionId == IntersectionId; });
		if (!Intersection)
		{
			return false;
		}

		const float Radius = Intersection->Radius;
		if (Radius <= KINDA_SMALL_NUMBER || Lane.CenterlinePoints.Num() < 2)
		{
			return false;
		}

		const float BoundaryRadius = ComputeStopLineBoundaryRadiusCm_Test(&Lane, Radius);
		if (BoundaryRadius <= KINDA_SMALL_NUMBER)
		{
			return false;
		}
		const float BoundaryRadiusSq = BoundaryRadius * BoundaryRadius;

		TArray<float> CumulativeS;
		CumulativeS.SetNum(Lane.CenterlinePoints.Num());
		CumulativeS[0] = 0.f;
		for (int32 i = 1; i < Lane.CenterlinePoints.Num(); ++i)
		{
			CumulativeS[i] = CumulativeS[i - 1] + FVector::Distance(Lane.CenterlinePoints[i - 1], Lane.CenterlinePoints[i]);
		}

		const FVector Center = Intersection->Center;

		for (int32 i = Lane.CenterlinePoints.Num() - 2; i >= 0; --i)
		{
			const FVector P0 = Lane.CenterlinePoints[i];
			const FVector P1 = Lane.CenterlinePoints[i + 1];
			const float D0Sq = FVector::DistSquared(P0, Center);
			const float D1Sq = FVector::DistSquared(P1, Center);

			const bool bP0Outside = (D0Sq > BoundaryRadiusSq);
			const bool bP1InsideOrOn = (D1Sq <= BoundaryRadiusSq);
			if (!(bP0Outside && bP1InsideOrOn))
			{
				continue;
			}

			const FVector Segment = (P1 - P0);
			const float SegmentLen = Segment.Size();
			if (SegmentLen <= KINDA_SMALL_NUMBER)
			{
				continue;
			}

			const FVector D = Segment / SegmentLen;
			const FVector M = P0 - Center;
			const float A = FVector::DotProduct(D, D);
			const float B = 2.0f * FVector::DotProduct(D, M);
			const float CCoef = FVector::DotProduct(M, M) - BoundaryRadiusSq;
			const float Discriminant = (B * B) - (4.0f * A * CCoef);

			float T = -1.f;
			if (Discriminant >= 0.f && A > KINDA_SMALL_NUMBER)
			{
				const float SqrtDisc = FMath::Sqrt(Discriminant);
				const float T0 = (-B - SqrtDisc) / (2.0f * A);
				const float T1 = (-B + SqrtDisc) / (2.0f * A);
				if (T0 >= 0.f && T0 <= SegmentLen)
				{
					T = T0;
				}
				else if (T1 >= 0.f && T1 <= SegmentLen)
				{
					T = T1;
				}
			}

			if (T < 0.f)
			{
				const float D0 = FMath::Sqrt(D0Sq);
				const float D1 = FMath::Sqrt(D1Sq);
				const float Denom = (D0 - D1);
				const float Alpha = (FMath::Abs(Denom) > KINDA_SMALL_NUMBER) ? (D0 - BoundaryRadius) / Denom : 1.f;
				T = FMath::Clamp(Alpha * SegmentLen, 0.f, SegmentLen);
			}

			const float IntersectionS = CumulativeS[i] + T;
			OutStopS = FMath::Max(0.f, IntersectionS - StopLineOffsetCm);
			return true;
		}

		return false;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(FTrafficStopLineWaitForPIEWorldCommand, TSharedRef<FTrafficStopLineAutoState>, State, FAutomationTestBase*, Test, double, TimeoutSeconds, double, StartTime);
	bool FTrafficStopLineWaitForPIEWorldCommand::Update()
	{
		if (GEditor && GEditor->PlayWorld)
		{
			State->PIEWorld = GEditor->PlayWorld;
			return true;
		}

		const double Elapsed = FPlatformTime::Seconds() - StartTime;
		if (Elapsed > TimeoutSeconds)
		{
			if (Test)
			{
				Test->AddError(TEXT("PIE world did not start for stop-line auto test."));
			}
			return true;
		}

		return false;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FTrafficStopLineAutoCheckCommand, TSharedRef<FTrafficStopLineAutoState>, State, FAutomationTestBase*, Test);
	bool FTrafficStopLineAutoCheckCommand::Update()
	{
		if (!State->PIEWorld.IsValid())
		{
			if (Test)
			{
				Test->AddError(TEXT("PIE world missing in stop-line auto test."));
			}
			return true;
		}

		UWorld* World = State->PIEWorld.Get();
		ATrafficSystemController* Controller = nullptr;
		for (TActorIterator<ATrafficSystemController> It(World); It; ++It)
		{
			Controller = *It;
			break;
		}
		if (!Controller)
		{
			if (Test)
			{
				Test->AddError(TEXT("Stop-line auto test could not find TrafficSystemController."));
			}
			return true;
		}

		UTrafficNetworkAsset* NetAsset = Controller->GetBuiltNetworkAsset();
		if (!NetAsset || NetAsset->Network.Lanes.Num() == 0 || NetAsset->Network.Intersections.Num() == 0)
		{
			if (Test)
			{
				Test->AddError(TEXT("Stop-line auto test network not built (no lanes or intersections)."));
			}
			return true;
		}

		const FTrafficNetwork& Net = NetAsset->Network;
		int32 BoundaryCandidates = 0;
		int32 InvalidStopCount = 0;
		int32 LogicVehiclesChecked = 0;
		int32 ChaosVehiclesChecked = 0;
		bool bChaosMissing = false;
		bool bChaosProjectionFailed = false;
		float MaxChaosPathErrorCm_Local = 0.f;

		for (const FTrafficLane& Lane : Net.Lanes)
		{
			const FTrafficMovement* NextMovement = TrafficRouting::ChooseDefaultMovementForIncomingLane(Net, Lane.LaneId);
			if (!NextMovement)
			{
				continue;
			}

			const float StopLineOffset = GetEffectiveStopLineOffsetCm_Test(&Lane);
			if (StopLineOffset <= 0.f)
			{
				InvalidStopCount++;
				continue;
			}

			float BoundaryStopS = 0.f;
			if (!TryComputeStopSFromIntersectionBoundary_Test(Net, Lane, NextMovement->IntersectionId, StopLineOffset, BoundaryStopS))
			{
				continue;
			}

			BoundaryCandidates++;
			const float LaneLen = TrafficLaneGeometry::ComputeLaneLengthCm(Lane);
			const float LaneEndStopS = FMath::Max(0.f, LaneLen - StopLineOffset);
			const float ClampedBoundaryStopS = FMath::Clamp(BoundaryStopS, 0.f, LaneLen);
			const float PreferredStopS = FMath::Max(ClampedBoundaryStopS, LaneEndStopS);
			if (PreferredStopS < (LaneEndStopS - 1.f) && Test)
			{
				Test->AddError(FString::Printf(TEXT("Preferred stop S before lane-end stop (lane=%d preferred=%.1f end=%.1f boundary=%.1f)."),
					Lane.LaneId, PreferredStopS, LaneEndStopS, BoundaryStopS));
			}
			if (PreferredStopS > (LaneLen + 1.f) && Test)
			{
				Test->AddError(FString::Printf(TEXT("Preferred stop S exceeds lane length (lane=%d preferred=%.1f len=%.1f boundary=%.1f)."),
					Lane.LaneId, PreferredStopS, LaneLen, BoundaryStopS));
			}
		}

		for (TActorIterator<ATrafficVehicleBase> It(World); It; ++It)
		{
			ATrafficVehicleBase* Logic = *It;
			if (!Logic)
			{
				continue;
			}

			++LogicVehiclesChecked;
			ATrafficVehicleAdapter* Adapter = nullptr;
			APawn* Chaos = nullptr;
			FString ChaosError;
			if (!TrafficChaosTestUtils::EnsureChaosForLogicVehicle(*World, *Logic, Adapter, Chaos, ChaosError))
			{
				bChaosMissing = true;
				continue;
			}

			EPathFollowTargetType FollowType = EPathFollowTargetType::None;
			int32 FollowId = INDEX_NONE;
			float FollowS = 0.f;
			if (!Logic->GetFollowTarget(FollowType, FollowId, FollowS))
			{
				continue;
			}

			float ChaosS = 0.f;
			float ChaosErrorCm = 0.f;
			if (!TrafficChaosTestUtils::ProjectChaosOntoFollowTarget(Net, *Chaos, FollowType, FollowId, ChaosS, ChaosErrorCm))
			{
				bChaosProjectionFailed = true;
				continue;
			}

			++ChaosVehiclesChecked;
			MaxChaosPathErrorCm_Local = FMath::Max(MaxChaosPathErrorCm_Local, ChaosErrorCm);
		}

		if (InvalidStopCount > 0 && Test)
		{
			Test->AddError(TEXT("Stop-line auto test detected non-positive stop-line offsets."));
		}

		if (BoundaryCandidates < 4 && Test)
		{
			Test->AddError(FString::Printf(TEXT("Stop-line auto test found only %d lanes with boundary crossings (expected >= 4)."),
				BoundaryCandidates));
		}

		if (LogicVehiclesChecked == 0 && Test)
		{
			Test->AddError(TEXT("Stop-line auto test did not find any logic vehicles to validate Chaos coupling."));
		}

		if (bChaosMissing && Test)
		{
			Test->AddError(TEXT("Stop-line auto test detected logic vehicles missing Chaos adapters."));
		}

		if (bChaosProjectionFailed && Test)
		{
			Test->AddError(TEXT("Stop-line auto test failed to project Chaos vehicles onto their follow targets."));
		}

		if (ChaosVehiclesChecked == 0 && Test)
		{
			Test->AddError(TEXT("Stop-line auto test could not validate any Chaos vehicles."));
		}

		if (MaxChaosPathErrorCm_Local > StopLineMaxChaosPathErrorCm && Test)
		{
			Test->AddError(FString::Printf(TEXT("Stop-line auto test Chaos path error too large (max=%.1fcm)."), MaxChaosPathErrorCm_Local));
		}

		return true;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FTrafficStopLineEndPIECommand, TSharedRef<FTrafficStopLineAutoState>, State, FAutomationTestBase*, Test);
	bool FTrafficStopLineEndPIECommand::Update()
	{
		if (GEditor)
		{
			GEditor->RequestEndPlayMap();
		}

		if (State->bSavedCVars)
		{
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.StopLineOffsetAuto")))
			{
				Var->Set(State->PrevStopLineAuto, ECVF_SetByCode);
			}
		}

		if (State->bSavedVisualMode)
		{
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Visual.Mode")))
			{
				Var->Set(State->PrevVisualMode, ECVF_SetByCode);
			}
			State->bSavedVisualMode = false;
		}

		return true;
	}
}

#endif // WITH_EDITOR

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficStopLineAutoBoundaryTest,
	"Traffic.Intersections.StopLineAuto.BaselineCurve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficStopLineAutoBoundaryTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	if (!AutomationOpenMap(StopLineMapPackage))
	{
		AddError(TEXT("Failed to load /AAAtrafficSystem/Maps/Traffic_BaselineCurve for stop-line auto test."));
		return false;
	}

	TSharedRef<FTrafficStopLineAutoState> State = MakeShared<FTrafficStopLineAutoState>();
	State->bSavedCVars = true;
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Intersections.StopLineOffsetAuto")))
	{
		State->PrevStopLineAuto = Var->GetInt();
		Var->Set(1, ECVF_SetByCode);
	}

	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("aaa.Traffic.Visual.Mode")))
	{
		State->PrevVisualMode = Var->GetInt();
		Var->Set(2, ECVF_SetByCode);
		State->bSavedVisualMode = true;
	}

	AddExpectedError(TEXT("The Editor is currently in a play mode."), EAutomationExpectedErrorFlags::Contains, 4);

	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficStopLineWaitForPIEWorldCommand(State, this, 15.0, FPlatformTime::Seconds()));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficStopLineAutoCheckCommand(State, this));
	ADD_LATENT_AUTOMATION_COMMAND(FTrafficStopLineEndPIECommand(State, this));

	return true;
#else
	return false;
#endif
}
