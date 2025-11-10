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


UAsyncTask_CreateSession* UAsyncTask_CreateSession::CreateSession(UObject* WorldContextObject, const FSessionSettingsData& SettingsData,
	const TArray<FSessionSearchFilter>& AdditionalSettings, USessionFilterPreset* Preset)
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
        UE_LOG(LogNexusOnlineFilter, Error, TEXT("[NexusOnline|Filter] Invalid Online Session Interface"));
        OnFailure.Broadcast();
        return;
    }

    const FName InternalSessionName = NexusOnline::SessionTypeToName(Data.SessionType);

    // Détruire l'ancienne session si elle existe
    if (Session->GetNamedSession(InternalSessionName))
    {
        UE_LOG(LogNexusOnlineFilter, Warning, TEXT("[NexusOnline|Filter] Existing session %s found, destroying before recreation."), *InternalSessionName.ToString());
        Session->DestroySession(InternalSessionName);
        FPlatformProcess::Sleep(0.25f);
    }

    // CONFIGURATION DE LA SESSION
    // ──────────────────────────────────────────────
    FOnlineSessionSettings Settings;

	const bool bIsLAN = Data.bIsLAN;
	const bool bIsPrivate = Data.bIsPrivate;
	const bool bFriendsOnly = Data.bFriendsOnly;
	const int32 MaxPlayers = FMath::Max(1, Data.MaxPlayers);
	
	Settings.bIsLANMatch = bIsLAN;
	Settings.bUsesPresence = true;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.bAllowJoinInProgress = true;
	Settings.bAllowInvites = true;

	Settings.NumPublicConnections = bIsPrivate ? 0 : MaxPlayers;
	Settings.NumPrivateConnections = bIsPrivate ? MaxPlayers : 0;
	
	if (bFriendsOnly)
	{
		// Friends-only mode
		Settings.bShouldAdvertise = true;
		Settings.bAllowJoinViaPresence = true;
		Settings.bAllowJoinViaPresenceFriendsOnly = true;
		Settings.bAllowInvites = true;
		Settings.Set(TEXT("ACCESS_TYPE"), FString(TEXT("FRIENDS_ONLY")), EOnlineDataAdvertisementType::ViaOnlineService);
	}
	else if (bIsPrivate)
	{
		// Private (invitation only)
		Settings.bShouldAdvertise = false;
		Settings.bAllowJoinViaPresence = false;
		Settings.bAllowJoinViaPresenceFriendsOnly = false;
		Settings.bAllowInvites = true;
		Settings.Set(TEXT("ACCESS_TYPE"), FString(TEXT("PRIVATE")), EOnlineDataAdvertisementType::ViaOnlineService);
	}
	else
	{
		// Public session
		Settings.bShouldAdvertise = true;
		Settings.bAllowJoinViaPresence = true;
		Settings.bAllowJoinViaPresenceFriendsOnly = false;
		Settings.bAllowInvites = true;
		Settings.Set(TEXT("ACCESS_TYPE"), FString(TEXT("PUBLIC")), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	}

    // MÉTADONNÉES
    // ──────────────────────────────────────────────
    Settings.Set(TEXT("SESSION_DISPLAY_NAME"), Data.SessionName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set(TEXT("MAP_NAME_KEY"), Data.MapName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set(TEXT("GAME_MODE_KEY"), Data.GameMode, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set(TEXT("SESSION_TYPE_KEY"), NexusOnline::SessionTypeToName(Data.SessionType).ToString(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set(TEXT("USES_PRESENCE"), true, EOnlineDataAdvertisementType::ViaOnlineService);

    // ID de l’hôte
    FString HostName = UGameplayStatics::GetPlatformName();
    Settings.Set(TEXT("HOST_PLATFORM"), HostName, EOnlineDataAdvertisementType::ViaOnlineService);

	
    // APPLICATION DES FILTRES / PRÉSETS
    // ──────────────────────────────────────────────
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
	

    // CRÉATION DE LA SESSION
    // ──────────────────────────────────────────────
    CreateDelegateHandle = Session->AddOnCreateSessionCompleteDelegate_Handle
	(
        FOnCreateSessionCompleteDelegate::CreateUObject(this, &UAsyncTask_CreateSession::OnCreateSessionComplete)
    );

    UE_LOG(LogNexusOnlineFilter, Log, TEXT("[NexusOnline|Filter] Creating session '%s' (%d max players, LAN=%d, Private=%d)"),
        *InternalSessionName.ToString(), MaxPlayers, bIsLAN, bIsPrivate);

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

