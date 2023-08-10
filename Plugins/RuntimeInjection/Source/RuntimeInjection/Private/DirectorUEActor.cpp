// Created by wangpeimin@corp.netease.com


#include "DirectorUEActor.h"
#include "RuntimeInjectionBPLibrary.h"
#include "EngineUtils.h"
#include "ActorEditorUtils.h"
#include "AssetRegistryModule.h"
#include "IAssetRegistry.h"

#if WITH_EDITOR
#include "Editor.h"
#include "ObjectTools.h"
#include "ClassIconFinder.h"
#endif



#include "IImageWrapperModule.h"
#include "IImageWrapper.h"

#include "Components/TextRenderComponent.h"

#define CCDirectorDebug 1


TMap<FString, ReceivedMessageEvent> CCDirector::ReceivedMessageEvents= TMap<FString, ReceivedMessageEvent>();

CCDirector::CCDirector()
{
	if (!GDirectorRunnable)
	{
		GDirectorRunnable = new UCCDirectorRunnable();
		GDirectorRunnable->bOwnerEndPlay = false;
		GDirectorRunnableThread = FRunnableThread::Create(GDirectorRunnable, TEXT("GDirectorRunnable"), 0);
	}
	ReceivedMessageEvents.Add(TTuple<FString, ReceivedMessageEvent>("GetMaps", &CCDirector::OnReceivedGetMaps));
	ReceivedMessageEvents.Add(TTuple<FString, ReceivedMessageEvent>("LoadMap", &CCDirector::OnReceivedLoadMap));
	ReceivedMessageEvents.Add(TTuple<FString, ReceivedMessageEvent>("GetActors", &CCDirector::OnReceivedGetActors));
	ReceivedMessageEvents.Add(TTuple<FString, ReceivedMessageEvent>("GetGetDetails", &CCDirector::OnReceivedGetDetails));

	ReceivedMessageEvents.Add(TTuple<FString, ReceivedMessageEvent>("SetProperty", &CCDirector::OnReceivedSetProperty));
	ReceivedMessageEvents.Add(TTuple<FString, ReceivedMessageEvent>("CallFunction", &CCDirector::OnReceivedCallFunction));

	ReceivedMessageEvents.Add(TTuple<FString, ReceivedMessageEvent>("SearchAssets", &CCDirector::OnReceivedSearchAssets));
}

CCDirector::~CCDirector()
{
	if (GDirectorRunnable)
	{
		delete GDirectorRunnable;
		GDirectorRunnable = nullptr;
	}
}

void CCDirector::StaticOnReceivedMessage(FCCSocketConnection CCConnection)
{
	FString MessaeType = "";
	if (CCDirectorDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnReceivedMessage: %s"), *CCConnection.Message);
	}
	
	TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create(CCConnection.Message);
	TSharedPtr<FJsonValue> JsonRoot;
	bool bSuccessful = FJsonSerializer::Deserialize(Reader, JsonRoot);
	if (bSuccessful) 
	{
		MessaeType = JsonRoot->AsObject()->GetStringField(TEXT("MessageType"));
		TSharedPtr<FJsonObject> Data = JsonRoot->AsObject()->GetObjectField(TEXT("Data"));
		auto Event=ReceivedMessageEvents.Find(MessaeType);
		Event ? (*Event)(CCConnection, Data) : 0;
	}
}

void CCDirector::OnReceivedGetMaps(const FCCSocketConnection& CCConnection, TSharedPtr<FJsonObject> Data)
{
	auto SocketServerRunnable = UCCSocketServerRunnable::GlobalSocketServerRunnable();
	TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);
	TArray<TSharedPtr<FJsonValue>> MapsJsonArray;
	TArray<FString> Maps = URuntimeInjectionBPLibrary::GetAllMapNames();	
	for (auto Map : Maps)
	{
		MapsJsonArray.Add(MakeShareable(new FJsonValueString(Map)));
	}
	Root->SetStringField("Result", "ok");
	Root->SetArrayField("Maps", MapsJsonArray);	
	FCCSocketConnection Connection = CCConnection;
	TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Connection.Message);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer); 
	SocketServerRunnable->ToSendData.Enqueue(Connection);
}

