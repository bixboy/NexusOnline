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

#define LOCTEXT_NAMESPACE "NexusOnline|FindSessions"

// ──────────────────────────────────────────────
// Create and configure node
// ──────────────────────────────────────────────
UAsyncTask_FindSessions* UAsyncTask_FindSessions::FindSessions(
	UObject* WorldContextObject,
	ENexusSessionType SessionType,
	int32 MaxResults,
	bool bIsLANQuery,
	const TArray<FSessionSearchFilter>& SimpleFilters,
	const TArray<USessionFilterRule*>& AdvancedRules,
	const TArray<USessionSortRule*>& SortRules,
	USessionFilterPreset* Preset
	)
{
	UAsyncTask_FindSessions* Node = NewObject<UAsyncTask_FindSessions>();
	Node->WorldContextObject = WorldContextObject;
	Node->DesiredType = SessionType;

	// Configure online search
	Node->SearchSettings = MakeShareable(new FOnlineSessionSearch());
	Node->SearchSettings->MaxSearchResults = MaxResults;
	Node->SearchSettings->bIsLanQuery = bIsLANQuery;

	// Store user-defined configuration
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
// Activate : Start the async session search
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

    // ──────────────────────────────────────────────
    // 1. Récupération du PlayerController (Ta demande)
    // ──────────────────────────────────────────────
    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogNexusOnlineFilter, Error, TEXT("[FindSessions] No PlayerController found."));
        OnCompleted.Broadcast(false, {});
        return;
    }

    // On récupère l'ID Unique du joueur local (Nécessaire pour Steam/EOS)
    FUniqueNetIdRepl PlayerID;
    if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
    {
        PlayerID = LocalPlayer->GetPreferredUniqueNetId();
    }
    else
    {
        UE_LOG(LogNexusOnlineFilter, Error, TEXT("[FindSessions] PlayerController has no LocalPlayer."));
        OnCompleted.Broadcast(false, {});
        return;
    }

    if (!PlayerID.IsValid())
    {
        UE_LOG(LogNexusOnlineFilter, Error, TEXT("[FindSessions] Invalid Player UniqueNetId."));
        OnCompleted.Broadcast(false, {});
        return;
    }

    // ──────────────────────────────────────────────
    // 2. Configuration Subsystem
    // ──────────────────────────────────────────────
    RebuildResolvedFilters();

    IOnlineSubsystem* Subsystem = Online::GetSubsystem(World);
    if (!Subsystem)
    {
       UE_LOG(LogNexusOnlineFilter, Error, TEXT("[FindSessions] No valid OnlineSubsystem."));
       OnCompleted.Broadcast(false, {});
       return;
    }

    const FName SubsystemName = Subsystem->GetSubsystemName();
    UE_LOG(LogNexusOnlineFilter, Log, TEXT("[FindSessions] Using Subsystem: %s"), *SubsystemName.ToString());

    // AUTO-FIX: NULL Subsystem only supports LAN
    if (SubsystemName == TEXT("NULL") && !SearchSettings->bIsLanQuery)
    {
        UE_LOG(LogNexusOnlineFilter, Warning, TEXT("[FindSessions] 'NULL' subsystem detected but bIsLanQuery is FALSE. Forcing LAN query."));
        SearchSettings->bIsLanQuery = true;
    }

    IOnlineSessionPtr Session = Subsystem->GetSessionInterface();
    if (!Session.IsValid())
    {
       UE_LOG(LogNexusOnlineFilter, Error, TEXT("[FindSessions] Invalid Session Interface."));
       OnCompleted.Broadcast(false, {});
       return;
    }

    ApplyQueryFilters();

    FindSessionsHandle = Session->AddOnFindSessionsCompleteDelegate_Handle
    (
       FOnFindSessionsCompleteDelegate::CreateUObject(this, &UAsyncTask_FindSessions::OnFindSessionsComplete)
    );

    UE_LOG(LogNexusOnlineFilter, Log, TEXT("[FindSessions] Searching sessions of type '%s' (Max: %d, LAN: %d)"),
       *NexusOnline::SessionTypeToName(DesiredType).ToString(),
       SearchSettings->MaxSearchResults,
       SearchSettings->bIsLanQuery);
	
    Session->FindSessions(*PlayerID, SearchSettings.ToSharedRef());
}

