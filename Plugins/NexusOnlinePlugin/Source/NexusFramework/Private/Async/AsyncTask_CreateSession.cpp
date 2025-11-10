#include "Async/AsyncTask_CreateSession.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Utils/NexusOnlineHelpers.h"
#include "Data/SessionSearchFilter.h"
#include "Filters/SessionFilterRule.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"


UAsyncTask_CreateSession* UAsyncTask_CreateSession::CreateSession(UObject* WorldContextObject, const FSessionSettingsData& SettingsData, const TArray<FSessionSearchFilter>& AdditionalSettings, USessionFilterPreset* Preset)
{
    UAsyncTask_CreateSession* Node = NewObject<UAsyncTask_CreateSession>();
    Node->WorldContextObject = WorldContextObject;
    Node->Data = SettingsData;
    Node->SessionAdditionalSettings = AdditionalSettings;
    Node->SessionPreset = Preset;

    return Node;
}

void UAsyncTask_CreateSession::Activate()
{
    if (!WorldContextObject)
    {
        UE_LOG(LogNexusOnlineFilter, Error, TEXT("[NexusOnline|Filter] Invalid WorldContextObject in CreateSession"));
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
        UE_LOG(LogNexusOnlineFilter, Warning, TEXT("[NexusOnline|Filter] Existing %s found, destroying before recreation."), *InternalSessionName.ToString());
        Session->DestroySession(InternalSessionName);

        FPlatformProcess::Sleep(0.5f);
    }

    FOnlineSessionSettings Settings;
    Settings.bIsLANMatch = Data.bIsLAN;
    Settings.bShouldAdvertise = !Data.bIsPrivate;
    Settings.bUsesPresence = true;
    Settings.bUseLobbiesIfAvailable = true;
    Settings.bAllowJoinInProgress = true;
    Settings.NumPublicConnections = Data.MaxPlayers;

    Settings.Set("SESSION_DISPLAY_NAME", Data.SessionName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set("MAP_NAME_KEY", Data.MapName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set("GAME_MODE_KEY", Data.GameMode, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set("SESSION_TYPE_KEY", NexusOnline::SessionTypeToName(Data.SessionType).ToString(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set("USES_PRESENCE", true, EOnlineDataAdvertisementType::ViaOnlineService);

    TArray<FSessionSearchFilter> CombinedSettings = SessionAdditionalSettings;
    if (SessionPreset)
    {
        CombinedSettings.Append(SessionPreset->SimpleFilters);
    }

    if (!CombinedSettings.IsEmpty())
    {
        NexusSessionFilterUtils::ApplyFiltersToSettings(CombinedSettings, Settings);
        UE_LOG(LogNexusOnlineFilter, Log, TEXT("[NexusOnline|Filter] Applied %d custom session key/value pairs."), CombinedSettings.Num());
    }

    CreateDelegateHandle = Session->AddOnCreateSessionCompleteDelegate_Handle
        (
        FOnCreateSessionCompleteDelegate::CreateUObject(this, &UAsyncTask_CreateSession::OnCreateSessionComplete)
    );

    UE_LOG(LogNexusOnlineFilter, Log, TEXT("[NexusOnline|Filter] Creating %s (Display: %s)"), *InternalSessionName.ToString(), *Data.SessionName);

    if (!Session->CreateSession(0, InternalSessionName, Settings))
    {
        UE_LOG(LogNexusOnlineFilter, Error, TEXT("[NexusOnline|Filter] CreateSession() failed immediately!"));
        OnFailure.Broadcast();
    }
}

void UAsyncTask_CreateSession::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
    IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);
	
    if (Session.IsValid())
    {
        Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateDelegateHandle);
    }
    else
    {
    	OnFailure.Broadcast();
        return;
    }

    if (!bWasSuccessful)
    {
        OnFailure.Broadcast();
        return;
    }

    OnSuccess.Broadcast();
    UGameplayStatics::OpenLevel(World, FName(*Data.MapName), true, TEXT("listen"));
}