void CCDirector::OnReceivedLoadMap(const FCCSocketConnection& CCConnection, TSharedPtr<FJsonObject> Data)
{
	auto SocketServerRunnable = UCCSocketServerRunnable::GlobalSocketServerRunnable();
	FString MapName = Data->GetStringField(TEXT("MapName"));

	TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);

	TArray<FString> Maps = URuntimeInjectionBPLibrary::GetAllMapNames();
	bool bMapValid = false;
	for (auto Map : Maps)
	{
		if (Map== MapName)
		{
			bMapValid = true;
		}
	}

	bMapValid?Root->SetStringField("Result", "ok"): Root->SetStringField("Result", "map name not valid");
	FCCSocketConnection Connection = CCConnection;
	TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Connection.Message);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	SocketServerRunnable->ToSendData.Enqueue(Connection);
	FGraphEventRef TaskRef = FFunctionGraphTask::CreateAndDispatchWhenReady([=]()
		{			
			UGameplayStatics::OpenLevel(DefaultWorlContex(), *MapName);
		}, TStatId(), nullptr, ENamedThreads::GameThread);
	
}

void CCDirector::OnReceivedGetActors(const FCCSocketConnection& CCConnection, TSharedPtr<FJsonObject> Data)
{
	auto SocketServerRunnable = UCCSocketServerRunnable::GlobalSocketServerRunnable();
	TGuardValue<bool> UnattendedScriptGuard(GIsRunningUnattendedScript, true);
	TArray<AActor*> Result;
	FindActors(Result);

	TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);
	TArray<TSharedPtr<FJsonValue>> ActorJsonArray;
	for (auto Actor : Result)
	{
		TSharedPtr<FJsonObject> ActorJson = MakeShareable(new FJsonObject);
		//ActorJson->SetStringField("Path", ExPathName(Actor->GetPathName()));
		//ActorJson->SetStringField("Class", Actor->GetClass()->GetName());
		SetActorJsonDetail(Actor, ActorJson);
		TSharedRef< FJsonValueObject > JsonValue = MakeShareable(new FJsonValueObject(ActorJson));
		ActorJsonArray.Add(JsonValue);
	}

	Root->SetStringField("Result", "ok");
	Root->SetArrayField("Actors", ActorJsonArray);

	FCCSocketConnection Connection = CCConnection;
	TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Connection.Message);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	SocketServerRunnable->ToSendData.Enqueue(Connection);
}

void CCDirector::OnReceivedGetDetails(const FCCSocketConnection& CCConnection, TSharedPtr<FJsonObject> Data)
{
	auto SocketServerRunnable = UCCSocketServerRunnable::GlobalSocketServerRunnable();
	FString ActorPath = Data->GetStringField(TEXT("Path"));

	TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);

	AActor* TargetActor = nullptr;
	TArray<AActor*> Result;
	FindActors(Result);
	for (auto LoopActor : Result)
	{
		if (ExPathName(LoopActor->GetPathName()) == ActorPath)
		{
			TargetActor = LoopActor;
			break;
		}
	}
	TSharedPtr<FJsonObject> ActorJson = MakeShareable(new FJsonObject);
	SetActorJsonDetail(TargetActor, ActorJson);
	Root->SetObjectField("Actor", ActorJson);
	TargetActor ? Root->SetStringField("Result", "ok") : Root->SetStringField("Result", "actor not found");
	FCCSocketConnection Connection = CCConnection;
	TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Connection.Message);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	SocketServerRunnable->ToSendData.Enqueue(Connection);
}

