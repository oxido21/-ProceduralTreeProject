// Copyright 2018 Elhoussine Mehnik (Mhousse1247). All Rights Reserved.
//******************* http://ue4resources.com/ *********************//

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#include "Components/MeshComponent.h"
#include "TreeMeshComponent.generated.h"

class FPrimitiveSceneProxy;



USTRUCT(meta = (ShowOnlyInnerProperties))
struct PROCEDURALTREE_API FProcTreeGenProperties
{

	GENERATED_USTRUCT_BODY()
public:

	static const FProcTreeGenProperties Min;

	static const FProcTreeGenProperties Max;

	UPROPERTY(EditAnywhere, DisplayName = "Random seed", Category = "General", meta = (ClampMin = "0", UIMin = "0"))
		int32 Seed;

	UPROPERTY(EditAnywhere, DisplayName = "Branch Half Segments", Category = General, meta = (ClampMin = "1", ClampMax = "16", UIMin = "1", UIMax = "16"))
		int32 HalfSegments;

	UPROPERTY(EditAnywhere, DisplayName = "Branch levels", Category = General, meta = (ClampMin = "1", ClampMax = "10", UIMin = "1", UIMax = "10"))
		int32 Levels;

	UPROPERTY(EditAnywhere, DisplayName = "Trunk forks", Category = General, meta = (ClampMin = "0", ClampMax = "32", UIMin = "0", UIMax = "32"))
		int32 TreeSteps;

	UPROPERTY(EditAnywhere, DisplayName = "Texture V multiplier", Category = General, meta = (ClampMin = "0.05", ClampMax = "10.0", UIMin = "0.05", UIMax = "10.0"))
		float VMultiplier;

	UPROPERTY(EditAnywhere, DisplayName = "Twig scale", Category = General, meta = (ClampMin = "0.05", ClampMax = "2.0", UIMin = "0.05", UIMax = "2.0"))
		float TwigScale;

	UPROPERTY(EditAnywhere, DisplayName = "Initial length", Category = Branching, meta = (ClampMin = "0.05", ClampMax = "5.0", UIMin = "0.05", UIMax = "5.0"))
		float InitialBranchLength;

	UPROPERTY(EditAnywhere, DisplayName = "Length falloff rate", Category = Branching, meta = (ClampMin = "0.05", ClampMax = "1.5", UIMin = "0.05", UIMax = "1.5"))
		float LengthFalloffFactor;

	UPROPERTY(EditAnywhere, DisplayName = "Length falloff power", Category = Branching, meta = (ClampMin = "-2.0", ClampMax = "2.0", UIMin = "-2.0", UIMax = "2.0"))
		float LengthFalloffPower;

	UPROPERTY(EditAnywhere, DisplayName = "Min clumping", Category = Branching, meta = (ClampMin = "0.05", ClampMax = "10.0", UIMin = "0.05", UIMax = "10.0"))
		float ClumpMin;

	UPROPERTY(EditAnywhere, DisplayName = "Max clumping", Category = Branching, meta = (ClampMin = "0.05", ClampMax = "10.0", UIMin = "0.05", UIMax = "10.0"))
		float ClumpMax;

	UPROPERTY(EditAnywhere, DisplayName = "Symmetry", Category = Branching, meta = (ClampMin = "2.0", ClampMax = "4.0", UIMin = "2.0", UIMax = "4.0"))
		float BranchFactor;

	UPROPERTY(EditAnywhere, DisplayName = "Droop", Category = Branching, meta = (ClampMin = "-2.0", ClampMax = "2.0", UIMin = "-2.0", UIMax = "2.0"))
		float DropAmount;

	UPROPERTY(EditAnywhere, DisplayName = "Growth", Category = Branching, meta = (ClampMin = "-4.0", ClampMax = "4.0", UIMin = "-4.0", UIMax = "4.0"))
		float GrowAmount;

	UPROPERTY(EditAnywhere, DisplayName = "Sweep", Category = Branching, meta = (ClampMin = "-1.0", ClampMax = "1.0", UIMin = "-1.0", UIMax = "1.0"))
		float SweepAmount;

	UPROPERTY(EditAnywhere, DisplayName = "Trunk radius", Category = Trunk, meta = (ClampMin = "0.05", ClampMax = "0.5", UIMin = "0.05", UIMax = "0.5"))
		float MaxRadius;

