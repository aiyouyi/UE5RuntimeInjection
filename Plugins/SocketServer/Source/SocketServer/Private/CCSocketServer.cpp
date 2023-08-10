// Created by wangpeimin@corp.netease.com

#include "CCSocketServer.h"
#include "GameFramework/GameModeBase.h"
#include <string>
#include <xstring>
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include "Windows/HideWindowsPlatformTypes.h"

std::string TCHAR2STRING(const TCHAR* str)
{
	std::string strstr;
	//try
	{
		int iLen = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
		char* chRtn = new char[iLen * sizeof(char)];
		WideCharToMultiByte(CP_ACP, 0, str, -1, chRtn, iLen, NULL, NULL);
		strstr = chRtn;
		delete[] chRtn;
	}
	//catch (std::exception e)
	{
	}
	return strstr;
}

std::wstring STRING2TCHAR(std::string str)
{
	int iLen;
	iLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size() + 1, NULL, 0);
	wchar_t* chRtn = new wchar_t[iLen * sizeof(wchar_t)];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size() + 1, chRtn, iLen);
	std::wstring wstr(chRtn);
	delete[] chRtn;
	return wstr;
}

UCCSocketServerRunnable::UCCSocketServerRunnable()
{
	bOwnerEndPlay = false;
	bGameIsStart = false;
	ListenerSocket = nullptr;
	CurrentGameMode = nullptr;

	FString ConfigPath = FPaths::ProjectPluginsDir() + TEXT("SocketServer/") + TEXT("Config/") + TEXT("Server.ini");
	FString FileString;
	bool Res = FFileHelper::LoadFileToString(FileString, *ConfigPath);
	if (Res)
	{
		ConfigFile = MakeShareable(new FConfigFile());
		ConfigFile->ProcessInputFileContents(FileString);
	}
}

UCCSocketServerRunnable::~UCCSocketServerRunnable()
{
}

bool UCCSocketServerRunnable::Init()
{
	FGameModeEvents::OnGameModeInitializedEvent().AddLambda([=](AGameModeBase* GameMode) {
		this->bGameIsStart = true;
		CurrentGameMode = GameMode;
		});
	return true;
}

uint32 UCCSocketServerRunnable::Run()
{
	
	TArray<uint8> ReceivedData;
	uint32 Size;
	while (!bOwnerEndPlay)
	{
		if (CurrentGameMode&& !CurrentGameMode->IsValidLowLevel())
		{
			bGameIsStart = false;
			CurrentGameMode = nullptr;
		}
		while (!bOwnerEndPlay&&!bGameIsStart)
		{
			for (FSocket* Socket : RemoteConnection)
			{
				Socket->Close();
				ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
			}
			RemoteConnection.Empty();
			if (ListenerSocket)
			{
				ListenerSocket->Close();
				ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenerSocket);
				ListenerSocket = nullptr;
			}
			FPlatformProcess::Sleep(0.1f);
		}
		InitSocket();
		bool bPending = false;
		if (ListenerSocket&& ListenerSocket->HasPendingConnection(bPending) && bPending)
		{
			TSharedRef<FInternetAddr> RemoteAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
			FSocket* RecSocket = ListenerSocket->Accept(*RemoteAddress, TEXT("Receive Socket"));
			if (RecSocket)
			{
				RemoteConnection.Add(RecSocket);
			}			
		}
		TArray<FSocket*> TempRemoteConnection;
		TempRemoteConnection.Empty();
		for (FSocket* Socket: RemoteConnection)
		{
			if (Socket&& Socket->GetConnectionState()== ESocketConnectionState::SCS_Connected)
			{
				TempRemoteConnection.Add(Socket);
				if (Socket->HasPendingData(Size))
				{
					FCCSocketConnection CConnection;
					CConnection.Socket = Socket;

					ReceivedData.Init(0, FMath::Min(Size+1, 65507u));
					int32 Readed;
					Socket->Recv(ReceivedData.GetData(), ReceivedData.Num(), Readed);
					CConnection.Message = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(ReceivedData.GetData())));
					ReceiveData.Enqueue(CConnection);
				}
			}
			else if(Socket)
			{
				Socket->Close();
				ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
			}
		}
		RemoteConnection = TempRemoteConnection;

		ProcessReceiveMessage();

		FPlatformProcess::Sleep(0.1f);
		FCCSocketConnection CConnection;
		int32 Sended=0;
		while (!ToSendData.IsEmpty())
		{
			ToSendData.Dequeue(CConnection);
			if (CConnection.Socket)
			{
				std::string ToSend(TCHAR_TO_UTF8(*CConnection.Message));
				CConnection.Socket->Send((uint8*)ToSend.c_str(), ToSend.size(), Sended);
			}
		}		
		FPlatformProcess::Sleep(0.1f);
	}


	
	for (FSocket* Socket : RemoteConnection)
	{
		Socket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
	}
	RemoteConnection.Empty();

	if (ListenerSocket)
	{
		ListenerSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenerSocket);
		ListenerSocket = nullptr;
	}
	bOwnerEndPlay = false;
	return 0;
}

void UCCSocketServerRunnable::Stop()
{
	
}

void UCCSocketServerRunnable::Exit()
{
}

void UCCSocketServerRunnable::ProcessReceiveMessage()
{
	if (!ReceiveData.IsEmpty())
	{
		AsyncTask(ENamedThreads::GameThread, [=]()
			{
				while (!ReceiveData.IsEmpty())
				{
					FCCSocketConnection CConnection;
					ReceiveData.Dequeue(CConnection);
					ReceiveMessageEvent.Broadcast(CConnection);

					for (auto RawEvent : ReceiveMessageRawEveent)
					{
						RawEvent(CConnection);
					}

				}
				
			});
		
	}
}

UCCSocketServerRunnable* UCCSocketServerRunnable::GlobalSocketServerRunnable()
{
	if (!GSocketServerRunnable)
	{
		GSocketServerRunnable = new UCCSocketServerRunnable();
		GSocketServerRunnable->bOwnerEndPlay = false;
		GRunnableThread = FRunnableThread::Create(GSocketServerRunnable, TEXT("CCSocketServerRunnable"), 0);
	}
	return GSocketServerRunnable;
}


void UCCSocketServerRunnable::InitSocket()
{
	if (!ListenerSocket)
	{
		int32 ListenPort = 6666;
		FString ListenIP = "0.0.0.0";

		if (ConfigFile.IsValid())
		{
			int64 Port64 = 6666;
			ConfigFile->GetString(TEXT("SocketServer"), TEXT("Host"), ListenIP);
			ConfigFile->GetInt64(TEXT("SocketServer"), TEXT("Port"), Port64);
			ListenPort = Port64;
		}

		int32 BufferSize = 2 * 1024 * 1024;
		FIPv4Address IPv4Address;
		FIPv4Address::Parse(ListenIP, IPv4Address);
		FIPv4Endpoint Endpoint(IPv4Address, ListenPort);
		ListenerSocket = FTcpSocketBuilder("CCSocketServerRunnable")
			.AsNonBlocking()
			.AsReusable()
			.BoundToEndpoint(Endpoint)
			.WithReceiveBufferSize(2 * 1024 * 1024)
			.Listening(20)
			.WithReceiveBufferSize(BufferSize)
			;
	}
}