// Copyright 2018 Elhoussine Mehnik (Mhousse1247). All Rights Reserved.
//******************* http://ue4resources.com/ *********************//

#include "TreeMeshComponent.h"
#include "PrimitiveViewRelevance.h"
#include "RenderResource.h"
#include "RenderingThread.h"
#include "PrimitiveSceneProxy.h"
#include "Containers/ResourceArray.h"
#include "EngineGlobals.h"
#include "VertexFactory.h"
#include "MaterialShared.h"
#include "Materials/Material.h"
#include "LocalVertexFactory.h"
#include "Engine/Engine.h"
#include "SceneManagement.h"
#include "PhysicsEngine/BodySetup.h"
#include "ProceduralTreeStats.h"
#include "DynamicMeshBuilder.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "StaticMeshResources.h"


#include "proctree.h"

DECLARE_CYCLE_STAT(TEXT("Create TreeMesh Proxy"), STAT_ProceduralTreeMesh_CreateSceneProxy, STATGROUP_ProceduralTreeMesh);
DECLARE_CYCLE_STAT(TEXT("Create Tree Mesh Section"), STAT_ProceduralTreeMesh_CreateMeshSection, STATGROUP_ProceduralTreeMesh);
DECLARE_CYCLE_STAT(TEXT("Get Tree Mesh Elements"), STAT_ProceduralTreeMesh_GetMeshElements, STATGROUP_ProceduralTreeMesh);
DECLARE_CYCLE_STAT(TEXT("Update Collision"), STAT_ProceduralTreeMesh_UpdateCollision, STATGROUP_ProceduralTreeMesh);

/** Class representing a single section of the proc tree mesh */
class FProcTreeMeshProxySection
{
public:
	/** Material applied to this section */
	UMaterialInterface* Material;
	/** Vertex buffer for this section */
	FStaticMeshVertexBuffers VertexBuffers;
	/** Index buffer for this section */
	FDynamicMeshIndexBuffer32 IndexBuffer;
	/** Vertex factory for this section */
	FLocalVertexFactory VertexFactory;
	/** Whether this section is currently visible */
	bool bSectionVisible;

	FProcTreeMeshProxySection(ERHIFeatureLevel::Type InFeatureLevel)
	: Material(NULL)
	, VertexFactory(InFeatureLevel, "FProcTreeMeshSceneProxy")
	, bSectionVisible(true)
	{}
};