	UPROPERTY(EditAnywhere, DisplayName = "Radius falloff", Category = Trunk, meta = (ClampMin = "0.1", ClampMax = "1.0", UIMin = "0.1", UIMax = "1.0"))
		float RadiusFalloffRate;

	UPROPERTY(EditAnywhere, DisplayName = "Climb rate", Category = Trunk, meta = (ClampMin = "0.05", ClampMax = "1.0", UIMin = "0.05", UIMax = "1.0"))
		float ClimbRate;

	UPROPERTY(EditAnywhere, DisplayName = "Kink", Category = Trunk, meta = (ClampMin = "-2.0", ClampMax = "2.0", UIMin = "-2.0", UIMax = "2.0"))
		float TrunkKink;

	UPROPERTY(EditAnywhere, DisplayName = "Taper rate", Category = Trunk, meta = (ClampMin = "0.5", ClampMax = "2.0", UIMin = "0.5", UIMax = "2.0"))
		float TaperRate;

	UPROPERTY(EditAnywhere, DisplayName = "Twists", Category = Trunk, meta = (ClampMin = "0.05", ClampMax = "10.0", UIMin = "0.05", UIMax = "10.0"))
		float TwistRate;

	UPROPERTY(EditAnywhere, DisplayName = "Trunk length", Category = Trunk, meta = (ClampMin = "0.05", ClampMax = "5.0", UIMin = "0.05", UIMax = "5.0"))
		float TrunkLength;

	FProcTreeGenProperties(
		float aClumpMax,
		float aClumpMin,
		float aLengthFalloffFactor,
		float aLengthFalloffPower,
		float aBranchFactor,
		float aRadiusFalloffRate,
		float aClimbRate,
		float aTrunkKink,
		float aMaxRadius,
		int aTreeSteps,
		float aTaperRate,
		float aTwistRate,
		int aSegments,
		int aLevels,
		float aSweepAmount,
		float aInitialBranchLength,
		float aTrunkLength,
		float aDropAmount,
		float aGrowAmount,
		float aVMultiplier,
		float aTwigScale,
		int aSeed)
	{
		Seed = aSeed;
		HalfSegments = aSegments;
		Levels = aLevels;
		VMultiplier = aVMultiplier;
		TwigScale = aTwigScale;
		InitialBranchLength = aInitialBranchLength;
		LengthFalloffFactor = aLengthFalloffFactor;
		LengthFalloffPower = aLengthFalloffPower;
		ClumpMax = aClumpMax;
		ClumpMin = aClumpMin;
		BranchFactor = aBranchFactor;
		DropAmount = aDropAmount;
		GrowAmount = aGrowAmount;
		SweepAmount = aSweepAmount;
		MaxRadius = aMaxRadius;
		ClimbRate = aClimbRate;
		TrunkKink = aTrunkKink;
		TreeSteps = aTreeSteps;
		TaperRate = aTaperRate;
		RadiusFalloffRate = aRadiusFalloffRate;
		TwistRate = aTwistRate;
		TrunkLength = aTrunkLength;
	}

	FProcTreeGenProperties()
	{
		Seed = 262;
		HalfSegments = 6;
		Levels = 5;
		VMultiplier = 0.36f;
		TwigScale = 0.39f;
		InitialBranchLength = 0.49f;
		LengthFalloffFactor = 0.85f;
		LengthFalloffPower = 0.99f;
		ClumpMax = 0.454f;
		ClumpMin = 0.404f;
		BranchFactor = 2.45f;
		DropAmount = -0.1f;
		GrowAmount = 0.235f;
		SweepAmount = 0.05f;
		MaxRadius = 0.139f;
		ClimbRate = 0.371f;
		TrunkKink = 0.093f;
		TreeSteps = 5;
		TaperRate = 0.947f;
		RadiusFalloffRate = 0.73f;
		TwistRate = 3.02f;
		TrunkLength = 2.4f;
	}
};


/**
*	Struct used to specify a tangent vector for a vertex
*	The Y tangent is computed from the cross product of the vertex normal (Tangent Z) and the TangentX member.
*/
USTRUCT()
struct FProcTreeMeshTangent
{
	GENERATED_USTRUCT_BODY()

		/** Direction of X tangent for this vertex */
		UPROPERTY()
		FVector TangentX;

	/** Bool that indicates whether we should flip the Y tangent when we compute it using cross product */
	UPROPERTY()
		bool bFlipTangentY;