void CCDirector::OnReceivedSetProperty(const FCCSocketConnection& CCConnection, TSharedPtr<FJsonObject> Data)
{
	auto SocketServerRunnable = UCCSocketServerRunnable::GlobalSocketServerRunnable();
	bool SetPropertyResult = true;
	auto PropertyArrayJson = Data->GetArrayField("PropertyArray");
	FString ActorPath = Data->GetStringField(TEXT("Path"));
	AActor* TargetActor = nullptr;
	TArray<AActor*> Result;
	FindActors(Result);
	for (auto LoopActor : Result)
	{
		if (ExPathName(LoopActor->GetPathName()) == ActorPath)
		{
			TargetActor = LoopActor;
			break;
		}
	}
	for (auto PropertyJson: PropertyArrayJson)
	{
		FString PropertyName = PropertyJson->AsObject()->GetStringField(TEXT("Property"));
		FString PropertyValue = PropertyJson->AsObject()->GetStringField(TEXT("Value"));

		SetPropertyResult = URuntimeInjectionBPLibrary::SetObjectProperty(TargetActor, TargetActor, PropertyName, PropertyValue);
		if (!SetPropertyResult && TargetActor)
		{
			USceneComponent* RootComponent = TargetActor->GetRootComponent();
			SetPropertyResult = URuntimeInjectionBPLibrary::SetObjectProperty(RootComponent, RootComponent, PropertyName, PropertyValue);
		}
	}
	
	
	TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);	
	

	SetPropertyResult ? Root->SetStringField("Result", "ok") : Root->SetStringField("Result", "property not found");
	FCCSocketConnection Connection = CCConnection;
	TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Connection.Message);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	SocketServerRunnable->ToSendData.Enqueue(Connection);
}

void CCDirector::OnReceivedCallFunction(const FCCSocketConnection& CCConnection, TSharedPtr<FJsonObject> Data)
{
	auto SocketServerRunnable = UCCSocketServerRunnable::GlobalSocketServerRunnable();
	FString ActorPath = Data->GetStringField(TEXT("Path"));
	FString PropertyPath = Data->GetStringField(TEXT("PropertyPath"));
	FString FunctionName = Data->GetStringField(TEXT("Function"));
	auto FunctionParamsJson= Data->GetArrayField("FunctionParams");
	TArray<FString> Params;
	for (auto ParamJson : FunctionParamsJson)
	{
		Params.Add(ParamJson->AsString());
	}
	TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);

	AActor* TargetActor = nullptr;
	TArray<AActor*> Result;
	FindActors(Result);
	for (auto LoopActor : Result)
	{
		if (ExPathName(LoopActor->GetPathName()) == ActorPath)
		{
			TargetActor = LoopActor;
			break;
		}
	}
	UObject* FunctionOwner = TargetActor;
	UFunction* Function = TargetActor ? TargetActor->FindFunction(*FunctionName) : nullptr;
	if (!Function && !PropertyPath.IsEmpty() && TargetActor)
	{
		FProperty* Property = nullptr;
		for (TFieldIterator<FProperty> It(TargetActor->GetClass()); It; ++It)
		{
			if (It->GetName() == PropertyPath)
			{
				Property = *It;
				break;
			}
		}
		if (Property)
		{
			UObject** PropertyObject = Property->ContainerPtrToValuePtr<UObject*>(TargetActor);
			Function = (PropertyObject && (*PropertyObject) && (*PropertyObject)->IsValidLowLevel()) 
				? (*PropertyObject)->FindFunction(*FunctionName) : nullptr;
			FunctionOwner = *(PropertyObject);
		}
	}
	Function ? Root->SetStringField("Result", "ok") :
		(TargetActor ? Root->SetStringField("Result", "function not found") : Root->SetStringField("Result", "actor not found"));

	
	FCCSocketConnection Connection = CCConnection;
	TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Connection.Message);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	FGraphEventRef TaskRef = FFunctionGraphTask::CreateAndDispatchWhenReady([=]()
		{
			Function ? URuntimeInjectionBPLibrary::CallFunctionByName(FunctionOwner, FunctionOwner, FunctionName, Params) : 0;
			SocketServerRunnable->ToSendData.Enqueue(Connection);
		}, TStatId(), nullptr, ENamedThreads::GameThread);	
}

