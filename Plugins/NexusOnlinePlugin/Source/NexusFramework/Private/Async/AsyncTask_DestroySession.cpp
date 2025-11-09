#include "Async/AsyncTask_DestroySession.h"

#include "Engine/Engine.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Utils/NexusOnlineHelpers.h"

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
                UE_LOG(LogNexusOnline, Error, TEXT("DestroySession failed: invalid WorldContextObject."));

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
                UE_LOG(LogNexusOnline, Error, TEXT("DestroySession failed: session interface unavailable."));

                OnFailure.Broadcast();
                return;
        }

        const FName InternalSessionName = NexusOnline::SessionTypeToName(TargetSessionType);

        if (!Session->GetNamedSession(InternalSessionName))
        {
                UE_LOG(LogNexusOnline, Warning, TEXT("DestroySession aborted: '%s' does not exist."), *InternalSessionName.ToString());

                OnFailure.Broadcast();
                return;
        }

        DestroyDelegateHandle = Session->AddOnDestroySessionCompleteDelegate_Handle(
                FOnDestroySessionCompleteDelegate::CreateUObject(this, &UAsyncTask_DestroySession::OnDestroySessionComplete)
        );

        UE_LOG(LogNexusOnline, Log, TEXT("Destroying session '%s'"), *InternalSessionName.ToString());

        if (!Session->DestroySession(InternalSessionName))
        {
                UE_LOG(LogNexusOnline, Error, TEXT("DestroySession call failed immediately."));

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

        UE_LOG(LogNexusOnline, Log, TEXT("Session '%s' destruction result: %s"), *SessionName.ToString(),
                bWasSuccessful ? TEXT("SUCCESS") : TEXT("FAILURE"));

        if (bWasSuccessful)
        {
                NexusOnline::StopTrackingSession(World);
                NexusOnline::BroadcastSessionPlayersChanged(World, TargetSessionType);
                OnSuccess.Broadcast();
        }
        else
        {
                OnFailure.Broadcast();
        }
}
