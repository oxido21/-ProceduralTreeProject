// Copyright 2018 Elhoussine Mehnik (Mhousse1247). All Rights Reserved.
//******************* http://ue4resources.com/ *********************//

#include "TreeMeshComponentDetails.h"
#include "Modules/ModuleManager.h"
#include "Misc/PackageName.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Application/SlateWindowHelper.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Engine/StaticMesh.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "DetailCategoryBuilder.h"
#include "IDetailsView.h"
#include "TreeMeshComponent.h"
#include "RawMesh.h"

#include "Dialogs/DlgPickAssetPath.h"
#include "AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "FTreeMeshComponentDetails"

TSharedRef<IDetailCustomization> FTreeMeshComponentDetails::MakeInstance()
{
	return MakeShareable(new FTreeMeshComponentDetails);
}

void FTreeMeshComponentDetails::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	IDetailCategoryBuilder& TreeMeshCategory = DetailBuilder.EditCategory("General");

	const FText ConvertToStaticMeshText = LOCTEXT("ConvertToStaticMesh", "Create StaticMesh");

	// Cache set of selected things
	SelectedObjectsList = DetailBuilder.GetSelectedObjects();

	TreeMeshCategory.AddCustomRow(ConvertToStaticMeshText, false)
	.NameContent()
	[
		SNullWidget::NullWidget
	]
	.ValueContent()
	.VAlign(VAlign_Center)
	.MaxDesiredWidth(250)
	[
		SNew(SButton)
		.VAlign(VAlign_Center)
		.ToolTipText(LOCTEXT("ConvertToStaticMeshTooltip", "Create a new StaticMesh asset using current geometry from this TreeMeshComponent. Does not modify instance."))
		.OnClicked(this, &FTreeMeshComponentDetails::ClickedOnConvertToStaticMesh)
		.IsEnabled(this, &FTreeMeshComponentDetails::ConvertToStaticMeshEnabled)
		.Content()
		[
			SNew(STextBlock)
			.Text(ConvertToStaticMeshText)
		]
	];
}

UProceduralTreeComponent* FTreeMeshComponentDetails::GetFirstSelectedTreeMeshComp() const
{
	// Find first selected valid TreeMeshComp
	UProceduralTreeComponent* TreeMeshComp = nullptr;
	for (const TWeakObjectPtr<UObject>& Object : SelectedObjectsList)
	{
		UProceduralTreeComponent* TestProcComp = Cast<UProceduralTreeComponent>(Object.Get());
		// See if this one is good
		if (TestProcComp != nullptr && !TestProcComp->IsTemplate())
		{
			TreeMeshComp = TestProcComp;
			break;
		}
	}

	return TreeMeshComp;
}


bool FTreeMeshComponentDetails::ConvertToStaticMeshEnabled() const
{
	return GetFirstSelectedTreeMeshComp() != nullptr;
}


