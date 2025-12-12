#include "StaticMeshRoadGeometryProvider.h"

#include "RoadKitProfile.h"
#include "TrafficGeometryProvider.h"
#include "TrafficDrivableSurfaceComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "TrafficRuntimeModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "StaticMeshResources.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace
{
	static void ApplyDefaultProfileValues(URoadKitProfile* Profile)
	{
		if (!Profile)
		{
			return;
		}

		Profile->DrivableMaterialKeywords = { TEXT("Asphalt"), TEXT("Road") };
		Profile->ExcludedMeshNameKeywords = { TEXT("Tree"), TEXT("Lamp") };
		Profile->DefaultLaneCount = 2;
		Profile->DefaultLaneWidth = 350.f;
		Profile->MaxLateralOffset = 500.f;
		Profile->MaxHeight = 100.f;
	}

	static URoadKitProfile* CreateTransientDefaultProfile(UObject* Outer)
	{
		URoadKitProfile* Profile = NewObject<URoadKitProfile>(Outer);
		ApplyDefaultProfileValues(Profile);
		return Profile;
	}

#if WITH_EDITOR
	static URoadKitProfile* CreateProfileAsset(const FString& PackagePath)
	{
		const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
		UPackage* Package = CreatePackage(*PackagePath);
		if (!Package)
		{
			return nullptr;
		}

		Package->FullyLoad();

		URoadKitProfile* Profile = NewObject<URoadKitProfile>(
			Package,
			*AssetName,
			RF_Public | RF_Standalone | RF_MarkAsRootSet);

		if (!Profile)
		{
			return nullptr;
		}

		ApplyDefaultProfileValues(Profile);

		FAssetRegistryModule::AssetCreated(Profile);
		Package->MarkPackageDirty();
		Profile->MarkPackageDirty();

		const FString Filename = FPackageName::LongPackageNameToFilename(
			PackagePath,
			FPackageName::GetAssetPackageExtension());

		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		if (!UPackage::SavePackage(Package, Profile, *Filename, SaveArgs))
		{
			UE_LOG(LogTraffic, Warning, TEXT("[StaticMeshRoadGeometryProvider] Failed to save default profile to %s"), *Filename);
		}

		return Profile;
	}
#endif // WITH_EDITOR
}

UStaticMeshRoadGeometryProvider::UStaticMeshRoadGeometryProvider()
{
	constexpr TCHAR DefaultProfilePath[] = TEXT("/AAAtrafficSystem/DefaultRoadKitProfile.DefaultRoadKitProfile");

	static ConstructorHelpers::FObjectFinder<URoadKitProfile> DefaultProfile(DefaultProfilePath);
	if (DefaultProfile.Succeeded() && DefaultProfile.Object)
	{
		ActiveProfile = DefaultProfile.Object;
	}

	const FString PackagePath = TEXT("/AAAtrafficSystem/DefaultRoadKitProfile");

	if (!ActiveProfile && GIsEditor && !IsRunningCommandlet())
	{
#if WITH_EDITOR
		ActiveProfile = CreateProfileAsset(PackagePath);
#endif
	}

	if (!ActiveProfile)
	{
		ActiveProfile = CreateTransientDefaultProfile(this);
	}
}

bool UStaticMeshRoadGeometryProvider::GetDisplayCenterlineForActor(AActor* RoadActor, TArray<FVector>& OutPoints) const
{
	return BuildCenterlineFromActor(RoadActor, OutPoints);
}

