#include "TestDebugs/WBP_NexusOnlineDebug.h"
#include "Async/AsyncTask_CreateSession.h"
#include "Async/AsyncTask_DestroySession.h"
#include "Async/AsyncTask_FindSessions.h"
#include "Async/AsyncTask_JoinSession.h"
#include "Filters/Rules/SessionFilterRule_Ping.h"
#include "Kismet/GameplayStatics.h"
#include "Managers/OnlineSessionManager.h"
#include "Utils/NexusOnlineHelpers.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/NexusMigrationSubsystem.h" 

#define LOCTEXT_NAMESPACE "NexusOnline|DebugWidget"


void UWBP_NexusOnlineDebug::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Create)
		Button_Create->OnClicked.AddDynamic(this, &UWBP_NexusOnlineDebug::OnCreateClicked);
	
	if (Button_Join)
		Button_Join->OnClicked.AddDynamic(this, &UWBP_NexusOnlineDebug::OnJoinClicked);
	
	if (Button_Leave)
		Button_Leave->OnClicked.AddDynamic(this, &UWBP_NexusOnlineDebug::OnLeaveClicked);

	UpdateSessionDisplay();
	TryBindToSessionManager();
}

void UWBP_NexusOnlineDebug::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	// Auto-Reset Binding if Manager died (Level Travel)
	if (bIsBoundToManager && !CachedManager.IsValid())
	{
		bIsBoundToManager = false;
	}

	if (!bIsBoundToManager)
	{
		TryBindToSessionManager();
	}
}

// ──────────────────────────────────────────────
// CREATE
// ──────────────────────────────────────────────
void UWBP_NexusOnlineDebug::OnCreateClicked()
{
	FSessionSettingsData Settings;
	Settings.SessionName = TEXT("Nexus Demo Session");
	Settings.SessionType = ENexusSessionType::GameSession;
	Settings.MaxPlayers = 5;
	Settings.bIsPrivate = false;
	Settings.bIsLAN = IsLan;
	Settings.MapName = MapName;
	Settings.GameMode = TEXT("Default");
	Settings.SessionIdLength = 10;
	
	// ENABLE MIGRATION TEST
	Settings.bAllowHostMigration = true;
	Settings.MigrationSessionID = NexusOnline::GenerateRandomSessionId(16);

	// Cache Settings for Migration Subsystem
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UNexusMigrationSubsystem* MigSub = GI->GetSubsystem<UNexusMigrationSubsystem>())
		{
			MigSub->CacheSessionSettings(Settings);
		}
	}

	FSessionSearchFilter BuildFilter;
	BuildFilter.Key = FName("BUILD_VERSION");
	BuildFilter.Value.Type = ENexusSessionFilterValueType::Int32;
	BuildFilter.Value.IntValue = 1;

	TArray<FSessionSearchFilter> ExtraSettings;
	ExtraSettings.Add(BuildFilter);

	UE_LOG(LogTemp, Log, TEXT("[DebugWidget] Starting CreateSession Task..."));

	UAsyncTask_CreateSession* Task = UAsyncTask_CreateSession::CreateSession(
	   this, 
	   Settings, 
	   ExtraSettings,
	   true, 
	   nullptr
	);

	Task->OnSuccess.AddDynamic(this, &UWBP_NexusOnlineDebug::OnCreateSuccess);
	Task->OnFailure.AddDynamic(this, &UWBP_NexusOnlineDebug::OnCreateFailure);
	Task->Activate();
}