/** Procedural mesh scene proxy */
class FProcTreeMeshSceneProxy final : public FPrimitiveSceneProxy
{
public:
	SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	FProcTreeMeshSceneProxy(UProceduralTreeComponent* Component)
		: FPrimitiveSceneProxy(Component)
		, BodySetup(Component->GetBodySetup())
		, MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
	{

		const FColor DefaultVertColor = FColor::White;
		const FVector ZeroVector = FVector(0.f, 0.0f, 0.f);
		const FVector2D ZeroVector2D = FVector2D(0.f, 0.0f);
		const FProcTreeMeshTangent DefaultTangent = FProcTreeMeshTangent();
		const FVector DefaultNormal = FVector(0.f, 0.f, 1.f);

		// Copy each section
		const int32 NumSections = Component->TreeMeshSections.Num();
		Sections.AddZeroed(NumSections);
		for (int SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
		{
			FProcTreeMeshSection& SrcSection = Component->TreeMeshSections[SectionIdx];
			if (SrcSection.IndexBuffer.Num() > 0 && SrcSection.Vertices.Num() > 0)
			{
				FProcTreeMeshProxySection* NewSection = new FProcTreeMeshProxySection(GetScene().GetFeatureLevel());

				// Copy data from vertex buffer
				const int32 NumVerts = SrcSection.Vertices.Num();

				// Allocate verts

				TArray<FDynamicMeshVertex> Vertices;
				Vertices.SetNumUninitialized(NumVerts);
				// Copy verts
				for (int VertIdx = 0; VertIdx < NumVerts; VertIdx++)
				{
					FDynamicMeshVertex& Vert = Vertices[VertIdx];
					Vert.Position = SrcSection.Vertices[VertIdx];
					Vert.Color = FColor::White;
					Vert.TextureCoordinate[0] = (SrcSection.TextureCoordinates0.Num() == NumVerts) ? SrcSection.TextureCoordinates0[VertIdx] : FVector2D::ZeroVector;
					Vert.TangentX = (SrcSection.Tangents.Num() == NumVerts) ? SrcSection.Tangents[VertIdx].TangentX : DefaultTangent.TangentX;
					Vert.TangentZ = (SrcSection.Normals.Num() == NumVerts) ? SrcSection.Normals[VertIdx] : DefaultNormal;
					Vert.TangentZ.Vector.W = ((SrcSection.Tangents.Num() == NumVerts) && SrcSection.Tangents[VertIdx].bFlipTangentY) ? -127 : 127;
				}

				// Copy index buffer
				NewSection->IndexBuffer.Indices = SrcSection.IndexBuffer;

				NewSection->VertexBuffers.InitFromDynamicVertex(&NewSection->VertexFactory, Vertices, 4);

				// Enqueue initialization of render resource
				BeginInitResource(&NewSection->VertexBuffers.PositionVertexBuffer);
				BeginInitResource(&NewSection->VertexBuffers.StaticMeshVertexBuffer);
				BeginInitResource(&NewSection->VertexBuffers.ColorVertexBuffer);
				BeginInitResource(&NewSection->IndexBuffer);
				BeginInitResource(&NewSection->VertexFactory);

				// Grab material
				NewSection->Material = Component->GetMaterial(SectionIdx);
				if (NewSection->Material == NULL)
				{
					NewSection->Material = UMaterial::GetDefaultMaterial(MD_Surface);
				}

				// Copy visibility info
				NewSection->bSectionVisible = SrcSection.bSectionVisible;

				// Save ref to new section
				Sections[SectionIdx] = NewSection;
			}
		}
	}

	virtual ~FProcTreeMeshSceneProxy()
	{
		for (FProcTreeMeshProxySection* Section : Sections)
		{
			if (Section != nullptr)
			{
				Section->VertexBuffers.PositionVertexBuffer.ReleaseResource();
				Section->VertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
				Section->VertexBuffers.ColorVertexBuffer.ReleaseResource();
				Section->IndexBuffer.ReleaseResource();
				Section->VertexFactory.ReleaseResource();
				delete Section;
			}
		}
	}

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		SCOPE_CYCLE_COUNTER(STAT_ProceduralTreeMesh_GetMeshElements);


		// Set up wireframe material (if needed)
		const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;

		FColoredMaterialRenderProxy* WireframeMaterialInstance = NULL;
		if (bWireframe)
		{
			WireframeMaterialInstance = new FColoredMaterialRenderProxy(
				GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy(IsSelected()) : NULL,
				FLinearColor(0, 0.5f, 1.f)
				);

			Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);
		}

		// Iterate over sections
		for (const FProcTreeMeshProxySection* Section : Sections)
		{
			if (Section != nullptr && Section->bSectionVisible)
			{
				FMaterialRenderProxy* MaterialProxy = bWireframe ? WireframeMaterialInstance : Section->Material->GetRenderProxy(IsSelected());

				// For each view..
				for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
				{
					if (VisibilityMap & (1 << ViewIndex))
					{
						const FSceneView* View = Views[ViewIndex];
						// Draw the mesh.
						FMeshBatch& Mesh = Collector.AllocateMesh();
						FMeshBatchElement& BatchElement = Mesh.Elements[0];
						BatchElement.IndexBuffer = &Section->IndexBuffer;
						Mesh.bWireframe = bWireframe;
						Mesh.VertexFactory = &Section->VertexFactory;
						Mesh.MaterialRenderProxy = MaterialProxy;
						BatchElement.PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(GetLocalToWorld(), GetBounds(), GetLocalBounds(), true, UseEditorDepthTest());
						BatchElement.FirstIndex = 0;
						BatchElement.NumPrimitives = Section->IndexBuffer.Indices.Num() / 3;
						BatchElement.MinVertexIndex = 0;
						BatchElement.MaxVertexIndex = Section->VertexBuffers.PositionVertexBuffer.GetNumVertices() - 1;
						Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
						Mesh.Type = PT_TriangleList;
						Mesh.DepthPriorityGroup = SDPG_World;
						Mesh.bCanApplyViewModeOverrides = false;
						Collector.AddMesh(ViewIndex, Mesh);
					}
				}
			}
		}