// ──────────────────────────────────────────────
// Callback : Session search complete
// ──────────────────────────────────────────────
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
		UE_LOG(LogNexusOnlineFilter, Warning, TEXT("[FindSessions] Search failed or invalid settings."));
		OnCompleted.Broadcast(false, {});
		return;
	}

	UE_LOG(LogNexusOnlineFilter, Log, TEXT("[FindSessions] Search completed. Raw results found: %d"), SearchSettings->SearchResults.Num());

	ProcessSearchResults(SearchSettings->SearchResults);
}

// ──────────────────────────────────────────────
// Process & filter session results
// ──────────────────────────────────────────────
void UAsyncTask_FindSessions::ProcessSearchResults(const TArray<FOnlineSessionSearchResult>& InResults)
{
	const FString DesiredTypeStr = NexusOnline::SessionTypeToName(DesiredType).ToString();

	TWeakObjectPtr WeakThis(this);
	const TArray<FSessionSearchFilter> SimpleFiltersCopy = ResolvedSimpleFilters;

	TArray<TWeakObjectPtr<USessionFilterRule>> FilterRules;
	for (const TObjectPtr<USessionFilterRule>& Rule : ResolvedAdvancedRules)
	{
		if (Rule)
			FilterRules.Add(Rule);
	}

	TArray<TWeakObjectPtr<USessionSortRule>> SortRules;
	for (const TObjectPtr<USessionSortRule>& Rule : ResolvedSortRules)
	{
		if (Rule)
			SortRules.Add(Rule);
	}

	// Capture subsystem type for the async task
	bool bIsNullSubsystem = false;
	if (UAsyncTask_FindSessions* Self = WeakThis.Get())
	{
		if (UWorld* World = GEngine->GetWorldFromContextObject(Self->WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
		{
			if (IOnlineSubsystem* Subsystem = Online::GetSubsystem(World))
			{
				bIsNullSubsystem = (Subsystem->GetSubsystemName() == TEXT("NULL"));
			}
		}
	}

	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [WeakThis, DesiredTypeStr, InResults, SimpleFiltersCopy, FilterRules, SortRules, bIsNullSubsystem]()
	{
		if (!WeakThis.IsValid()) return;

		TArray<FOnlineSessionSearchResult> FilteredResults;

		for (const FOnlineSessionSearchResult& Result : InResults)
		{
			FString ResultId = Result.GetSessionIdStr();
			FString FoundType;
			Result.Session.SessionSettings.Get(TEXT("SESSION_TYPE_KEY"), FoundType);

			// Match type
			if (!FoundType.IsEmpty() && FoundType != DesiredTypeStr)
			{
				UE_LOG(LogNexusOnlineFilter, Verbose, TEXT("[FindSessions] Rejected '%s' (Type mismatch: %s != %s)"), *ResultId, *FoundType, *DesiredTypeStr);
				continue;
			}
			
			// Special handling for NULL subsystem where we might have stripped the type from the beacon
			if (FoundType.IsEmpty())
			{
				if (bIsNullSubsystem)
				{
					UE_LOG(LogNexusOnlineFilter, Warning, TEXT("[FindSessions] Session '%s' has no type (likely due to NULL subsystem optimization). Accepting tentatively."), *ResultId);
				}
				else
				{
					UE_LOG(LogNexusOnlineFilter, Verbose, TEXT("[FindSessions] Rejected '%s' (No type found)"), *ResultId);
					continue;
				}
			}

			// Simple filters
			if (!NexusSessionFilterUtils::PassesAllFilters(SimpleFiltersCopy, Result))
			{
				UE_LOG(LogNexusOnlineFilter, Verbose, TEXT("[FindSessions] Rejected '%s' (Failed simple filters)"), *ResultId);
				continue;
			}

			// Advanced rules
			bool bRejected = false;
			for (const TWeakObjectPtr<USessionFilterRule>& RuleWeak : FilterRules)
			{
				const USessionFilterRule* RuleInstance = RuleWeak.Get();
				if (RuleInstance && RuleInstance->bEnabled && !RuleInstance->PassesFilter(Result))
				{
					UE_LOG(LogNexusOnlineFilter, Verbose, TEXT("[FindSessions] Rejected '%s' (Failed rule: %s)"), *ResultId, *RuleInstance->GetRuleDescription());
					bRejected = true;
					break;
				}
			}
			if (bRejected) continue;

			UE_LOG(LogNexusOnlineFilter, Log, TEXT("[FindSessions] Accepted session '%s'"), *ResultId);
			FilteredResults.Add(Result);
		}

		// Sorting
		if (!SortRules.IsEmpty())
		{
			Algo::Sort(FilteredResults, [&SortRules](const FOnlineSessionSearchResult& A, const FOnlineSessionSearchResult& B)
			{
				for (const TWeakObjectPtr<USessionSortRule>& RuleWeak : SortRules)
				{
					const USessionSortRule* Rule = RuleWeak.Get();
					if (!Rule || !Rule->bEnabled) continue;

					const bool bABeforeB = Rule->Compare(A, B);
					const bool bBBeforeA = Rule->Compare(B, A);
					if (bABeforeB != bBBeforeA)
						return bABeforeB;
				}
				return false;
			});
		}

		// Build simplified result structure
		TArray<FOnlineSessionSearchResultData> FinalResults;
		for (const FOnlineSessionSearchResult& Result : FilteredResults)
		{
			FOnlineSessionSearchResultData Data;
			Result.Session.SessionSettings.Get(TEXT("SESSION_DISPLAY_NAME"), Data.SessionDisplayName);
			Result.Session.SessionSettings.Get(TEXT("MAP_NAME_KEY"), Data.MapName);
			Result.Session.SessionSettings.Get(TEXT("GAME_MODE_KEY"), Data.GameMode);
			Result.Session.SessionSettings.Get(TEXT("SESSION_TYPE_KEY"), Data.SessionType);

			Data.CurrentPlayers = Result.Session.SessionSettings.NumPublicConnections - Result.Session.NumOpenPublicConnections;
			Data.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
			Data.RawResult = Result;

			FinalResults.Add(MoveTemp(Data));
		}

		AsyncTask(ENamedThreads::GameThread, [WeakThis, FinalResults = MoveTemp(FinalResults)]() mutable
		{
			if (!WeakThis.IsValid()) return;

			if (UAsyncTask_FindSessions* Self = WeakThis.Get())
			{
				UE_LOG(LogNexusOnlineFilter, Log, TEXT("[FindSessions] Completed with %d valid results."), FinalResults.Num());
				Self->OnCompleted.Broadcast(true, FinalResults);
			}
		});
	});
}

// ──────────────────────────────────────────────
// Build the merged list of filters
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

	// Sort by priority
	ResolvedAdvancedRules.Sort([](const TObjectPtr<USessionFilterRule>& A, const TObjectPtr<USessionFilterRule>& B)
	{
		return A && B ? (A->Priority < B->Priority) : (B != nullptr);
	});
	
	ResolvedSortRules.Sort([](const TObjectPtr<USessionSortRule>& A, const TObjectPtr<USessionSortRule>& B)
	{
		return A && B ? (A->Priority < B->Priority) : (B != nullptr);
	});
}

