// Copyright Epic Games, Inc. All Rights Reserved.

#include "SocketServer.h"
#include "CoreGlobals.h"
#include "CCSocketServer.h"
#include "HAL/Runnable.h"
#include "Misc\OutputDeviceConsole.h"

#define LOCTEXT_NAMESPACE "FSocketServerModule"



void FSocketServerModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	UCCSocketServerRunnable::GlobalSocketServerRunnable();
}

void FSocketServerModule::ShutdownModule()
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
	if (GSocketServerRunnable)
	{
		delete GSocketServerRunnable;
		GSocketServerRunnable = nullptr;
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSocketServerModule, SocketServer)