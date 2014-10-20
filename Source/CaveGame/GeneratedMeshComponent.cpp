// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved. 

#include "CaveGame.h"
#include "GeneratedMeshComponent.h"
//#include "DynamicMeshBuilder.h"

/** Vertex Buffer */
class FGeneratedMeshVertexBuffer : public FVertexBuffer
{
public:
	TArray<FDynamicMeshVertex> Vertices;

	virtual void InitRHI() override
	{
		FRHIResourceCreateInfo CreateInfo;
		VertexBufferRHI = RHICreateVertexBuffer(Vertices.Num() * sizeof(FDynamicMeshVertex), BUF_Static, CreateInfo);

		// Copy the vertex data into the vertex buffer.
		void* VertexBufferData = RHILockVertexBuffer(VertexBufferRHI, 0, Vertices.Num() * sizeof(FDynamicMeshVertex), RLM_WriteOnly);
		FMemory::Memcpy(VertexBufferData, Vertices.GetTypedData(), Vertices.Num() * sizeof(FDynamicMeshVertex));
		RHIUnlockVertexBuffer(VertexBufferRHI);
	}

};

/** Index Buffer */
class FGeneratedMeshIndexBuffer : public FIndexBuffer
{
public:
	TArray<int32> Indices;

	virtual void InitRHI() override
	{
		FRHIResourceCreateInfo CreateInfo;
		IndexBufferRHI = RHICreateIndexBuffer(sizeof(int32), Indices.Num() * sizeof(int32), BUF_Static, CreateInfo);

		// Write the indices to the index buffer.
		void* Buffer = RHILockIndexBuffer(IndexBufferRHI, 0, Indices.Num() * sizeof(int32), RLM_WriteOnly);
		FMemory::Memcpy(Buffer, Indices.GetTypedData(), Indices.Num() * sizeof(int32));
		RHIUnlockIndexBuffer(IndexBufferRHI);
	}
};

/** Vertex Factory */
class FGeneratedMeshVertexFactory : public FLocalVertexFactory
{
public:

	FGeneratedMeshVertexFactory()
	{}


	/** Initialization */
	void Init(const FGeneratedMeshVertexBuffer* VertexBuffer)
	{
		check(!IsInRenderingThread());

		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			InitGeneratedMeshVertexFactory,
			FGeneratedMeshVertexFactory*, VertexFactory, this,
			const FGeneratedMeshVertexBuffer*, VertexBuffer, VertexBuffer,
			{
			// Initialize the vertex factory's stream components.
			DataType NewData;
			NewData.PositionComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FDynamicMeshVertex, Position, VET_Float3);
			NewData.TextureCoordinates.Add(
				FVertexStreamComponent(VertexBuffer, STRUCT_OFFSET(FDynamicMeshVertex, TextureCoordinate), sizeof(FDynamicMeshVertex), VET_Float2)
				);
			NewData.TangentBasisComponents[0] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FDynamicMeshVertex, TangentX, VET_PackedNormal);
			NewData.TangentBasisComponents[1] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FDynamicMeshVertex, TangentZ, VET_PackedNormal);
			NewData.ColorComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FDynamicMeshVertex, Color, VET_Color);
			VertexFactory->SetData(NewData);
			});
	}
};

/** Scene proxy */
class FGeneratedMeshSceneProxy : public FPrimitiveSceneProxy
{
public:

	FGeneratedMeshSceneProxy(UGeneratedMeshComponent* Component)
		: FPrimitiveSceneProxy(Component)
		, MaterialRelevance(Component->GetMaterialRelevance(GetScene()->GetFeatureLevel()))
	{
		VertexBuffer.Vertices = Component->Vertices;
		IndexBuffer.Indices = Component->Indices;
		// Init vertex factory
		VertexFactory.Init(&VertexBuffer);

		// Enqueue initialization of render resource
		BeginInitResource(&VertexBuffer);
		BeginInitResource(&IndexBuffer);
		BeginInitResource(&VertexFactory);

		// Grab material
		Material = Component->GetMaterial(0);
		if (Material == NULL)
		{
			Material = UMaterial::GetDefaultMaterial(MD_Surface);
		}
	}

	virtual ~FGeneratedMeshSceneProxy()
	{
		VertexBuffer.ReleaseResource();
		IndexBuffer.ReleaseResource();
		VertexFactory.ReleaseResource();
	}

	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_GeneratedMeshSceneProxy_DrawDynamicElements);

		const bool bWireframe = AllowDebugViewmodes() && View->Family->EngineShowFlags.Wireframe;

		FColoredMaterialRenderProxy WireframeMaterialInstance(
			GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy(IsSelected()) : NULL,
			FLinearColor(0, 0.5f, 1.f)
			);

		FMaterialRenderProxy* MaterialProxy = NULL;
		if (bWireframe)
		{
			MaterialProxy = &WireframeMaterialInstance;
		}
		else
		{
			MaterialProxy = Material->GetRenderProxy(IsSelected());
		}

		// Draw the mesh.
		FMeshBatch Mesh;
		FMeshBatchElement& BatchElement = Mesh.Elements[0];
		BatchElement.IndexBuffer = &IndexBuffer;
		Mesh.bWireframe = bWireframe;
		Mesh.VertexFactory = &VertexFactory;
		Mesh.MaterialRenderProxy = MaterialProxy;
		BatchElement.PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(GetLocalToWorld(), GetBounds(), GetLocalBounds(), true, UseEditorDepthTest());
		BatchElement.FirstIndex = 0; 
		BatchElement.NumPrimitives = IndexBuffer.Indices.Num() / 3;
		BatchElement.MinVertexIndex = 0;
		BatchElement.MaxVertexIndex = VertexBuffer.Vertices.Num() - 1;
		Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
		Mesh.Type = PT_TriangleList;
		Mesh.DepthPriorityGroup = SDPG_World;
		PDI->DrawMesh(Mesh);
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bDynamicRelevance = true;
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		return Result;
	}

	virtual bool CanBeOccluded() const override
	{
		return !MaterialRelevance.bDisableDepthTest;
	}

	virtual uint32 GetMemoryFootprint(void) const { return(sizeof(*this) + GetAllocatedSize()); }

	uint32 GetAllocatedSize(void) const { return(FPrimitiveSceneProxy::GetAllocatedSize()); }