// ──────────────────────────────────────────────
// Apply query filters
// ──────────────────────────────────────────────
void UAsyncTask_FindSessions::ApplyQueryFilters()
{
    if (!SearchSettings.IsValid())
       return;

    SearchSettings->MaxSearchResults = 9999; 

    if (SearchSettings->bIsLanQuery)
    	return;

    SearchSettings->QuerySettings.Set(TEXT("SEARCH_PRESENCE"), true, EOnlineComparisonOp::Equals);
    SearchSettings->QuerySettings.Set(TEXT("SEARCH_LOBBIES"), true, EOnlineComparisonOp::Equals);
    SearchSettings->QuerySettings.Set(TEXT("LOBBYDISTANCE"), 3, EOnlineComparisonOp::Equals); // 3 = Worldwide
    SearchSettings->QuerySettings.Set(TEXT("SESSION_TYPE_KEY"), NexusOnline::SessionTypeToName(DesiredType).ToString(), EOnlineComparisonOp::Equals);
    
    NexusSessionFilterUtils::ApplyFiltersToSettings(ResolvedSimpleFilters, *SearchSettings);

    for (const TObjectPtr<USessionFilterRule>& Rule : ResolvedAdvancedRules)
    {
       if (Rule && Rule->bEnabled && Rule->bApplyToSearchQuery)
       {
          Rule->ConfigureSearchSettings(*SearchSettings);
          UE_LOG(LogNexusOnlineFilter, Verbose, TEXT("[FindSessions] Applied query rule: %s"), *Rule->GetRuleDescription());
       }
    }

    for (const TObjectPtr<USessionSortRule>& Rule : ResolvedSortRules)
    {
       if (Rule && Rule->bEnabled)
       {
          Rule->ConfigureSearchSettings(*SearchSettings);
          UE_LOG(LogNexusOnlineSort, Verbose, TEXT("[FindSessions] Applied sort rule: %s"), *Rule->GetRuleDescription());
       }
    }
}

#undef LOCTEXT_NAMESPACE
