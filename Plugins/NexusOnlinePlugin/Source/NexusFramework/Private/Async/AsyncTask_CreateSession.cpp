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
// Factory Function
// ──────────────────────────────────────────────
UAsyncTask_CreateSession* UAsyncTask_CreateSession::CreateSession(UObject* WorldContextObject, const FSessionSettingsData& SettingsData,
	const TArray<FSessionSearchFilter>& AdditionalSettings, bool bAutoTravel, USessionFilterPreset* Preset)
{
	UAsyncTask_CreateSession* Node = NewObject<UAsyncTask_CreateSession>();
	Node->WorldContextObject = WorldContextObject;
	Node->Data = SettingsData;
	Node->bShouldAutoTravel = bAutoTravel;
	Node->SessionAdditionalSettings = AdditionalSettings;
	Node->SessionPreset = Preset;
	
	return Node;
}

// ──────────────────────────────────────────────
// Entry Point (Activate)
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
	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);
	if (!Session.IsValid())
	{
		UE_LOG(LogNexusOnlineFilter, Error, TEXT("[CreateSession] Invalid Online Session Interface."));
		OnFailure.Broadcast();
		return;
	}

	const FName InternalSessionName = NexusOnline::SessionTypeToName(Data.SessionType);

	// 1. VÉRIFICATION
	if (Session->GetNamedSession(InternalSessionName))
	{
		UE_LOG(LogNexusOnlineFilter, Warning, TEXT("[CreateSession] Existing session '%s' found. Destroying asynchronously..."), *InternalSessionName.ToString());

		DestroyDelegateHandle = Session->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this,
			&UAsyncTask_CreateSession::OnOldSessionDestroyed));

		if (!Session->DestroySession(InternalSessionName))
		{
			Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroyDelegateHandle);
			OnFailure.Broadcast();
		}
		
		return; 
	}

	// 2. Si aucune session n'existe, on passe directement à la création
	CreateSessionInternal();
}

// ──────────────────────────────────────────────
// Step 1.5: Old Session Destroyed
// ──────────────────────────────────────────────
void UAsyncTask_CreateSession::OnOldSessionDestroyed(FName SessionName, bool bWasSuccessful)
{
	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);
	
	if (Session.IsValid())
	{
		Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroyDelegateHandle);
	}

	if (bWasSuccessful)
	{
		UE_LOG(LogNexusOnlineFilter, Log, TEXT("[CreateSession] Old session destroyed. Proceeding to creation."));
		CreateSessionInternal();
	}
	else
	{
		UE_LOG(LogNexusOnlineFilter, Error, TEXT("[CreateSession] Failed to destroy old session. Cannot create new one safely."));
		OnFailure.Broadcast();
	}
}