FReply FTreeMeshComponentDetails::ClickedOnConvertToStaticMesh()
{
	// Find first selected TreeMeshComp
	UProceduralTreeComponent* TreeMeshComp = GetFirstSelectedTreeMeshComp();
	if (TreeMeshComp != nullptr)
	{
		FString NewNameSuggestion = FString(TEXT("TreeMesh"));
		FString PackageName = FString(TEXT("/Game/Meshes/")) + NewNameSuggestion;
		FString Name;
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		AssetToolsModule.Get().CreateUniqueAssetName(PackageName, TEXT(""), PackageName, Name);

		TSharedPtr<SDlgPickAssetPath> PickAssetPathWidget =
			SNew(SDlgPickAssetPath)
			.Title(LOCTEXT("ConvertToStaticMeshPickName", "Choose New StaticMesh Location"))
			.DefaultAssetPath(FText::FromString(PackageName));

		if (PickAssetPathWidget->ShowModal() == EAppReturnType::Ok)
		{
			// Get the full name of where we want to create the physics asset.
			FString UserPackageName = PickAssetPathWidget->GetFullAssetPath().ToString();
			FName MeshName(*FPackageName::GetLongPackageAssetName(UserPackageName));

			// Check if the user inputed a valid asset name, if they did not, give it the generated default name
			if (MeshName == NAME_None)
			{
				// Use the defaults that were already generated.
				UserPackageName = PackageName;
				MeshName = *Name;
			}

			// Raw mesh data we are filling in
			FRawMesh RawMesh;
			// Materials to apply to new mesh
			TArray<UMaterialInterface*> MeshMaterials;

			const int32 NumSections = TreeMeshComp->TreeMeshSections.Num();
			int32 VertexBase = 0;
			for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
			{
				FProcTreeMeshSection* ProcSection = &TreeMeshComp->TreeMeshSections[SectionIdx];

				// Copy verts
				for (FVector& Vert : ProcSection->Vertices)
				{
					RawMesh.VertexPositions.Add(Vert);
				}

				// Copy 'wedge' info
				int32 NumIndices = ProcSection->IndexBuffer.Num();
				int32 NumVerts = ProcSection->Vertices.Num();


				for (int32 IndexIdx = 0; IndexIdx < NumIndices; IndexIdx++)
				{
					int32 Index = ProcSection->IndexBuffer[IndexIdx];

					RawMesh.WedgeIndices.Add(Index + VertexBase);


					const FVector TangentX = (ProcSection->Tangents.Num() == NumVerts) ? ProcSection->Tangents[Index].TangentX : FVector::ForwardVector;
					const FVector TangentZ = (ProcSection->Normals.Num() == NumVerts) ? ProcSection->Normals[Index] : FVector::UpVector;
					const FVector TangentY = (ProcSection->Tangents.Num() == NumVerts) ? (TangentX ^ TangentZ).GetSafeNormal() * (ProcSection->Tangents[Index].bFlipTangentY ? -1.f : 1.f) : FVector::RightVector;
					const FVector2D UV0 = (ProcSection->TextureCoordinates0.Num() == NumVerts) ? ProcSection->TextureCoordinates0[Index] : FVector2D::ZeroVector;


					RawMesh.WedgeTangentX.Add(TangentX);
					RawMesh.WedgeTangentY.Add(TangentY);
					RawMesh.WedgeTangentZ.Add(TangentZ);

					RawMesh.WedgeTexCoords[0].Add(UV0);

					RawMesh.WedgeColors.Add(FColor::White);
				}

				// copy face info
				int32 NumTris = NumIndices / 3;
				for (int32 TriIdx = 0; TriIdx < NumTris; TriIdx++)
				{
					RawMesh.FaceMaterialIndices.Add(SectionIdx);
					RawMesh.FaceSmoothingMasks.Add(0); // Assume this is ignored as bRecomputeNormals is false
				}

				// Remember material
				MeshMaterials.Add(TreeMeshComp->GetMaterial(SectionIdx));

				// Update offset for creating one big index/vertex buffer
				VertexBase += ProcSection->Vertices.Num();
			}

			// If we got some valid data.
			if (RawMesh.VertexPositions.Num() >= 3 && RawMesh.WedgeIndices.Num() >= 3)
			{
				// Then find/create it.
				UPackage* Package = CreatePackage(NULL, *UserPackageName);
				check(Package);

				// Create StaticMesh object
				UStaticMesh* StaticMesh = NewObject<UStaticMesh>(Package, MeshName, RF_Public | RF_Standalone);
				StaticMesh->InitResources();

				StaticMesh->LightingGuid = FGuid::NewGuid();

				// Add source to new StaticMesh
				FStaticMeshSourceModel& SrcModel = StaticMesh->AddSourceModel();
				SrcModel.BuildSettings.bRecomputeNormals = false;
				SrcModel.BuildSettings.bRecomputeTangents = false;
				SrcModel.BuildSettings.bRemoveDegenerates = false;
				SrcModel.BuildSettings.bUseHighPrecisionTangentBasis = false;
				SrcModel.BuildSettings.bUseFullPrecisionUVs = false;
				SrcModel.BuildSettings.bGenerateLightmapUVs = true;
				SrcModel.BuildSettings.SrcLightmapIndex = 0;
				SrcModel.BuildSettings.DstLightmapIndex = 1;
				SrcModel.SaveRawMesh(RawMesh);

				// Copy materials to new mesh
				for (UMaterialInterface* Material : MeshMaterials)
				{
					StaticMesh->StaticMaterials.Add(FStaticMaterial(Material));
				}

				//Set the Imported version before calling the build
				StaticMesh->ImportVersion = EImportStaticMeshVersion::LastVersion;

				// Build mesh from source
				StaticMesh->Build(false);
				StaticMesh->PostEditChange();

				// Notify asset registry of new asset
				FAssetRegistryModule::AssetCreated(StaticMesh);
			}
		}
	}

	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE
