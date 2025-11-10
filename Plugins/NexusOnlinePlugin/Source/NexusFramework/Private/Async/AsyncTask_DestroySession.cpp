#include "Async/AsyncTask_DestroySession.h"
#include "Utils/NexusOnlineHelpers.h"
#include "Managers/OnlineSessionManager.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

#define LOCTEXT_NAMESPACE "NexusOnline|DestroySession"

// ──────────────────────────────────────────────
// Create & configure async node
// ──────────────────────────────────────────────

UAsyncTask_DestroySession* UAsyncTask_DestroySession::DestroySession(UObject* WorldContextObject, ENexusSessionType SessionType)
{
	UAsyncTask_DestroySession* Node = NewObject<UAsyncTask_DestroySession>();
	Node->WorldContextObject = WorldContextObject;
	Node->TargetSessionType = SessionType;

	return Node;
}

// ──────────────────────────────────────────────
// Start the destruction sequence
// ──────────────────────────────────────────────

void UAsyncTask_DestroySession::Activate()
{
	// Validate context
	if (!WorldContextObject)
	{
		UE_LOG(LogTemp, Error, TEXT("[DestroySession] Invalid WorldContextObject."));
		OnFailure.Broadcast();
		
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	if (!World)
	{
		OnFailure.Broadcast();
		
		return;
	}

	// Get OnlineSubsystem interface
	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);
	if (!Session.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[DestroySession] No valid OnlineSubsystem or SessionInterface."));
		OnFailure.Broadcast();
		
		return;
	}

	const FName InternalSessionName = NexusOnline::SessionTypeToName(TargetSessionType);

	// Ensure session exists
	if (!Session->GetNamedSession(InternalSessionName))
	{
		UE_LOG(LogTemp, Warning, TEXT("[DestroySession] '%s' does not exist or is already destroyed."), *InternalSessionName.ToString());
		OnFailure.Broadcast();
		
		return;
	}

	// Bind delegate & initiate destruction
	DestroyDelegateHandle = Session->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &UAsyncTask_DestroySession::OnDestroySessionComplete)
	);

	UE_LOG(LogTemp, Log, TEXT("[DestroySession] Destroying session: %s"), *InternalSessionName.ToString());

	if (!Session->DestroySession(InternalSessionName))
	{
		UE_LOG(LogTemp, Error, TEXT("[DestroySession] DestroySession() call failed immediately."));
		Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroyDelegateHandle);
		
		OnFailure.Broadcast();
	}
}

// ──────────────────────────────────────────────
// OnDestroySessionComplete
// ──────────────────────────────────────────────

void UAsyncTask_DestroySession::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);

	if (Session.IsValid())
	{
		Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroyDelegateHandle);
	}

	UE_LOG(LogTemp, Log, TEXT("[DestroySession] Session '%s' destruction result: %s"), *SessionName.ToString(),
		bWasSuccessful ? TEXT("SUCCESS") : TEXT("FAILURE"));

	if (bWasSuccessful && World)
	{
		if (World->GetNetMode() != NM_Client)
		{
			// Server host: fully destroy the runtime manager
			if (AOnlineSessionManager* Manager = AOnlineSessionManager::Get(World))
			{
				Manager->Destroy();
			}
		}
		else
		{
			// Client: just refresh player count
			if (AOnlineSessionManager* Manager = AOnlineSessionManager::Get(World))
			{
				Manager->ForceUpdatePlayerCount();
			}
		}
	}

	(bWasSuccessful ? OnSuccess : OnFailure).Broadcast();
}

#undef LOCTEXT_NAMESPACE