// ──────────────────────────────────────────────
// Step 2: Internal Creation Logic
// ──────────────────────────────────────────────
void UAsyncTask_CreateSession::CreateSessionInternal()
{
	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);
	if (!Session.IsValid()) 
	{
		OnFailure.Broadcast();
		return;
	}

	const FName InternalSessionName = NexusOnline::SessionTypeToName(Data.SessionType);

	// --- Configuration des Settings ---
	FOnlineSessionSettings Settings;

	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(World);
	const bool bIsNullSubsystem = Subsystem && Subsystem->GetSubsystemName() == TEXT("NULL");
	
	if (bIsNullSubsystem)
	{
		UE_LOG(LogNexusOnlineFilter, Warning, TEXT("[CreateSession] 'NULL' subsystem detected. Forcing LAN match."));
	}

	const bool bIsLAN = bIsNullSubsystem ? true : Data.bIsLAN;
	const bool bIsPrivate = Data.bIsPrivate;
	const bool bFriendsOnly = Data.bFriendsOnly;
	const int32 MaxPlayers = FMath::Max(1, Data.MaxPlayers);

	Settings.bIsLANMatch = bIsLAN;
	Settings.bUsesPresence = true;
	Settings.bUseLobbiesIfAvailable = !bIsNullSubsystem; 
	Settings.bAllowJoinInProgress = true;
	Settings.NumPublicConnections = bIsPrivate ? 0 : MaxPlayers;
	Settings.NumPrivateConnections = bIsPrivate ? MaxPlayers : 0;

	// Access Type
	if (bFriendsOnly)
	{
		Settings.Set(TEXT("ACCESS_TYPE"), FString("FRIENDS_ONLY"), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
		Settings.bAllowJoinViaPresenceFriendsOnly = true;
	}
	else if (bIsPrivate)
	{
		Settings.Set(TEXT("ACCESS_TYPE"), FString("PRIVATE"), EOnlineDataAdvertisementType::ViaOnlineService);
	}
	else
	{
		Settings.Set(TEXT("ACCESS_TYPE"), FString("PUBLIC"), EOnlineDataAdvertisementType::ViaOnlineService);
	}
	Settings.bShouldAdvertise = !bIsPrivate;
	Settings.bAllowJoinViaPresence = true;
	Settings.bAllowInvites = true;

	// Metadata Optimisation (Ping vs Service)
	const auto CosmeticType = bIsNullSubsystem ? EOnlineDataAdvertisementType::ViaOnlineService : EOnlineDataAdvertisementType::ViaOnlineServiceAndPing;
	const auto FilterType = EOnlineDataAdvertisementType::ViaOnlineServiceAndPing;

	Settings.Set(TEXT("SESSION_DISPLAY_NAME"), Data.SessionName, CosmeticType);
	Settings.Set(TEXT("MAP_NAME_KEY"), Data.MapName, CosmeticType);
	Settings.Set(TEXT("GAME_MODE_KEY"), Data.GameMode, CosmeticType);
	Settings.Set(TEXT("HOST_PLATFORM"), UGameplayStatics::GetPlatformName(), EOnlineDataAdvertisementType::ViaOnlineService);

	// Core Data
	Settings.Set(TEXT("SESSION_TYPE_KEY"), NexusOnline::SessionTypeToName(Data.SessionType).ToString(), FilterType);
	Settings.Set(FName("BUILD_VERSION"), 1, FilterType);
	Settings.Set(NexusOnline::SESSION_KEY_PROJECT_ID_INT, NexusOnline::PROJECT_ID_VALUE_INT, FilterType);
    
	if (Data.SessionId.IsEmpty())
		Data.SessionId = NexusOnline::GenerateRandomSessionId(Data.SessionIdLength);
	
	Settings.Set(TEXT("SESSION_ID_KEY"), FString(Data.SessionId), FilterType);
	
	if (!Data.MigrationSessionID.IsEmpty())
	{
		Settings.Set(TEXT("MIGRATION_ID_KEY"), Data.MigrationSessionID, FilterType);
	}

	Settings.Set(TEXT("USES_PRESENCE"), true, EOnlineDataAdvertisementType::ViaOnlineService);
	
	TArray<FSessionSearchFilter> CombinedSettings = SessionAdditionalSettings;
	if (SessionPreset)
		CombinedSettings.Append(SessionPreset->SimpleFilters);
	
	if (!CombinedSettings.IsEmpty())
		NexusSessionFilterUtils::ApplyFiltersToSettings(CombinedSettings, Settings);

	// Launch 
	CreateDelegateHandle = Session->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this,
		&UAsyncTask_CreateSession::OnCreateSessionComplete));
	
	UE_LOG(LogNexusOnlineFilter, Log, TEXT("[CreateSession] Creating session '%s'..."), *InternalSessionName.ToString());

	if (!Session->CreateSession(0, InternalSessionName, Settings))
	{
		Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateDelegateHandle);
		OnFailure.Broadcast();
	}
}

// ──────────────────────────────────────────────
// Step 3: Completion
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
		UE_LOG(LogNexusOnlineFilter, Error, TEXT("[CreateSession] Failed to create session."));
		OnFailure.Broadcast();
		return;
	}

	if (Session.IsValid())
	{
		Session->StartSession(SessionName);
	}

	UE_LOG(LogNexusOnlineFilter, Log, TEXT("[CreateSession] Success. AutoTravel = %s"), bShouldAutoTravel ? TEXT("TRUE") : TEXT("FALSE"));
	
	OnSuccess.Broadcast();

	if (bShouldAutoTravel && World && !Data.MapName.IsEmpty())
	{
		UGameplayStatics::OpenLevel(World, FName(*Data.MapName), true, TEXT("listen"));
	}
}
#undef LOCTEXT_NAMESPACE