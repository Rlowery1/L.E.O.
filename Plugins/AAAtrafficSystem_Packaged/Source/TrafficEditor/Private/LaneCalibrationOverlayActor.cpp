#include "LaneCalibrationOverlayActor.h"
#include "TrafficCalibrationVisuals.h"
#include "TrafficRuntimeModule.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "TrafficLaneGeometry.h"
#include "TrafficVisualSettings.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/MessageDialog.h"
#include "UObject/UnrealType.h"

ALaneCalibrationOverlayActor::ALaneCalibrationOverlayActor()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	NumLanesPerSideForward = 1;
	NumLanesPerSideBackward = 1;
	LaneWidthCm = 350.f;
	CenterlineOffsetCm = 175.f;
	CachedRoadTransform = FTransform::Identity;

	FallbackArrowMesh = FTrafficCalibrationVisuals::GetOrCreateCalibrationArrowMesh();
	DefaultArrowMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!DefaultArrowMaterial)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[LaneCalibrationOverlay] Failed to load default arrow material."));
	}
}

void ALaneCalibrationOverlayActor::BeginPlay()
{
	Super::BeginPlay();

	for (UInstancedStaticMeshComponent* ISM : ChevronArrowComponents)
	{
		if (ISM && ISM->GetMaterial(0) == nullptr && DefaultArrowMaterial)
		{
			UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(DefaultArrowMaterial, this);
			if (DynMat)
			{
				DynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor::Green);
				ISM->SetMaterial(0, DynMat);
			}
		}
	}
}

void ALaneCalibrationOverlayActor::ClearOverlay()
{
	for (UProceduralMeshComponent* Ribbon : LaneRibbonMeshes)
	{
		if (Ribbon)
		{
			Ribbon->ClearAllMeshSections();
			Ribbon->DestroyComponent();
		}
	}
	LaneRibbonMeshes.Empty();

	for (UInstancedStaticMeshComponent* ISM : ChevronArrowComponents)
	{
		if (ISM)
		{
			ISM->ClearInstances();
			ISM->DestroyComponent();
		}
	}
	ChevronArrowComponents.Empty();
}

TArray<FVector> ALaneCalibrationOverlayActor::ComputeLaneCenterline(
	const TArray<FVector>& RoadCenterline,
	float LateralOffset)
{
	TArray<FVector> LanePoints;
	TrafficLaneGeometry::ComputeLanePoints(RoadCenterline, LateralOffset, LanePoints);
	return LanePoints;
}

void ALaneCalibrationOverlayActor::ApplyCalibrationSettings(int32 InNumForward, int32 InNumBackward, float InLaneWidthCm, float InCenterOffsetCm)
{
	NumLanesPerSideForward = FMath::Max(0, InNumForward);
	NumLanesPerSideBackward = FMath::Max(0, InNumBackward);
	LaneWidthCm = FMath::Max(1.f, InLaneWidthCm);
	CenterlineOffsetCm = InCenterOffsetCm;
}

bool ALaneCalibrationOverlayActor::IsCalibrationValid(const FTrafficLaneFamilyCalibration& Calib) const
{
	const int32 TotalLanes = Calib.NumLanesPerSideForward + Calib.NumLanesPerSideBackward;
	return TotalLanes > 0 && Calib.LaneWidthCm > KINDA_SMALL_NUMBER;
}

UStaticMesh* ALaneCalibrationOverlayActor::GetArrowMesh(bool bForwardDirection, const UTrafficVisualSettings* VisualSettings) const
{
	if (VisualSettings)
	{
		if (bForwardDirection)
		{
			if (VisualSettings->ForwardLaneArrowMesh.IsValid())
			{
				UStaticMesh* Mesh = VisualSettings->ForwardLaneArrowMesh.LoadSynchronous();
				if (Mesh)
				{
					return Mesh;
				}
			}
		}
		else
		{
			if (VisualSettings->BackwardLaneArrowMesh.IsValid())
			{
				UStaticMesh* Mesh = VisualSettings->BackwardLaneArrowMesh.LoadSynchronous();
				if (Mesh)
				{
					return Mesh;
				}
			}
			if (VisualSettings->ForwardLaneArrowMesh.IsValid())
			{
				UStaticMesh* Mesh = VisualSettings->ForwardLaneArrowMesh.LoadSynchronous();
				if (Mesh)
				{
					return Mesh;
				}
			}
		}
	}
	return FallbackArrowMesh ? FallbackArrowMesh : FTrafficCalibrationVisuals::GetOrCreateCalibrationArrowMesh();
}

