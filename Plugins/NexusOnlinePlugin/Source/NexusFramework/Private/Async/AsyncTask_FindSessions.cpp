#include "Async/AsyncTask_FindSessions.h"
#include "Algo/Sort.h"
#include "Async/Async.h"
#include "Data/SessionSearchFilter.h"
#include "Filters/SessionFilterRule.h"
#include "Filters/SessionSortRule.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Data/SessionFilterPreset.h"
#include "Utils/NexusOnlineHelpers.h"


namespace
{
	TArray<FOnlineSessionSearchResult> GLastCachedRawResults;
	double GLastSessionSearchTime = 0.0;
	constexpr double kSessionCacheLifetime = 5.0;
}

UAsyncTask_FindSessions* UAsyncTask_FindSessions::FindSessions(UObject* WorldContextObject, ENexusSessionType SessionType, int32 MaxResults, const TArray<FSessionSearchFilter>& SimpleFilters, const TArray<USessionFilterRule*>& AdvancedRules, const TArray<USessionSortRule*>& SortRules, USessionFilterPreset* Preset)
{
    UAsyncTask_FindSessions* Node = NewObject<UAsyncTask_FindSessions>();
    Node->WorldContextObject = WorldContextObject;
    Node->DesiredType = SessionType;

    Node->SearchSettings = MakeShareable(new FOnlineSessionSearch());
    Node->SearchSettings->MaxSearchResults = MaxResults;
    Node->SearchSettings->bIsLanQuery = true;

    Node->UserSimpleFilters = SimpleFilters;
    Node->UserPreset = Preset;

    Node->UserAdvancedRules.Reserve(AdvancedRules.Num());
    for (USessionFilterRule* Rule : AdvancedRules)
    {
        if (Rule)
        {
            Node->UserAdvancedRules.Add(Rule);
        }
    }

    Node->UserSortRules.Reserve(SortRules.Num());
    for (USessionSortRule* Rule : SortRules)
    {
        if (Rule)
        {
            Node->UserSortRules.Add(Rule);
        }
    }

    return Node;
}

void UAsyncTask_FindSessions::Activate()
{
    if (!WorldContextObject)
    {
        UE_LOG(LogNexusOnlineFilter, Error, TEXT("[NexusOnline|Filter] Invalid WorldContextObject in FindSessions"));
        OnCompleted.Broadcast(false, {});
        return;
    }

    UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
    if (!World)
    {
        OnCompleted.Broadcast(false, {});
        return;
    }

    RebuildResolvedFilters();

    const double Now = FPlatformTime::Seconds();
    if (Now - GLastSessionSearchTime <= kSessionCacheLifetime && GLastCachedRawResults.Num() > 0)
    {
        UE_LOG(LogNexusOnlineFilter, Verbose, TEXT("[NexusOnline|Filter] Using cached session results (%d entries)."), GLastCachedRawResults.Num());
        ProcessSearchResults(GLastCachedRawResults);
        return;
    }

    IOnlineSubsystem* Subsystem = Online::GetSubsystem(World);
    if (!Subsystem)
    {
        UE_LOG(LogNexusOnlineFilter, Error, TEXT("[NexusOnline|Filter] No OnlineSubsystem found."));
        OnCompleted.Broadcast(false, {});
        return;
    }

    IOnlineSessionPtr Session = Subsystem->GetSessionInterface();
    if (!Session.IsValid())
    {
        UE_LOG(LogNexusOnlineFilter, Error, TEXT("[NexusOnline|Filter] Session interface invalid."));
        OnCompleted.Broadcast(false, {});
        return;
    }

    ApplyQueryFilters();

    FindSessionsHandle = Session->AddOnFindSessionsCompleteDelegate_Handle
    (
        FOnFindSessionsCompleteDelegate::CreateUObject(this, &UAsyncTask_FindSessions::OnFindSessionsComplete)
    );

    UE_LOG(LogNexusOnlineFilter, Log, TEXT("[NexusOnline|Filter] Searching sessions of type: %s (Max: %d)"), *NexusOnline::SessionTypeToName(DesiredType).ToString(), SearchSettings->MaxSearchResults);

    Session->FindSessions(0, SearchSettings.ToSharedRef());
}

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
        UE_LOG(LogNexusOnlineFilter, Warning, TEXT("[NexusOnline|Filter] FindSessions failed or invalid settings."));
        OnCompleted.Broadcast(false, {});
        return;
    }

    TArray<FOnlineSessionSearchResult> ResultsCopy = SearchSettings->SearchResults;
    ProcessSearchResults(ResultsCopy);
}

