#include "ZoneGraphLaneOverlayUtils.h"

#if WITH_EDITOR
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

#include "ZoneGraphData.h"
#include "ZoneGraphSubsystem.h"
#endif

namespace ZoneGraphLaneOverlayUtils
{
#if WITH_EDITOR
	namespace
	{
		static const FName TrafficCityBLDCalibrationZoneShapeTag(TEXT("AAA_CityBLD_CalibrationZoneGraph"));

		const AZoneGraphData* FindCalibrationZoneGraphDataActorForRoad(const UWorld* World, const AActor* RoadActor)
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

				if (!DataActor->HasAnyFlags(RF_Transient))
				{
					continue;
				}

				if (!DataActor->ActorHasTag(TrafficCityBLDCalibrationZoneShapeTag))
				{
					continue;
				}

				return DataActor;
			}

			return nullptr;
		}

		void AppendLanePolylinesFromStorage(
			const FZoneGraphStorage& Storage,
			const FBox& QueryBounds,
			const FZoneGraphTagFilter& TagFilter,
			TArray<TArray<FVector>>& OutLanePolylines)
		{
			for (int32 LaneIndex = 0; LaneIndex < Storage.Lanes.Num(); ++LaneIndex)
			{
				const FZoneLaneData& Lane = Storage.Lanes[LaneIndex];
				if (!TagFilter.Pass(Lane.Tags))
				{
					continue;
				}

				const int32 NumLanePoints = Lane.PointsEnd - Lane.PointsBegin;
				if (NumLanePoints < 2 ||
					Lane.PointsBegin < 0 ||
					Lane.PointsEnd > Storage.LanePoints.Num())
				{
					continue;
				}

				FBox LaneBounds(EForceInit::ForceInit);
				for (int32 PointIdx = Lane.PointsBegin; PointIdx < Lane.PointsEnd; ++PointIdx)
				{
					LaneBounds += Storage.LanePoints[PointIdx];
				}

				if (!LaneBounds.Intersect(QueryBounds))
				{
					continue;
				}

				TArray<FVector> Polyline;
				Polyline.Reserve(NumLanePoints);
				for (int32 PointIdx = Lane.PointsBegin; PointIdx < Lane.PointsEnd; ++PointIdx)
				{
					Polyline.Add(Storage.LanePoints[PointIdx]);
				}

				OutLanePolylines.Add(MoveTemp(Polyline));
			}
		}
	}

	bool GetZoneGraphLanePolylinesNearActor(
		UWorld* World,
		AActor* RoadActor,
		TArray<TArray<FVector>>& OutLanePolylines)
	{
		OutLanePolylines.Reset();

		if (!World || !RoadActor)
		{
			return false;
		}

		UZoneGraphSubsystem* ZGS = World->GetSubsystem<UZoneGraphSubsystem>();
		if (!ZGS)
		{
			return false;
		}

		FVector Origin = FVector::ZeroVector;
		FVector Extent = FVector::ZeroVector;
		RoadActor->GetActorBounds(/*bOnlyCollidingComponents=*/false, Origin, Extent);
		FBox QueryBounds(Origin - Extent, Origin + Extent);
		QueryBounds = QueryBounds.ExpandBy(2000.f);

		FZoneGraphTagFilter TagFilter;
		const FZoneGraphTag VehiclesTag = ZGS->GetTagByName(FName(TEXT("Vehicles")));
		if (VehiclesTag.IsValid())
		{
			TagFilter.AnyTags = FZoneGraphTagMask(VehiclesTag);
		}

		// Prefer the transient calibration ZoneGraphData (if present) to avoid picking up unrelated lanes
		// from other ZoneGraphData actors in the world.
		if (const AZoneGraphData* CalibrationDataActor = FindCalibrationZoneGraphDataActorForRoad(World, RoadActor))
		{
			const FZoneGraphStorage& Storage = CalibrationDataActor->GetStorage();
			if (Storage.Lanes.Num() > 0 && Storage.LanePoints.Num() > 0)
			{
				AppendLanePolylinesFromStorage(Storage, QueryBounds, TagFilter, OutLanePolylines);

				// If tags were missing (common in early setups), fall back to all lanes from the calibration graph.
				if (OutLanePolylines.Num() == 0 && VehiclesTag.IsValid())
				{
					FZoneGraphTagFilter UntaggedFilter = TagFilter;
					UntaggedFilter.AnyTags = FZoneGraphTagMask::None;
					AppendLanePolylinesFromStorage(Storage, QueryBounds, UntaggedFilter, OutLanePolylines);
				}

				return OutLanePolylines.Num() > 0;
			}
		}

		TArray<FZoneGraphLaneHandle> Lanes;
		if (!ZGS->FindOverlappingLanes(QueryBounds, TagFilter, Lanes))
		{
			return false;
		}

		TSet<FZoneGraphLaneHandle> Seen;
		for (const FZoneGraphLaneHandle& Handle : Lanes)
		{
			if (!Handle.IsValid() || Seen.Contains(Handle))
			{
				continue;
			}
			Seen.Add(Handle);

			const FZoneGraphStorage* Storage = ZGS->GetZoneGraphStorage(Handle.DataHandle);
			if (!Storage || !Storage->Lanes.IsValidIndex(Handle.Index))
			{
				continue;
			}

			const FZoneLaneData& Lane = Storage->Lanes[Handle.Index];
			const int32 NumLanePoints = Lane.PointsEnd - Lane.PointsBegin;
			if (NumLanePoints < 2 ||
				Lane.PointsBegin < 0 ||
				Lane.PointsEnd > Storage->LanePoints.Num())
			{
				continue;
			}

			TArray<FVector> Polyline;
			Polyline.Reserve(NumLanePoints);
			for (int32 PointIdx = Lane.PointsBegin; PointIdx < Lane.PointsEnd; ++PointIdx)
			{
				Polyline.Add(Storage->LanePoints[PointIdx]);
			}

			OutLanePolylines.Add(MoveTemp(Polyline));
		}

		return OutLanePolylines.Num() > 0;
	}
#endif
}
