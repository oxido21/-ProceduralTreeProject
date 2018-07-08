// Copyright 2018 Elhoussine Mehnik (Mhousse1247). All Rights Reserved.
//******************* http://ue4resources.com/ *********************//


#include "ProceduralTreeModule.h"



class FProceduralTreeModule : public IProceduralTreeModule
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE( FProceduralTreeModule, ProceduralTree)



void FProceduralTreeModule::StartupModule()
{
	
}


void FProceduralTreeModule::ShutdownModule()
{
	
}