void UAsyncTask_FindSessions::ProcessSearchResults(const TArray<FOnlineSessionSearchResult>& InResults, bool bUpdateCache)
{
    const FString DesiredTypeStr = NexusOnline::SessionTypeToName(DesiredType).ToString();
    const TArray<FSessionSearchFilter> SimpleFiltersCopy = ResolvedSimpleFilters;

    TArray<TWeakObjectPtr<USessionFilterRule>> FilterRuleWeak;
    FilterRuleWeak.Reserve(ResolvedAdvancedRules.Num());
    for (const TObjectPtr<USessionFilterRule>& Rule : ResolvedAdvancedRules)
    {
        if (Rule)
        {
            FilterRuleWeak.Add(Rule);
        }
    }

    TArray<TWeakObjectPtr<USessionSortRule>> SortRuleWeak;
    SortRuleWeak.Reserve(ResolvedSortRules.Num());
    for (const TObjectPtr<USessionSortRule>& Rule : ResolvedSortRules)
    {
        if (Rule)
        {
            SortRuleWeak.Add(Rule);
        }
    }

    TWeakObjectPtr WeakThis(this);

    AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [WeakThis, DesiredTypeStr, ResultsCopy = InResults, SimpleFiltersCopy, FilterRuleWeak, SortRuleWeak, bUpdateCache]()
    {
        if (!WeakThis.IsValid())
        	return;

        TArray<FOnlineSessionSearchResult> WorkingResults;
        WorkingResults.Reserve(ResultsCopy.Num());

        for (const FOnlineSessionSearchResult& Result : ResultsCopy)
        {
            FString FoundType;
            Result.Session.SessionSettings.Get(FName("SESSION_TYPE_KEY"), FoundType);

            if (!FoundType.IsEmpty() && FoundType != DesiredTypeStr)
				continue;

            if (!NexusSessionFilterUtils::PassesAllFilters(SimpleFiltersCopy, Result))
				continue;

            bool bRejectedByRule = false;
            for (const TWeakObjectPtr<USessionFilterRule>& RuleWeak : FilterRuleWeak)
            {
                const USessionFilterRule* RuleInstance = RuleWeak.Get();
                if (!RuleInstance || !RuleInstance->bEnabled)
                	continue;

                if (!RuleInstance->PassesFilter(Result))
                {
                    bRejectedByRule = true;
                    break;
                }
            }

            if (bRejectedByRule)
				continue;

            WorkingResults.Add(Result);
        }

        if (!SortRuleWeak.IsEmpty())
        {
            Algo::Sort(WorkingResults, [&SortRuleWeak](const FOnlineSessionSearchResult& A, const FOnlineSessionSearchResult& B)
            {
                for (const TWeakObjectPtr<USessionSortRule>& RuleWeak : SortRuleWeak)
                {
                    const USessionSortRule* RuleInstance = RuleWeak.Get();
                    if (!RuleInstance || !RuleInstance->bEnabled)
						continue;

                    const bool bABeforeB = RuleInstance->Compare(A, B);
                    const bool bBBeforeA = RuleInstance->Compare(B, A);
                	
                    if (bABeforeB != bBBeforeA)
						return bABeforeB;
                }

                return false;
            });
        }

        TArray<FOnlineSessionSearchResultData> FinalResults;
        FinalResults.Reserve(WorkingResults.Num());

        for (const FOnlineSessionSearchResult& Result : WorkingResults)
        {
            FOnlineSessionSearchResultData Data;
            Result.Session.SessionSettings.Get(FName("SESSION_DISPLAY_NAME"), Data.SessionDisplayName);
            Result.Session.SessionSettings.Get(FName("MAP_NAME_KEY"), Data.MapName);
            Result.Session.SessionSettings.Get(FName("GAME_MODE_KEY"), Data.GameMode);
            Result.Session.SessionSettings.Get(FName("SESSION_TYPE_KEY"), Data.SessionType);

            Data.CurrentPlayers = Result.Session.SessionSettings.NumPublicConnections - Result.Session.NumOpenPublicConnections;
            Data.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
            Data.RawResult = Result;

            FinalResults.Add(MoveTemp(Data));
        }

        AsyncTask(ENamedThreads::GameThread, [WeakThis, FinalResults = MoveTemp(FinalResults), ResultsCopy, bUpdateCache]() mutable
        {
            if (!WeakThis.IsValid())
				return;

            UAsyncTask_FindSessions* StrongThis = WeakThis.Get();
            if (!StrongThis)
            	return;

            if (bUpdateCache)
            {
                GLastCachedRawResults = ResultsCopy;
                GLastSessionSearchTime = FPlatformTime::Seconds();
            }

            UE_LOG(LogNexusOnlineFilter, Log, TEXT("[NexusOnline|Filter] FindSessions complete: %d valid results."), FinalResults.Num());

            StrongThis->OnCompleted.Broadcast(true, FinalResults);
        });
    });
}