int32 ALaneCalibrationOverlayActor::GetArrowInstanceCount() const
{
	int32 Count = 0;
	for (const UInstancedStaticMeshComponent* ISM : ChevronArrowComponents)
	{
		if (ISM)
		{
			Count += ISM->GetInstanceCount();
		}
	}
	return Count;
}

UMaterialInstanceDynamic* ALaneCalibrationOverlayActor::CreateArrowMaterialInstance(
	UInstancedStaticMeshComponent* ISM,
	const FLinearColor& Color)
{
	if (!ISM)
	{
		return nullptr;
	}

	UMaterialInterface* BaseMaterial = DefaultArrowMaterial;
	if (!BaseMaterial)
	{
		BaseMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
	}

	if (!BaseMaterial)
	{
		return nullptr;
	}

	UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	if (DynMat)
	{
		static const FName ColorParam(TEXT("Color"));
		static const FName BaseColorParam(TEXT("BaseColor"));
		DynMat->SetVectorParameterValue(ColorParam, Color);
		DynMat->SetVectorParameterValue(BaseColorParam, Color);
		ISM->SetMaterial(0, DynMat);
	}

	return DynMat;
}

void ALaneCalibrationOverlayActor::BuildLaneRibbon(
	const TArray<FVector>& LaneCenterPoints,
	float LaneWidth,
	bool bForwardDirection,
	int32 LaneIndex,
	const UTrafficVisualSettings* VisualSettings)
{
	if (LaneCenterPoints.Num() < 2)
	{
		return;
	}

	UProceduralMeshComponent* RibbonMesh = NewObject<UProceduralMeshComponent>(this);
	RibbonMesh->SetupAttachment(RootComponent);
	RibbonMesh->RegisterComponent();
	RibbonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RibbonMesh->SetCastShadow(false);

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;

	const float HalfWidth = LaneWidth * 0.45f;
	// Lift overlay significantly above the road to avoid z-fighting and make it visually obvious.
	const float ZOffset = 30.f + (LaneIndex * 1.0f);

	FColor LaneColor = bForwardDirection 
		? FColor(40, 200, 80, 180)
		: FColor(220, 60, 40, 180);

	float AccumulatedLength = 0.f;
	for (int32 i = 0; i < LaneCenterPoints.Num(); ++i)
	{
		FVector Tangent;
		if (i < LaneCenterPoints.Num() - 1)
		{
			Tangent = (LaneCenterPoints[i + 1] - LaneCenterPoints[i]).GetSafeNormal();
			if (i > 0)
			{
				AccumulatedLength += FVector::Dist(LaneCenterPoints[i - 1], LaneCenterPoints[i]);
			}
		}
		else
		{
			Tangent = (LaneCenterPoints[i] - LaneCenterPoints[i - 1]).GetSafeNormal();
			AccumulatedLength += FVector::Dist(LaneCenterPoints[i - 1], LaneCenterPoints[i]);
		}

		FVector Right = FVector::CrossProduct(FVector::UpVector, Tangent).GetSafeNormal();

		Vertices.Add(LaneCenterPoints[i] - Right * HalfWidth + FVector(0, 0, ZOffset));
		Vertices.Add(LaneCenterPoints[i] + Right * HalfWidth + FVector(0, 0, ZOffset));

		Normals.Add(FVector::UpVector);
		Normals.Add(FVector::UpVector);

		float U = AccumulatedLength / 1000.f;
		UVs.Add(FVector2D(0.f, U));
		UVs.Add(FVector2D(1.f, U));

		Colors.Add(LaneColor);
		Colors.Add(LaneColor);
	}

	for (int32 i = 0; i < LaneCenterPoints.Num() - 1; ++i)
	{
		int32 BL = i * 2;
		int32 BR = i * 2 + 1;
		int32 TL = (i + 1) * 2;
		int32 TR = (i + 1) * 2 + 1;

		Triangles.Add(BL);
		Triangles.Add(TL);
		Triangles.Add(BR);

		Triangles.Add(BR);
		Triangles.Add(TL);
		Triangles.Add(TR);
	}

	RibbonMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);

	if (VisualSettings && VisualSettings->RoadRibbonMaterial.IsValid())
	{
		UMaterialInterface* RoadMat = VisualSettings->RoadRibbonMaterial.LoadSynchronous();
		if (RoadMat)
		{
			RibbonMesh->SetMaterial(0, RoadMat);
		}
	}

	LaneRibbonMeshes.Add(RibbonMesh);
}

