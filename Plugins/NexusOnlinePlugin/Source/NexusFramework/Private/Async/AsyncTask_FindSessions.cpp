#include "Async/AsyncTask_FindSessions.h"
#include "Async/Async.h"
#include "Algo/Sort.h"
#include "Data/SessionSearchFilter.h"
#include "Filters/SessionFilterRule.h"
#include "Filters/SessionSortRule.h"
#include "Data/SessionFilterPreset.h"
#include "Utils/NexusOnlineHelpers.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"

#define LOCTEXT_NAMESPACE "NexusOnline|FindSessions"

// ──────────────────────────────────────────────
// Factory : Configuration du Nœud
// ──────────────────────────────────────────────
UAsyncTask_FindSessions* UAsyncTask_FindSessions::FindSessions(UObject* WorldContextObject, ENexusSessionType SessionType, int32 MaxResults, bool bIsLANQuery,
	const TArray<FSessionSearchFilter>& SimpleFilters, const TArray<USessionFilterRule*>& AdvancedRules,
	const TArray<USessionSortRule*>& SortRules, USessionFilterPreset* Preset)
{
	UAsyncTask_FindSessions* Node = NewObject<UAsyncTask_FindSessions>();
	Node->WorldContextObject = WorldContextObject;
	Node->DesiredType = SessionType;
	
	Node->SearchSettings = MakeShareable(new FOnlineSessionSearch());
	Node->SearchSettings->MaxSearchResults = (MaxResults > 0) ? MaxResults : 50;
	
	Node->SearchSettings->bIsLanQuery = bIsLANQuery;
	Node->UserSimpleFilters = SimpleFilters;
	Node->UserPreset = Preset;

	for (USessionFilterRule* Rule : AdvancedRules)
	{
		if (Rule)
			Node->UserAdvancedRules.Add(Rule);	
	}

	for (USessionSortRule* Rule : SortRules)
	{
		if (Rule)
			Node->UserSortRules.Add(Rule);
	}

	return Node;
}

// ──────────────────────────────────────────────
// Activate : Lancement de la recherche
// ──────────────────────────────────────────────
void UAsyncTask_FindSessions::Activate()
{
	if (!WorldContextObject)
	{
		UE_LOG(LogNexusOnlineFilter, Error, TEXT("[FindSessions] Invalid WorldContextObject."));
		OnCompleted.Broadcast(false, {});
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	if (!World)
	{
		OnCompleted.Broadcast(false, {});
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		OnCompleted.Broadcast(false, {});
		return;
	}

	FUniqueNetIdRepl PlayerID;
	if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
	{
		PlayerID = LocalPlayer->GetPreferredUniqueNetId();
	}

	if (!PlayerID.IsValid())
	{
		UE_LOG(LogNexusOnlineFilter, Warning, TEXT("[FindSessions] Invalid Player UniqueNetId. Proceeding, but Steam/EOS might fail."));
	}

	RebuildResolvedFilters();

	IOnlineSubsystem* Subsystem = Online::GetSubsystem(World);
	if (!Subsystem)
	{
		OnCompleted.Broadcast(false, {});
		return;
	}

	const FName SubsystemName = Subsystem->GetSubsystemName();
	if (SubsystemName == TEXT("NULL") && !SearchSettings->bIsLanQuery)
	{
		UE_LOG(LogNexusOnlineFilter, Warning, TEXT("[FindSessions] 'NULL' subsystem detected. Forcing LAN query."));
		SearchSettings->bIsLanQuery = true;
	}

	IOnlineSessionPtr Session = Subsystem->GetSessionInterface();
	if (!Session.IsValid())
	{
		OnCompleted.Broadcast(false, {});
		return;
	}

	ApplyQueryFilters();

	FindSessionsHandle = Session->AddOnFindSessionsCompleteDelegate_Handle
	(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UAsyncTask_FindSessions::OnFindSessionsComplete)
	);

	UE_LOG(LogNexusOnlineFilter, Log, TEXT("[FindSessions] Searching... (Type: %s, Max: %d, LAN: %s)"),
		*NexusOnline::SessionTypeToName(DesiredType).ToString(),
		SearchSettings->MaxSearchResults,
		SearchSettings->bIsLanQuery ? TEXT("YES") : TEXT("NO"));
	
	if (!Session->FindSessions(*PlayerID, SearchSettings.ToSharedRef()))
	{
		Session->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsHandle);
		OnCompleted.Broadcast(false, {});
	}
}

void UAsyncTask_FindSessions::OnFindSessionsComplete(bool bWasSuccessful)
{
	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	if (!World)
		return;

	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World);
	if (Session.IsValid())
	{
		Session->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsHandle);
	}

	if (!bWasSuccessful || !SearchSettings.IsValid())
	{
		UE_LOG(LogNexusOnlineFilter, Warning, TEXT("[FindSessions] Search failed or returned no results."));
		OnCompleted.Broadcast(false, {});
		return;
	}

	if (SearchSettings->SearchResults.Num() == 0)
	{
		UE_LOG(LogNexusOnlineFilter, Log, TEXT("[FindSessions] Search successful but found 0 sessions."));
		OnCompleted.Broadcast(true, {});
		return;
	}

	UE_LOG(LogNexusOnlineFilter, Log, TEXT("[FindSessions] Raw results found: %d. Processing filters..."), SearchSettings->SearchResults.Num());
	
	ProcessSearchResults(SearchSettings->SearchResults);
}

// ──────────────────────────────────────────────
// Traitement, Filtrage et Tri
// ──────────────────────────────────────────────
void UAsyncTask_FindSessions::ProcessSearchResults(const TArray<FOnlineSessionSearchResult>& InResults)
{
	const FString DesiredTypeStr = NexusOnline::SessionTypeToName(DesiredType).ToString();
	
	bool bIsNullSubsystem = false;
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (IOnlineSubsystem* Subsystem = Online::GetSubsystem(World))
		{
			bIsNullSubsystem = (Subsystem->GetSubsystemName() == TEXT("NULL"));
		}
	}

	TArray<FOnlineSessionSearchResult> FilteredResults;
	FilteredResults.Reserve(InResults.Num());

	// ---------------------------------------------------------
	// PHASE 1 : FILTRAGE
	// ---------------------------------------------------------
	for (const FOnlineSessionSearchResult& Result : InResults)
	{
		if (!Result.IsValid())
			continue;

		FString FoundType;
		Result.Session.SessionSettings.Get(TEXT("SESSION_TYPE_KEY"), FoundType);

		if (!FoundType.IsEmpty() && FoundType != DesiredTypeStr)
		{
			continue;
		}
		
		if (FoundType.IsEmpty() && !bIsNullSubsystem)
		{
			continue;
		}

		if (!NexusSessionFilterUtils::PassesAllFilters(ResolvedSimpleFilters, Result))
		{
			continue;
		}
		
		bool bRejected = false;
		for (const TObjectPtr<USessionFilterRule>& Rule : ResolvedAdvancedRules)
		{
			if (Rule && Rule->bEnabled && !Rule->PassesFilter(Result))
			{
				bRejected = true;
				break;
			}
		}
		if (bRejected)
			continue;

		FilteredResults.Add(Result);
	}

	// ---------------------------------------------------------
	// PHASE 2 : TRI
	// ---------------------------------------------------------
	if (!ResolvedSortRules.IsEmpty() && FilteredResults.Num() > 1)
	{
		Algo::Sort(FilteredResults, [this](const FOnlineSessionSearchResult& A, const FOnlineSessionSearchResult& B)
		{
			for (const TObjectPtr<USessionSortRule>& Rule : ResolvedSortRules)
			{
				if (!Rule || !Rule->bEnabled)
					continue;

				if (Rule->Compare(A, B))
					
					return true;
				if (Rule->Compare(B, A))
					return false;
			}
			return false; 
		});
	}

	// ---------------------------------------------------------
	// PHASE 3 : CONVERSION & SORTIE
	// ---------------------------------------------------------
	TArray<FOnlineSessionSearchResultData> FinalResults;
	FinalResults.Reserve(FilteredResults.Num());

	for (const FOnlineSessionSearchResult& Result : FilteredResults)
	{
		FOnlineSessionSearchResultData Data;
		
		Result.Session.SessionSettings.Get(TEXT("SESSION_DISPLAY_NAME"), Data.SessionDisplayName);
		Result.Session.SessionSettings.Get(TEXT("MAP_NAME_KEY"), Data.MapName);
		Result.Session.SessionSettings.Get(TEXT("GAME_MODE_KEY"), Data.GameMode);
		Result.Session.SessionSettings.Get(TEXT("SESSION_TYPE_KEY"), Data.SessionType);

		Data.CurrentPlayers = Result.Session.SessionSettings.NumPublicConnections - Result.Session.NumOpenPublicConnections;
		Data.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
		Data.Ping = Result.PingInMs;
		
		Data.RawResult = Result;

		FinalResults.Add(MoveTemp(Data));
	}

	UE_LOG(LogNexusOnlineFilter, Log, TEXT("[FindSessions] Completed. %d sessions kept out of %d."), FinalResults.Num(), InResults.Num());
	OnCompleted.Broadcast(true, FinalResults);
}

