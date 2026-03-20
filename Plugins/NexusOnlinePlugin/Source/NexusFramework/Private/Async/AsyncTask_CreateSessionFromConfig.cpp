#include "Async/AsyncTask_CreateSessionFromConfig.h"
#include "Data/NexusSessionConfig.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Utils/NexusOnlineHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

#define LOCTEXT_NAMESPACE "NexusOnline|CreateSessionConfig"

UAsyncTask_CreateSessionFromConfig* UAsyncTask_CreateSessionFromConfig::CreateSessionFromConfig(
	UObject* WorldContextObject, 
	UNexusSessionConfig* SessionConfig,
	bool bAutoTravel)
{
	UAsyncTask_CreateSessionFromConfig* Node = NewObject<UAsyncTask_CreateSessionFromConfig>();
	Node->WorldContextObject = WorldContextObject;
	Node->SessionConfig = SessionConfig;
	Node->bShouldAutoTravel = bAutoTravel;
	return Node;
}

void UAsyncTask_CreateSessionFromConfig::Activate()
{
	if (!WorldContextObject || !SessionConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("[CreateSessionConfig] Invalid Context or Config Asset."));
		OnFailure.Broadcast();
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);
	if (!Session.IsValid())
	{
		OnFailure.Broadcast();
		return;
	}

	const FName InternalSessionName = NexusOnline::SessionTypeToName(SessionConfig->SessionSettings.SessionType);

	// 1. Check existing
	if (Session->GetNamedSession(InternalSessionName))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CreateSessionConfig] Existing session found. Destroying..."));
		DestroyDelegateHandle = Session->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this,
			&UAsyncTask_CreateSessionFromConfig::OnOldSessionDestroyed));

		if (!Session->DestroySession(InternalSessionName))
		{
			Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroyDelegateHandle);
			OnFailure.Broadcast();
		}
		return; 
	}

	// 2. Proceed
	CreateSessionInternal();
}

void UAsyncTask_CreateSessionFromConfig::OnOldSessionDestroyed(FName SessionName, bool bWasSuccessful)
{
	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);
	
	if (Session.IsValid())
	{
		Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroyDelegateHandle);
	}

	if (bWasSuccessful)
	{
		CreateSessionInternal();
	}
	else
	{
		OnFailure.Broadcast();
	}
}

void UAsyncTask_CreateSessionFromConfig::CreateSessionInternal()
{
	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);
	if (!Session.IsValid()) 
	{
		OnFailure.Broadcast();
		return;
	}

	const FName InternalSessionName = NexusOnline::SessionTypeToName(SessionConfig->SessionSettings.SessionType);

	// --- KEY DIFFERENCE: Use the Config Asset to generate settings ---
	FOnlineSessionSettings Settings;
	
	// Delegate logic to the DataAsset (allows for C++ overrides!)
	SessionConfig->ModifySessionSettings(Settings);
	
	// Apply Subsystem overrides (NULL/LAN) if necessary
	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(World);
	if (Subsystem && Subsystem->GetSubsystemName() == TEXT("NULL"))
	{
		Settings.bIsLANMatch = true;
	}

	// Launch
	CreateDelegateHandle = Session->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this,
		&UAsyncTask_CreateSessionFromConfig::OnCreateSessionComplete));
	
	UE_LOG(LogTemp, Log, TEXT("[CreateSessionConfig] Creating session '%s' using Config '%s'..."), 
		*InternalSessionName.ToString(), *SessionConfig->GetName());

	if (!Session->CreateSession(0, InternalSessionName, Settings))
	{
		Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateDelegateHandle);
		OnFailure.Broadcast();
	}
}

void UAsyncTask_CreateSessionFromConfig::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);

	if (Session.IsValid())
	{
		Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateDelegateHandle);
	}

	if (!bWasSuccessful)
	{
		OnFailure.Broadcast();
		return;
	}

	if (Session.IsValid())
	{
		Session->StartSession(SessionName);
	}
	
	OnSuccess.Broadcast();

	if (bShouldAutoTravel && World && !SessionConfig->SessionSettings.MapName.IsEmpty())
	{
		UGameplayStatics::OpenLevel(World, FName(*SessionConfig->SessionSettings.MapName), true, TEXT("listen"));
	}
}
#undef LOCTEXT_NAMESPACE
