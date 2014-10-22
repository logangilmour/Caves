// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "GeneratedMeshComponent.generated.h"






struct FNormalList
{
	TArray<FVector> Normals;
};

struct FDynamicMeshVertex
{
	FDynamicMeshVertex() {}
	FDynamicMeshVertex(const FVector& InPosition) :
		Position(InPosition),
		TextureCoordinate(FVector2D::ZeroVector),
		TangentX(FVector(1, 0, 0)),
		TangentZ(FVector(0, 0, 1)),
		Color(FColor(255, 255, 255))
	{
		// basis determinant default to +1.0
		TangentZ.Vector.W = 255;
	}

	FDynamicMeshVertex(const FVector& InPosition, const FVector& InTangentX, const FVector& InTangentZ, const FVector2D& InTexCoord, const FColor& InColor) :
		Position(InPosition),
		TextureCoordinate(InTexCoord),
		TangentX(InTangentX),
		TangentZ(InTangentZ),
		Color(InColor)
	{
		// basis determinant default to +1.0
		TangentZ.Vector.W = 255;
	}

	void SetTangents(const FVector& InTangentX, const FVector& InTangentY, const FVector& InTangentZ)
	{
		TangentX = InTangentX;
		TangentZ = InTangentZ;
		// store determinant of basis in w component of normal vector
		TangentZ.Vector.W = GetBasisDeterminantSign(InTangentX, InTangentY, InTangentZ) < 0.0f ? 0 : 255;
	}

	FVector GetTangentY()
	{
		return (FVector(TangentZ) ^ FVector(TangentX)) * ((float)TangentZ.Vector.W / 127.5f - 1.0f);
	};

	FVector Position;
	FVector2D TextureCoordinate;
	FPackedNormal TangentX;
	FPackedNormal TangentZ;
	FColor Color;
};

/** Component that allows you to specify Generated triangle mesh geometry */
UCLASS(editinlinenew, meta = (BlueprintSpawnableComponent), ClassGroup = Rendering)
class UGeneratedMeshComponent : public UMeshComponent, public IInterface_CollisionDataProvider
{
	GENERATED_UCLASS_BODY()

	

public:
	/** Set the geometry to use on this triangle mesh */
//	UFUNCTION(BlueprintCallable, Category = "Components|GeneratedMesh")
		bool SetGeneratedMeshTriangles(const TArray<FDynamicMeshVertex>& vertices, const TArray<int32>& indices);

	UPROPERTY(BlueprintReadOnly, Category = "Collision")
	class UBodySetup* ModelBodySetup;

	virtual int32 GetNumMaterials() const override;

	// Begin Interface_CollisionDataProvider Interface
	virtual bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData) OVERRIDE;
	virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const OVERRIDE;
	virtual bool WantsNegXTriMesh() OVERRIDE{ return false; }
		// End Interface_CollisionDataProvider Interface

	virtual class UBodySetup* GetBodySetup() OVERRIDE;



	void UpdateBodySetup();
	void UpdateCollision();

	void RecalculateNormals();

	TArray<FDynamicMeshVertex> Vertices;
	TArray<int32> Indices;


private:

	// Begin UPrimitiveComponent interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	// End UPrimitiveComponent interface.

	// Begin UMeshComponent interface.
	// End UMeshComponent interface.

	// Begin USceneComponent interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const override;
	// Begin USceneComponent interface.

	/** */


	friend class FGeneratedMeshSceneProxy;
};


