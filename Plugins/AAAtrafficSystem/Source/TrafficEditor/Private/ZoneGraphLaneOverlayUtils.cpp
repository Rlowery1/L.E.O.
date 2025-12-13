#include "ZoneGraphLaneOverlayUtils.h"

#if WITH_EDITOR
#include "GameFramework/Actor.h"

#include "ZoneGraphData.h"
#include "ZoneGraphSubsystem.h"
#endif

namespace ZoneGraphLaneOverlayUtils
{
#if WITH_EDITOR
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