void UAsyncTask_FindSessions::RebuildResolvedFilters()
{
    ResolvedSimpleFilters = UserSimpleFilters;
    ResolvedAdvancedRules.Reset();
    ResolvedSortRules.Reset();

    if (UserPreset)
    {
        ResolvedSimpleFilters.Append(UserPreset->SimpleFilters);

        for (USessionFilterRule* Rule : UserPreset->AdvancedRules)
        {
            if (Rule)
            {
                ResolvedAdvancedRules.Add(Rule);
            }
        }

        for (USessionSortRule* Rule : UserPreset->SortRules)
        {
            if (Rule)
            {
                ResolvedSortRules.Add(Rule);
            }
        }
    }

    for (USessionFilterRule* Rule : UserAdvancedRules)
    {
        if (Rule)
        {
            ResolvedAdvancedRules.Add(Rule);
        }
    }

    for (USessionSortRule* Rule : UserSortRules)
    {
        if (Rule)
        {
            ResolvedSortRules.Add(Rule);
        }
    }

    ResolvedAdvancedRules.Sort([](const TObjectPtr<USessionFilterRule>& A, const TObjectPtr<USessionFilterRule>& B)
    {
        const USessionFilterRule* RuleA = A.Get();
        const USessionFilterRule* RuleB = B.Get();
        if (RuleA && RuleB)
        {
            if (RuleA->Priority == RuleB->Priority)
            {
                return RuleA < RuleB;
            }
        	
            return RuleA->Priority < RuleB->Priority;
        }

        return RuleB != nullptr;
    });

    ResolvedSortRules.Sort([](const TObjectPtr<USessionSortRule>& A, const TObjectPtr<USessionSortRule>& B)
    {
        const USessionSortRule* RuleA = A.Get();
        const USessionSortRule* RuleB = B.Get();
        if (RuleA && RuleB)
        {
            if (RuleA->Priority == RuleB->Priority)
            {
                return RuleA < RuleB;
            }
        	
            return RuleA->Priority < RuleB->Priority;
        }

        return RuleB != nullptr;
    });
}

void UAsyncTask_FindSessions::ApplyQueryFilters()
{
    if (!SearchSettings.IsValid())
		return;

    SearchSettings->QuerySettings.Set(FName("SEARCH_PRESENCE"), true, EOnlineComparisonOp::Equals);
    SearchSettings->QuerySettings.Set(FName("SESSION_TYPE_KEY"), NexusOnline::SessionTypeToName(DesiredType).ToString(), EOnlineComparisonOp::Equals);

    NexusSessionFilterUtils::ApplyFiltersToSettings(ResolvedSimpleFilters, *SearchSettings);

    for (const TObjectPtr<USessionFilterRule>& Rule : ResolvedAdvancedRules)
    {
        if (!Rule || !Rule->bEnabled || !Rule->bApplyToSearchQuery)
			continue;

        Rule->ConfigureSearchSettings(*SearchSettings);
        UE_LOG(LogNexusOnlineFilter, Verbose, TEXT("[NexusOnline|Filter] Applied advanced query rule: %s"), *Rule->GetRuleDescription());
    }

    for (const TObjectPtr<USessionSortRule>& Rule : ResolvedSortRules)
    {
        if (!Rule || !Rule->bEnabled)
            continue;

        Rule->ConfigureSearchSettings(*SearchSettings);
        UE_LOG(LogNexusOnlineSort, Verbose, TEXT("[NexusOnline|Sort] Applied query sort rule: %s"), *Rule->GetRuleDescription());
    }
}

