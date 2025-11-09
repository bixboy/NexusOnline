#include "TestDebugs/WBP_NexusOnlineDebug.h"
#include "Async/AsyncTask_CreateSession.h"
#include "Async/AsyncTask_DestroySession.h"
#include "Async/AsyncTask_FindSessions.h"
#include "Async/AsyncTask_JoinSession.h"
#include "Kismet/GameplayStatics.h"
#include "Utils/NexusOnlineHelpers.h"
#include "Blueprint/NexusOnlineBlueprintLibrary.h"

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

#pragma region CREATE SESSION
void UWBP_NexusOnlineDebug::OnCreateClicked()
{
	FSessionSettingsData Settings;
	Settings.SessionName = TEXT("DebugSession");
	Settings.SessionType = ENexusSessionType::GameSession;
	Settings.MaxPlayers = 4;
	Settings.bIsPrivate = false;
	Settings.bIsLAN = true;
	Settings.MapName = TEXT("TestMap");
	Settings.GameMode = TEXT("Default");

	UAsyncTask_CreateSession* Task = UAsyncTask_CreateSession::CreateSession(GetOwningPlayer(), Settings);
	Task->OnSuccess.AddDynamic(this, &UWBP_NexusOnlineDebug::OnCreateSuccess);
	Task->OnFailure.AddDynamic(this, &UWBP_NexusOnlineDebug::OnCreateFailure);
	Task->Activate();

	UE_LOG(LogTemp, Log, TEXT("[DebugWidget] Attempting to create session..."));
}

void UWBP_NexusOnlineDebug::OnCreateSuccess()
{
	UE_LOG(LogTemp, Log, TEXT("[DebugWidget] ✅ Session created successfully!"));
	UpdateSessionInfo();

	// Ici tu n'as plus besoin d'OpenLevel si ton AsyncTask_CreateSession s'en occupe déjà.
	// Si tu préfères le gérer ici, déplace-le juste après OnSuccess.
}

void UWBP_NexusOnlineDebug::OnCreateFailure()
{
	UE_LOG(LogTemp, Error, TEXT("[DebugWidget] ❌ Failed to create session!"));
}
#pragma endregion

#pragma region 🔍 FIND & JOIN SESSION
void UWBP_NexusOnlineDebug::OnJoinClicked()
{
	UAsyncTask_FindSessions* Task = UAsyncTask_FindSessions::FindSessions(GetOwningPlayer(), ENexusSessionType::GameSession, 20);
	Task->OnCompleted.AddDynamic(this, &UWBP_NexusOnlineDebug::OnFindSessionsCompleted);
	Task->Activate();

	UE_LOG(LogTemp, Log, TEXT("[DebugWidget] Searching for sessions..."));
}

void UWBP_NexusOnlineDebug::OnFindSessionsCompleted(bool bWasSuccessful, const TArray<FOnlineSessionSearchResultData>& Results)
{
	if (!bWasSuccessful || Results.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[DebugWidget] ❌ No sessions found."));
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
	UpdateSessionInfo();
}

void UWBP_NexusOnlineDebug::OnJoinFailure()
{
	UE_LOG(LogTemp, Error, TEXT("[DebugWidget] ❌ Failed to join session!"));
}
#pragma endregion

#pragma region 🚪 LEAVE / DESTROY SESSION
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
#pragma endregion

#pragma region 🧠 SESSION INFO
void UWBP_NexusOnlineDebug::UpdateSessionInfo()
{
        IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(GetWorld());
        if (Session.IsValid())
        {
                const FName TargetSessionName = NexusOnline::SessionTypeToName(ENexusSessionType::GameSession);
                if (FNamedOnlineSession* Active = Session->GetNamedSession(TargetSessionName))
                {
                        CurrentSessionName = Active->SessionName.ToString();

                        int32 RetrievedCurrentPlayers = 0;
                        int32 RetrievedMaxPlayers = 0;
                        if (UNexusOnlineBlueprintLibrary::GetSessionPlayerCounts(this, ENexusSessionType::GameSession, RetrievedCurrentPlayers, RetrievedMaxPlayers))
                        {
                                CurrentPlayers = RetrievedCurrentPlayers;
                                MaxPlayers = RetrievedMaxPlayers;
                                const int32 OpenCount = FMath::Max(0, MaxPlayers - CurrentPlayers);
                                UE_LOG(LogTemp, Log, TEXT("[DebugWidget] Session: %s (%d/%d players registered, %d open slots)"), *CurrentSessionName, CurrentPlayers, MaxPlayers, OpenCount);
                        }
                        else
                        {
                                CurrentPlayers = Active->RegisteredPlayers.Num();
                                MaxPlayers = Active->SessionSettings.NumPublicConnections;
                                const int32 OpenCount = FMath::Max(0, MaxPlayers - CurrentPlayers);
                                UE_LOG(LogTemp, Warning, TEXT("[DebugWidget] Fallback session info used for %s."), *CurrentSessionName);
                                UE_LOG(LogTemp, Log, TEXT("[DebugWidget] Session: %s (%d/%d players registered, %d open slots)"), *CurrentSessionName, CurrentPlayers, MaxPlayers, OpenCount);
                        }
                        return;
                }
        }

	CurrentSessionName = TEXT("No session active");
	CurrentPlayers = 0;
	MaxPlayers = 0;
	UE_LOG(LogTemp, Warning, TEXT("[DebugWidget] No active session."));
}

FText UWBP_NexusOnlineDebug::GetSessionInfoText() const
{
	if (CurrentSessionName != "No session active")
	{
		return FText::FromString(FString::Printf(TEXT("%s (%d/%d Players)"), *CurrentSessionName, CurrentPlayers, MaxPlayers));
	}

	return FText::FromString(TEXT("No session active"));
}
#pragma endregion