void CCDirector::OnReceivedSearchAssets(const FCCSocketConnection& CCConnection, TSharedPtr<FJsonObject> Data)
{
	static int32 MaxLimit = 50;
	auto SocketServerRunnable = UCCSocketServerRunnable::GlobalSocketServerRunnable();
	TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	FARFilter Filter;
	FString Query = Data->GetStringField(TEXT("Query"));
	FString ClassName = Data->GetStringField(TEXT("Class"));
	auto ConvertShortClassNameToPathName = [](FName ShortClassFName)
	{
		FTopLevelAssetPath ClassPathName;
		if (ShortClassFName != NAME_None)
		{
			FString ShortClassName = ShortClassFName.ToString();
			ClassPathName = UClass::TryConvertShortTypeNameToPathName<UStruct>(*ShortClassName, ELogVerbosity::Warning, TEXT("FARFilter::PostSerialize"));
			UE_CLOG(ClassPathName.IsNull(), LogAssetData, Error, TEXT("Failed to convert short class name %s to class path name."), *ShortClassName);
		}
		return ClassPathName;
	};
	if (!ClassName.IsEmpty())
	{
		FTopLevelAssetPath ClassPathName = ConvertShortClassNameToPathName(FName(ClassName));
		Filter.ClassPaths.Add(ClassPathName);
	}
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	Filter.bIncludeOnlyOnDiskAssets = false;

	TArray<FAssetData> Assets;
	AssetRegistryModule.Get().GetAssets(Filter, Assets);
	int32 ArrayEnd = FMath::Min(MaxLimit, Assets.Num());

	TArray<TSharedPtr<FJsonValue>> AssetsJsonArray;

	for (const FAssetData& AssetData : Assets)
	{
		FString Path;
		if (!Query.IsEmpty())
		{
			if (AssetData.AssetName.ToString().Contains(*Query))
			{
				Path = AssetData.AssetClassPath.GetAssetName().ToString() +"'"+ AssetData.GetObjectPathString() +"'";
			}
		}
		else
		{
			Path = AssetData.AssetClassPath.GetAssetName().ToString() + "'" + AssetData.GetObjectPathString() + "'";
		}
		AssetsJsonArray.Add(MakeShareable(new FJsonValueString(Path)));
		if (ArrayEnd--<0)
		{
			break;
		}
	}

	Root->SetStringField("Result", "ok");
	Root->SetArrayField("Assets", AssetsJsonArray);

	
	FCCSocketConnection Connection = CCConnection;
	TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Connection.Message);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	SocketServerRunnable->ToSendData.Enqueue(Connection);
}

bool CCDirector::CheckIfInEditorAndPIE()
{
	if (!IsInGameThread())
	{
		return false;
	}
	if (!GIsEditor)
	{
		return false;
	}
	return true;
}

void CCDirector::FindActors(TArray<AActor*>& Result)
{
	//if (CCDirector::CheckIfInEditorAndPIE())
	{
		const EActorIteratorFlags Flags = EActorIteratorFlags::SkipPendingKill;
		//for (TActorIterator<AActor> It(GEditor->GetEditorWorldContext(false).World(), AActor::StaticClass(), Flags); It; ++It)
		for (TActorIterator<AActor> It(DefaultWorlContex(), AActor::StaticClass(), Flags); It; ++It)
		{
			AActor* Actor = *It;
			if (
#if WITH_EDITOR
				Actor->IsEditable() &&
				Actor->IsListedInSceneOutliner() &&
#endif
				!Actor->IsTemplate() &&
				!Actor->HasAnyFlags(RF_Transient) &&
				!FActorEditorUtils::IsABuilderBrush(Actor) &&
				!Actor->IsA(AWorldSettings::StaticClass()))
			{
				Result.Add(*It);
			}
		}
	}
}

