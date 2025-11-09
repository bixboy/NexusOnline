#include "Async/AsyncTask_CreateSession.h"

#include "Engine/Engine.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Utils/NexusOnlineHelpers.h"

UAsyncTask_CreateSession* UAsyncTask_CreateSession::CreateSession(UObject* WorldContextObject, const FSessionSettingsData& SettingsData)
{
        UAsyncTask_CreateSession* Node = NewObject<UAsyncTask_CreateSession>();
        Node->WorldContextObject = WorldContextObject;
        Node->Data = SettingsData;

        return Node;
}

void UAsyncTask_CreateSession::Activate()
{
        if (!WorldContextObject)
        {
                UE_LOG(LogNexusOnline, Error, TEXT("CreateSession failed: invalid WorldContextObject."));

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
                UE_LOG(LogNexusOnline, Error, TEXT("CreateSession failed: no session interface available."));

                OnFailure.Broadcast();
                return;
        }

        const FName InternalSessionName = NexusOnline::SessionTypeToName(Data.SessionType);
        if (Session->GetNamedSession(InternalSessionName))
        {
                UE_LOG(LogNexusOnline, Warning, TEXT("Existing session '%s' detected. Destroying before recreation."), *InternalSessionName.ToString());
                Session->DestroySession(InternalSessionName);
        }

        FOnlineSessionSettings Settings;
        Settings.bIsLANMatch = Data.bIsLAN;
        Settings.bShouldAdvertise = !Data.bIsPrivate;
        Settings.bUsesPresence = true;
        Settings.bAllowJoinInProgress = true;
        Settings.NumPublicConnections = Data.MaxPlayers;

        Settings.Set(TEXT("SESSION_DISPLAY_NAME"), Data.SessionName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
        Settings.Set(TEXT("MAP_NAME_KEY"), Data.MapName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
        Settings.Set(TEXT("GAME_MODE_KEY"), Data.GameMode, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
        Settings.Set(TEXT("SESSION_TYPE_KEY"), InternalSessionName.ToString(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
        Settings.Set(TEXT("USES_PRESENCE"), true, EOnlineDataAdvertisementType::ViaOnlineService);

        CreateDelegateHandle = Session->AddOnCreateSessionCompleteDelegate_Handle(
                FOnCreateSessionCompleteDelegate::CreateUObject(this, &UAsyncTask_CreateSession::OnCreateSessionComplete)
        );

        UE_LOG(LogNexusOnline, Log, TEXT("Creating session '%s' (Display: %s)"), *InternalSessionName.ToString(), *Data.SessionName);
        Session->CreateSession(0, InternalSessionName, Settings);
}

void UAsyncTask_CreateSession::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
        UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);

        if (IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World))
        {
                Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateDelegateHandle);
        }

        UE_LOG(LogNexusOnline, Log, TEXT("Session '%s' creation result: %s"), *SessionName.ToString(),
                bWasSuccessful ? TEXT("SUCCESS") : TEXT("FAILURE"));

        if (bWasSuccessful)
        {
                IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);
                IOnlineIdentityPtr Identity = Online::GetIdentityInterface(World);

                if (Session.IsValid() && Identity.IsValid())
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
                                                UE_LOG(LogNexusOnline, Verbose, TEXT("RegisterPlayer failed during creation, manually appended local user."));
                                        }
                                }

                                UE_LOG(LogNexusOnline, Log, TEXT("Local player registered to session. RegisterPlayer result: %s"),
                                       bRegisterResult ? TEXT("Success") : TEXT("Failure"));
                        }
                        else
                        {
                                UE_LOG(LogNexusOnline, Warning, TEXT("Local player registration skipped: invalid unique net id."));
                        }
                }

                NexusOnline::BeginTrackingSession(World);
                NexusOnline::BroadcastSessionPlayersChanged(World, Data.SessionType);
                OnSuccess.Broadcast();
        }
        else
        {
                OnFailure.Broadcast();
        }
}