	FProcTreeMeshTangent()
		: TangentX(1.f, 0.f, 0.f)
		, bFlipTangentY(false)
	{}

	FProcTreeMeshTangent(float X, float Y, float Z)
		: TangentX(X, Y, Z)
		, bFlipTangentY(false)
	{}

	FProcTreeMeshTangent(FVector InTangentX, bool bInFlipTangentY)
		: TangentX(InTangentX)
		, bFlipTangentY(bInFlipTangentY)
	{}
};

/** One section of the tree mesh. Each material has its own section. */
USTRUCT()
struct FProcTreeMeshSection
{
	GENERATED_USTRUCT_BODY()

		/** Vertex position */
		UPROPERTY()
		TArray<FVector> Vertices;

	/** Vertex normal */
	UPROPERTY()
		TArray<FVector> Normals;

	/** Vertex tangent */
	UPROPERTY()
		TArray<FProcTreeMeshTangent> Tangents;

	/** Vertex texture co-ordinate */
	UPROPERTY()
		TArray<FVector2D> TextureCoordinates0;

	/** Index buffer for this section */
	UPROPERTY()
		TArray<uint32> IndexBuffer;

	/** Local bounding box of section */
	UPROPERTY()
		FBox SectionLocalBox;

	/** Should we build collision data for triangles in this section */
	UPROPERTY()
		bool bEnableCollision;

	/** Should we display this section */
	UPROPERTY()
		bool bSectionVisible;

	FProcTreeMeshSection()
		: SectionLocalBox(ForceInit)
		, bEnableCollision(false)
		, bSectionVisible(true)
	{}

	/** Reset this section, clear all mesh info. */
	void Reset()
	{
		Vertices.Reset();
		Normals.Reset();
		Tangents.Reset();
		TextureCoordinates0.Reset();
		IndexBuffer.Reset();
		SectionLocalBox.Init();
		bEnableCollision = false;
		bSectionVisible = true;
	}
};


UCLASS(hidecategories = (Object, LOD, Physics), meta = (BlueprintSpawnableComponent), ClassGroup = Rendering)
class PROCEDURALTREE_API UProceduralTreeComponent : public UMeshComponent, public IInterface_CollisionDataProvider
{
	GENERATED_UCLASS_BODY()

		/** Array of sections of mesh */
		UPROPERTY()
		TArray<FProcTreeMeshSection> TreeMeshSections;


public:

	UPROPERTY(EditAnywhere, Category = "General")
		bool bEnableCollision;

	UPROPERTY(EditAnywhere, Category = ProceduralTree, meta = (ShowOnlyInnerProperties))
		FProcTreeGenProperties Props;

	void GenerateTreeMesh();


#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif //WITH_EDITOR

	//~ Begin Interface_CollisionDataProvider Interface
	virtual bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData) override;
	virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const override;
	virtual bool WantsNegXTriMesh() override{ return false; }
	//~ End Interface_CollisionDataProvider Interface

	/** Collision data */
	UPROPERTY(Instanced)
	class UBodySetup* MeshBodySetup;

	//~ Begin UPrimitiveComponent Interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual class UBodySetup* GetBodySetup() override;
	virtual UMaterialInterface* GetMaterialFromCollisionFaceIndex(int32 FaceIndex, int32& SectionIndex) const override;
	//~ End UPrimitiveComponent Interface.

	//~ Begin UMeshComponent Interface.
	virtual int32 GetNumMaterials() const override;
	//~ End UMeshComponent Interface.

	//~ Begin UObject Interface
	virtual void PostLoad() override;
	//~ End UObject Interface.

	//~ Begin UActorComponent Interface.
	virtual void OnRegister() override;
	//~ Begin UActorComponent Interface.

private:
	//~ Begin USceneComponent Interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ Begin USceneComponent Interface.


	/** Update LocalBounds member from the local box of each section */
	void UpdateLocalBounds();
	/** Ensure MeshBodySetup is allocated and configured */
	void CreateTreeBodySetup();
	/** Mark collision data as dirty, and re-create on instance if necessary */
	void UpdateCollision();

	/** Helper to create new body setup objects */
	UBodySetup* CreateBodySetupHelper();

	/** Local space bounds of mesh */
	UPROPERTY()
	FBoxSphereBounds LocalBounds;
	
	friend class FProcTreeMeshSceneProxy;
};


