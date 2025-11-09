#include "Async/AsyncTask_DestroySession.h"
#include "Utils/NexusOnlineHelpers.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Managers/OnlineSessionManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

UAsyncTask_DestroySession* UAsyncTask_DestroySession::DestroySession(UObject* WorldContextObject, ENexusSessionType SessionType)
{
    UAsyncTask_DestroySession* Node = NewObject<UAsyncTask_DestroySession>();
	Node->WorldContextObject = WorldContextObject;
    Node->TargetSessionType = SessionType;
	
    return Node;
}

void UAsyncTask_DestroySession::Activate()
{
    if (!WorldContextObject)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NexusOnline] Null WorldContextObject in DestroySession, attempting fallback world."));
    }

    UWorld* World = NexusOnline::ResolveWorld(WorldContextObject);
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("[NexusOnline] Unable to resolve world in DestroySession."));
        OnFailure.Broadcast();
        return;
    }

    IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);
    if (!Session.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[NexusOnline] DestroySession failed: no valid OnlineSubsystem."));
    	
        OnFailure.Broadcast();
        return;
    }

    const FName InternalSessionName = NexusOnline::SessionTypeToName(TargetSessionType);

    if (!Session->GetNamedSession(InternalSessionName))
    {
        UE_LOG(LogTemp, Warning, TEXT("[NexusOnline] DestroySession: '%s' does not exist."), *InternalSessionName.ToString());
    	
        OnFailure.Broadcast();
        return;
    }

    DestroyDelegateHandle = Session->AddOnDestroySessionCompleteDelegate_Handle
	(
        FOnDestroySessionCompleteDelegate::CreateUObject(this, &UAsyncTask_DestroySession::OnDestroySessionComplete)
    );

    UE_LOG(LogTemp, Log, TEXT("[NexusOnline] Destroying session: %s"), *InternalSessionName.ToString());

    if (!Session->DestroySession(InternalSessionName))
    {
        UE_LOG(LogTemp, Error, TEXT("[NexusOnline] DestroySession() call failed immediately."));
    	
        Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroyDelegateHandle);
        OnFailure.Broadcast();
    }
}

void UAsyncTask_DestroySession::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
    UWorld* World = NexusOnline::ResolveWorld(WorldContextObject);
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("[NexusOnline] World invalid during OnDestroySessionComplete."));
        OnFailure.Broadcast();
        return;
    }
    IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);

    if (Session.IsValid())
    {
        Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroyDelegateHandle);
    }

    UE_LOG(LogTemp, Log, TEXT("[NexusOnline] Session '%s' destruction result: %s"), *SessionName.ToString(),
        bWasSuccessful ? TEXT("SUCCESS") : TEXT("FAILURE"));

    if (bWasSuccessful && World)
    {
        if (World->GetNetMode() != NM_Client)
        {
            if (AOnlineSessionManager* Manager = AOnlineSessionManager::Get(World))
            {
                Manager->Destroy();
            }
        }
        else if (AOnlineSessionManager* Manager = AOnlineSessionManager::Get(World))
        {
            Manager->ForceUpdatePlayerCount();
        }
    }

    (bWasSuccessful ? OnSuccess : OnFailure).Broadcast();
}
