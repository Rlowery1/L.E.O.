#include "StaticMeshRoadGeometryProvider.h"

#include "RoadKitProfile.h"
#include "TrafficGeometryProvider.h"
#include "TrafficDrivableSurfaceComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "TrafficRuntimeModule.h"
#include "MeshDescription.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/DynamicMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "StaticMeshResources.h"
#include "StaticMeshAttributes.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "VectorTypes.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

// Helper to harvest vertices from a static mesh. Uses render data if available,
// otherwise falls back to the mesh's MeshDescription so CPU access is never needed (editor only).
static void CollectVerticesFromStaticMesh(
	const UStaticMesh* Mesh,
	const FTransform& Xf,
	TArray<FVector>& OutVerts)
{
	if (!Mesh)
	{
		return;
	}

	// ---- Fast path: RenderData (if available) ----
	if (const FStaticMeshRenderData* RenderData = Mesh->GetRenderData())
	{
		if (RenderData->LODResources.Num() > 0)
		{
			const FStaticMeshLODResources& LOD0 = RenderData->LODResources[0];
			const uint32 Num = LOD0.VertexBuffers.PositionVertexBuffer.GetNumVertices();
			if (Num > 0)
			{
				OutVerts.Reserve(OutVerts.Num() + (int32)Num);
				for (uint32 i = 0; i < Num; ++i)
				{
					const FVector3f P3 = LOD0.VertexBuffers.PositionVertexBuffer.VertexPosition(i);
					OutVerts.Add(Xf.TransformPosition((FVector)P3));
				}
				return; // success
			}
		}
	}

	// ---- Reliable path: MeshDescription (Editor geometry, does NOT need Allow CPU Access) ----
#if WITH_EDITORONLY_DATA
	const FMeshDescription* MD = Mesh->GetMeshDescription(0);
	if (!MD)
	{
		return;
	}

	const TVertexAttributesConstRef<FVector3f> Positions =
		MD->VertexAttributes().GetAttributesRef<FVector3f>(MeshAttribute::Vertex::Position);

	const int32 NumVerts = MD->Vertices().Num();
	OutVerts.Reserve(OutVerts.Num() + NumVerts);

	for (const FVertexID Vid : MD->Vertices().GetElementIDs())
	{
		const FVector3f P3 = Positions[Vid];
		OutVerts.Add(Xf.TransformPosition((FVector)P3));
	}
#endif // WITH_EDITORONLY_DATA
}

static void CollectVerticesFromDynamicMeshComponent(
	UDynamicMeshComponent* DynComp,
	const FTransform& Xf,
	TArray<FVector>& OutVerts)
{
	if (!DynComp)
	{
		return;
	}

	const UDynamicMesh* DynMesh = DynComp->GetDynamicMesh();
	const UE::Geometry::FDynamicMesh3* Mesh = DynMesh ? DynMesh->GetMeshPtr() : nullptr;
	if (!Mesh)
	{
		return;
	}

	const int32 VertexCount = Mesh->VertexCount();
	OutVerts.Reserve(OutVerts.Num() + VertexCount);

	for (int32 Vid : Mesh->VertexIndicesItr())
	{
		const FVector3d PosD = Mesh->GetVertex(Vid);
		OutVerts.Add(Xf.TransformPosition(static_cast<FVector>(PosD)));
	}
}

