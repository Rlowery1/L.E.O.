#include "TrafficLaneEndpointMarkerActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

ATrafficLaneEndpointMarkerActor::ATrafficLaneEndpointMarkerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
	MarkerMesh->SetupAttachment(SceneRoot);
	MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarkerMesh->SetCastShadow(false);

	if (UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")))
	{
		MarkerMesh->SetStaticMesh(Sphere);
		MarkerMesh->SetWorldScale3D(FVector(0.25f));
	}

	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!BaseMat)
	{
		BaseMat = UMaterial::GetDefaultMaterial(MD_Surface);
	}

	if (BaseMat)
	{
		if (UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMat, this))
		{
			DynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.9f, 0.2f, 1.0f));
			DynMat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(1.0f, 0.9f, 0.2f, 1.0f));
			MarkerMesh->SetMaterial(0, DynMat);
		}
	}

	SetActorHiddenInGame(true);
}