void UStaticMeshRoadGeometryProvider::CollectRoads(UWorld* World, TArray<FTrafficRoad>& OutRoads)
{
	OutRoads.Reset();

	if (!World)
	{
		UE_LOG(LogTraffic, Warning, TEXT("[StaticMeshRoadGeometryProvider] World is null."));
		return;
	}

	int32 NextRoadId = 0;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}

		TArray<FVector> Centerline;
		if (!BuildCenterlineFromActor(Actor, Centerline) || Centerline.Num() < 2)
		{
			continue;
		}

		FTrafficRoad Road;
		Road.RoadId = NextRoadId++;
		Road.FamilyId = 0;
		Road.SourceActor = Actor;
		Road.CenterlinePoints = MoveTemp(Centerline);

		const int32 LaneCount = ActiveProfile ? FMath::Max(1, ActiveProfile->DefaultLaneCount) : 1;
		const float LaneWidth = ActiveProfile ? ActiveProfile->DefaultLaneWidth : 350.f;
		Road.Lanes.Reserve(LaneCount);
		for (int32 LaneIdx = 0; LaneIdx < LaneCount; ++LaneIdx)
		{
			FTrafficLane Lane;
			Lane.LaneId = Road.Lanes.Num();
			Lane.RoadId = Road.RoadId;
			Lane.SideIndex = LaneIdx;
			Lane.Width = LaneWidth;
			Lane.CenterlinePoints = Road.CenterlinePoints;
			Road.Lanes.Add(Lane);
		}

		OutRoads.Add(MoveTemp(Road));
	}

	UE_LOG(LogTraffic, Log,
		TEXT("[StaticMeshRoadGeometryProvider] Collected %d roads from static meshes."),
		OutRoads.Num());

	// If no roads were found via static meshes, fall back to the generic spline provider for compatibility.
	if (OutRoads.Num() == 0)
	{
		if (UGenericSplineRoadGeometryProvider* SplineProvider = NewObject<UGenericSplineRoadGeometryProvider>(GetTransientPackage()))
		{
			SplineProvider->CollectRoads(World, OutRoads);
		}
	}
}

bool UStaticMeshRoadGeometryProvider::IsComponentDrivable(const UStaticMeshComponent* Comp) const
{
	if (!Comp)
	{
		return false;
	}

	// Treat spline mesh components as drivable so their underlying static mesh can be sampled.
	if (const USplineMeshComponent* SplineMesh = Cast<USplineMeshComponent>(Comp))
	{
		// Optional: apply lateral/height filters here if needed.
		return true;
	}

	if (const AActor* Owner = Comp->GetOwner())
	{
		if (Owner->GetComponentByClass(UTrafficDrivableSurfaceComponent::StaticClass()))
		{
			return true;
		}
	}

	const URoadKitProfile* Profile = ActiveProfile ? ActiveProfile.Get() : nullptr;
	if (!Profile)
	{
		return false;
	}

	const FVector LocalPos = Comp->GetRelativeLocation();
	if (FMath::Abs(LocalPos.Y) > Profile->MaxLateralOffset ||
		LocalPos.Z > Profile->MaxHeight)
	{
		return false;
	}

	const UStaticMesh* Mesh = Comp->GetStaticMesh();
	const FString MeshName = Mesh ? Mesh->GetName() : FString();
	for (const FString& Exclude : Profile->ExcludedMeshNameKeywords)
	{
		if (!Exclude.IsEmpty() && MeshName.Contains(Exclude, ESearchCase::IgnoreCase))
		{
			return false;
		}
	}

	if (Profile->DrivableMaterialKeywords.Num() == 0)
	{
		return true;
	}

	for (int32 MatIndex = 0; MatIndex < Comp->GetNumMaterials(); ++MatIndex)
	{
		const UMaterialInterface* Mat = Comp->GetMaterial(MatIndex);
		if (!Mat)
		{
			continue;
		}

		const FString MatName = Mat->GetName();
		for (const FString& Include : Profile->DrivableMaterialKeywords)
		{
			if (!Include.IsEmpty() && MatName.Contains(Include, ESearchCase::IgnoreCase))
			{
				return true;
			}
		}
	}

	return false;
}

