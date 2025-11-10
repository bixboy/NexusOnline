#include "Async/AsyncTask_FindSessions.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Utils/NexusOnlineHelpers.h"
#include "Async/Async.h"


// ────────────────────────────────────────────────
//  Configuration & cache local
// ────────────────────────────────────────────────

static TArray<FOnlineSessionSearchResultData> GLastCachedSessions;
static double GLastSessionSearchTime = 0.0;
static constexpr double kSessionCacheLifetime = 5.0;

// ────────────────────────────────────────────────
// Construction
// ────────────────────────────────────────────────

UAsyncTask_FindSessions* UAsyncTask_FindSessions::FindSessions(UObject* WorldContextObject, ENexusSessionType SessionType, int32 MaxResults)
{
	UAsyncTask_FindSessions* Node = NewObject<UAsyncTask_FindSessions>();
	Node->WorldContextObject = WorldContextObject;
	Node->DesiredType = SessionType;

	Node->SearchSettings = MakeShareable(new FOnlineSessionSearch());
	Node->SearchSettings->MaxSearchResults = MaxResults;
	Node->SearchSettings->bIsLanQuery = true;

	// 🎯 Recherche ciblée : présence, type de session, région, etc
	Node->SearchSettings->QuerySettings.Set(FName("SEARCH_PRESENCE"), true, EOnlineComparisonOp::Equals);
	Node->SearchSettings->QuerySettings.Set(FName("SESSION_TYPE_KEY"), NexusOnline::SessionTypeToName(SessionType).ToString(), EOnlineComparisonOp::Equals);

	return Node;
}

// ────────────────────────────────────────────────
// Activation
// ────────────────────────────────────────────────

void UAsyncTask_FindSessions::Activate()
{
	if (!WorldContextObject)
	{
		UE_LOG(LogTemp, Error, TEXT("[NexusOnline] ❌ Invalid WorldContextObject in FindSessions"));
		OnCompleted.Broadcast(false, {});
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	if (!World)
	{
		OnCompleted.Broadcast(false, {});
		return;
	}

	const double Now = FPlatformTime::Seconds();
	if (Now - GLastSessionSearchTime <= kSessionCacheLifetime)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[NexusOnline] ⚡ Using cached session results (%d entries)."), GLastCachedSessions.Num());
		OnCompleted.Broadcast(true, GLastCachedSessions);
		return;
	}

	IOnlineSubsystem* Subsystem = Online::GetSubsystem(World);
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("[NexusOnline] ❌ No OnlineSubsystem found."));
		OnCompleted.Broadcast(false, {});
		return;
	}

	IOnlineSessionPtr Session = Subsystem->GetSessionInterface();
	if (!Session.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[NexusOnline] ❌ Session interface invalid."));
		OnCompleted.Broadcast(false, {});
		return;
	}

	FindSessionsHandle = Session->AddOnFindSessionsCompleteDelegate_Handle
	(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UAsyncTask_FindSessions::OnFindSessionsComplete)
	);

	UE_LOG(LogTemp, Log, TEXT("[NexusOnline] 🔍 Searching sessions of type: %s (Max: %d)"),
		*NexusOnline::SessionTypeToName(DesiredType).ToString(), SearchSettings->MaxSearchResults);

	Session->FindSessions(0, SearchSettings.ToSharedRef());
}

// ────────────────────────────────────────────────
// Callback principal
// ────────────────────────────────────────────────

void UAsyncTask_FindSessions::OnFindSessionsComplete(bool bWasSuccessful)
{
	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);
	if (!Session.IsValid())
	{
		OnCompleted.Broadcast(false, {});
		return;
	}

	Session->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsHandle);

	if (!bWasSuccessful || !SearchSettings.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[NexusOnline] ⚠️ FindSessions failed or invalid settings."));
		OnCompleted.Broadcast(false, {});
		return;
	}

	// Copie locale des résultats
	TArray<FOnlineSessionSearchResult> ResultsCopy = SearchSettings->SearchResults;
	const FString DesiredTypeStr = NexusOnline::SessionTypeToName(DesiredType).ToString();

	// Traitement des résultats
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, DesiredTypeStr, ResultsCopy]()
	{
		TArray<FOnlineSessionSearchResultData> FilteredResults;
		FilteredResults.Reserve(ResultsCopy.Num());

		for (const auto& Result : ResultsCopy)
		{
			FString FoundType;
			Result.Session.SessionSettings.Get(FName("SESSION_TYPE_KEY"), FoundType);

			if (FoundType == DesiredTypeStr)
			{
				FOnlineSessionSearchResultData Data;
				Result.Session.SessionSettings.Get(FName("SESSION_DISPLAY_NAME"), Data.SessionDisplayName);
				Result.Session.SessionSettings.Get(FName("MAP_NAME_KEY"), Data.MapName);
				Result.Session.SessionSettings.Get(FName("GAME_MODE_KEY"), Data.GameMode);
				Result.Session.SessionSettings.Get(FName("SESSION_TYPE_KEY"), Data.SessionType);

				Data.CurrentPlayers = Result.Session.SessionSettings.NumPublicConnections - Result.Session.NumOpenPublicConnections;
				Data.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
				Data.RawResult = Result;

				FilteredResults.Add(MoveTemp(Data));
			}
		}

		GLastCachedSessions = FilteredResults;
		GLastSessionSearchTime = FPlatformTime::Seconds();

		AsyncTask(ENamedThreads::GameThread, [this, FilteredResults]()
		{
			UE_LOG(LogTemp, Log, TEXT("[NexusOnline] ✅ FindSessions complete: %d valid results."), FilteredResults.Num());
			OnCompleted.Broadcast(true, FilteredResults);
		});
	});
}