private:

	UMaterialInterface* Material;
	FGeneratedMeshVertexBuffer VertexBuffer;
	FGeneratedMeshIndexBuffer IndexBuffer;
	FGeneratedMeshVertexFactory VertexFactory;

	FMaterialRelevance MaterialRelevance;
};

//////////////////////////////////////////////////////////////////////////

UGeneratedMeshComponent::UGeneratedMeshComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PrimaryComponentTick.bCanEverTick = false;

	SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
}

bool UGeneratedMeshComponent::SetGeneratedMeshTriangles(const TArray<FDynamicMeshVertex>& vertices, const TArray<int32>& indices)
{
	Vertices = vertices;
	Indices = indices;

	RecalculateNormals();
	// Need to recreate scene proxy to send it over
	MarkRenderStateDirty();

	return true;
}


FPrimitiveSceneProxy* UGeneratedMeshComponent::CreateSceneProxy()
{
	FPrimitiveSceneProxy* Proxy = NULL;
	if (Indices.Num() > 0)
	{
		Proxy = new FGeneratedMeshSceneProxy(this);
	}
	return Proxy;
}

void UGeneratedMeshComponent::RecalculateNormals()
{

	TMap<int32, FNormalList> faceLookup;
	
	for (int32 i = 0; i < Indices.Num(); i+=3)
	{
		FVector v0 = Vertices[Indices[i]].Position;
		FVector v1 = Vertices[Indices[i + 1]].Position;
		FVector v2 = Vertices[Indices[i + 2]].Position;

		const FVector Edge01 = (v1 - v0);
		const FVector Edge02 = (v2 - v0);



		const FVector Normal = (Edge02 ^ Edge01);

		faceLookup.FindOrAdd(Indices[i]).Normals.Add(Normal);
		faceLookup.FindOrAdd(Indices[i+1]).Normals.Add(Normal);
		faceLookup.FindOrAdd(Indices[i+2]).Normals.Add(Normal);


		/*
		const FVector TangentX = Edge01.SafeNormal();
		const FVector TangentZ = (Edge02 ^ Edge01).SafeNormal();
		const FVector TangentY = (TangentX ^ TangentZ).SafeNormal();
		*/
	}
	for (int32 i = 0; i < Vertices.Num(); i++){
		
		FVector normal = FVector(0, 0, 0);

		for (FVector v : faceLookup[i].Normals){
			normal += v.SafeNormal();
		}
		


		FVector TangentZ = normal.SafeNormal();

		Vertices[i].SetTangents(TangentZ, TangentZ, TangentZ);
	}
}

int32 UGeneratedMeshComponent::GetNumMaterials() const
{
	return 1;
}


FBoxSphereBounds UGeneratedMeshComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	FBoxSphereBounds NewBounds;
	NewBounds.Origin = FVector::ZeroVector;
	NewBounds.BoxExtent = FVector(HALF_WORLD_MAX, HALF_WORLD_MAX, HALF_WORLD_MAX);
	NewBounds.SphereRadius = FMath::Sqrt(3.0f * FMath::Square(HALF_WORLD_MAX));
	return NewBounds;
}




bool UGeneratedMeshComponent::GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData)
{
	FTriIndices Triangle;

	for (int32 i = 0; i<Indices.Num(); i+=3) {
		

		Triangle.v0 = CollisionData->Vertices.Add(Vertices[i].Position);
		Triangle.v1 = CollisionData->Vertices.Add(Vertices[i + 1].Position);
		Triangle.v2 = CollisionData->Vertices.Add(Vertices[i+2].Position);

		CollisionData->Indices.Add(Triangle);
		CollisionData->MaterialIndices.Add(i);
	}

	CollisionData->bFlipNormals = true;

	return true;
}

bool UGeneratedMeshComponent::ContainsPhysicsTriMeshData(bool InUseAllTriData) const
{
	return (Indices.Num() > 0);
}

void UGeneratedMeshComponent::UpdateBodySetup() {
	if (ModelBodySetup == NULL)	{
		ModelBodySetup = ConstructObject<UBodySetup>(UBodySetup::StaticClass(), this);
		ModelBodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
		ModelBodySetup->bMeshCollideAll = true;
	}
}

void UGeneratedMeshComponent::UpdateCollision() {
	if (bPhysicsStateCreated) {
		DestroyPhysicsState();
		UpdateBodySetup();
		CreatePhysicsState();

		ModelBodySetup->InvalidatePhysicsData(); //Will not work in Packaged build
		//Epic needs to add support for this
		ModelBodySetup->CreatePhysicsMeshes();
	}
}

UBodySetup* UGeneratedMeshComponent::GetBodySetup() {
	UpdateBodySetup();
	return ModelBodySetup;
}