static void CollectVerticesFromInstancedStaticMeshComponent(
	const UInstancedStaticMeshComponent* ISMC,
	TArray<FVector>& OutVerts)
{
	if (!ISMC)
	{
		return;
	}

	const UStaticMesh* Mesh = ISMC->GetStaticMesh();
	if (!Mesh)
	{
		return;
	}

	TArray<FVector> BaseVerts;
	CollectVerticesFromStaticMesh(Mesh, FTransform::Identity, BaseVerts);

	const int32 NumInstances = ISMC->GetInstanceCount();
	if (NumInstances <= 0 || BaseVerts.Num() <= 0)
	{
		return;
	}

	const int64 AdditionalVerts = static_cast<int64>(NumInstances) * static_cast<int64>(BaseVerts.Num());
	const int64 DesiredCapacity = static_cast<int64>(OutVerts.Num()) + AdditionalVerts;
	if (DesiredCapacity > OutVerts.Num() && DesiredCapacity <= MAX_int32)
	{
		OutVerts.Reserve(static_cast<int32>(DesiredCapacity));
	}

	for (int32 InstanceIndex = 0; InstanceIndex < NumInstances; ++InstanceIndex)
	{
		FTransform InstanceXf;
		if (!ISMC->GetInstanceTransform(InstanceIndex, InstanceXf, /*bWorldSpace=*/true))
		{
			continue;
		}

		for (const FVector& V : BaseVerts)
		{
			OutVerts.Add(InstanceXf.TransformPosition(V));
		}
	}
}

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

		UE_LOG(LogTraffic, Warning,
			TEXT("[Debug][StaticMeshRoadGeometryProvider] Added TrafficRoad for %s with %d centreline points."),
			Actor ? *Actor->GetName() : TEXT("None"),
			Road.CenterlinePoints.Num());
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

	// CityBLD baked road detection must run first so later heuristics don't filter it out.
	const UStaticMesh* Mesh = Comp->GetStaticMesh();
	if (Mesh && IsGeneratedCityBLDRoad(Mesh))
	{
		return true;
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

	// If material keywords didn't match, allow instanced static meshes that look like "Road" meshes by name/size.
	if (Cast<UInstancedStaticMeshComponent>(Comp) && Mesh)
	{
		const bool bRoadLikeName =
			MeshName.Contains(TEXT("Road"), ESearchCase::IgnoreCase) ||
			MeshName.Contains(TEXT("Street"), ESearchCase::IgnoreCase) ||
			MeshName.Contains(TEXT("Highway"), ESearchCase::IgnoreCase);

		const bool bExcludedName =
			MeshName.Contains(TEXT("Sidewalk"), ESearchCase::IgnoreCase) ||
			MeshName.Contains(TEXT("Curb"), ESearchCase::IgnoreCase) ||
			MeshName.Contains(TEXT("Scatter"), ESearchCase::IgnoreCase);

		if (bRoadLikeName && !bExcludedName)
		{
			const FBoxSphereBounds Bounds = Mesh->GetBounds();
			const float SizeX = Bounds.BoxExtent.X * 2.f;
			const float SizeY = Bounds.BoxExtent.Y * 2.f;
			const float LengthCm = FMath::Max(SizeX, SizeY);
			const float WidthCm = FMath::Min(SizeX, SizeY);
			if (WidthCm >= 300.f && LengthCm > WidthCm)
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
		if (const UInstancedStaticMeshComponent* InstComp = Cast<UInstancedStaticMeshComponent>(Comp))
		{
			CollectVerticesFromInstancedStaticMeshComponent(InstComp, CollectedVerts);
		}
		else
		{
			CollectVerticesFromStaticMesh(Mesh, Comp->GetComponentTransform(), CollectedVerts);
		}

		UE_LOG(LogTraffic, Warning,
			TEXT("[Verts] Actor=%s Comp=%s Mesh=%s CollectedSoFar=%d RenderData=%s AllowCPU=%s"),
			*Actor->GetName(),
			*Comp->GetName(),
			Mesh ? *Mesh->GetPathName() : TEXT("NULL"),
			CollectedVerts.Num(),
			(Mesh && Mesh->GetRenderData()) ? TEXT("YES") : TEXT("NO"),
			(Mesh && Mesh->bAllowCPUAccess) ? TEXT("YES") : TEXT("NO"));
	}

	TArray<UDynamicMeshComponent*> DynMeshComps;
	Actor->GetComponents<UDynamicMeshComponent>(DynMeshComps);
	for (UDynamicMeshComponent* DynComp : DynMeshComps)
	{
		CollectVerticesFromDynamicMeshComponent(DynComp, DynComp->GetComponentTransform(), CollectedVerts);

		UE_LOG(LogTraffic, Warning,
			TEXT("[Verts] Actor=%s Comp=%s Mesh=%s CollectedSoFar=%d RenderData=%s AllowCPU=%s"),
			*Actor->GetName(),
			DynComp ? *DynComp->GetName() : TEXT("NULL"),
			TEXT("DYNAMIC"),
			CollectedVerts.Num(),
			TEXT("NO"),
			TEXT("N/A"));
	}

	if (CollectedVerts.Num() < 2)
	{
		// Fallback: sample the actor's spline component. If we have mesh vertices, use them to refine;
		// otherwise just use the spline positions directly.
		UE_LOG(LogTraffic, Warning,
			TEXT("[Debug][StaticMeshRoadGeometryProvider] Actor %s collected %d vertices; attempting spline fallback."),
			Actor ? *Actor->GetName() : TEXT("None"),
			CollectedVerts.Num());
		const USplineComponent* SplineComp = Actor ? Actor->FindComponentByClass<USplineComponent>() : nullptr;
		if (SplineComp)
		{
			UE_LOG(LogTraffic, Warning,
				TEXT("[Debug][StaticMeshRoadGeometryProvider] Actor %s has a SplineComponent; length %.2f cm."),
				Actor ? *Actor->GetName() : TEXT("None"),
				SplineComp->GetSplineLength());
			const float Length = SplineComp->GetSplineLength();
			const int32 SampleCount = FMath::Clamp(static_cast<int32>(Length / 200.f), 10, 200);
			OutPoints.Reserve(SampleCount + 1);
			const FVector UpVector(0.f, 0.f, 1.f);
			for (int32 i = 0; i <= SampleCount; ++i)
			{
				const float Distance = Length * static_cast<float>(i) / static_cast<float>(SampleCount);
				const FVector Pos = SplineComp->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
				// If we have any vertices, try to refine using left/right extremes; otherwise use Pos.
				if (CollectedVerts.Num() >= 2)
				{
					FVector Tangent = SplineComp->GetTangentAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
					Tangent.Z = 0.f;
					if (Tangent.Normalize())
					{
						FVector Right = FVector::CrossProduct(Tangent, UpVector);
						Right.Normalize();
						float MinDist = FLT_MAX;
						float MaxDist = -FLT_MAX;
						for (const FVector& V : CollectedVerts)
						{
							const float DistLR = FVector::DotProduct(V - Pos, Right);
							if (DistLR < MinDist) { MinDist = DistLR; }
							if (DistLR > MaxDist) { MaxDist = DistLR; }
						}
						const float MidDist = 0.5f * (MinDist + MaxDist);
						OutPoints.Add(Pos + Right * MidDist);
						continue;
					}
				}
				// No vertices or failed normalization; fall back to using spline location directly.
				OutPoints.Add(Pos);
			}
			return OutPoints.Num() >= 2;
		}
		else
		{
			UE_LOG(LogTraffic, Warning,
				TEXT("[Debug][StaticMeshRoadGeometryProvider] Actor %s has no SplineComponent."),
				Actor ? *Actor->GetName() : TEXT("None"));
		}
		return false;
	}

	ExtractCentreline(CollectedVerts, OutPoints);
	return OutPoints.Num() >= 2;
}

bool UStaticMeshRoadGeometryProvider::IsGeneratedCityBLDRoad(const UStaticMesh* Mesh) const
{
	if (!Mesh)
	{
		return false;
	}

	const FString Path = Mesh->GetPathName();
	// Must be under CityBLD/Meshes/Generated
	if (!Path.Contains(TEXT("/CityBLD/Meshes/Generated")))
	{
		return false;
	}

	const FString Name = Mesh->GetName();
	// Name must include "Road" and must NOT include "Sidewalk", "Curb", "Scatter"
	if (!Name.Contains(TEXT("Road")) ||
		Name.Contains(TEXT("Sidewalk")) ||
		Name.Contains(TEXT("Curb")) ||
		Name.Contains(TEXT("Scatter")))
	{
		return false;
	}

	// Check size: require road to be at least 3m wide and longer than it is wide
	const FBoxSphereBounds Bounds = Mesh->GetBounds();
	const float WidthCm  = Bounds.BoxExtent.Y * 2.f;
	const float LengthCm = Bounds.BoxExtent.X * 2.f;
	const float MinWidthCm = 300.f;    // 3m minimum width
	if (WidthCm < MinWidthCm || LengthCm <= WidthCm)
	{
		return false;
	}

	// Check material: ensure first material contains Asphalt or Road
	if (const UMaterialInterface* FirstMat = Mesh->GetMaterial(0))
	{
		const FString MatName = FirstMat->GetName();
		if (!MatName.Contains(TEXT("Asphalt")) && !MatName.Contains(TEXT("Road")))
		{
			return false;
		}
	}

	return true;
}

void UStaticMeshRoadGeometryProvider::ExtractCentreline(const TArray<FVector>& Vertices, TArray<FVector>& OutPoints) const
{
	OutPoints.Reset();

	const int32 NumVerts = Vertices.Num();
	if (NumVerts < 2)
	{
		return;
	}

	// Compute initial global mean and projections to sort vertices along approximate length.
	FVector GlobalMean = FVector::ZeroVector;
	for (const FVector& V : Vertices)
	{
		GlobalMean += V;
	}
	GlobalMean /= static_cast<float>(NumVerts);

	// Estimate a rough global axis via PCA for sorting.
	double CovXX = 0.0, CovXY = 0.0, CovYY = 0.0;
	for (const FVector& V : Vertices)
	{
		const double DX = static_cast<double>(V.X - GlobalMean.X);
		const double DY = static_cast<double>(V.Y - GlobalMean.Y);
		CovXX += DX * DX;
		CovXY += DX * DY;
		CovYY += DY * DY;
	}
	const double InvN = 1.0 / FMath::Max(1, NumVerts);
	CovXX *= InvN;
	CovXY *= InvN;
	CovYY *= InvN;
	const double Angle = 0.5 * FMath::Atan2(2.0 * CovXY, CovXX - CovYY);
	const FVector2D GlobalDir(static_cast<float>(FMath::Cos(Angle)), static_cast<float>(FMath::Sin(Angle)));

	// Project vertices onto the rough axis and sort.
	TArray<float> Projections;
	Projections.SetNumUninitialized(NumVerts);
	for (int32 i = 0; i < NumVerts; ++i)
	{
		const FVector& V = Vertices[i];
		const FVector2D Delta(V.X - GlobalMean.X, V.Y - GlobalMean.Y);
		Projections[i] = FVector2D::DotProduct(Delta, GlobalDir);
	}
	TArray<int32> SortedIndices;
	SortedIndices.SetNumUninitialized(NumVerts);
	for (int32 i = 0; i < NumVerts; ++i)
	{
		SortedIndices[i] = i;
	}
	SortedIndices.Sort([&](int32 A, int32 B) { return Projections[A] < Projections[B]; });

	// Define the number of sections (use more sections for longer roads).
	const int32 NumSections = 30;
	const int32 SectionSize = FMath::Max(1, NumVerts / NumSections);

	const FVector UpVector(0.f, 0.f, 1.f);
	for (int32 SectionStart = 0; SectionStart < SortedIndices.Num(); SectionStart += SectionSize)
	{
		const int32 SectionEnd = FMath::Min(SectionStart + SectionSize, SortedIndices.Num());
		if (SectionEnd - SectionStart < 2)
		{
			continue;
		}

		// Compute local mean of this section.
		FVector LocalMean(0.f, 0.f, 0.f);
		for (int32 i = SectionStart; i < SectionEnd; ++i)
		{
			LocalMean += Vertices[SortedIndices[i]];
		}
		LocalMean /= static_cast<float>(SectionEnd - SectionStart);

		// Estimate local forward direction using endpoints of this section.
		const FVector& StartV = Vertices[SortedIndices[SectionStart]];
		const FVector& EndV = Vertices[SortedIndices[SectionEnd - 1]];
		FVector LocalForward = (EndV - StartV);
		LocalForward.Z = 0.f;
		if (!LocalForward.Normalize())
		{
			LocalForward = FVector(GlobalDir.X, GlobalDir.Y, 0.f);
		}

		// Compute local right vector (perpendicular in XY plane).
		FVector LocalRight = FVector::CrossProduct(LocalForward, UpVector);
		LocalRight.Normalize();

		// Find extreme distances along LocalRight.
		float MinDist = FLT_MAX;
		float MaxDist = -FLT_MAX;
		for (int32 i = SectionStart; i < SectionEnd; ++i)
		{
			const FVector& V = Vertices[SortedIndices[i]];
			const float Dist = FVector::DotProduct(V - LocalMean, LocalRight);
			MinDist = FMath::Min(MinDist, Dist);
			MaxDist = FMath::Max(MaxDist, Dist);
		}
		// Midpoint between extremes projected back to world space.
		const float MidDist = 0.5f * (MinDist + MaxDist);
		const FVector CenterPoint = LocalMean + LocalRight * MidDist;
		OutPoints.Add(CenterPoint);
	}

	// Ensure the last sorted vertex contributes a final point.
	if (SortedIndices.Num() > 0)
	{
		OutPoints.Add(Vertices[SortedIndices.Last()]);
	}
}