bool UStaticMeshRoadGeometryProvider::BuildCenterlineFromActor(const AActor* Actor, TArray<FVector>& OutPoints) const
{
	OutPoints.Reset();

	if (!Actor || !ActiveProfile)
	{
		return false;
	}

	TArray<UStaticMeshComponent*> MeshComps;
	Actor->GetComponents<UStaticMeshComponent>(MeshComps);

	TArray<FVector> CollectedVerts;

	for (UStaticMeshComponent* Comp : MeshComps)
	{
		if (!IsComponentDrivable(Comp))
		{
			continue;
		}

		const UStaticMesh* Mesh = Comp->GetStaticMesh();
		if (!Mesh)
		{
			continue;
		}

		const FStaticMeshRenderData* RenderData = Mesh->GetRenderData();
		if (!RenderData || RenderData->LODResources.Num() == 0)
		{
			continue;
		}

		const FStaticMeshLODResources& LOD = RenderData->LODResources[0];
		const FPositionVertexBuffer& VB = LOD.VertexBuffers.PositionVertexBuffer;
		const FTransform CompToWorld = Comp->GetComponentTransform();

		CollectedVerts.Reserve(CollectedVerts.Num() + VB.GetNumVertices());

		for (uint32 i = 0; i < VB.GetNumVertices(); ++i)
		{
			CollectedVerts.Add(CompToWorld.TransformPosition(FVector(VB.VertexPosition(i))));
		}
	}

	if (CollectedVerts.Num() < 2)
	{
		// Fallback: sample the actor's spline component if present
		const USplineComponent* SplineComp = Actor ? Actor->FindComponentByClass<USplineComponent>() : nullptr;
		if (SplineComp)
		{
			const float Length = SplineComp->GetSplineLength();
			const int32 SampleCount = FMath::Clamp(static_cast<int32>(Length / 200.f), 10, 1000);
			OutPoints.Reserve(SampleCount + 1);
			for (int32 i = 0; i <= SampleCount; ++i)
			{
				const float Distance = Length * static_cast<float>(i) / static_cast<float>(SampleCount);
				OutPoints.Add(SplineComp->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World));
			}
			return OutPoints.Num() >= 2;
		}
		return false;
	}

	ExtractCentreline(CollectedVerts, OutPoints);
	return OutPoints.Num() >= 2;
}

void UStaticMeshRoadGeometryProvider::ExtractCentreline(const TArray<FVector>& Vertices, TArray<FVector>& OutPoints) const
{
	OutPoints.Reset();

	const int32 NumVerts = Vertices.Num();
	if (NumVerts < 2)
	{
		return;
	}

	// Compute PCA as before to obtain the principal axis direction.
	FVector Mean = FVector::ZeroVector;
	for (const FVector& V : Vertices)
	{
		Mean += V;
	}
	Mean /= static_cast<float>(NumVerts);

	double CovXX = 0.0;
	double CovXY = 0.0;
	double CovYY = 0.0;

	for (const FVector& V : Vertices)
	{
		const double DX = static_cast<double>(V.X - Mean.X);
		const double DY = static_cast<double>(V.Y - Mean.Y);
		CovXX += DX * DX;
		CovXY += DX * DY;
		CovYY += DY * DY;
	}

	const double InvN = 1.0 / FMath::Max(1, NumVerts);
	CovXX *= InvN;
	CovXY *= InvN;
	CovYY *= InvN;

	const double Angle = 0.5 * FMath::Atan2(2.0 * CovXY, CovXX - CovYY);
	const FVector2D Dir(static_cast<float>(FMath::Cos(Angle)), static_cast<float>(FMath::Sin(Angle)));

	// Project all vertices onto the principal axis and sort indices.
	TArray<float> Projections;
	Projections.Reserve(NumVerts);
	for (const FVector& V : Vertices)
	{
		const FVector2D Delta(V.X - Mean.X, V.Y - Mean.Y);
		Projections.Add(FVector2D::DotProduct(Delta, Dir));
	}

	TArray<int32> Indices;
	Indices.Reserve(NumVerts);
	for (int32 i = 0; i < NumVerts; ++i)
	{
		Indices.Add(i);
	}
	Indices.Sort([&](int32 A, int32 B) { return Projections[A] < Projections[B]; });

	// Divide the sorted vertices into equal-sized sections and average each section.
	const int32 NumSections = 20;
	const int32 SectionSize = FMath::Max(1, NumVerts / NumSections);
	for (int32 SectionStart = 0; SectionStart < Indices.Num(); SectionStart += SectionSize)
	{
		const int32 SectionEnd = FMath::Min(SectionStart + SectionSize, Indices.Num());
		FVector Sum(0.f, 0.f, 0.f);
		const int32 SectionCount = SectionEnd - SectionStart;
		for (int32 i = SectionStart; i < SectionEnd; ++i)
		{
			Sum += Vertices[Indices[i]];
		}
		OutPoints.Add(Sum / static_cast<float>(SectionCount));
	}

	// Ensure the last point is included exactly.
	if (Indices.Num() > 0)
	{
		OutPoints.Add(Vertices[Indices.Last()]);
	}
}