void ALaneCalibrationOverlayActor::BuildChevronArrows(
	UInstancedStaticMeshComponent* ISM,
	const TArray<FVector>& LaneCenterPoints,
	bool bForwardDirection,
	const UTrafficVisualSettings* VisualSettings)
{
	if (!ISM || LaneCenterPoints.Num() < 2)
	{
		return;
	}

	const float DesiredArrowLength = (VisualSettings ? VisualSettings->ArrowLength : 450.f); // cm
	const float DesiredArrowWidth = (VisualSettings ? VisualSettings->ArrowWidth : 180.f);   // cm
	const float ArrowSpacing = (VisualSettings ? VisualSettings->ArrowSpacing : 600.f);      // cm
	// Match ribbon hover height so arrows are clearly visible above the road.
	const float ZOffset = 35.f;
	constexpr float SourceArrowLength = 200.f; // cm, matches procedural arrow mesh
	constexpr float SourceArrowWidth = 160.f;  // cm, matches procedural arrow mesh

	for (int32 i = 1; i < LaneCenterPoints.Num(); ++i)
	{
		const FVector Prev = LaneCenterPoints[i - 1];
		const FVector Curr = LaneCenterPoints[i];
		const float SegmentLength = FVector::Dist(Prev, Curr);
		if (SegmentLength <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const FVector Dir = (Curr - Prev).GetSafeNormal();

		for (float d = 0.f; d < SegmentLength; d += ArrowSpacing)
		{
			const float Alpha = d / SegmentLength;
			const FVector Pos = FMath::Lerp(Prev, Curr, Alpha) + FVector(0.f, 0.f, ZOffset);

			FRotator Rot = Dir.Rotation();
			if (!bForwardDirection)
			{
				Rot.Yaw += 180.f;
			}

			const float ArrowScaleX = DesiredArrowLength / SourceArrowLength;
			const float ArrowScaleY = DesiredArrowWidth / SourceArrowWidth;
			const FVector Scale(ArrowScaleX, ArrowScaleY, 1.f);
			const FTransform XForm(Rot, Pos, Scale);
			ISM->AddInstance(XForm);
		}
	}
}

void ALaneCalibrationOverlayActor::BuildForRoad(
	const TArray<FVector>& CenterlinePoints,
	const FRoadFamilyDefinition& Family,
	const FTransform& RoadTransform,
	bool bHasUnderlyingMesh)
{
	ClearOverlay();

	if (CenterlinePoints.Num() < 2)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[LaneCalibrationOverlay] Not enough centerline points."));
		return;
	}

	const UTrafficVisualSettings* VisualSettings = GetDefault<UTrafficVisualSettings>();

	// Match overlay actor to the road transform.
	SetActorTransform(RoadTransform);

	// Convert provider world-space centerline points into local space relative to the road transform.
	TArray<FVector> LocalCenterlinePoints;
	LocalCenterlinePoints.Reserve(CenterlinePoints.Num());
	const FTransform InverseRoadTransform = RoadTransform.Inverse();
	for (const FVector& WorldPos : CenterlinePoints)
	{
		LocalCenterlinePoints.Add(InverseRoadTransform.TransformPosition(WorldPos));
	}

	// Seed editable preview settings from the provided family definition.
	NumLanesPerSideForward = Family.Forward.NumLanes;
	NumLanesPerSideBackward = Family.Backward.NumLanes;
	LaneWidthCm = Family.Forward.LaneWidthCm;
	CenterlineOffsetCm = Family.Forward.InnerLaneCenterOffsetCm;

	// Cache original world-space data and current calibration for live rebuild.
	CachedCenterlinePoints = CenterlinePoints;
	CachedRoadTransform = RoadTransform;
	CachedCalibration.NumLanesPerSideForward = NumLanesPerSideForward;
	CachedCalibration.NumLanesPerSideBackward = NumLanesPerSideBackward;
	CachedCalibration.LaneWidthCm = LaneWidthCm;
	CachedCalibration.CenterlineOffsetCm = CenterlineOffsetCm;

	if (!IsCalibrationValid(CachedCalibration))
	{
		UE_LOG(LogTraffic, Warning,
			TEXT("[LaneCalibrationOverlay] Invalid calibration (TotalLanes=%d, Width=%.1f). "
				 "At least one lane and a positive width are required."),
			CachedCalibration.NumLanesPerSideForward + CachedCalibration.NumLanesPerSideBackward,
			CachedCalibration.LaneWidthCm);
		return;
	}

	int32 LaneIndex = 0;

	for (int32 i = 0; i < NumLanesPerSideForward; ++i)
	{
		float Offset = CenterlineOffsetCm + (i * LaneWidthCm);
		TArray<FVector> LanePoints = ComputeLaneCenterline(LocalCenterlinePoints, Offset);

		UStaticMesh* ArrowMesh = GetArrowMesh(true, VisualSettings);
		if (ArrowMesh)
		{
			UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(this);
			ISM->SetupAttachment(RootComponent);
			ISM->SetStaticMesh(ArrowMesh);
			ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ISM->SetCastShadow(false);
			ISM->RegisterComponent();

			CreateArrowMaterialInstance(ISM, FLinearColor(0.2f, 0.95f, 0.3f, 1.0f));

			BuildChevronArrows(ISM, LanePoints, true, VisualSettings);
			ChevronArrowComponents.Add(ISM);
		}
		++LaneIndex;
	}

	for (int32 i = 0; i < NumLanesPerSideBackward; ++i)
	{
		float Offset = -(CenterlineOffsetCm + (i * LaneWidthCm));
		TArray<FVector> LanePoints = ComputeLaneCenterline(LocalCenterlinePoints, Offset);

		UStaticMesh* ArrowMesh = GetArrowMesh(false, VisualSettings);
		if (ArrowMesh)
		{
			UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(this);
			ISM->SetupAttachment(RootComponent);
			ISM->SetStaticMesh(ArrowMesh);
			ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ISM->SetCastShadow(false);
			ISM->RegisterComponent();

			CreateArrowMaterialInstance(ISM, FLinearColor(0.95f, 0.3f, 0.2f, 1.0f));

			BuildChevronArrows(ISM, LanePoints, false, VisualSettings);
			ChevronArrowComponents.Add(ISM);
		}
		++LaneIndex;
	}

	const int32 NumArrowInstances = GetArrowInstanceCount();
	UE_LOG(LogTraffic, Log, TEXT("[LaneCalibrationOverlay] Built chevron overlay with %d lanes (%d forward, %d backward) and %d arrow instances."),
		LaneIndex, NumLanesPerSideForward, NumLanesPerSideBackward, NumArrowInstances);
	if (NumArrowInstances == 0)
	{
		UE_LOG(LogTraffic, Warning,
			TEXT("[LaneCalibrationOverlay] Built chevron overlay but placed 0 arrow instances. "
				 "Check centerline validity and calibration settings."));
	}
}

