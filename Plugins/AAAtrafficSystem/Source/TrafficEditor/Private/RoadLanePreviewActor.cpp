#include "RoadLanePreviewActor.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"

ARoadLanePreviewActor::ARoadLanePreviewActor()
{
	PrimaryActorTick.bCanEverTick = true;
#if WITH_EDITOR
	PrimaryActorTick.bStartWithTickEnabled = true;
#endif

	CenterlinePreview = CreateDefaultSubobject<USplineComponent>(TEXT("CenterlinePreview"));
	RootComponent = CenterlinePreview;
}

#if WITH_EDITOR
void ARoadLanePreviewActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}
#endif

USplineComponent* ARoadLanePreviewActor::CreateOrGetCenterlineComponent()
{
	if (!CenterlinePreview)
	{
		CenterlinePreview = NewObject<USplineComponent>(this, TEXT("CenterlinePreview"));
		CenterlinePreview->SetupAttachment(RootComponent);
		CenterlinePreview->RegisterComponent();
	}
	return CenterlinePreview;
}

USplineComponent* ARoadLanePreviewActor::CreateLaneSplineComponent()
{
	USplineComponent* NewSpline = NewObject<USplineComponent>(this);
	NewSpline->SetupAttachment(RootComponent);
	NewSpline->RegisterComponent();
	LaneSplines.Add(NewSpline);
	return NewSpline;
}

void ARoadLanePreviewActor::ClearLaneSplines()
{
	for (USplineComponent* Spline : LaneSplines)
	{
		if (Spline)
		{
			Spline->DestroyComponent();
		}
	}
	LaneSplines.Empty();
}

void ARoadLanePreviewActor::BuildSideLanes(
	USplineComponent* SourceSpline,
	const FTrafficLaneLayoutSide& Side,
	bool bForwardDirection,
	TArray<USplineComponent*>& OutCreatedSplines)
{
	if (!SourceSpline || Side.NumLanes <= 0)
	{
		return;
	}

	const int32 NumSourcePoints = SourceSpline->GetNumberOfSplinePoints();
	if (NumSourcePoints < 2)
	{
		return;
	}

	const float InnerOffset = Side.InnerLaneCenterOffsetCm;
	const float LaneWidth = Side.LaneWidthCm;
	const float DirectionSign = bForwardDirection ? 1.f : -1.f;

	for (int32 LaneIndex = 0; LaneIndex < Side.NumLanes; ++LaneIndex)
	{
		const float CenterOffset = InnerOffset + LaneIndex * LaneWidth;
		const float LateralOffset = DirectionSign * CenterOffset;

		USplineComponent* LaneSpline = CreateLaneSplineComponent();
		LaneSpline->ClearSplinePoints(false);

		for (int32 i = 0; i < NumSourcePoints; ++i)
		{
			const FVector CenterPos = SourceSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
			const FVector Tangent = SourceSpline->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::World);
			const FVector UpVector = FVector::UpVector;
			const FVector RightVector = FVector::CrossProduct(UpVector, Tangent).GetSafeNormal();
			const FVector OffsetPos = CenterPos + RightVector * LateralOffset;

			LaneSpline->AddSplinePoint(OffsetPos, ESplineCoordinateSpace::World, false);
		}

		LaneSpline->SetClosedLoop(SourceSpline->IsClosedLoop(), false);
		LaneSpline->UpdateSpline();
		OutCreatedSplines.Add(LaneSpline);
	}
}

void ARoadLanePreviewActor::BuildPreviewFromSpline(USplineComponent* SourceSpline, const FRoadFamilyDefinition& Family)
{
	if (!SourceSpline)
	{
		return;
	}

	CreateOrGetCenterlineComponent();

	CenterlinePreview->ClearSplinePoints(false);
	const int32 NumPoints = SourceSpline->GetNumberOfSplinePoints();
	for (int32 i = 0; i < NumPoints; ++i)
	{
		const FVector Pos = SourceSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
		CenterlinePreview->AddSplinePoint(Pos, ESplineCoordinateSpace::World, false);
	}
	CenterlinePreview->SetClosedLoop(SourceSpline->IsClosedLoop(), false);
	CenterlinePreview->UpdateSpline();

	ClearLaneSplines();

	TArray<USplineComponent*> Created;
	BuildSideLanes(SourceSpline, Family.Forward, true, Created);
	BuildSideLanes(SourceSpline, Family.Backward, false, Created);
}

