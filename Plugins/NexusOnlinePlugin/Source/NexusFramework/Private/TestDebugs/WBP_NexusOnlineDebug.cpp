#include "TestDebugs/WBP_NexusOnlineDebug.h"
#include "Async/AsyncTask_CreateSession.h"
#include "Async/AsyncTask_DestroySession.h"
#include "Async/AsyncTask_FindSessions.h"
#include "Async/AsyncTask_JoinSession.h"
#include "Filters/Rules/SessionFilterRule_Ping.h"
#include "Filters/Rules/SessionSortRule_Ping.h"
#include "Kismet/GameplayStatics.h"
#include "Managers/OnlineSessionManager.h"
#include "Utils/NexusOnlineHelpers.h"
#include "Utils/NexusSteamUtils.h"

#define LOCTEXT_NAMESPACE "NexusOnline|DebugWidget"

// ──────────────────────────────────────────────
// Initialization
// ──────────────────────────────────────────────
void UWBP_NexusOnlineDebug::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Create)
		Button_Create->OnClicked.AddDynamic(this, &UWBP_NexusOnlineDebug::OnCreateClicked);

	if (Button_Join)
		Button_Join->OnClicked.AddDynamic(this, &UWBP_NexusOnlineDebug::OnJoinClicked);

	if (Button_Leave)
		Button_Leave->OnClicked.AddDynamic(this, &UWBP_NexusOnlineDebug::OnLeaveClicked);

	UpdateSessionInfo();
}

// ──────────────────────────────────────────────
// CREATE SESSION
// ──────────────────────────────────────────────
void UWBP_NexusOnlineDebug::OnCreateClicked()
{
	FSessionSettingsData Settings;
	Settings.SessionName = TEXT("DebugSession");
	Settings.SessionType = ENexusSessionType::GameSession;
	Settings.MaxPlayers = 2;
	Settings.bIsPrivate = false;
	Settings.bIsLAN = true;
	Settings.MapName = TEXT("TestMap");
	Settings.GameMode = TEXT("Default");
	Settings.SessionIdLength = 15;

	// Region filter
	FSessionSearchFilter RegionFilter;
	RegionFilter.Key = FName("REGION_KEY");
	RegionFilter.Value.Type = ENexusSessionFilterValueType::String;
	RegionFilter.Value.StringValue = TEXT("EU");
	RegionFilter.ComparisonOp = ENexusSessionComparisonOp::Equals;

	// Build version filter
	FSessionSearchFilter BuildFilter;
	BuildFilter.Key = FName("BUILD_VERSION");
	BuildFilter.Value.Type = ENexusSessionFilterValueType::Int32;
	BuildFilter.Value.IntValue = 1;

	TArray ExtraSettings = { RegionFilter, BuildFilter };

	// Launch async task
	UAsyncTask_CreateSession* Task = UAsyncTask_CreateSession::CreateSession(GetOwningPlayer(), Settings, ExtraSettings, nullptr);
	Task->OnSuccess.AddDynamic(this, &UWBP_NexusOnlineDebug::OnCreateSuccess);
	Task->OnFailure.AddDynamic(this, &UWBP_NexusOnlineDebug::OnCreateFailure);
	Task->Activate();

	UE_LOG(LogTemp, Log, TEXT("[DebugWidget] Creating session..."));
}