void UWBP_NexusOnlineDebug::OnCreateSuccess()
{
	UE_LOG(LogTemp, Log, TEXT("[DebugWidget] ✅ Session Created!"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Session Created! Loading Map..."));
}

void UWBP_NexusOnlineDebug::OnCreateFailure()
{
	UE_LOG(LogTemp, Error, TEXT("[DebugWidget] ❌ Creation Failed!"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Creation Failed!"));
}

// ──────────────────────────────────────────────
// JOIN
// ──────────────────────────────────────────────
void UWBP_NexusOnlineDebug::OnJoinClicked()
{
	FSessionSearchFilter BuildFilter;
	BuildFilter.Key = FName("BUILD_VERSION");
	BuildFilter.Value.Type = ENexusSessionFilterValueType::Int32;
	BuildFilter.Value.IntValue = 1;
	BuildFilter.ComparisonOp = ENexusSessionComparisonOp::Equals;

	TArray<FSessionSearchFilter> SimpleFilters;
	SimpleFilters.Add(BuildFilter);

	USessionFilterRule_Ping* PingRule = NewObject<USessionFilterRule_Ping>(this);
	PingRule->MaxPing = 200; 
	
	UE_LOG(LogTemp, Log, TEXT("[DebugWidget] Searching sessions..."));
	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("Searching..."));

	UAsyncTask_FindSessions* Task = UAsyncTask_FindSessions::FindSessions(
		this,
		ENexusSessionType::GameSession,
		20,
		IsLan,
		SimpleFilters,
		{ PingRule },
		{ },
		nullptr
	);

	Task->OnCompleted.AddDynamic(this, &UWBP_NexusOnlineDebug::OnFindSessionsCompleted);
	Task->Activate();
}

void UWBP_NexusOnlineDebug::OnFindSessionsCompleted(bool bWasSuccessful, const TArray<FOnlineSessionSearchResultData>& Results)
{
	if (!bWasSuccessful || Results.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[DebugWidget] No sessions found."));
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, TEXT("No sessions found."));
		return;
	}

	const FOnlineSessionSearchResultData& Target = Results[0];

	UE_LOG(LogTemp, Log, TEXT("[DebugWidget] Found %d. Joining '%s' (%d/%d)..."),
		Results.Num(), *Target.SessionDisplayName, Target.CurrentPlayers, Target.MaxPlayers);

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::Printf(TEXT("Joining %s..."), *Target.SessionDisplayName));

	UAsyncTask_JoinSession* JoinTask = UAsyncTask_JoinSession::JoinSession(
		this, 
		Target, 
		true,
		ENexusSessionType::GameSession
	);
	
	JoinTask->OnSuccess.AddDynamic(this, &UWBP_NexusOnlineDebug::OnJoinSuccess);
	JoinTask->OnFailure.AddDynamic(this, &UWBP_NexusOnlineDebug::OnJoinFailure);
	JoinTask->Activate();
}

void UWBP_NexusOnlineDebug::OnJoinSuccess()
{
	UE_LOG(LogTemp, Log, TEXT("[DebugWidget] ✅ Join Success!"));
	
	// Cache Information for Migration
	if (IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(GetWorld()))
	{
		if (FNamedOnlineSession* Active = Session->GetNamedSession(NAME_GameSession))
		{
			FString MigrationId;
			if (Active->SessionSettings.Get(TEXT("MIGRATION_ID_KEY"), MigrationId))
			{
				FSessionSettingsData ClientSettings;
				ClientSettings.SessionType = ENexusSessionType::GameSession;
				ClientSettings.bAllowHostMigration = true;
				ClientSettings.MigrationSessionID = MigrationId;
				
				// Restore Settings
				FString DisplayName;
				if (Active->SessionSettings.Get(TEXT("SESSION_DISPLAY_NAME"), DisplayName))
				{
					ClientSettings.SessionName = DisplayName;
				}

				Active->SessionSettings.Get(TEXT("MAP_NAME_KEY"), ClientSettings.MapName);
				Active->SessionSettings.Get(TEXT("GAME_MODE_KEY"), ClientSettings.GameMode);
				ClientSettings.MaxPlayers = Active->SessionSettings.NumPublicConnections;

				if (UGameInstance* GI = GetGameInstance())
				{
					if (UNexusMigrationSubsystem* MigSub = GI->GetSubsystem<UNexusMigrationSubsystem>())
					{
						MigSub->CacheSessionSettings(ClientSettings);
						UE_LOG(LogTemp, Log, TEXT("[DebugWidget] Migration Info Cached on Client. Map: %s"), *ClientSettings.MapName);
					}
				}
			}
		}
	}
}

void UWBP_NexusOnlineDebug::OnJoinFailure()
{
	UE_LOG(LogTemp, Error, TEXT("[DebugWidget] ❌ Join Failed!"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Join Failed!"));
}

// ──────────────────────────────────────────────
// LEAVE
// ──────────────────────────────────────────────
void UWBP_NexusOnlineDebug::OnLeaveClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UNexusMigrationSubsystem* MigSub = GI->GetSubsystem<UNexusMigrationSubsystem>())
		{
			MigSub->SetIntentionalLeave(true);
		}
	}

	UAsyncTask_DestroySession* Task = UAsyncTask_DestroySession::DestroySession(this, ENexusSessionType::GameSession);
	Task->OnSuccess.AddDynamic(this, &UWBP_NexusOnlineDebug::OnLeaveSuccess);
	Task->OnFailure.AddDynamic(this, &UWBP_NexusOnlineDebug::OnLeaveFailure);
	Task->Activate();
}