// ──────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────

void UAsyncTask_FindSessions::RebuildResolvedFilters()
{
	ResolvedSimpleFilters = UserSimpleFilters;
	ResolvedAdvancedRules.Reset();
	ResolvedSortRules.Reset();

	if (UserPreset)
	{
		ResolvedSimpleFilters.Append(UserPreset->SimpleFilters);
		ResolvedAdvancedRules.Append(UserPreset->AdvancedRules);
		ResolvedSortRules.Append(UserPreset->SortRules);
	}

	ResolvedAdvancedRules.Append(UserAdvancedRules);
	ResolvedSortRules.Append(UserSortRules);

	Algo::Sort(ResolvedAdvancedRules, [](const TObjectPtr<USessionFilterRule>& A, const TObjectPtr<USessionFilterRule>& B)
	{
		return A && B ? (A->Priority > B->Priority) : (A != nullptr);
	});
	
	Algo::Sort(ResolvedSortRules, [](const TObjectPtr<USessionSortRule>& A, const TObjectPtr<USessionSortRule>& B)
	{
		return A && B ? (A->Priority > B->Priority) : (A != nullptr);
	});
}

void UAsyncTask_FindSessions::ApplyQueryFilters()
{
	if (!SearchSettings.IsValid())
		return;
	
	if (SearchSettings->bIsLanQuery)
		return;
	
	auto SetDefaultIfMissing = [&](FName Key, const auto& Value, EOnlineComparisonOp::Type Op)
	{
		bool bFound = false;
		for(const auto& Filter : ResolvedSimpleFilters)
		{
			if(Filter.Key == Key)
			{
				bFound = true;
				break;
			}
		}

		if(!bFound)
		{
			SearchSettings->QuerySettings.Set(Key, Value, Op);
		}
	};

	SetDefaultIfMissing(TEXT("SEARCH_PRESENCE"), true, EOnlineComparisonOp::Equals);
	SetDefaultIfMissing(TEXT("SEARCH_LOBBIES"), true, EOnlineComparisonOp::Equals);
	SetDefaultIfMissing(TEXT("LOBBYDISTANCE"), 3, EOnlineComparisonOp::Equals);

	SearchSettings->QuerySettings.Set(TEXT("SESSION_TYPE_KEY"), NexusOnline::SessionTypeToName(DesiredType).ToString(), EOnlineComparisonOp::Equals);
	NexusSessionFilterUtils::ApplyFiltersToSettings(ResolvedSimpleFilters, *SearchSettings);

	for (const TObjectPtr<USessionFilterRule>& Rule : ResolvedAdvancedRules)
	{
		if (Rule && Rule->bEnabled && Rule->bApplyToSearchQuery)
		{
			Rule->ConfigureSearchSettings(*SearchSettings);
		}
	}
}

#undef LOCTEXT_NAMESPACE