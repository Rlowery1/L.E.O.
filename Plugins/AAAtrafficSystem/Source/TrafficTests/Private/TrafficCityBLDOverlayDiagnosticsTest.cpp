#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"

#include "TrafficAutomationLogger.h"
#include "TrafficCityBLDAdapterSettings.h"
#include "TrafficRuntimeModule.h"
#include "TrafficSystemEditorSubsystem.h"
#include "TrafficZoneLaneProfile.h"
#include "RoadFamilyRegistry.h"

#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SplineComponent.h"

#include "ZoneGraphData.h"
#include "ZoneGraphSettings.h"
#include "ZoneGraphSubsystem.h"

#include <cfloat>

namespace
{
	static const FName TrafficCityBLDCalibrationZoneGraphTag(TEXT("AAA_CityBLD_CalibrationZoneGraph"));
	static const float ZoneGraphQueryBoundsExpandCm = 2000.f;
	static const float PolylineKeyQuantizeCm = 1.f;
	static const int32 MaxLaneProfileNamesToLog = 25;
	static const int32 MaxZoneGraphDataActorsToLog = 25;
	static const int32 MaxLanePolylinesToLog = 12;
	static const int32 MaxLaneWidthsToLog = 12;
	static const int32 MaxDuplicateKeysToLog = 12;

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
#
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

	static const AZoneGraphData* FindCalibrationZoneGraphDataActorForRoad(const UWorld* World, const AActor* RoadActor)
	{
		if (!World || !RoadActor)
		{
			return nullptr;
		}

		ULevel* TargetLevel = RoadActor->GetLevel();
		if (!TargetLevel)
		{
			return nullptr;
		}

		for (TActorIterator<AZoneGraphData> It(World); It; ++It)
		{
			const AZoneGraphData* DataActor = *It;
			if (!DataActor)
			{
				continue;
			}

			if (DataActor->GetLevel() != TargetLevel)
			{
				continue;
			}
#
			if (!DataActor->HasAnyFlags(RF_Transient))
			{
				continue;
			}

			if (!DataActor->ActorHasTag(TrafficCityBLDCalibrationZoneGraphTag))
			{
				continue;
			}
#
			return DataActor;
		}

		return nullptr;
	}

	static FString QuantizeVectorKey(const FVector& V, const float GridCm)
	{
		const float Grid = FMath::Max(GridCm, 0.001f);
		const FVector Snapped(
			FMath::GridSnap(V.X, Grid),
			FMath::GridSnap(V.Y, Grid),
			FMath::GridSnap(V.Z, Grid));
		return FString::Printf(TEXT("%.1f,%.1f,%.1f"), Snapped.X, Snapped.Y, Snapped.Z);
	}

	static FString MakePolylineKey(const TArray<FVector>& Polyline)
	{
		if (Polyline.Num() < 2)
		{
			return TEXT("Degenerate");
		}
#
		const FString A = QuantizeVectorKey(Polyline[0], PolylineKeyQuantizeCm);
		const FString B = QuantizeVectorKey(Polyline.Last(), PolylineKeyQuantizeCm);
		const FString KeyAB = FString::Printf(TEXT("%s->%s|N=%d"), *A, *B, Polyline.Num());
		const FString KeyBA = FString::Printf(TEXT("%s->%s|N=%d"), *B, *A, Polyline.Num());
		return (KeyAB < KeyBA) ? KeyAB : KeyBA;
	}

	struct FZoneGraphLaneExtractStats
	{
		int32 StorageLanesTotal = 0;
		int32 StorageLanePointsTotal = 0;
		int32 LanesTagFilteredOut = 0;
		int32 LanesDegenerate = 0;
		int32 LanesOutsideBounds = 0;
#
		int32 PolylinesExtracted = 0;
		int32 DuplicatePolylineCount = 0;
#
		float WidthMinCm = FLT_MAX;
		float WidthMaxCm = 0.f;
		float WidthSumCm = 0.f;
		int32 WidthSamples = 0;
	};