void UWBP_NexusOnlineDebug::OnCreateSuccess()
{
	UE_LOG(LogTemp, Log, TEXT("[DebugWidget] ✅ Session created successfully!"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("[DebugWidget] Session created successfully!"));

	UpdateSessionInfo();
	UNexusSteamUtils::ShowInviteOverlay(this, "GameSession");
}

void UWBP_NexusOnlineDebug::OnCreateFailure()
{
	UE_LOG(LogTemp, Error, TEXT("[DebugWidget] ❌ Failed to create session!"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("[DebugWidget] Failed to create session!"));
}

// ──────────────────────────────────────────────
// FIND & JOIN SESSION
// ──────────────────────────────────────────────
void UWBP_NexusOnlineDebug::OnJoinClicked()
{
	// Basic filters
	FSessionSearchFilter RegionFilter;
	RegionFilter.Key = FName("REGION_KEY");
	RegionFilter.Value.Type = ENexusSessionFilterValueType::String;
	RegionFilter.Value.StringValue = TEXT("EU");
	RegionFilter.ComparisonOp = ENexusSessionComparisonOp::Equals;

	FSessionSearchFilter BuildFilter;
	BuildFilter.Key = FName("BUILD_VERSION");
	BuildFilter.Value.Type = ENexusSessionFilterValueType::Int32;
	BuildFilter.Value.IntValue = 1;
	BuildFilter.ComparisonOp = ENexusSessionComparisonOp::Equals;

	TArray SimpleFilters = { RegionFilter, BuildFilter };

	// Advanced ping rule (exclude >150ms)
	USessionFilterRule_Ping* PingRule = NewObject<USessionFilterRule_Ping>(this);
	PingRule->MaxPing = 150;

	// Sort sessions by lowest ping first
	USessionSortRule_Ping* SortByPing = NewObject<USessionSortRule_Ping>(this);

	// Launch search
	UAsyncTask_FindSessions* Task = UAsyncTask_FindSessions::FindSessions(
		GetOwningPlayer(),
		ENexusSessionType::GameSession,
		20,
		SimpleFilters,
		{ PingRule },
		{ SortByPing },
		nullptr
	);

	Task->OnCompleted.AddDynamic(this, &UWBP_NexusOnlineDebug::OnFindSessionsCompleted);
	Task->Activate();

	UE_LOG(LogTemp, Log, TEXT("[DebugWidget] Searching for sessions (ping < 150)..."));
}

void UWBP_NexusOnlineDebug::OnFindSessionsCompleted(bool bWasSuccessful, const TArray<FOnlineSessionSearchResultData>& Results)
{
	if (!bWasSuccessful || Results.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[DebugWidget] ❌ No sessions found."));
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, TEXT("[DebugWidget] No sessions found."));
		
		return;
	}

	FoundSessions = Results;
	const FOnlineSessionSearchResultData& First = Results[0];

	UE_LOG(LogTemp, Log, TEXT("[DebugWidget] Found %d session(s). Joining '%s'..."), Results.Num(), *First.SessionDisplayName);

	UAsyncTask_JoinSession* JoinTask = UAsyncTask_JoinSession::JoinSession(GetOwningPlayer(), First, ENexusSessionType::GameSession);
	JoinTask->OnSuccess.AddDynamic(this, &UWBP_NexusOnlineDebug::OnJoinSuccess);
	JoinTask->OnFailure.AddDynamic(this, &UWBP_NexusOnlineDebug::OnJoinFailure);
	JoinTask->Activate();

	CurrentSessionName = First.SessionDisplayName;
	CurrentPlayers = First.CurrentPlayers;
	MaxPlayers = First.MaxPlayers;
}

void UWBP_NexusOnlineDebug::OnJoinSuccess()
{
	UE_LOG(LogTemp, Log, TEXT("[DebugWidget] ✅ Joined session successfully!"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("[DebugWidget] Joined session successfully!"));
	UpdateSessionInfo();
}

void UWBP_NexusOnlineDebug::OnJoinFailure()
{
	UE_LOG(LogTemp, Error, TEXT("[DebugWidget] ❌ Failed to join session!"));
	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("[DebugWidget] Failed to join session!"));
}

// ──────────────────────────────────────────────
// LEAVE / DESTROY SESSION
// ──────────────────────────────────────────────

void UWBP_NexusOnlineDebug::OnLeaveClicked()
{
	UAsyncTask_DestroySession* Task = UAsyncTask_DestroySession::DestroySession(GetOwningPlayer(), ENexusSessionType::GameSession);
	Task->OnSuccess.AddDynamic(this, &UWBP_NexusOnlineDebug::OnLeaveSuccess);
	Task->OnFailure.AddDynamic(this, &UWBP_NexusOnlineDebug::OnLeaveFailure);
	Task->Activate();
}

void UWBP_NexusOnlineDebug::OnLeaveSuccess()
{
	UE_LOG(LogTemp, Log, TEXT("[DebugWidget] ✅ Session destroyed."));
	
	UGameplayStatics::OpenLevel(this, FName("MainMenu"));
	CurrentSessionName = TEXT("No session");
	
	UpdateSessionInfo();
}

void UWBP_NexusOnlineDebug::OnLeaveFailure()
{
	UE_LOG(LogTemp, Error, TEXT("[DebugWidget] ❌ Failed to destroy session!"));
}

// ──────────────────────────────────────────────
// SESSION INFO DISPLAY
// ──────────────────────────────────────────────

void UWBP_NexusOnlineDebug::UpdateSessionInfo()
{
	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(GetWorld());
	if (Session.IsValid())
	{
		if (FNamedOnlineSession* Active = Session->GetNamedSession(NAME_GameSession))
		{
			// ────────────────────────────────
			// Basic session stats
			// ────────────────────────────────
			CurrentSessionName = Active->SessionName.ToString();

			const int32 PublicCount = Active->SessionSettings.NumPublicConnections;
			const int32 OpenCount   = Active->NumOpenPublicConnections;

			CurrentPlayers = PublicCount - OpenCount;
			MaxPlayers     = PublicCount;

			// ────────────────────────────────
			// Retrieve Session ID if available
			// ────────────────────────────────
			FString SessionId;
			if (!Active->SessionSettings.Get(TEXT("SESSION_ID_KEY"), SessionId))
			{
				SessionId = TEXT("Unknown");
			}
			
			CurrentSessionId = SessionId;

			// ────────────────────────────────
			// Debug output
			// ────────────────────────────────
			UE_LOG(LogTemp, Log,
				TEXT("[DebugWidget] Session: %s | ID: %s | %d/%d players (%d open slots)"),
				*CurrentSessionName,
				*SessionId,
				CurrentPlayers,
				MaxPlayers,
				OpenCount
			);

			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,
				FString::Printf(TEXT("Session: %s\nID: %s\nPlayers: %d/%d"),
					*CurrentSessionName,
					*SessionId,
					CurrentPlayers,
					MaxPlayers));

			return;
		}
	}

	// ────────────────────────────────
	// No session active
	// ────────────────────────────────
	CurrentSessionName = TEXT("No session active");
	CurrentSessionId = TEXT("N/A");
	CurrentPlayers = 0;
	MaxPlayers = 0;

	UE_LOG(LogTemp, Warning, TEXT("[DebugWidget] No active session."));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, TEXT("[DebugWidget] No active session."));
}


FText UWBP_NexusOnlineDebug::GetSessionInfoText()
{
	if (AOnlineSessionManager* Manager = AOnlineSessionManager::Get(this))
	{
		return FText::FromString(FString::Printf(TEXT("%s (%d Players) "),*Manager->TrackedSessionName.ToString(), Manager->PlayerCount));
	}

	return FText::FromString(TEXT("No session active"));
}

#undef LOCTEXT_NAMESPACE
