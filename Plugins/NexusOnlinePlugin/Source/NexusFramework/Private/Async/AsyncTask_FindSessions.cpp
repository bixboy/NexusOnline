#include "Async/AsyncTask_FindSessions.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Utils/NexusOnlineHelpers.h"


UAsyncTask_FindSessions* UAsyncTask_FindSessions::FindSessions(UObject* WorldContextObject, ENexusSessionType SessionType, int32 MaxResults)
{
    UAsyncTask_FindSessions* Node = NewObject<UAsyncTask_FindSessions>();
	Node->WorldContextObject = WorldContextObject;
    Node->DesiredType = SessionType;

    Node->SearchSettings = MakeShareable(new FOnlineSessionSearch());
    Node->SearchSettings->MaxSearchResults = MaxResults;
    Node->SearchSettings->bIsLanQuery = false;
	Node->SearchSettings->QuerySettings.Set(FName("USES_PRESENCE"), true, EOnlineComparisonOp::Equals);

    return Node;
}

void UAsyncTask_FindSessions::Activate()
{
	if (!WorldContextObject)
	{
		UE_LOG(LogTemp, Error, TEXT("[NexusOnline] Invalid WorldContextObject in FindSessions"));
		
        OnCompleted.Broadcast(false, {});
		return;
	}
	
    UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
    if (!World)
    {
        OnCompleted.Broadcast(false, {});
        return;
    }

    IOnlineSubsystem* Subsystem = Online::GetSubsystem(World);
    if (!Subsystem)
    {
        OnCompleted.Broadcast(false, {});
        return;
    }

    IOnlineSessionPtr Session = Subsystem->GetSessionInterface();
    if (!Session.IsValid())
    {
        OnCompleted.Broadcast(false, {});
        return;
    }

    FindSessionsHandle = Session->AddOnFindSessionsCompleteDelegate_Handle
	(
        FOnFindSessionsCompleteDelegate::CreateUObject(this, &UAsyncTask_FindSessions::OnFindSessionsComplete)
    );

    UE_LOG(LogTemp, Log, TEXT("[NexusOnline] Searching sessions of type: %s"),
        *NexusOnline::SessionTypeToName(DesiredType).ToString());

    Session->FindSessions(0, SearchSettings.ToSharedRef());
}

void UAsyncTask_FindSessions::OnFindSessionsComplete(bool bWasSuccessful)
{
    TArray<FOnlineSessionSearchResultData> ResultsData;

    UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);
	
	if (!Session.IsValid())
	{
		OnCompleted.Broadcast(false, {});
		return;
	}

	Session->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsHandle);

    if (bWasSuccessful && SearchSettings.IsValid())
    {
        FName DesiredSessionName = NexusOnline::SessionTypeToName(DesiredType);

        for (auto& Result : SearchSettings->SearchResults)
        {
            FString FoundType;
            Result.Session.SessionSettings.Get(FName("SESSION_TYPE_KEY"), FoundType);

            // filtre uniquement le type demandé
            if (FoundType == DesiredSessionName.ToString())
            {
                FOnlineSessionSearchResultData Data;
                Result.Session.SessionSettings.Get(FName("SESSION_DISPLAY_NAME"), Data.SessionDisplayName);
                Result.Session.SessionSettings.Get(FName("MAP_NAME_KEY"), Data.MapName);
                Result.Session.SessionSettings.Get(FName("GAME_MODE_KEY"), Data.GameMode);
                Result.Session.SessionSettings.Get(FName("SESSION_TYPE_KEY"), Data.SessionType);

                Data.CurrentPlayers = Result.Session.SessionSettings.NumPublicConnections - Result.Session.NumOpenPublicConnections;
                Data.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
                Data.RawResult = Result;

                ResultsData.Add(Data);
            }
        }
    }

	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("[NexusOnline] FindSessions failed to complete properly."));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[NexusOnline] FindSessions complete. Found: %d"), ResultsData.Num());
	}

    OnCompleted.Broadcast(bWasSuccessful, ResultsData);
}
