// Copyright Epic Games, Inc. All Rights Reserved.

#include "RuntimeInjection.h"
#include "DirectorUEActor.h"


#define LOCTEXT_NAMESPACE "FRuntimeInjectionModule"

void FRuntimeInjectionModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	if (!GDirector)
	{
		GDirector = new CCDirector();
	}
}

void FRuntimeInjectionModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	if (GSocketServerRunnable)
	{
		GSocketServerRunnable->bOwnerEndPlay = true;
		while (GSocketServerRunnable->bOwnerEndPlay)
		{
			FPlatformProcess::Sleep(0.01);
		}
	}
	
	if (GDirector)
	{
		delete GDirector;
		GDirector = nullptr;
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRuntimeInjectionModule, RuntimeInjection)