void ALaneCalibrationOverlayActor::BuildFromCenterline(const TArray<FVector>& CenterlinePoints, const FTrafficLaneFamilyCalibration& Calibration, const FTransform& RoadTransform)
{
	CachedCenterlinePoints = CenterlinePoints;
	CachedCalibration = Calibration;
	CachedRoadTransform = RoadTransform;

	if (!IsCalibrationValid(CachedCalibration))
	{
		UE_LOG(LogTraffic, Warning,
			TEXT("[LaneCalibrationOverlay] Invalid calibration (TotalLanes=%d, Width=%.1f). "
				 "At least one lane and a positive width are required."),
			CachedCalibration.NumLanesPerSideForward + CachedCalibration.NumLanesPerSideBackward,
			CachedCalibration.LaneWidthCm);
		return;
	}

	FRoadFamilyDefinition TempFamily;
	TempFamily.Forward.NumLanes = CachedCalibration.NumLanesPerSideForward;
	TempFamily.Backward.NumLanes = CachedCalibration.NumLanesPerSideBackward;
	TempFamily.Forward.LaneWidthCm = CachedCalibration.LaneWidthCm;
	TempFamily.Backward.LaneWidthCm = CachedCalibration.LaneWidthCm;
	TempFamily.Forward.InnerLaneCenterOffsetCm = CachedCalibration.CenterlineOffsetCm;
	TempFamily.Backward.InnerLaneCenterOffsetCm = CachedCalibration.CenterlineOffsetCm;

	BuildForRoad(CachedCenterlinePoints, TempFamily, CachedRoadTransform, false);
}

