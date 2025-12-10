#include "Async/AsyncTask_CreateSession.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Utils/NexusOnlineHelpers.h"
#include "Data/SessionSearchFilter.h"
#include "Filters/SessionFilterRule.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

#define LOCTEXT_NAMESPACE "NexusOnline|CreateSession"

// ──────────────────────────────────────────────
// Create and configure async node
// ──────────────────────────────────────────────
UAsyncTask_CreateSession* UAsyncTask_CreateSession::CreateSession(UObject* WorldContextObject,
	const FSessionSettingsData& SettingsData,
	const TArray<FSessionSearchFilter>& AdditionalSettings,
	USessionFilterPreset* Preset)
{
	UAsyncTask_CreateSession* Node = NewObject<UAsyncTask_CreateSession>();
	Node->WorldContextObject = WorldContextObject;
	Node->Data = SettingsData;
	Node->SessionAdditionalSettings = AdditionalSettings;
	Node->SessionPreset = Preset;
	return Node;
}

// ──────────────────────────────────────────────
// Start the session creation process
// ──────────────────────────────────────────────
void UAsyncTask_CreateSession::Activate()
{
	if (!WorldContextObject)
	{
		UE_LOG(LogNexusOnlineFilter, Error, TEXT("[CreateSession] Invalid WorldContextObject."));
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
		UE_LOG(LogNexusOnlineFilter, Error, TEXT("[CreateSession] Invalid Online Session Interface."));
		OnFailure.Broadcast();
		return;
	}

	const FName InternalSessionName = NexusOnline::SessionTypeToName(Data.SessionType);

	// Destroy existing session
	if (Session->GetNamedSession(InternalSessionName))
	{
		UE_LOG(LogNexusOnlineFilter, Warning, TEXT("[CreateSession] Existing session '%s' found, destroying before recreation."),
			*InternalSessionName.ToString());

		Session->DestroySession(InternalSessionName);
		FPlatformProcess::Sleep(0.25f);
	}

	// ──────────────────────────────────────────────
	// ⚙Configure new session settings
	// ──────────────────────────────────────────────
	FOnlineSessionSettings Settings;

	// Detect NULL subsystem
	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(World);
	const bool bIsNullSubsystem = Subsystem && Subsystem->GetSubsystemName() == TEXT("NULL");
	
	if (bIsNullSubsystem)
	{
		UE_LOG(LogNexusOnlineFilter, Warning, TEXT("[CreateSession] 'NULL' subsystem detected. Forcing LAN match, disabling Lobbies, and minimizing packet size."));
	}

	const bool bIsLAN = bIsNullSubsystem ? true : Data.bIsLAN; // Force LAN on NULL
	const bool bIsPrivate = Data.bIsPrivate;
	const bool bFriendsOnly = Data.bFriendsOnly;
	const int32 MaxPlayers = FMath::Max(1, Data.MaxPlayers);

	Settings.bIsLANMatch = bIsLAN;
	Settings.bUsesPresence = true;
	Settings.bUseLobbiesIfAvailable = bIsNullSubsystem ? false : true; // Disable Lobbies on NULL
	Settings.bAllowJoinInProgress = true;

	Settings.NumPublicConnections = bIsPrivate ? 0 : MaxPlayers;
	Settings.NumPrivateConnections = bIsPrivate ? MaxPlayers : 0;

	// ──────────────────────────────────────────────
	// Access Type Configuration
	// ──────────────────────────────────────────────
	if (bFriendsOnly)
	{
		Settings.bShouldAdvertise = true;
		Settings.bAllowJoinViaPresence = true;
		Settings.bAllowJoinViaPresenceFriendsOnly = true;
		Settings.bAllowInvites = true;
		Settings.Set(TEXT("ACCESS_TYPE"), FString("FRIENDS_ONLY"), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	}
	else if (bIsPrivate)
	{
		Settings.bShouldAdvertise = false;
		Settings.bAllowJoinViaPresence = false;
		Settings.bAllowJoinViaPresenceFriendsOnly = false;
		Settings.bAllowInvites = true;
		Settings.Set(TEXT("ACCESS_TYPE"), FString("PRIVATE"), EOnlineDataAdvertisementType::ViaOnlineService);
	}
	else
	{
		Settings.bShouldAdvertise = true;
		Settings.bAllowJoinViaPresence = true;
		Settings.bAllowJoinViaPresenceFriendsOnly = false;
		Settings.bAllowInvites = true;
		Settings.Set(TEXT("ACCESS_TYPE"), FString("PUBLIC"), EOnlineDataAdvertisementType::ViaOnlineService);
	}

	// ──────────────────────────────────────────────
	// Metadata
	// ──────────────────────────────────────────────
    
	// Optimisation : Pour le subsystem NULL (LAN), on n'envoie dans le Ping QUE ce qui est nécessaire au filtrage.
	// Les infos cosmétiques (Map, Mode) ne seront récupérées qu'après le Join pour économiser la bande passante du Beacon.
	const EOnlineDataAdvertisementType::Type CosmeticAdType = bIsNullSubsystem ? EOnlineDataAdvertisementType::ViaOnlineService : EOnlineDataAdvertisementType::ViaOnlineServiceAndPing;
    
	// CRITIQUE : Ces clés sont utilisées par les filtres (FindSessions). Elles DOIVENT être dans le Ping.
	const EOnlineDataAdvertisementType::Type FilterableAdType = EOnlineDataAdvertisementType::ViaOnlineServiceAndPing;

	// Données cosmétiques (peuvent être cachées du ping en LAN pour optimiser)
	Settings.Set(TEXT("SESSION_DISPLAY_NAME"), Data.SessionName, CosmeticAdType);
	Settings.Set(TEXT("MAP_NAME_KEY"), Data.MapName, CosmeticAdType);
	Settings.Set(TEXT("GAME_MODE_KEY"), Data.GameMode, CosmeticAdType);
	Settings.Set(TEXT("HOST_PLATFORM"), UGameplayStatics::GetPlatformName(), EOnlineDataAdvertisementType::ViaOnlineService); // Pas besoin dans le ping

	// Données de filtrage (OBLIGATOIRES dans le ping)
	Settings.Set(TEXT("SESSION_TYPE_KEY"), NexusOnline::SessionTypeToName(Data.SessionType).ToString(), FilterableAdType);
	Settings.Set(FName("BUILD_VERSION"), 1, FilterableAdType);
	Settings.Set(NexusOnline::SESSION_KEY_PROJECT_ID_INT, NexusOnline::PROJECT_ID_VALUE_INT, FilterableAdType);
	Settings.Set(TEXT("SESSION_ID_KEY"), FString(Data.SessionId), FilterableAdType);
    
	// Toujours via service uniquement
	Settings.Set(TEXT("USES_PRESENCE"), true, EOnlineDataAdvertisementType::ViaOnlineService);
	
	// ──────────────────────────────────────────────
	// Merge user filters and preset filters
	// ──────────────────────────────────────────────
	TArray<FSessionSearchFilter> CombinedSettings = SessionAdditionalSettings;
	if (SessionPreset)
	{
		CombinedSettings.Append(SessionPreset->SimpleFilters);
	}

	if (!CombinedSettings.IsEmpty())
	{
		NexusSessionFilterUtils::ApplyFiltersToSettings(CombinedSettings, Settings);
		UE_LOG(LogNexusOnlineFilter, Log, TEXT("[CreateSession] Applied %d custom session key/value pairs."), CombinedSettings.Num());
	}

	// ──────────────────────────────
	// Generate session ID
	// ──────────────────────────────
	if (Data.SessionId.IsEmpty())
	{
		Data.SessionId = NexusOnline::GenerateRandomSessionId(Data.SessionIdLength);
	}

	Settings.Set(TEXT("SESSION_ID_KEY"), FString(Data.SessionId), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// ──────────────────────────────────────────────
	// Start the session creation
	// ──────────────────────────────────────────────
	CreateDelegateHandle = Session->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UAsyncTask_CreateSession::OnCreateSessionComplete)
	);
	
	UE_LOG(LogNexusOnlineFilter, Log, TEXT("[CreateSession] Creating session '%s' (MaxPlayers=%d, LAN=%d, Private=%d, ID=%s)"),
		*InternalSessionName.ToString(), MaxPlayers, bIsLAN, bIsPrivate, *Data.SessionId);

	if (!Session->CreateSession(0, InternalSessionName, Settings))
	{
		UE_LOG(LogNexusOnlineFilter, Error, TEXT("[CreateSession] CreateSession() failed immediately!"));
		OnFailure.Broadcast();
	}
}

// ──────────────────────────────────────────────
// OnCreateSessionComplete
// ──────────────────────────────────────────────

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
		UE_LOG(LogNexusOnlineFilter, Error, TEXT("[CreateSession] Session creation failed (%s)."), *SessionName.ToString());
		OnFailure.Broadcast();
		return;
	}

	UE_LOG(LogNexusOnlineFilter, Log, TEXT("[CreateSession] ✅ Session '%s' created successfully."), *SessionName.ToString());
    
	if (Session.IsValid())
	{
		Session->StartSession(SessionName);
	}

	OnSuccess.Broadcast();

	if (World)
	{
		UGameplayStatics::OpenLevel(World, FName(*Data.MapName), true, TEXT("listen"));
	}
}

#undef LOCTEXT_NAMESPACE
