#include "TrafficCalibrationVisuals.h"

#include "Engine/StaticMesh.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

UStaticMesh* FTrafficCalibrationVisuals::GetOrCreateCalibrationArrowMesh()
{
	static UStaticMesh* ArrowMeshSingleton = nullptr;
	if (ArrowMeshSingleton && ArrowMeshSingleton->IsValidLowLevelFast())
	{
		return ArrowMeshSingleton;
	}

	UPackage* Package = GetTransientPackage();
	ArrowMeshSingleton = NewObject<UStaticMesh>(Package, TEXT("TrafficCalibArrowMesh"));
	ArrowMeshSingleton->AddToRoot();

	FMeshDescription MeshDesc;
	FStaticMeshAttributes Attributes(MeshDesc);
	Attributes.Register();

	TPolygonGroupAttributesRef<FName> PolygonGroupMaterialSlotNames = Attributes.GetPolygonGroupMaterialSlotNames();
	const FPolygonGroupID PolygonGroupID = MeshDesc.CreatePolygonGroup();
	PolygonGroupMaterialSlotNames[PolygonGroupID] = FName(TEXT("ArrowMaterialSlot"));

	TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
	TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
	TVertexInstanceAttributesRef<FVector3f> VertexInstanceTangents = Attributes.GetVertexInstanceTangents();
	TVertexInstanceAttributesRef<float> VertexInstanceBinormalSigns = Attributes.GetVertexInstanceBinormalSigns();
	TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();
	TVertexInstanceAttributesRef<FVector4f> VertexInstanceColors = Attributes.GetVertexInstanceColors();
	VertexInstanceUVs.SetNumChannels(1);

	const float ArrowLength = 200.f;
	const float ArrowHalfHeadWidth = 80.f;
	const float ArrowHalfBodyWidth = 40.f;
	const float UVWidthDenom = ArrowHalfHeadWidth * 2.f;
	const float UVLengthDenom = FMath::Max(ArrowLength, 1.f);

	TArray<FVector> Positions;
	Positions.Reserve(7);
	Positions.Add(FVector(0.f, -ArrowHalfBodyWidth, 0.f));   // 0: body rear left
	Positions.Add(FVector(0.f, ArrowHalfBodyWidth, 0.f));    // 1: body rear right
	Positions.Add(FVector(100.f, -ArrowHalfBodyWidth, 0.f)); // 2: body front left
	Positions.Add(FVector(100.f, ArrowHalfBodyWidth, 0.f));  // 3: body front right
	Positions.Add(FVector(100.f, -ArrowHalfHeadWidth, 0.f)); // 4: head left base
	Positions.Add(FVector(100.f, ArrowHalfHeadWidth, 0.f));  // 5: head right base
	Positions.Add(FVector(ArrowLength, 0.f, 0.f));           // 6: head tip

	TArray<FVertexID> VertexIDs;
	VertexIDs.Reserve(Positions.Num());

	for (const FVector& Pos : Positions)
	{
		const FVertexID VertexID = MeshDesc.CreateVertex();
		VertexIDs.Add(VertexID);
		VertexPositions[VertexID] = FVector3f(Pos);
	}

	const auto SetInstanceAttributes = [&](const FVertexInstanceID InstanceID, int32 PositionIndex)
	{
		const FVector& Position = Positions[PositionIndex];
		VertexInstanceNormals[InstanceID] = FVector3f(FVector::UpVector);
		VertexInstanceTangents[InstanceID] = FVector3f(FVector::ForwardVector);
		VertexInstanceBinormalSigns[InstanceID] = 1.f;

		const float U = FMath::Clamp(Position.X / UVLengthDenom, 0.f, 1.f);
		const float V = FMath::Clamp((Position.Y + ArrowHalfHeadWidth) / UVWidthDenom, 0.f, 1.f);
		VertexInstanceUVs.Set(InstanceID, 0, FVector2f(U, V));

		VertexInstanceColors[InstanceID] = FVector4f(FLinearColor::White);
	};

	const auto AddTriangle = [&](int32 A, int32 B, int32 C)
	{
		const FVertexInstanceID V0 = MeshDesc.CreateVertexInstance(VertexIDs[A]);
		const FVertexInstanceID V1 = MeshDesc.CreateVertexInstance(VertexIDs[B]);
		const FVertexInstanceID V2 = MeshDesc.CreateVertexInstance(VertexIDs[C]);

		SetInstanceAttributes(V0, A);
		SetInstanceAttributes(V1, B);
		SetInstanceAttributes(V2, C);

		TArray<FVertexInstanceID> InstanceIDs;
		InstanceIDs.Add(V0);
		InstanceIDs.Add(V1);
		InstanceIDs.Add(V2);

		const FPolygonID PolyID = MeshDesc.CreatePolygon(PolygonGroupID, InstanceIDs);
		MeshDesc.ComputePolygonTriangulation(PolyID);
	};

	AddTriangle(0, 1, 3);
	AddTriangle(0, 3, 2);
	AddTriangle(4, 5, 6);
	AddTriangle(5, 4, 6);

	FStaticMaterial ArrowMaterial(UMaterial::GetDefaultMaterial(EMaterialDomain::MD_Surface));
	ArrowMaterial.MaterialSlotName = PolygonGroupMaterialSlotNames[PolygonGroupID];
	ArrowMeshSingleton->GetStaticMaterials().Add(ArrowMaterial);

	ArrowMeshSingleton->SetLightingGuid(FGuid::NewGuid());
	ArrowMeshSingleton->BuildFromMeshDescriptions({ &MeshDesc });

	return ArrowMeshSingleton;
}
