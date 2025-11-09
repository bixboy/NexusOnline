#include "TestDebugs/WBP_NexusOnlineDebug.h"

#include "Async/AsyncTask_CreateSession.h"
#include "Async/AsyncTask_DestroySession.h"
#include "Async/AsyncTask_FindSessions.h"
#include "Async/AsyncTask_JoinSession.h"
#include "Kismet/GameplayStatics.h"
#include "Utils/NexusOnlineHelpers.h"

void UWBP_NexusOnlineDebug::NativeConstruct()
{
        Super::NativeConstruct();

        if (Button_Create)
        {
                Button_Create->OnClicked.AddDynamic(this, &UWBP_NexusOnlineDebug::OnCreateClicked);
        }
        if (Button_Join)
        {
                Button_Join->OnClicked.AddDynamic(this, &UWBP_NexusOnlineDebug::OnJoinClicked);
        }
        if (Button_Leave)
        {
                Button_Leave->OnClicked.AddDynamic(this, &UWBP_NexusOnlineDebug::OnLeaveClicked);
        }

        if (UWorld* World = GetWorld())
        {
                PlayersChangedHandle = NexusOnline::OnSessionPlayersChanged().AddUObject(this, &UWBP_NexusOnlineDebug::HandleSessionPlayersChanged);

                int32 Current = 0;
                int32 Max = 0;
                FName SessionName = NAME_None;
                if (NexusOnline::ResolveSessionPlayerCounts(World, ENexusSessionType::GameSession, Current, Max, SessionName))
                {
                        ApplyActiveSession(SessionName.ToString(), Current, Max);
                }
                else
                {
                        ApplyNoSession();
                }
        }
        else
        {
                ApplyNoSession();
        }
}

void UWBP_NexusOnlineDebug::NativeDestruct()
{
        if (PlayersChangedHandle.IsValid())
        {
                NexusOnline::OnSessionPlayersChanged().Remove(PlayersChangedHandle);
                PlayersChangedHandle = FDelegateHandle();
        }

        Super::NativeDestruct();
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

        UE_LOG(LogNexusOnline, Log, TEXT("[DebugWidget] Attempting to create session..."));
}

void UWBP_NexusOnlineDebug::OnCreateSuccess()
{
        UE_LOG(LogNexusOnline, Log, TEXT("[DebugWidget] ✅ Session created successfully!"));
}

void UWBP_NexusOnlineDebug::OnCreateFailure()
{
        UE_LOG(LogNexusOnline, Error, TEXT("[DebugWidget] ❌ Failed to create session!"));
}
#pragma endregion

#pragma region 🔍 FIND & JOIN SESSION
void UWBP_NexusOnlineDebug::OnJoinClicked()
{
        UAsyncTask_FindSessions* Task = UAsyncTask_FindSessions::FindSessions(GetOwningPlayer(), ENexusSessionType::GameSession, 20);
        Task->OnCompleted.AddDynamic(this, &UWBP_NexusOnlineDebug::OnFindSessionsCompleted);
        Task->Activate();

        UE_LOG(LogNexusOnline, Log, TEXT("[DebugWidget] Searching for sessions..."));
}

void UWBP_NexusOnlineDebug::OnFindSessionsCompleted(bool bWasSuccessful, const TArray<FOnlineSessionSearchResultData>& Results)
{
        if (!bWasSuccessful || Results.IsEmpty())
        {
                UE_LOG(LogNexusOnline, Warning, TEXT("[DebugWidget] ❌ No sessions found."));
                return;
        }

        FoundSessions = Results;
        const FOnlineSessionSearchResultData& First = Results[0];

        UE_LOG(LogNexusOnline, Log, TEXT("[DebugWidget] Found %d session(s). Joining '%s'..."), Results.Num(), *First.SessionDisplayName);

        UAsyncTask_JoinSession* JoinTask = UAsyncTask_JoinSession::JoinSession(GetOwningPlayer(), First, ENexusSessionType::GameSession);
        JoinTask->OnSuccess.AddDynamic(this, &UWBP_NexusOnlineDebug::OnJoinSuccess);
        JoinTask->OnFailure.AddDynamic(this, &UWBP_NexusOnlineDebug::OnJoinFailure);
        JoinTask->Activate();

        CurrentSessionName = First.SessionDisplayName;
        CurrentPlayers = First.CurrentPlayers;
        MaxPlayers = First.MaxPlayers;
        if (Text_SessionInfo)
        {
                Text_SessionInfo->SetText(GetSessionInfoText());
        }
}

void UWBP_NexusOnlineDebug::OnJoinSuccess()
{
        UE_LOG(LogNexusOnline, Log, TEXT("[DebugWidget] ✅ Joined session successfully!"));
}

void UWBP_NexusOnlineDebug::OnJoinFailure()
{
        UE_LOG(LogNexusOnline, Error, TEXT("[DebugWidget] ❌ Failed to join session!"));
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
        UE_LOG(LogNexusOnline, Log, TEXT("[DebugWidget] ✅ Session destroyed."));
        UGameplayStatics::OpenLevel(this, FName("MainMenu"));
}

void UWBP_NexusOnlineDebug::OnLeaveFailure()
{
        UE_LOG(LogNexusOnline, Error, TEXT("[DebugWidget] ❌ Failed to destroy session!"));
}
#pragma endregion

#pragma region 🧠 SESSION INFO
void UWBP_NexusOnlineDebug::HandleSessionPlayersChanged(UWorld* InWorld, ENexusSessionType SessionType, FName SessionName, int32 CurrentPlayersValue, int32 MaxPlayersValue)
{
        if (InWorld != GetWorld() || SessionType != ENexusSessionType::GameSession)
        {
                return;
        }

        if (SessionName.IsNone())
        {
                ApplyNoSession();
                return;
        }

        ApplyActiveSession(SessionName.ToString(), CurrentPlayersValue, MaxPlayersValue);
}

void UWBP_NexusOnlineDebug::ApplyActiveSession(const FString& SessionLabel, int32 CurrentPlayersValue, int32 MaxPlayersValue)
{
        CurrentSessionName = SessionLabel;
        CurrentPlayers = CurrentPlayersValue;
        MaxPlayers = MaxPlayersValue;

        if (Text_SessionInfo)
        {
                Text_SessionInfo->SetText(GetSessionInfoText());
        }

        UE_LOG(LogNexusOnline, Log, TEXT("[DebugWidget] Session: %s (%d/%d players)"), *CurrentSessionName, CurrentPlayers, MaxPlayers);
}

void UWBP_NexusOnlineDebug::ApplyNoSession()
{
        CurrentSessionName = TEXT("No session active");
        CurrentPlayers = 0;
        MaxPlayers = 0;

        if (Text_SessionInfo)
        {
                Text_SessionInfo->SetText(GetSessionInfoText());
        }

        UE_LOG(LogNexusOnline, Log, TEXT("[DebugWidget] No active session."));
}

FText UWBP_NexusOnlineDebug::GetSessionInfoText() const
{
        if (CurrentSessionName.Equals(TEXT("No session active")))
        {
                return FText::FromString(CurrentSessionName);
        }

        return FText::FromString(FString::Printf(TEXT("%s (%d/%d Players)"), *CurrentSessionName, CurrentPlayers, MaxPlayers));
}
#pragma endregion