		// Draw bounds
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				// Draw simple collision as wireframe if 'show collision', and collision is enabled, and we are not using the complex as the simple
				if (ViewFamily.EngineShowFlags.Collision && IsCollisionEnabled() && BodySetup->GetCollisionTraceFlag() != ECollisionTraceFlag::CTF_UseComplexAsSimple)
				{
					FTransform GeomTransform(GetLocalToWorld());
					BodySetup->AggGeom.GetAggGeom(GeomTransform, GetSelectionColor(FColor(157, 149, 223, 255), IsSelected(), IsHovered()).ToFColor(true), NULL, false, false, UseEditorDepthTest(), ViewIndex, Collector);
				}

				// Render bounds
				RenderBounds(Collector.GetPDI(ViewIndex), ViewFamily.EngineShowFlags, GetBounds(), IsSelected());
			}
		}
#endif
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bDynamicRelevance = true;
		Result.bRenderInMainPass = ShouldRenderInMainPass();
		Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
		Result.bRenderCustomDepth = ShouldRenderCustomDepth();
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		return Result;
	}

	virtual bool CanBeOccluded() const override
	{
		return !MaterialRelevance.bDisableDepthTest;
	}

	virtual uint32 GetMemoryFootprint(void) const
	{
		return(sizeof(*this) + GetAllocatedSize());
	}

	uint32 GetAllocatedSize(void) const
	{
		return(FPrimitiveSceneProxy::GetAllocatedSize());
	}

private:
	/** Array of sections */
	TArray<FProcTreeMeshProxySection*> Sections;

	UBodySetup* BodySetup;

	FMaterialRelevance MaterialRelevance;
};

//////////////////////////////////////////////////////////////////////////


UProceduralTreeComponent::UProceduralTreeComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, MeshBodySetup(nullptr)
	, bEnableCollision(false)
{

}

void UProceduralTreeComponent::PostLoad()
{
	Super::PostLoad();

	if (MeshBodySetup && IsTemplate())
	{
		MeshBodySetup->SetFlags(RF_Public);
	}


}

void UProceduralTreeComponent::OnRegister()
{
	Super::OnRegister();
	GenerateTreeMesh();
}

#if WITH_EDITOR
void UProceduralTreeComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	GenerateTreeMesh();
}
#endif //WITH_EDITOR