UWorld* CCDirector::DefaultWorlContex()
{
	auto WorldContexts = GEngine->GetWorldContexts();
	for (auto WC : WorldContexts)
	{
		if (WC.WorldType == EWorldType::Type::PIE
			|| WC.WorldType == EWorldType::Type::Game
			)
		{
			return WC.World();
		}
	}
	return nullptr;
}

void CCDirector::SetActorJsonDetail(AActor* Actor, TSharedPtr<FJsonObject> ActorJson)
{
	UFunction* Function = nullptr;
	if (Actor)
	{
		ActorJson->SetStringField("Path", ExPathName(Actor->GetPathName()));
		ActorJson->SetStringField("Class", Actor->GetClass()->GetName());

		FVector Loc = Actor->GetActorLocation();
		ActorJson->SetStringField("Location", FString::Printf(TEXT("(X=%3.3f ,Y=%3.3f ,Z=%3.3f)"), Loc.X, Loc.Y, Loc.Z));

		FRotator Roc = Actor->GetActorRotation();
		ActorJson->SetStringField("Rotation", FString::Printf(TEXT("(Pitch=%3.3f ,Yaw=%3.3f ,Roll=%3.3f)"), Roc.Pitch, Roc.Yaw, Roc.Roll));

		FVector Sca = Actor->GetActorScale();
		ActorJson->SetStringField("Scale", FString::Printf(TEXT("(X=%3.3f ,Y=%3.3f ,Z=%3.3f)"), Sca.X, Sca.Y, Sca.Z));

		ActorJson->SetStringField("IsBlueprint", (Actor->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint)?"True":"False"));
		ActorDetailVisiable(Actor, ActorJson);
		ActorDetailMobility(Actor, ActorJson);
		ActorDetailText(Actor, ActorJson);
		ActorDetailLevelSequence(Actor, ActorJson);
		ObjectDetail(Actor, ActorJson);
		USceneComponent* RootComponent = Actor->GetRootComponent();
		RootComponent ? ObjectDetail(RootComponent, ActorJson) : 0;

		TArray<TSharedPtr<FJsonValue>> FunctionJsonArray;
		for (TFieldIterator<UFunction> It(Actor->GetClass()); It; ++It)
		{
			Function = *It;
			if ((Function->FunctionFlags & EFunctionFlags::FUNC_BlueprintCallable)
				&& (Function->FunctionFlags & EFunctionFlags::FUNC_BlueprintEvent)
				/*&& Function->NumParms == 0*/)
			{
				FunctionJsonArray.Add(MakeShareable(new FJsonValueString(Function->GetName())));
			}
		}
		ActorJson->SetArrayField("Functions", FunctionJsonArray);
	}
}

void CCDirector::ActorDetailVisiable(AActor* Actor, TSharedPtr<FJsonObject> ActorJson)
{
	if (Actor)
	{
		ActorJson->SetStringField("IsHidden", Actor->IsHidden() ? "true" : "false");
	}
}

void CCDirector::ActorDetailMobility(AActor* Actor, TSharedPtr<FJsonObject> ActorJson)
{
	if (Actor)
	{
		USceneComponent* SceneComponent = Cast<USceneComponent>(Actor->GetRootComponent());
		if (SceneComponent)
		{
			ActorJson->SetStringField("Mobility", StaticEnum<EComponentMobility::Type>()->GetNameStringByValue(SceneComponent->Mobility));
			
		}
	}
	else
	{
		ActorJson->SetStringField("Mobility", "None");
	}
}

