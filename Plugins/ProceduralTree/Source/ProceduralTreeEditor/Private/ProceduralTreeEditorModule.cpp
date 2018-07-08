// Copyright 2018 Elhoussine Mehnik (Mhousse1247). All Rights Reserved.
//******************* http://ue4resources.com/ *********************//

#include "ProceduralTreeEditorModule.h"
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "TreeMeshComponent.h"

#include "TreeMeshComponentDetails.h"




class FProceduralTreeEditorModule : public IProceduralTreeEditorModule
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE( FProceduralTreeEditorModule, ProceduralTreeEditor)



void FProceduralTreeEditorModule::StartupModule()
{
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomClassLayout(UProceduralTreeComponent::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FTreeMeshComponentDetails::MakeInstance));
	}
}


void FProceduralTreeEditorModule::ShutdownModule()
{
	
}