void UProceduralTreeComponent::GenerateTreeMesh()
{
	SCOPE_CYCLE_COUNTER(STAT_ProceduralTreeMesh_CreateMeshSection);

	Proctree::Tree TempTree;



	{
		TempTree.mProperties.mSeed = Props.Seed;
		TempTree.mProperties.mSegments = Props.HalfSegments * 2;
		TempTree.mProperties.mLevels = Props.Levels;
		TempTree.mProperties.mVMultiplier = Props.VMultiplier;
		TempTree.mProperties.mTwigScale = Props.TwigScale;
		TempTree.mProperties.mInitialBranchLength = Props.InitialBranchLength;
		TempTree.mProperties.mLengthFalloffFactor = Props.LengthFalloffFactor;
		TempTree.mProperties.mLengthFalloffPower = Props.LengthFalloffPower;
		TempTree.mProperties.mClumpMax = Props.ClumpMax;
		TempTree.mProperties.mClumpMin = Props.ClumpMin;
		TempTree.mProperties.mBranchFactor = Props.BranchFactor;
		TempTree.mProperties.mDropAmount = Props.DropAmount;
		TempTree.mProperties.mGrowAmount = Props.GrowAmount;
		TempTree.mProperties.mSweepAmount = Props.SweepAmount;
		TempTree.mProperties.mMaxRadius = Props.MaxRadius;
		TempTree.mProperties.mClimbRate = Props.ClimbRate;
		TempTree.mProperties.mTrunkKink = Props.TrunkKink;
		TempTree.mProperties.mTreeSteps = Props.TreeSteps;
		TempTree.mProperties.mTaperRate = Props.TaperRate;
		TempTree.mProperties.mRadiusFalloffRate = Props.RadiusFalloffRate;
		TempTree.mProperties.mTwistRate = Props.TwistRate;
		TempTree.mProperties.mTrunkLength = Props.TrunkLength;
	}

	TempTree.generate();

	if (TreeMeshSections.Num() != 2)
	{
		TreeMeshSections.SetNumZeroed(2);
	}

	TreeMeshSections[0].Reset();
	TreeMeshSections[1].Reset();

	TreeMeshSections[0].bEnableCollision = bEnableCollision;
	TreeMeshSections[1].bEnableCollision = bEnableCollision;



	for (int32 SectionIdex = 0; SectionIdex < 2; SectionIdex++)
	{


		int32 VertNum = (SectionIdex == 0) ? TempTree.mVertCount : TempTree.mTwigVertCount;
		int32 FacesNum = (SectionIdex == 0) ? TempTree.mFaceCount : TempTree.mTwigFaceCount;

		Proctree::fvec3 *Verts = (SectionIdex == 0) ? TempTree.mVert : TempTree.mTwigVert;
		Proctree::fvec3 *Norms = (SectionIdex == 0) ? TempTree.mNormal : TempTree.mTwigNormal;
		Proctree::fvec2 *UVs = (SectionIdex == 0) ? TempTree.mUV : TempTree.mTwigUV;
		Proctree::ivec3 *Faces = (SectionIdex == 0) ? TempTree.mFace : TempTree.mTwigFace;


		for (int32 VertIdx = 0; VertIdx < VertNum; VertIdx++)
		{
			TreeMeshSections[SectionIdex].Vertices.Add(FVector(Verts[VertIdx].x, Verts[VertIdx].z, Verts[VertIdx].y) * 100.0f);
			TreeMeshSections[SectionIdex].Normals.Add(FVector(Norms[VertIdx].x, Norms[VertIdx].z, Norms[VertIdx].y));
			TreeMeshSections[SectionIdex].TextureCoordinates0.Add(FVector2D(UVs[VertIdx].u, UVs[VertIdx].v));
			// Update bounding box
			TreeMeshSections[SectionIdex].SectionLocalBox += TreeMeshSections[SectionIdex].Vertices[VertIdx];
		}

		for (int32 FaceIdx = 0; FaceIdx < FacesNum; FaceIdx++)
		{
			TreeMeshSections[SectionIdex].IndexBuffer.Add(Faces[FaceIdx].x);
			TreeMeshSections[SectionIdex].IndexBuffer.Add(Faces[FaceIdx].y);
			TreeMeshSections[SectionIdex].IndexBuffer.Add(Faces[FaceIdx].z);
		}

	}

	UpdateLocalBounds(); // Update overall bounds
	UpdateCollision(); // Mark collision as dirty
	MarkRenderStateDirty(); // New section requires recreating scene proxy
}


void UProceduralTreeComponent::UpdateLocalBounds()
{
	FBox LocalBox(ForceInit);

	for (const FProcTreeMeshSection& Section : TreeMeshSections)
	{
		LocalBox += Section.SectionLocalBox;
	}

	LocalBounds = LocalBox.IsValid ? FBoxSphereBounds(LocalBox) : FBoxSphereBounds(FVector(0, 0, 0), FVector(0, 0, 0), 0); // fallback to reset box sphere bounds

	// Update global bounds
	UpdateBounds();
	// Need to send to render thread
	MarkRenderTransformDirty();
}

FPrimitiveSceneProxy* UProceduralTreeComponent::CreateSceneProxy()
{
	SCOPE_CYCLE_COUNTER(STAT_ProceduralTreeMesh_CreateSceneProxy);

	return new FProcTreeMeshSceneProxy(this);
}

int32 UProceduralTreeComponent::GetNumMaterials() const
{
	return TreeMeshSections.Num();
}

FBoxSphereBounds UProceduralTreeComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBoxSphereBounds Ret(LocalBounds.TransformBy(LocalToWorld));

	Ret.BoxExtent *= BoundsScale;
	Ret.SphereRadius *= BoundsScale;

	return Ret;
}

bool UProceduralTreeComponent::GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData)
{
	int32 VertexBase = 0; // Base vertex index for current section

	// See if we should copy UVs
	bool bCopyUVs = UPhysicsSettings::Get()->bSupportUVFromHitResults; 
	if (bCopyUVs)
	{
		CollisionData->UVs.AddZeroed(1); // only one UV channel
	}

	// For each section..
	for (int32 SectionIdx = 0; SectionIdx < TreeMeshSections.Num(); SectionIdx++)
	{
		FProcTreeMeshSection& Section = TreeMeshSections[SectionIdx];
		// Do we have collision enabled?
		if (Section.bEnableCollision)
		{
			// Copy vert data
			for (int32 VertIdx = 0; VertIdx < Section.Vertices.Num(); VertIdx++)
			{
				CollisionData->Vertices.Add(Section.Vertices[VertIdx]);

				// Copy UV if desired
				if (bCopyUVs)
				{
					CollisionData->UVs[0].Add(Section.TextureCoordinates0[VertIdx]);
				}
			}

			// Copy triangle data
			const int32 NumTriangles = Section.IndexBuffer.Num() / 3;
			for (int32 TriIdx = 0; TriIdx < NumTriangles; TriIdx++)
			{
				// Need to add base offset for indices
				FTriIndices Triangle;
				Triangle.v0 = Section.IndexBuffer[(TriIdx * 3) + 0] + VertexBase;
				Triangle.v1 = Section.IndexBuffer[(TriIdx * 3) + 1] + VertexBase;
				Triangle.v2 = Section.IndexBuffer[(TriIdx * 3) + 2] + VertexBase;
				CollisionData->Indices.Add(Triangle);

				// Also store material info
				CollisionData->MaterialIndices.Add(SectionIdx);
			}

			// Remember the base index that new verts will be added from in next section
			VertexBase = CollisionData->Vertices.Num();
		}
	}

	CollisionData->bFlipNormals = true;
	CollisionData->bDeformableMesh = true;
	CollisionData->bFastCook = true;

	return true;
}

bool UProceduralTreeComponent::ContainsPhysicsTriMeshData(bool InUseAllTriData) const
{
	for (const FProcTreeMeshSection& Section : TreeMeshSections)
	{
		if (Section.IndexBuffer.Num() >= 3 && Section.bEnableCollision)
		{
			return true;
		}
	}

	return false;
}

UBodySetup* UProceduralTreeComponent::CreateBodySetupHelper()
{
	// The body setup in a template needs to be public since the property is Tnstanced and thus is the archetype of the instance meaning there is a direct reference
	UBodySetup* NewBodySetup = NewObject<UBodySetup>(this, NAME_None, (IsTemplate() ? RF_Public : RF_NoFlags));
	NewBodySetup->BodySetupGuid = FGuid::NewGuid();

	NewBodySetup->bGenerateMirroredCollision = true;
	NewBodySetup->bDoubleSidedGeometry = true;
	NewBodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;

	return NewBodySetup;
}

void UProceduralTreeComponent::CreateTreeBodySetup()
{
	if (MeshBodySetup == nullptr)
	{
		MeshBodySetup = CreateBodySetupHelper();
	}
}

void UProceduralTreeComponent::UpdateCollision()
{
	SCOPE_CYCLE_COUNTER(STAT_ProceduralTreeMesh_UpdateCollision);

	CreateTreeBodySetup();
	// Set trace flag
	MeshBodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;


	// New GUID as collision has changed
	MeshBodySetup->BodySetupGuid = FGuid::NewGuid();
	// Also we want cooked data for this
	MeshBodySetup->bHasCookedCollisionData = true;
	MeshBodySetup->InvalidatePhysicsData();
	MeshBodySetup->CreatePhysicsMeshes();
	RecreatePhysicsState();

}

UBodySetup* UProceduralTreeComponent::GetBodySetup()
{
	CreateTreeBodySetup();
	return MeshBodySetup;
}

UMaterialInterface* UProceduralTreeComponent::GetMaterialFromCollisionFaceIndex(int32 FaceIndex, int32& SectionIndex) const
{
	UMaterialInterface* Result = nullptr;
	SectionIndex = 0;

	if (FaceIndex >= 0)
	{
		// Look for element that corresponds to the supplied face
		int32 TotalFaceCount = 0;
		for (int32 SectionIdx = 0; SectionIdx < TreeMeshSections.Num(); SectionIdx++)
		{
			const FProcTreeMeshSection& Section = TreeMeshSections[SectionIdx];
			int32 NumFaces = Section.IndexBuffer.Num() / 3;
			TotalFaceCount += NumFaces;

			if (FaceIndex < TotalFaceCount)
			{
				// Grab the material
				Result = GetMaterial(SectionIdx);
				SectionIndex = SectionIdx;
				break;
			}
		}
	}

	return Result;
}