// Created by wangpeimin@corp.netease.com

#include "RuntimeInjectionBPLibrary.h"
#include "RuntimeInjection.h"
#include "GameFramework/Actor.h"
#include "Engine/LevelStreaming.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistryModule.h"
#include "IAssetRegistry.h"
#include "AssetRegistry/AssetRegistryHelpers.h"
#include "EngineUtils.h"
#include "Net/Core/PushModel/PushModel.h"
#include "UObject/Field.h"

URuntimeInjectionBPLibrary::URuntimeInjectionBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

float URuntimeInjectionBPLibrary::RuntimeInjectionSampleFunction(float Param)
{
	return -1;
}

void URuntimeInjectionBPLibrary::GetAllActors(const UObject* WorldContextObject, TArray<AActor*>& OutActors)
{
	OutActors.Reset();
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			OutActors.Add(Actor);
		}
	}
	
}


AActor* URuntimeInjectionBPLibrary::GetActorByName(const UObject* WorldContextObject, const FString& Name)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
#if WITH_EDITOR
			if (Actor->GetActorLabel()== Name)
#else
			if (Actor->GetPathName() == Name)
#endif
			{
				return Actor;
			}
		}
	}
	return nullptr;
}

void URuntimeInjectionBPLibrary::GetActorPropertyByName(const UObject* WorldContextObject, const FString& Name, TArray<FRIProperty>& RIPropertys)
{
	AActor* Actor = GetActorByName(WorldContextObject, Name);
	GetActorProperty(WorldContextObject, Actor, RIPropertys);
}

void URuntimeInjectionBPLibrary::GetActorProperty(const UObject* WorldContextObject, const AActor* Actor, TArray<FRIProperty>& RIPropertys)
{
	RIPropertys.Reset();
	if (Actor)
	{
		for (TFieldIterator<FProperty> It(Actor->GetClass()); It; ++It)
		{
			FProperty* Property = *It;
			FRIProperty RIProperty;
			RIProperty.PropertyName = Property->GetName();
			RIProperty.Type = Property->GetCPPType();

			const void* PropertyValuePtr = Property->ContainerPtrToValuePtr<void>(Actor);
			Property->ExportTextItem(RIProperty.ValueString, PropertyValuePtr, nullptr, nullptr, PPF_None);
			RIPropertys.Add(RIProperty);
		}
	}
}

UObject* URuntimeInjectionBPLibrary::GetActorPropertyObjectByName(const UObject* WorldContextObject, const UObject* Object, const FString& PropertyName)
{
	UObject* Result = nullptr;
	if (Object && Object->GetClass())
	{
		FObjectProperty* Property = Cast<FObjectProperty>(Object->GetClass()->FindPropertyByName(*PropertyName));
		Result = Property ? Property->GetObjectPropertyValue_InContainer(Object) : nullptr;
	}
	return Result;
}

bool URuntimeInjectionBPLibrary::SetObjectProperty(const UObject* WorldContextObject, UObject* Object, const FString& Name, const FString& Value)
{
	if (Name==TEXT("Location"))
	{
		TArray<FString> Param;
		Param.Add(Value); Param.Add(TEXT("False")); Param.Add(TEXT("False")); Param.Add(TEXT("{}")); Param.Add(TEXT("False"));
		CallFunctionByName(WorldContextObject, Object, TEXT("K2_SetActorLocation"), Param);
		return true;
	}
	else if (Name == TEXT("Rotation"))
	{
		TArray<FString> Param;
		Param.Add(Value); Param.Add(TEXT("True")); Param.Add(TEXT("")); Param.Add(TEXT("False"));
		CallFunctionByName(WorldContextObject, Object, TEXT("K2_SetActorRotation"), Param);
		return true;
	}
	else if (Name == TEXT("Scale"))
	{
		TArray<FString> Param;
		Param.Add(Value); Param.Add(TEXT("False")); Param.Add(TEXT("False")); Param.Add(TEXT("{}")); Param.Add(TEXT("False"));
		CallFunctionByName(WorldContextObject, Object, TEXT("SetActorScale3D"), Param);
		return true;
	}
	else if (Name == TEXT("IsHidden"))
	{
		TArray<FString> Param;
		Param.Add(Value);
		CallFunctionByName(WorldContextObject, Object, TEXT("SetActorHiddenInGame"), Param);
		return true;
	}
	else if (Name == TEXT("Text"))
	{
		TArray<FString> Param;
		Param.Add(Value);

		UFunction* Function = Object ? Object->FindFunction("SetText") : nullptr;
		if (Function)
		{
			CallFunctionByName(WorldContextObject, Object, TEXT("SetText"), Param);
			return true;
		}
		Function = Object ? Object->FindFunction("K2_SetText") : nullptr;
		if (Function)
		{
			CallFunctionByName(WorldContextObject, Object, TEXT("K2_SetText"), Param);
			return true;
		}
	}
	FProperty* Property = nullptr;
	if (Object)
	{
		for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
		{
			if (It->GetName()== Name)
			{
				Property = *It;
				break;
			}
		}
	}
	if (Property)
	{
		Object->Modify();
#if WITH_EDITOR
		Object->PreEditChange(Property);
#endif
		Property->ImportText_Direct(*Value, Property->ContainerPtrToValuePtr<void*>(Object), Object, PPF_None );
		FPropertyChangedEvent PropertyEvent(Property);
#if WITH_EDITOR
		Object->PostEditChangeProperty(PropertyEvent);
#endif
	}
	return Property != nullptr;
}

