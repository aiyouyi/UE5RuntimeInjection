// Created by wangpeimin@corp.netease.com

#pragma once

#include "CoreMinimal.h"
#include "CCSocketServer.h"
#include "GameFramework/Actor.h"


class RUNTIMEINJECTION_API UCCDirectorRunnable : public FRunnable
{
public:
	UCCDirectorRunnable();
	~UCCDirectorRunnable();

	bool bOwnerEndPlay;
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;

};

typedef void(*ReceivedMessageEvent)(const FCCSocketConnection& CCConnection, TSharedPtr<FJsonObject> Data);

class RUNTIMEINJECTION_API CCDirector
{
	
public:	
	CCDirector();

	~CCDirector();

	static void StaticOnReceivedMessage(FCCSocketConnection CCConnection);

	
	static TMap<FString,ReceivedMessageEvent> ReceivedMessageEvents;
	static void OnReceivedGetMaps(const FCCSocketConnection& CCConnection, TSharedPtr<FJsonObject> Data);
	static void OnReceivedLoadMap(const FCCSocketConnection& CCConnection, TSharedPtr<FJsonObject> Data);
	static void OnReceivedGetActors(const FCCSocketConnection& CCConnection, TSharedPtr<FJsonObject> Data);
	static void OnReceivedGetDetails(const FCCSocketConnection& CCConnection, TSharedPtr<FJsonObject> Data);

	static void OnReceivedSetProperty(const FCCSocketConnection& CCConnection, TSharedPtr<FJsonObject> Data);
	static void OnReceivedCallFunction(const FCCSocketConnection& CCConnection, TSharedPtr<FJsonObject> Data);

	static void OnReceivedSearchAssets(const FCCSocketConnection& CCConnection, TSharedPtr<FJsonObject> Data);

	// help funcion
	static bool CheckIfInEditorAndPIE();
	static void FindActors(TArray<AActor*>& Result);
	static UWorld* DefaultWorlContex();

	static void SetActorJsonDetail(AActor* Actor, TSharedPtr<FJsonObject> ActorJson);
	static void ActorDetailVisiable(AActor* Actor, TSharedPtr<FJsonObject> ActorJson);
	static void ActorDetailMobility(AActor* Actor, TSharedPtr<FJsonObject> ActorJson);
	static void ActorDetailText(AActor* Actor, TSharedPtr<FJsonObject> ActorJson);
	static void ActorDetailLevelSequence(AActor* Actor, TSharedPtr<FJsonObject> ActorJson);
	static void ObjectDetail(UObject* Object, TSharedPtr<FJsonObject> ActorJson);

	static FString ExPathName(const FString& PathName);
	static const FString EnumToString(const TCHAR* Enum, int32 EnumValue);
};

extern CCDirector* GDirector=nullptr;

extern UCCDirectorRunnable* GDirectorRunnable = nullptr;
extern FRunnableThread* GDirectorRunnableThread = nullptr;