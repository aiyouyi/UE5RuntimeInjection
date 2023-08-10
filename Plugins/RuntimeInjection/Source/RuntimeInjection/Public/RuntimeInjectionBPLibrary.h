// Created by wangpeimin@corp.netease.com

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "RuntimeInjectionBPLibrary.generated.h"





USTRUCT(BlueprintType)
struct FRIProperty
{
	GENERATED_USTRUCT_BODY()

	FRIProperty()
	{
		PropertyName = "";
		Type = "";
		ValueString = "";
	}

	UPROPERTY(BlueprintReadWrite)
	FString PropertyName;

	UPROPERTY(BlueprintReadWrite)
	FString Type;

	UPROPERTY(BlueprintReadWrite)
	FString ValueString;
};

USTRUCT(BlueprintType)
struct FRIFunction
{
	GENERATED_USTRUCT_BODY()

	FRIFunction()
	{
		FunctionName = "";
	}

	UPROPERTY(BlueprintReadWrite)
	FString FunctionName;
};



UCLASS()
class URuntimeInjectionBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Execute Sample function", Keywords = "RuntimeInjection sample test testing"), Category = "RuntimeInjectionTesting")
	static float RuntimeInjectionSampleFunction(float Param);

	UFUNCTION(BlueprintCallable, Category = "URuntimeInjectionBPLibrary", meta = (WorldContext = "WorldContextObject", DynamicOutputParam = "OutActors"))
	static void GetAllActors(const UObject* WorldContextObject, TArray<AActor*>& OutActors);

	UFUNCTION(BlueprintCallable, Category = "URuntimeInjectionBPLibrary", meta = (WorldContext = "WorldContextObject"))
	static AActor* GetActorByName(const UObject* WorldContextObject, const FString& Name);

	UFUNCTION(BlueprintCallable, Category = "URuntimeInjectionBPLibrary", meta = (WorldContext = "WorldContextObject", DynamicOutputParam = "RIPropertys"))
	static void GetActorPropertyByName(const UObject* WorldContextObject, const FString& Name, TArray<FRIProperty>& RIPropertys);

	UFUNCTION(BlueprintCallable, Category = "URuntimeInjectionBPLibrary", meta = (WorldContext = "WorldContextObject", DynamicOutputParam = "RIPropertys"))
	static void GetActorProperty(const UObject* WorldContextObject, const AActor* Actor, TArray<FRIProperty>& RIPropertys);

	UFUNCTION(BlueprintCallable, Category = "URuntimeInjectionBPLibrary", meta = (WorldContext = "WorldContextObject"))
	static UObject* GetActorPropertyObjectByName(const UObject* WorldContextObject, const UObject* Object, const FString& PropertyName); 

	/*
	* Only base type are supported! bool,int,float,etc
	* 仅支持基本类型参数
	*/
	UFUNCTION(BlueprintCallable, Category = "URuntimeInjectionBPLibrary", meta = (WorldContext = "WorldContextObject"))
	static bool SetObjectProperty(const UObject* WorldContextObject, UObject* Object, const FString& Name, const FString& Value);

	UFUNCTION(BlueprintCallable, Category = "URuntimeInjectionBPLibrary", meta = (WorldContext = "WorldContextObject", DynamicOutputParam = "RIFunctions"))
	static void GetActorFunctionByName(const UObject* WorldContextObject, const FString& Name, TArray<FRIFunction>& RIFunctions);

	UFUNCTION(BlueprintCallable, Category = "URuntimeInjectionBPLibrary", meta = (WorldContext = "WorldContextObject", DynamicOutputParam = "RIFunctions"))
	static void GetActorFunction(const UObject* WorldContextObject, const AActor* Actor, TArray<FRIFunction>& RIFunctions);

	/* 
	* Only base type are supported! bool,int,float,etc
	* 仅支持基本类型参数
	*/
	UFUNCTION(BlueprintCallable, Category = "URuntimeInjectionBPLibrary", meta = (WorldContext = "WorldContextObject"))
	static void CallFunctionByName(const UObject* WorldContextObject, UObject* Object, const FString& Name, const TArray<FString>& Param);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "GetAllMapNames", Keywords = "GAMNMaps"), Category = "MapNames")
	static  TArray<FString> GetAllMapNames();

	UFUNCTION(BlueprintCallable, Category = "URuntimeInjectionBPLibrary", meta = (WorldContext = "WorldContextObject"))
	static TArray<FString> GetAllLevelNames(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "URuntimeInjectionBPLibrary", meta = (WorldContext = "WorldContextObject"))
	static void CallComponentFunctionByName(const UObject* WorldContextObject, AActor* Actor, const FString& ComponentName, const FString& Name, const TArray<FString>& Param);
};