void UWBP_NexusOnlineDebug::OnLeaveSuccess()
{
	UE_LOG(LogTemp, Log, TEXT("[DebugWidget] Left session. returning to MainMenu..."));
	UGameplayStatics::OpenLevel(this, FName(MapName));
}

void UWBP_NexusOnlineDebug::OnLeaveFailure()
{
	UE_LOG(LogTemp, Error, TEXT("[DebugWidget] Failed to destroy session locally."));
	UGameplayStatics::OpenLevel(this, FName(MapName));
}

// ──────────────────────────────────────────────
// DISPLAY & MANAGER BINDING
// ──────────────────────────────────────────────

void UWBP_NexusOnlineDebug::TryBindToSessionManager()
{
	if (AOnlineSessionManager* Manager = AOnlineSessionManager::Get(this))
	{
		if (!Manager->OnPlayerCountChanged.IsAlreadyBound(this, &UWBP_NexusOnlineDebug::OnPlayerCountChanged))
		{
			Manager->OnPlayerCountChanged.AddDynamic(this, &UWBP_NexusOnlineDebug::OnPlayerCountChanged);
		}
		
		bIsBoundToManager = true;
		CachedManager = Manager;
		UpdateSessionDisplay();
	}
}

void UWBP_NexusOnlineDebug::OnPlayerCountChanged(int32 NewCount)
{
	UpdateSessionDisplay();
}

void UWBP_NexusOnlineDebug::UpdateSessionDisplay()
{
	FString StatusText = TEXT("Status: Offline / No Session");

	if (AOnlineSessionManager* Manager = AOnlineSessionManager::Get(this))
	{
		StatusText = FString::Printf(TEXT("Session: %s | Players: %d"), *Manager->TrackedSessionName.ToString(), Manager->PlayerCount);
	}
	else if (IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(GetWorld()))
	{
		if (FNamedOnlineSession* Active = Session->GetNamedSession(NAME_GameSession))
		{
			StatusText = FString::Printf(TEXT("Session: Active | Public: %d | Open: %d"),
				Active->SessionSettings.NumPublicConnections, Active->NumOpenPublicConnections);
		}
	}

	if (Text_SessionInfo)
		Text_SessionInfo->SetText(FText::FromString(StatusText));

	if (Text_SessionId)
	{
		FString SessionIdStr = TEXT("ID: Unknown");
		if (IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(GetWorld()))
		{
			FName TargetSessionName = NAME_GameSession;
			if (AOnlineSessionManager* Manager = AOnlineSessionManager::Get(this))
			{
				TargetSessionName = Manager->TrackedSessionName;
			}

			if (FNamedOnlineSession* Active = Session->GetNamedSession(TargetSessionName))
			{
				SessionIdStr = FString::Printf(TEXT("ID: %s"), *Active->GetSessionIdStr());
			}
		}
		
		Text_SessionId->SetText(FText::FromString(SessionIdStr));
	}
}

#undef LOCTEXT_NAMESPACE