void URuntimeInjectionBPLibrary::GetActorFunctionByName(const UObject* WorldContextObject, const FString& Name, TArray<FRIFunction>& RIFunctions)
{
	AActor* Actor = GetActorByName(WorldContextObject, Name);
	GetActorFunction(WorldContextObject, Actor, RIFunctions);
}

void URuntimeInjectionBPLibrary::GetActorFunction(const UObject* WorldContextObject, const AActor* Actor, TArray<FRIFunction>& RIFunctions)
{
	RIFunctions.Reset();
	if (Actor)
	{
		for (TFieldIterator<UFunction> It(Actor->GetClass()); It; ++It)
		{
			FRIFunction RIFunction;
			UFunction* Function = *It;
			RIFunction.FunctionName = Function->GetName();

			for (TFieldIterator<FProperty> PropertyIt(Function); PropertyIt; ++PropertyIt)
			{
				FProperty* Property = *PropertyIt;
				if (Property->PropertyFlags & CPF_ReturnParm)
				{

				}
			}
			RIFunctions.Add(RIFunction);
		}
	}
}

void URuntimeInjectionBPLibrary::CallFunctionByName(const UObject* WorldContextObject, UObject* Object, const FString& Name, const TArray<FString>& Param)
{
	UFunction* Function = Object ? Object->FindFunction(*Name) : nullptr;
	if (Function)
	{
		int32 InParamSize = 0;
		int32 OutParamSize = 0;
		for (TFieldIterator<FProperty> It(Function); It; ++It)
		{
			FProperty* Property = *It;
			if (Property->GetFName().ToString().StartsWith("__"))
			{
				continue;
			}
			if (Property->PropertyFlags & CPF_OutParm)
			{
				OutParamSize += Property->GetSize();
			}
			else if (Property->PropertyFlags & CPF_Parm)
			{
				InParamSize += Property->GetSize();
			}
		}
		void* PramsBuffer = (uint8*)FMemory_Alloca(InParamSize+ OutParamSize);
		void* OutPramsBuffer = (uint8*)FMemory_Alloca(OutParamSize);

		int32 Index = 0;
		for (TFieldIterator<FProperty> It(Function); It; ++It)
		{
			FProperty* Property = *It;
			if (Property->GetFName().ToString().StartsWith("__"))
			{
				continue;
			}
			if (Property->PropertyFlags & CPF_Parm)
			{
				if (Index >= Param.Num())
				{
					return;
				}
				FString ValueStr = Param[Index];
				Index++;

				if (Property->GetCPPType() == "FString")
				{
					FString* ValuePtr = Property->ContainerPtrToValuePtr<FString>(PramsBuffer);
					ValuePtr ? new(ValuePtr)FString() : nullptr;
					ValuePtr ? (*ValuePtr = ValueStr) : FString();
				}
				else if(PramsBuffer)
				{
					Property->ImportText(*ValueStr, Property->ContainerPtrToValuePtr<void*>(PramsBuffer), PPF_None, nullptr);
				}

			}
		}
		FEditorScriptExecutionGuard ScriptGuard;
		Object->ProcessEvent(Function, PramsBuffer);		
	}
}

TArray<FString> URuntimeInjectionBPLibrary::GetAllMapNames()
{
	return GetAllLevelNames(nullptr);
	//TArray<FString> MapFiles;
	//IFileManager::Get().FindFilesRecursive(MapFiles, *FPaths::ProjectContentDir(), TEXT("*.umap"), true, false, false);
	//for (int32 i = 0; i < MapFiles.Num(); i++)
	//{
	//	int32 LastSlashIndex = -1;
	//	if (MapFiles[i].FindLastChar('/', LastSlashIndex))
	//	{
	//		FString MapName;
	//		//length - 5 because of the ".umap" suffix
	//		for (int32 j = LastSlashIndex + 1; j < MapFiles[i].Len() - 5; j++)
	//		{
	//			MapName.AppendChar(MapFiles[i][j]);
	//		}
	//		MapFiles[i] = MapName;
	//	}
	//}
	//return MapFiles;
}

TArray<FString> URuntimeInjectionBPLibrary::GetAllLevelNames(const UObject* WorldContextObject)
{
	TArray<FString> MapFiles;
	TArray<FAssetData> OutAssetData;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.GetAllAssets(OutAssetData);
	//UAssetRegistryHelpers::GetAssetRegistry()->GetAllAssets(OutAssetData);
	for (auto& Data: OutAssetData)
	{
		if (Data.AssetClassPath.GetAssetName()==FName("World") && Data.PackagePath.ToString().StartsWith("/Game"))
		{
			MapFiles.AddUnique(Data.AssetName.ToString());
		}
	}
	return MapFiles;
}

void URuntimeInjectionBPLibrary::CallComponentFunctionByName(const UObject* WorldContextObject, AActor* Actor, const FString& ComponentName, const FString& Name, const TArray<FString>& Param)
{
	UActorComponent* FoundComponent = nullptr;

	if (Actor)
	{
		for (UActorComponent* Component : Actor->GetComponents())
		{
			if (Component && Component->GetName()== ComponentName)
			{
				FoundComponent = Component;
				CallFunctionByName(WorldContextObject, FoundComponent, Name, Param);
				break;
			}
		}
	}
}

