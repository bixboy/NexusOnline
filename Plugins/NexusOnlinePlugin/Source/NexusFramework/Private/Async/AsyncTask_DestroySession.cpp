#include "Async/AsyncTask_DestroySession.h"
#include "Utils/NexusOnlineHelpers.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineSessionInterface.h"

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
		UE_LOG(LogTemp, Error, TEXT("[NexusOnline] Invalid WorldContextObject in DestroySession"));
		
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
    UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
    IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);
	
    if (Session.IsValid())
    {
        Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroyDelegateHandle);
    }

    UE_LOG(LogTemp, Log, TEXT("[NexusOnline] Session '%s' destruction result: %s"), *SessionName.ToString(),
        bWasSuccessful ? TEXT("SUCCESS") : TEXT("FAILURE"));

    (bWasSuccessful ? OnSuccess : OnFailure).Broadcast();
}