void ALaneCalibrationOverlayActor::Editor_RebuildFromCachedCenterline()
{
#if WITH_EDITOR
	if (CachedCenterlinePoints.Num() < 2)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[LaneCalibrationOverlay] No cached centerline to rebuild from."));
		return;
	}

	CachedCalibration.NumLanesPerSideForward = NumLanesPerSideForward;
	CachedCalibration.NumLanesPerSideBackward = NumLanesPerSideBackward;
	CachedCalibration.LaneWidthCm = LaneWidthCm;
	CachedCalibration.CenterlineOffsetCm = CenterlineOffsetCm;

	if (!IsCalibrationValid(CachedCalibration))
	{
		const bool bIsAutomationOrCommandlet = IsRunningCommandlet() || GIsAutomationTesting;
		if (!bIsAutomationOrCommandlet)
		{
			FMessageDialog::Open(
				EAppMsgType::Ok,
				NSLOCTEXT("LaneCalibrationOverlay", "InvalidCalibration",
					"The current calibration has zero lanes or an invalid lane width.\n"
					"Set at least one forward or backward lane and a positive lane width before previewing."));
		}
		else
		{
			UE_LOG(LogTraffic, Warning, TEXT("[LaneCalibrationOverlay][Automation] Invalid calibration (zero lanes or invalid width); dialog suppressed."));
		}
		return;
	}

	BuildFromCenterline(CachedCenterlinePoints, CachedCalibration, CachedRoadTransform);
#endif
}

#if WITH_EDITOR
void ALaneCalibrationOverlayActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropName == GET_MEMBER_NAME_CHECKED(ALaneCalibrationOverlayActor, NumLanesPerSideForward) ||
		PropName == GET_MEMBER_NAME_CHECKED(ALaneCalibrationOverlayActor, NumLanesPerSideBackward) ||
		PropName == GET_MEMBER_NAME_CHECKED(ALaneCalibrationOverlayActor, LaneWidthCm) ||
		PropName == GET_MEMBER_NAME_CHECKED(ALaneCalibrationOverlayActor, CenterlineOffsetCm))
	{
		Editor_RebuildFromCachedCenterline();
	}
}
#endif
