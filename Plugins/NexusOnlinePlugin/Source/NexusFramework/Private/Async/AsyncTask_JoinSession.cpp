#include "Async/AsyncTask_JoinSession.h"
#include "OnlineSubsystem.h"
#include "Filters/SessionFilterRule.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "GameFramework/PlayerController.h"
#include "Utils/NexusOnlineHelpers.h"

#define LOCTEXT_NAMESPACE "NexusOnline|JoinSession"

// ──────────────────────────────────────────────
// Factory
// ──────────────────────────────────────────────
UAsyncTask_JoinSession* UAsyncTask_JoinSession::JoinSession( UObject* WorldContextObject, const FOnlineSessionSearchResultData& SessionResult, bool bAutoTravel, ENexusSessionType SessionType)
{
	UAsyncTask_JoinSession* Node = NewObject<UAsyncTask_JoinSession>();
	Node->WorldContextObject = WorldContextObject;
	Node->RawResult = SessionResult.RawResult;
	Node->DesiredType = SessionType;
	Node->bShouldAutoTravel = bAutoTravel;

	return Node;
}

// ──────────────────────────────────────────────
// Activate : Entry Point
// ──────────────────────────────────────────────
void UAsyncTask_JoinSession::Activate()
{
	if (!WorldContextObject)
	{
		UE_LOG(LogNexusOnlineFilter, Error, TEXT("[JoinSession] Invalid WorldContextObject."));
		OnFailure.Broadcast();
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	if (!World)
	{
		OnFailure.Broadcast();
		return;
	}

	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);
	if (!Session.IsValid())
	{
		UE_LOG(LogNexusOnlineFilter, Error, TEXT("[JoinSession] Online session interface invalid."));
		OnFailure.Broadcast();
		return;
	}

	const FName InternalSessionName = NexusOnline::SessionTypeToName(DesiredType);
	const int32 MaxPublic = RawResult.Session.SessionSettings.NumPublicConnections;
	const int32 OpenPublic = RawResult.Session.NumOpenPublicConnections;

	if (MaxPublic > 0 && OpenPublic <= 0)
	{
		UE_LOG(LogNexusOnlineFilter, Warning, TEXT("[JoinSession] Session '%s' appears full (%d/%d). Aborting."),
			*InternalSessionName.ToString(),
			(MaxPublic - OpenPublic),
			MaxPublic);
		
		OnFailure.Broadcast();
		return;
	}
	
	if (Session->GetNamedSession(InternalSessionName))
	{
		UE_LOG(LogNexusOnlineFilter, Warning, TEXT("[JoinSession] Existing session found. Leaving before joining new one..."));
		
		DestroyDelegateHandle = Session->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(this, &UAsyncTask_JoinSession::OnOldSessionDestroyed)
		);
		
		Session->DestroySession(InternalSessionName);
		return;
	}

	JoinSessionInternal();
}

// ──────────────────────────────────────────────
// Step 1.5: Old Session Destroyed
// ──────────────────────────────────────────────
void UAsyncTask_JoinSession::OnOldSessionDestroyed(FName SessionName, bool bWasSuccessful)
{
	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);
	
	if (Session.IsValid())
	{
		Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroyDelegateHandle);
	}
	
	JoinSessionInternal();
}

// ──────────────────────────────────────────────
// Step 2: Internal Join Logic
// ──────────────────────────────────────────────
void UAsyncTask_JoinSession::JoinSessionInternal()
{
	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);
	if (!Session.IsValid())
	{
		OnFailure.Broadcast();
		return;
	}

	const FName InternalSessionName = NexusOnline::SessionTypeToName(DesiredType);

	JoinDelegateHandle = Session->AddOnJoinSessionCompleteDelegate_Handle
	(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UAsyncTask_JoinSession::OnJoinSessionComplete)
	);

	UE_LOG(LogNexusOnlineFilter, Log, TEXT("[JoinSession] Joining session '%s'..."), *InternalSessionName.ToString());

	TSharedPtr<const FUniqueNetId> LocalPlayerId;
	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			LocalPlayerId = LP->GetPreferredUniqueNetId().GetUniqueNetId();
		}
	}

	if (!LocalPlayerId.IsValid())
	{
		if (!Session->JoinSession(0, InternalSessionName, RawResult))
		{
			Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinDelegateHandle);
			OnFailure.Broadcast();
		}
	}
	else
	{
		if (!Session->JoinSession(*LocalPlayerId, InternalSessionName, RawResult))
		{
			Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinDelegateHandle);
			OnFailure.Broadcast();
		}
	}
}

// ──────────────────────────────────────────────
// Step 3: Completion & Travel
// ──────────────────────────────────────────────
void UAsyncTask_JoinSession::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	if (!World)
		return; 

	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);
	if (Session.IsValid())
	{
		Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinDelegateHandle);
	}
	else
	{
		OnFailure.Broadcast();
		return;
	}

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogNexusOnlineFilter, Error, TEXT("[JoinSession] Failed with code %d."), static_cast<int32>(Result));
		OnFailure.Broadcast();
		return;
	}

	FString ConnectString;
	if (!Session->GetResolvedConnectString(SessionName, ConnectString))
	{
		UE_LOG(LogNexusOnlineFilter, Error, TEXT("[JoinSession] Failed to resolve connect string (URL)."));
		OnFailure.Broadcast();
		return;
	}
	
	UE_LOG(LogNexusOnlineFilter, Log, TEXT("[JoinSession] Success. Connect String: %s"), *ConnectString);

	OnSuccess.Broadcast();
	if (bShouldAutoTravel)
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			UE_LOG(LogNexusOnlineFilter, Log, TEXT("[JoinSession] Executing ClientTravel..."));
			PC->ClientTravel(ConnectString, TRAVEL_Absolute);
		}
	}
}

#undef LOCTEXT_NAMESPACE