#include "Async/AsyncTask_FindSessionById.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystem.h"
#include "Utils/NexusOnlineHelpers.h"
#include "Engine/World.h"
#include "Engine/Engine.h"


UAsyncTask_FindSessionById* UAsyncTask_FindSessionById::FindSessionById(UObject* WorldContextObject, const FString& SessionId)
{
    UAsyncTask_FindSessionById* Node = NewObject<UAsyncTask_FindSessionById>();
    Node->WorldContextObject = WorldContextObject;
    Node->TargetSessionId = SessionId;
    return Node;
}

void UAsyncTask_FindSessionById::Activate()
{
    IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(GEngine->GetWorldFromContextObjectChecked(WorldContextObject));
    if (!Session.IsValid())
    {
        OnCompleted.Broadcast(false, FOnlineSessionSearchResultData());
        return;
    }

    SearchSettings = MakeShareable(new FOnlineSessionSearch());
    SearchSettings->MaxSearchResults = 50;
	SearchSettings->QuerySettings.Set(FName("SESSION_ID_KEY"), TargetSessionId, EOnlineComparisonOp::Equals);

    Session->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this, &UAsyncTask_FindSessionById::OnFindSessionsComplete));

    if (!Session->FindSessions(0, SearchSettings.ToSharedRef()))
    {
        OnCompleted.Broadcast(false, FOnlineSessionSearchResultData());
    }
}

void UAsyncTask_FindSessionById::OnFindSessionsComplete(bool bWasSuccessful)
{
    if (!bWasSuccessful || SearchSettings->SearchResults.IsEmpty())
    {
        OnCompleted.Broadcast(false, FOnlineSessionSearchResultData());
        return;
    }

    for (const FOnlineSessionSearchResult& Result : SearchSettings->SearchResults)
    {
        FString FoundId;
        Result.Session.SessionSettings.Get(TEXT("SESSION_ID_KEY"), FoundId);

        if (FoundId == TargetSessionId)
        {
        	FOnlineSessionSearchResultData Data;
        	Data.RawResult = Result;
        	Data.SessionDisplayName = Result.GetSessionIdStr();
        	Data.CurrentPlayers = Result.Session.SessionSettings.NumPublicConnections - Result.Session.NumOpenPublicConnections;
        	Data.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;

        	Result.Session.SessionSettings.Get(TEXT("SESSION_DISPLAY_NAME"), Data.SessionDisplayName);

        	OnCompleted.Broadcast(true, Data);
        	return;
        }
    }

    OnCompleted.Broadcast(false, FOnlineSessionSearchResultData());
}
