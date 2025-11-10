#include "Async/AsyncTask_CreateSession.h"

#include "Data/SessionSearchFilter.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
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
        UE_LOG(LogTemp, Error, TEXT("[NexusOnline|Filter] Invalid WorldContextObject in CreateSession"));
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
        OnFailure.Broadcast();
        return;
    }

    const FName InternalSessionName = NexusOnline::SessionTypeToName(Data.SessionType);
    if (Session->GetNamedSession(InternalSessionName))
    {
        UE_LOG(LogTemp, Warning, TEXT("[NexusOnline|Filter] Existing %s found, destroying before recreation."), *InternalSessionName.ToString());
        Session->DestroySession(InternalSessionName);
    }

    // Création des paramètres
    FOnlineSessionSettings Settings;
    Settings.bIsLANMatch = Data.bIsLAN;
    Settings.bShouldAdvertise = !Data.bIsPrivate;
    Settings.bUsesPresence = true;
    Settings.bAllowJoinInProgress = true;
    Settings.NumPublicConnections = Data.MaxPlayers;

    // Paramètres exposés pour le FindSessions
    Settings.Set("SESSION_DISPLAY_NAME", Data.SessionName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set("MAP_NAME_KEY", Data.MapName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set("GAME_MODE_KEY", Data.GameMode, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set("SESSION_TYPE_KEY", InternalSessionName.ToString(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set("USES_PRESENCE", true, EOnlineDataAdvertisementType::ViaOnlineService);

    for (const FSessionSearchFilter& ExtraSetting : Data.AdditionalSettings)
    {
        if (ExtraSetting.Key.IsNone())
        {
            continue;
        }

        FOnlineSessionSetting Setting;
        Setting.Data = ExtraSetting.ToVariantData();
        Setting.AdvertisementType = EOnlineDataAdvertisementType::ViaOnlineServiceAndPing;
        Settings.Settings.Add(ExtraSetting.Key, Setting);
        UE_LOG(LogTemp, Verbose, TEXT("[NexusOnline|Filter] Added custom session setting %s"), *ExtraSetting.Key.ToString());
    }

    CreateDelegateHandle = Session->AddOnCreateSessionCompleteDelegate_Handle(
        FOnCreateSessionCompleteDelegate::CreateUObject(this, &UAsyncTask_CreateSession::OnCreateSessionComplete));

    UE_LOG(LogTemp, Log, TEXT("[NexusOnline|Filter] Creating %s (Display: %s)"), *InternalSessionName.ToString(), *Data.SessionName);
    Session->CreateSession(0, InternalSessionName, Settings);
}

void UAsyncTask_CreateSession::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);

    if (IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World))
    {
        Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateDelegateHandle);
    }

    UE_LOG(LogTemp, Log, TEXT("[NexusOnline|Filter] Session '%s' creation result: %s"), *SessionName.ToString(),
        bWasSuccessful ? TEXT("SUCCESS") : TEXT("FAILURE"));

    if (bWasSuccessful)
    {
        IOnlineSessionPtr SessionInterface = NexusOnline::GetSessionInterface(World);
        IOnlineIdentityPtr Identity = Online::GetIdentityInterface(World);

        if (SessionInterface.IsValid() && Identity.IsValid())
        {
            TSharedPtr<const FUniqueNetId> LocalUserId = Identity->GetUniquePlayerId(0);
            if (LocalUserId.IsValid())
            {
                SessionInterface->RegisterLocalPlayer(*LocalUserId, SessionName, FOnRegisterLocalPlayerCompleteDelegate());
                UE_LOG(LogTemp, Log, TEXT("[NexusOnline|Filter] Local player registered to session."));
            }
        }

        OnSuccess.Broadcast();
    }
    else
    {
        OnFailure.Broadcast();
    }
}