void CCDirector::ActorDetailText(AActor* Actor, TSharedPtr<FJsonObject> ActorJson)
{
	USceneComponent* SceneComponent = Actor ? Cast<UTextRenderComponent>(Actor->GetRootComponent()) : nullptr;
	if (SceneComponent)
	{
		FObjectProperty* Property = Cast<FObjectProperty>(SceneComponent->GetClass()->FindPropertyByName("Text"));
		if (Property)
		{
			FString TextStr;
			Property->ExportTextItem(TextStr, Property->ContainerPtrToValuePtr<void>(SceneComponent), nullptr, nullptr, PPF_None, nullptr);
			ActorJson->SetStringField("Text", TextStr);

		}
		ActorJson->SetStringField("isText", SceneComponent->GetClass()->GetName() == "TextRenderComponent" ? "True" : "False");
		ActorJson->SetStringField("is3DText", SceneComponent->GetClass()->GetName() == "Text3DComponent" ? "True" : "False");
	}
	else
	{
		ActorJson->SetStringField("isText", "False");
		ActorJson->SetStringField("is3DText", "False");
	}
}

void CCDirector::ActorDetailLevelSequence(AActor* Actor, TSharedPtr<FJsonObject> ActorJson)
{
	if (Actor && Actor->GetClass()->GetName()=="LevelSequenceActor")
	{
		ActorJson->SetStringField("isLevelSequenceActor", "True");
	}
	else
	{
		ActorJson->SetStringField("isLevelSequenceActor", "False");
	}
}

void CCDirector::ObjectDetail(UObject* Object, TSharedPtr<FJsonObject> ActorJson)
{
	for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
	{
		FProperty* Property = *It;
		if ((Property->PropertyFlags & EPropertyFlags::CPF_Edit)
			&& (Property->PropertyFlags & EPropertyFlags::CPF_BlueprintVisible)
			&& (Property->FlagsPrivate & EObjectFlags::RF_Public)
			&& Property->Owner.GetOwnerClass() == Object->GetClass()
			)
		{
			FRIProperty RIProperty;
			RIProperty.PropertyName = Property->GetName();
			RIProperty.Type = Property->GetCPPType();
			const void* PropertyValuePtr = Property->ContainerPtrToValuePtr<void>(Object);
			Property->ExportTextItem(RIProperty.ValueString, PropertyValuePtr, nullptr, nullptr, PPF_None);
			ActorJson->SetStringField(RIProperty.PropertyName, RIProperty.ValueString);
		}
	}
}

FString CCDirector::ExPathName(const FString& PathName)
{
	if (PathName.StartsWith("/Game"))
	{
		FString Left;
		FString Right;
		if (PathName.Split(".", &Left, &Right))
		{
			return Right;
		}
	}
	return PathName;
}

const FString CCDirector::EnumToString(const TCHAR* Enum, int32 EnumValue)
{
	const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, Enum, true);
	if (!EnumPtr)
	{
		return NSLOCTEXT("Invalid", "Invalid", "Invalid").ToString();
	}
#if WITH_EDITOR
	//return EnumPtr->GetDisplayNameText(EnumValue).ToString();
	return EnumPtr->GetDisplayNameText().ToString();
#else
	return EnumPtr->GetNameStringByIndex(EnumValue);
#endif
}

UCCDirectorRunnable::UCCDirectorRunnable()
{
}

UCCDirectorRunnable::~UCCDirectorRunnable()
{
}

bool UCCDirectorRunnable::Init()
{
	return true;
}

uint32 UCCDirectorRunnable::Run()
{
	bool bHasRegister = false;
	while (!bHasRegister)
	{
		auto SocketServerRunnable=UCCSocketServerRunnable::GlobalSocketServerRunnable();
		if (SocketServerRunnable)
		{
			SocketServerRunnable->ReceiveMessageRawEveent.Add(&CCDirector::StaticOnReceivedMessage);
			bHasRegister = true;
		}
	}
	/*while (!bOwnerEndPlay)
	{
		FPlatformProcess::Sleep(0.1f);
	}*/
	bOwnerEndPlay = false;
	return 0;
}

void UCCDirectorRunnable::Stop()
{
}

void UCCDirectorRunnable::Exit()
{
}