	static void AppendLanePolylinesFromStorage(
		const FZoneGraphStorage& Storage,
		const FBox& QueryBounds,
		const FZoneGraphTagFilter& TagFilter,
		TArray<TArray<FVector>>& OutPolylines,
		TArray<float>& OutLaneWidths,
		FZoneGraphLaneExtractStats& InOutStats)
	{
		InOutStats.StorageLanesTotal = Storage.Lanes.Num();
		InOutStats.StorageLanePointsTotal = Storage.LanePoints.Num();

		for (int32 LaneIndex = 0; LaneIndex < Storage.Lanes.Num(); ++LaneIndex)
		{
			const FZoneLaneData& Lane = Storage.Lanes[LaneIndex];
			if (!TagFilter.Pass(Lane.Tags))
			{
				++InOutStats.LanesTagFilteredOut;
				continue;
			}

			const int32 NumLanePoints = Lane.PointsEnd - Lane.PointsBegin;
			if (NumLanePoints < 2 ||
				Lane.PointsBegin < 0 ||
				Lane.PointsEnd > Storage.LanePoints.Num())
			{
				++InOutStats.LanesDegenerate;
				continue;
			}

			FBox LaneBounds(EForceInit::ForceInit);
			for (int32 PointIdx = Lane.PointsBegin; PointIdx < Lane.PointsEnd; ++PointIdx)
			{
				LaneBounds += Storage.LanePoints[PointIdx];
			}

			if (!LaneBounds.Intersect(QueryBounds))
			{
				++InOutStats.LanesOutsideBounds;
				continue;
			}

			TArray<FVector> Polyline;
			Polyline.Reserve(NumLanePoints);
			for (int32 PointIdx = Lane.PointsBegin; PointIdx < Lane.PointsEnd; ++PointIdx)
			{
				Polyline.Add(Storage.LanePoints[PointIdx]);
			}

			OutPolylines.Add(MoveTemp(Polyline));

			const float WidthCm = Lane.Width;
			if (WidthCm > KINDA_SMALL_NUMBER)
			{
				OutLaneWidths.Add(WidthCm);
				InOutStats.WidthMinCm = FMath::Min(InOutStats.WidthMinCm, WidthCm);
				InOutStats.WidthMaxCm = FMath::Max(InOutStats.WidthMaxCm, WidthCm);
				InOutStats.WidthSumCm += WidthCm;
				++InOutStats.WidthSamples;
			}
		}

		InOutStats.PolylinesExtracted = OutPolylines.Num();

		TMap<FString, int32> KeyCounts;
		for (const TArray<FVector>& Polyline : OutPolylines)
		{
			const FString Key = MakePolylineKey(Polyline);
			KeyCounts.FindOrAdd(Key) += 1;
		}

		for (const TPair<FString, int32>& Pair : KeyCounts)
		{
			if (Pair.Value > 1)
			{
				InOutStats.DuplicatePolylineCount += (Pair.Value - 1);
			}
		}
	}

