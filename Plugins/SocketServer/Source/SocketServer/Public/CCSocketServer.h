// Created by wangpeimin@corp.netease.com

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HAL/Runnable.h"
#include "TimerManager.h"
#include "Runtime/Networking/Public/Networking.h"
#include "CCSocketServer.generated.h"





USTRUCT(BlueprintType)
struct FCCSocketConnection
{
	GENERATED_USTRUCT_BODY()

	FSocket* Socket;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString Message;
};

typedef void(*ReceiveMessage)(FCCSocketConnection CConnection);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReceiveMessageEvent, FCCSocketConnection, CConnection);


class SOCKETSERVER_API UCCSocketServerRunnable : public FRunnable
{
public:
	UCCSocketServerRunnable();
	~UCCSocketServerRunnable();

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;

	FSocket* ListenerSocket;

	bool bOwnerEndPlay;

	TArray<FSocket*> RemoteConnection;
	TQueue<FCCSocketConnection> ReceiveData;
	TQueue<FCCSocketConnection> ToSendData;

	FReceiveMessageEvent ReceiveMessageEvent;
	TArray<ReceiveMessage> ReceiveMessageRawEveent;
	FTimerHandle Timer;
	void ProcessReceiveMessage();

	static UCCSocketServerRunnable* GlobalSocketServerRunnable();

	bool bGameIsStart;
	void InitSocket();
	class AGameModeBase* CurrentGameMode;

	TSharedPtr<FConfigFile> ConfigFile;
};

extern UCCSocketServerRunnable* GSocketServerRunnable = nullptr;
extern FRunnableThread* GRunnableThread = nullptr;
extern TArray<ReceiveMessage>* GReceiveMessageRawEveent = nullptr;