#include "Async/AsyncTask_JoinSession.h"

#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Utils/NexusOnlineHelpers.h"

UAsyncTask_JoinSession* UAsyncTask_JoinSession::JoinSession(UObject* WorldContextObject, const FOnlineSessionSearchResultData& SessionResult, ENexusSessionType SessionType)
{
        UAsyncTask_JoinSession* Node = NewObject<UAsyncTask_JoinSession>();
        Node->WorldContextObject = WorldContextObject;
        Node->RawResult = SessionResult.RawResult;
        Node->DesiredType = SessionType;

        return Node;
}

void UAsyncTask_JoinSession::Activate()
{
        if (!WorldContextObject)
        {
                UE_LOG(LogNexusOnline, Error, TEXT("JoinSession failed: invalid WorldContextObject."));

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
                UE_LOG(LogNexusOnline, Error, TEXT("JoinSession failed: no session interface available."));

                OnFailure.Broadcast();
                return;
        }

        const FName InternalSessionName = NexusOnline::SessionTypeToName(DesiredType);

        JoinDelegateHandle = Session->AddOnJoinSessionCompleteDelegate_Handle(
                FOnJoinSessionCompleteDelegate::CreateUObject(this, &UAsyncTask_JoinSession::OnJoinSessionComplete)
        );

        UE_LOG(LogNexusOnline, Log, TEXT("Joining session '%s'..."), *InternalSessionName.ToString());

        if (!Session->JoinSession(0, InternalSessionName, RawResult))
        {
                UE_LOG(LogNexusOnline, Error, TEXT("JoinSession call failed to start."));
                Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinDelegateHandle);
                OnFailure.Broadcast();
        }
}

void UAsyncTask_JoinSession::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
        UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
        if (!World)
        {
                OnFailure.Broadcast();
                return;
        }

        IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);
        if (!Session.IsValid())
        {
                OnFailure.Broadcast();
                return;
        }

        Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinDelegateHandle);

        if (Result != EOnJoinSessionCompleteResult::Success)
        {
                UE_LOG(LogNexusOnline, Error, TEXT("Join session failed (%d)."), static_cast<int32>(Result));
                OnFailure.Broadcast();
                return;
        }

        FString ConnectString;
        if (!Session->GetResolvedConnectString(SessionName, ConnectString))
        {
                UE_LOG(LogNexusOnline, Error, TEXT("Failed to resolve connection string for '%s'."), *SessionName.ToString());
                OnFailure.Broadcast();
                return;
        }

        if (IOnlineIdentityPtr Identity = Online::GetIdentityInterface(World))
        {
                TSharedPtr<const FUniqueNetId> LocalUserId = Identity->GetUniquePlayerId(0);
                if (LocalUserId.IsValid())
                {
                        Session->RegisterLocalPlayer(*LocalUserId, SessionName, FOnRegisterLocalPlayerCompleteDelegate());
                        const bool bRegisterResult = Session->RegisterPlayer(SessionName, *LocalUserId, /*bWasInvited*/ false);

                        if (!bRegisterResult)
                        {
                                if (FNamedOnlineSession* LocalSession = Session->GetNamedSession(SessionName))
                                {
                                        const TSharedRef<const FUniqueNetId> LocalUserRef = LocalUserId.ToSharedRef();
                                        LocalSession->RegisteredPlayers.AddUnique(LocalUserRef);
                                        UE_LOG(LogNexusOnline, Verbose, TEXT("RegisterPlayer failed after join, manually appended local user."));
                                }
                        }

                        UE_LOG(LogNexusOnline, Log, TEXT("Local player registered to session '%s'. RegisterPlayer result: %s"),
                               *SessionName.ToString(), bRegisterResult ? TEXT("Success") : TEXT("Failure"));
                }
                else
                {
                        UE_LOG(LogNexusOnline, Warning, TEXT("Unable to register local player: invalid unique net id."));
                }
        }
        else
        {
                UE_LOG(LogNexusOnline, Warning, TEXT("Unable to register local player: identity interface unavailable."));
        }

        NexusOnline::BeginTrackingSession(World);
        NexusOnline::BroadcastSessionPlayersChanged(World, DesiredType);

        UE_LOG(LogNexusOnline, Log, TEXT("Connecting to %s"), *ConnectString);

        if (APlayerController* PC = World->GetFirstPlayerController())
        {
                PC->ClientTravel(ConnectString, TRAVEL_Absolute);
        }

        OnSuccess.Broadcast();
}