	static void LogLaneProfileSummary(const UZoneGraphSettings* ZoneSettings)
	{
		if (!ZoneSettings)
		{
			return;
		}

		int32 AutoProfileCount = 0;
		int32 CityBldSampleCount = 0;

		int32 Logged = 0;
		for (const FZoneLaneProfile& Profile : ZoneSettings->GetLaneProfiles())
		{
			const FString NameStr = Profile.Name.ToString();
			if (NameStr.StartsWith(TEXT("AutoProfile_")))
			{
				++AutoProfileCount;
			}
			if (NameStr.Contains(TEXT("CityBLD-Sample-2Lane")))
			{
				++CityBldSampleCount;
			}

			if (Logged < MaxLaneProfileNamesToLog)
			{
				UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("ZoneGraph.LaneProfile[%d]=%s (Lanes=%d)"), Logged, *NameStr, Profile.Lanes.Num()));
				++Logged;
			}
		}

		UTrafficAutomationLogger::LogMetricInt(TEXT("ZoneGraph.LaneProfiles.Total"), ZoneSettings->GetLaneProfiles().Num());
		UTrafficAutomationLogger::LogMetricInt(TEXT("ZoneGraph.LaneProfiles.AutoProfileCount"), AutoProfileCount);
		UTrafficAutomationLogger::LogMetricInt(TEXT("ZoneGraph.LaneProfiles.CityBLDSampleCount"), CityBldSampleCount);
	}

	static void LogZoneGraphDataActorSummary(const UWorld* World, const AActor* RoadActor)
	{
		if (!World)
		{
			return;
		}

		int32 Count = 0;
		int32 Logged = 0;
		for (TActorIterator<AZoneGraphData> It(World); It; ++It)
		{
			const AZoneGraphData* DataActor = *It;
			if (!DataActor)
			{
				continue;
			}

			++Count;

			const bool bTransient = DataActor->HasAnyFlags(RF_Transient);
			const bool bHasTag = DataActor->ActorHasTag(TrafficCityBLDCalibrationZoneGraphTag);
			const bool bSameLevel = RoadActor && (DataActor->GetLevel() == RoadActor->GetLevel());

			const FZoneGraphStorage& Storage = DataActor->GetStorage();

			if (Logged < MaxZoneGraphDataActorsToLog)
			{
				UTrafficAutomationLogger::LogLine(FString::Printf(
					TEXT("ZoneGraph.DataActor[%d]=%s Transient=%d HasCalibTag=%d SameLevelAsRoad=%d Lanes=%d LanePoints=%d"),
					Logged,
					*GetNameSafe(DataActor),
					bTransient ? 1 : 0,
					bHasTag ? 1 : 0,
					bSameLevel ? 1 : 0,
					Storage.Lanes.Num(),
					Storage.LanePoints.Num()));
				++Logged;
			}
		}

		UTrafficAutomationLogger::LogMetricInt(TEXT("ZoneGraph.DataActors.Count"), Count);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrafficCityBLDOverlayDiagnosticsTest,
	"Traffic.Calibration.CityBLD.OverlayDiagnostics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficCityBLDOverlayDiagnosticsTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	const FString LocalTestName = TEXT("Traffic.Calibration.CityBLD.OverlayDiagnostics");
	UTrafficAutomationLogger::BeginTestLog(LocalTestName);
	UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("[AAA Traffic] Version=%s"), TEXT(AAA_TRAFFIC_PLUGIN_VERSION)));

	if (!GEditor)
	{
		AddError(TEXT("GEditor is null."));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	FAutomationEditorCommonUtils::CreateNewMap();

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		AddError(TEXT("Editor world is null after creating a new map."));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	UTrafficSystemEditorSubsystem* Subsys = GEditor->GetEditorSubsystem<UTrafficSystemEditorSubsystem>();
	if (!Subsys)
	{
		AddError(TEXT("TrafficSystemEditorSubsystem unavailable."));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	// Clean any prior AAA overlay/controller actors (do not delete user roads).
	Subsys->Editor_ResetRoadLabHard(false);

	FString MeshRoadClassPath;
	UClass* MeshRoadClass = LoadCityBLDMeshRoadClass(MeshRoadClassPath);
	if (!MeshRoadClass)
	{
		AddError(TEXT("Failed to load CityBLD BP_MeshRoad class (tried /CityBLD/Blueprints/... and /CityBLD/CityBLD/Blueprints/...)."));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}
	UTrafficAutomationLogger::LogMetric(TEXT("CityBLD.MeshRoadClassPath"), MeshRoadClassPath);

	auto SpawnCityBLDRoad = [&](const FTransform& Xform) -> AActor*
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* RoadActor = World->SpawnActor<AActor>(MeshRoadClass, Xform, SpawnParams);
		if (RoadActor)
		{
			EnableCollisionForActor(RoadActor);
		}
		return RoadActor;
	};

	AActor* RoadActor = SpawnCityBLDRoad(FTransform(FRotator::ZeroRotator, FVector::ZeroVector));
	if (!RoadActor)
	{
		AddError(TEXT("Failed to spawn CityBLD BP_MeshRoad actor."));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	// Ensure the road blueprint has a non-degenerate control spline so generated geometry/bounds and ZoneGraph lanes are meaningful.
	{
		const FVector Base = FVector::ZeroVector;
		if (!ConfigureControlSplineWorld(RoadActor, { Base, Base + FVector(3000.f, 0.f, 0.f), Base + FVector(3000.f, 3000.f, 0.f) }))
		{
			AddWarning(TEXT("Could not locate/configure a control spline on BP_MeshRoad; road may remain degenerate in automation."));
		}
		EnableCollisionForActor(RoadActor);
	}

	// Make calibration selection deterministic: BeginCalibration uses a selected actor if present.
	GEditor->SelectNone(/*NoteSelectionChange=*/false, /*DeselectBSPSurfs=*/true, /*WarnAboutManyActors=*/false);
	GEditor->SelectActor(RoadActor, /*InSelected=*/true, /*Notify=*/true);

	Subsys->DoPrepare();
	Subsys->Editor_PrepareMapForTraffic();

	URoadFamilyRegistry* Registry = URoadFamilyRegistry::Get();
	if (!Registry)
	{
		AddError(TEXT("RoadFamilyRegistry unavailable."));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	FGuid FamilyId;
	for (const FRoadFamilyInfo& Info : Registry->GetAllFamilies())
	{
		const FString ClassName = Info.RoadClassPath.GetAssetName();
		if (ClassName.Contains(TEXT("BP_MeshRoad")) || ClassName.Contains(TEXT("MeshRoad")))
		{
			if (Subsys->GetNumActorsForFamily(Info.FamilyId) > 0)
			{
				FamilyId = Info.FamilyId;
				break;
			}
		}
	}
	if (!FamilyId.IsValid())
	{
		if (const FRoadFamilyInfo* Info = Registry->FindFamilyByClass(MeshRoadClass))
		{
			if (Subsys->GetNumActorsForFamily(Info->FamilyId) > 0)
			{
				FamilyId = Info->FamilyId;
			}
		}
	}

	if (!FamilyId.IsValid())
	{
		AddError(TEXT("Could not find a CityBLD road family id for calibration."));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	const UTrafficCityBLDAdapterSettings* AdapterSettings = GetDefault<UTrafficCityBLDAdapterSettings>();
	const bool bUseZoneGraphLanePolylines = AdapterSettings && AdapterSettings->bUseZoneGraphLanePolylinesForCalibrationOverlay;
	UTrafficAutomationLogger::LogMetricInt(TEXT("Adapter.UseZoneGraphLanePolylines"), bUseZoneGraphLanePolylines ? 1 : 0);

	const FSoftObjectPath AdapterLaneProfilePath = AdapterSettings ? AdapterSettings->DefaultCityBLDVehicleLaneProfile : FSoftObjectPath();
	UTrafficAutomationLogger::LogMetric(TEXT("Adapter.DefaultVehicleLaneProfile"), AdapterLaneProfilePath.ToString());

	const FRoadFamilyInfo* FamilyBefore = Registry->FindFamilyById(FamilyId);
	if (FamilyBefore)
	{
		UTrafficAutomationLogger::LogMetric(TEXT("Family.VehicleLaneProfile.Before"), FamilyBefore->FamilyDefinition.VehicleLaneProfile.ToString());
	}

	const FSoftObjectPath LaneProfilePath = (FamilyBefore && FamilyBefore->FamilyDefinition.VehicleLaneProfile.IsValid())
		? FamilyBefore->FamilyDefinition.VehicleLaneProfile
		: AdapterLaneProfilePath;

	UTrafficAutomationLogger::LogMetric(TEXT("Calibration.LaneProfilePath.Selected"), LaneProfilePath.ToString());

	UObject* LoadedObj = LaneProfilePath.IsValid() ? LaneProfilePath.TryLoad() : nullptr;
	UTrafficAutomationLogger::LogMetricInt(TEXT("LaneProfileAsset.Loaded"), LoadedObj ? 1 : 0);
	if (const UTrafficZoneLaneProfile* LaneProfileAsset = Cast<UTrafficZoneLaneProfile>(LoadedObj))
	{
		UTrafficAutomationLogger::LogMetric(TEXT("LaneProfileAsset.ProfileName"), LaneProfileAsset->ProfileName.ToString());
		UTrafficAutomationLogger::LogMetricInt(TEXT("LaneProfileAsset.NumLanes"), LaneProfileAsset->NumLanes);
		UTrafficAutomationLogger::LogMetricFloat(TEXT("LaneProfileAsset.LaneWidthCm"), LaneProfileAsset->LaneWidthCm, 1);
		UTrafficAutomationLogger::LogMetric(TEXT("LaneProfileAsset.Direction"), StaticEnum<EZoneLaneDirection>()->GetNameStringByValue(static_cast<int64>(LaneProfileAsset->Direction)));
		UTrafficAutomationLogger::LogMetric(TEXT("LaneProfileAsset.LaneTagName"), LaneProfileAsset->LaneTagName.ToString());
	}

	// Run the actual calibration workflow to match the editor overlay path.
	Subsys->Editor_BeginCalibrationForFamily(FamilyId);

	const FRoadFamilyInfo* FamilyAfter = Registry->FindFamilyById(FamilyId);
	if (FamilyAfter)
	{
		UTrafficAutomationLogger::LogMetric(TEXT("Family.VehicleLaneProfile.After"), FamilyAfter->FamilyDefinition.VehicleLaneProfile.ToString());
	}

	// ZoneGraph-side diagnostics (where overlay lane polylines come from).
	UZoneGraphSubsystem* ZGS = World->GetSubsystem<UZoneGraphSubsystem>();
	if (!ZGS)
	{
		AddError(TEXT("ZoneGraphSubsystem missing. Ensure ZoneGraph plugin is enabled."));
		UTrafficAutomationLogger::LogLine(TEXT("Error=ZoneGraphSubsystemMissing"));
		UTrafficAutomationLogger::EndTestLog();
		return false;
	}

	const FZoneGraphTag VehiclesTag = ZGS->GetTagByName(FName(TEXT("Vehicles")));
	UTrafficAutomationLogger::LogMetricInt(TEXT("ZoneGraph.VehiclesTag.Valid"), VehiclesTag.IsValid() ? 1 : 0);

	const UZoneGraphSettings* ZoneSettings = GetDefault<UZoneGraphSettings>();
	LogLaneProfileSummary(ZoneSettings);
	LogZoneGraphDataActorSummary(World, RoadActor);

	// Prefer the transient calibration ZoneGraphData, matching ZoneGraphLaneOverlayUtils.
	const AZoneGraphData* CalibrationDataActor = FindCalibrationZoneGraphDataActorForRoad(World, RoadActor);
	UTrafficAutomationLogger::LogMetricInt(TEXT("ZoneGraph.CalibrationDataActor.Found"), CalibrationDataActor ? 1 : 0);
	if (!CalibrationDataActor)
	{
		AddError(TEXT("No transient, calibration-tagged ZoneGraphData actor found; overlay may be pulling from unrelated world ZoneGraph lanes."));
	}

	FVector Origin = FVector::ZeroVector;
	FVector Extent = FVector::ZeroVector;
	RoadActor->GetActorBounds(/*bOnlyCollidingComponents=*/false, Origin, Extent);
	UTrafficAutomationLogger::LogMetricFloat(TEXT("Road.BoundsExtentX"), Extent.X, 1);
	UTrafficAutomationLogger::LogMetricFloat(TEXT("Road.BoundsExtentY"), Extent.Y, 1);
	UTrafficAutomationLogger::LogMetricFloat(TEXT("Road.BoundsExtentZ"), Extent.Z, 1);

	FBox QueryBounds(Origin - Extent, Origin + Extent);
	QueryBounds = QueryBounds.ExpandBy(ZoneGraphQueryBoundsExpandCm);

	FZoneGraphTagFilter TagFilter;
	if (VehiclesTag.IsValid())
	{
		TagFilter.AnyTags = FZoneGraphTagMask(VehiclesTag);
	}

	TArray<TArray<FVector>> ExtractedPolylines;
	TArray<float> ExtractedWidths;
	FZoneGraphLaneExtractStats Stats;

	if (CalibrationDataActor)
	{
		const FZoneGraphStorage& Storage = CalibrationDataActor->GetStorage();
		UTrafficAutomationLogger::LogMetricInt(TEXT("ZoneGraph.CalibrationStorage.Lanes"), Storage.Lanes.Num());
		UTrafficAutomationLogger::LogMetricInt(TEXT("ZoneGraph.CalibrationStorage.LanePoints"), Storage.LanePoints.Num());

		AppendLanePolylinesFromStorage(Storage, QueryBounds, TagFilter, ExtractedPolylines, ExtractedWidths, Stats);

		// Match ZoneGraphLaneOverlayUtils fallback when tags are missing.
		if (ExtractedPolylines.Num() == 0 && VehiclesTag.IsValid())
		{
			FZoneGraphTagFilter Untagged = TagFilter;
			Untagged.AnyTags = FZoneGraphTagMask::None;
			AppendLanePolylinesFromStorage(Storage, QueryBounds, Untagged, ExtractedPolylines, ExtractedWidths, Stats);
			UTrafficAutomationLogger::LogMetricInt(TEXT("ZoneGraph.CalibrationStorage.UsedUntaggedFallback"), 1);
		}
	}

	UTrafficAutomationLogger::LogMetricInt(TEXT("ZoneGraph.LanePolylines.Count"), ExtractedPolylines.Num());
	UTrafficAutomationLogger::LogMetricInt(TEXT("ZoneGraph.LanePolylines.DuplicateCount"), Stats.DuplicatePolylineCount);

	UTrafficAutomationLogger::LogMetricInt(TEXT("ZoneGraph.Extract.StorageLanesTotal"), Stats.StorageLanesTotal);
	UTrafficAutomationLogger::LogMetricInt(TEXT("ZoneGraph.Extract.StorageLanePointsTotal"), Stats.StorageLanePointsTotal);
	UTrafficAutomationLogger::LogMetricInt(TEXT("ZoneGraph.Extract.LanesTagFilteredOut"), Stats.LanesTagFilteredOut);
	UTrafficAutomationLogger::LogMetricInt(TEXT("ZoneGraph.Extract.LanesDegenerate"), Stats.LanesDegenerate);
	UTrafficAutomationLogger::LogMetricInt(TEXT("ZoneGraph.Extract.LanesOutsideBounds"), Stats.LanesOutsideBounds);

	const float AvgWidth = (Stats.WidthSamples > 0) ? (Stats.WidthSumCm / Stats.WidthSamples) : 0.f;
	UTrafficAutomationLogger::LogMetricFloat(TEXT("ZoneGraph.Extract.WidthMinCm"), (Stats.WidthSamples > 0) ? Stats.WidthMinCm : 0.f, 1);
	UTrafficAutomationLogger::LogMetricFloat(TEXT("ZoneGraph.Extract.WidthMaxCm"), (Stats.WidthSamples > 0) ? Stats.WidthMaxCm : 0.f, 1);
	UTrafficAutomationLogger::LogMetricFloat(TEXT("ZoneGraph.Extract.WidthAvgCm"), AvgWidth, 1);
	UTrafficAutomationLogger::LogMetricInt(TEXT("ZoneGraph.Extract.WidthSamples"), Stats.WidthSamples);

	for (int32 i = 0; i < ExtractedWidths.Num() && i < MaxLaneWidthsToLog; ++i)
	{
		UTrafficAutomationLogger::LogLine(FString::Printf(TEXT("ZoneGraph.LaneWidth[%d]=%.1f"), i, ExtractedWidths[i]));
	}

	for (int32 i = 0; i < ExtractedPolylines.Num() && i < MaxLanePolylinesToLog; ++i)
	{
		const TArray<FVector>& Poly = ExtractedPolylines[i];
		if (Poly.Num() >= 2)
		{
			UTrafficAutomationLogger::LogLine(FString::Printf(
				TEXT("ZoneGraph.Polyline[%d]=Pts=%d Start=%.0f,%.0f,%.0f End=%.0f,%.0f,%.0f Key=%s"),
				i,
				Poly.Num(),
				Poly[0].X, Poly[0].Y, Poly[0].Z,
				Poly.Last().X, Poly.Last().Y, Poly.Last().Z,
				*MakePolylineKey(Poly)));
		}
	}

	// Strong assertions: if we expect ZoneGraph overlay polylines, missing lane profile assets should be treated as a failure.
	if (bUseZoneGraphLanePolylines && LaneProfilePath.IsValid() && !LoadedObj)
	{
		AddError(FString::Printf(TEXT("CityBLD lane profile asset failed to load: '%s' (ZoneGraph overlay polylines are enabled, so calibration overlay will use fallback lane profiles)."), *LaneProfilePath.ToString()));
	}

	if (bUseZoneGraphLanePolylines && CalibrationDataActor && ExtractedPolylines.Num() == 0)
	{
		AddError(TEXT("Calibration ZoneGraphData exists but yielded zero lane polylines near the road; overlay likely falls back to centerline or unrelated lanes."));
	}

	UTrafficAutomationLogger::EndTestLog();
	return !HasAnyErrors();
#else
	return true;
#endif
}
