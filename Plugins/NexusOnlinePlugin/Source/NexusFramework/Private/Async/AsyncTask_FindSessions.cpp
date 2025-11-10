#include "Async/AsyncTask_FindSessions.h"

#include "Algo/Sort.h"
#include "Filters/SessionFilterRule.h"
#include "Filters/SessionSortRule.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Utils/NexusOnlineHelpers.h"

namespace
{
    static FOnlineSessionSearchResultData BuildResultData(const FOnlineSessionSearchResult& Result)
    {
        FOnlineSessionSearchResultData Data;
        Result.Session.SessionSettings.Get(TEXT("SESSION_DISPLAY_NAME"), Data.SessionDisplayName);
        Result.Session.SessionSettings.Get(TEXT("MAP_NAME_KEY"), Data.MapName);
        Result.Session.SessionSettings.Get(TEXT("GAME_MODE_KEY"), Data.GameMode);
        Result.Session.SessionSettings.Get(TEXT("SESSION_TYPE_KEY"), Data.SessionType);
        Data.CurrentPlayers = Result.Session.SessionSettings.NumPublicConnections - Result.Session.NumOpenPublicConnections;
        Data.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
        Data.RawResult = Result;
        return Data;
    }
}

UAsyncTask_FindSessions* UAsyncTask_FindSessions::FindSessions(
    UObject* WorldContextObject,
    ENexusSessionType SessionType,
    int32 MaxResults,
    const TArray<FSessionSearchFilter>& SimpleFilters,
    const TArray<USessionFilterRule*>& AdvancedRules,
    const TArray<USessionSortRule*>& SortRules,
    USessionFilterPreset* Preset)
{
    UAsyncTask_FindSessions* Node = NewObject<UAsyncTask_FindSessions>();
    Node->WorldContextObject = WorldContextObject;
    Node->DesiredType = SessionType;

    Node->SearchSettings = MakeShareable(new FOnlineSessionSearch());
    Node->SearchSettings->MaxSearchResults = MaxResults;
    Node->SearchSettings->bIsLanQuery = false;
    Node->SearchSettings->QuerySettings.Set(FName("USES_PRESENCE"), true, EOnlineComparisonOp::Equals);

    Node->ResolvePresetFilters(Preset, SimpleFilters, AdvancedRules, SortRules);

    for (const FSessionSearchFilter& Filter : Node->ResolvedSimpleFilters)
    {
        Filter.ApplyToQuerySettings(*Node->SearchSettings);
    }

    return Node;
}

void UAsyncTask_FindSessions::Activate()
{
    if (!WorldContextObject)
    {
        UE_LOG(LogTemp, Error, TEXT("[NexusOnline|Filter] Invalid WorldContextObject in FindSessions"));
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

    FindSessionsHandle = Session->AddOnFindSessionsCompleteDelegate_Handle(
        FOnFindSessionsCompleteDelegate::CreateUObject(this, &UAsyncTask_FindSessions::OnFindSessionsComplete));

    UE_LOG(LogTemp, Log, TEXT("[NexusOnline|Filter] Searching sessions of type: %s (Max: %d, QueryFilters: %d, Advanced: %d)"),
        *NexusOnline::SessionTypeToName(DesiredType).ToString(),
        SearchSettings.IsValid() ? SearchSettings->MaxSearchResults : 0,
        ResolvedSimpleFilters.Num(),
        ResolvedAdvancedRules.Num());

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
        TArray<FOnlineSessionSearchResult> ValidResults;
        for (const FOnlineSessionSearchResult& Result : SearchSettings->SearchResults)
        {
            if (ApplyLocalFilters(Result))
            {
                ValidResults.Add(Result);
            }
        }

        ApplySortRules(ValidResults);

        for (const FOnlineSessionSearchResult& Result : ValidResults)
        {
            ResultsData.Add(BuildResultData(Result));
        }

        UE_LOG(LogTemp, Log, TEXT("[NexusOnline|Filter] FindSessions complete. Valid sessions: %d"), ResultsData.Num());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[NexusOnline|Filter] FindSessions failed to complete properly."));
    }

    OnCompleted.Broadcast(bWasSuccessful, ResultsData);
}

void UAsyncTask_FindSessions::ResolvePresetFilters(USessionFilterPreset* Preset, const TArray<FSessionSearchFilter>& SimpleFilters, const TArray<USessionFilterRule*>& AdvancedRules, const TArray<USessionSortRule*>& SortRules)
{
    ResolvedSimpleFilters.Reset();
    ResolvedAdvancedRules.Reset();
    ResolvedSortRules.Reset();

    if (Preset)
    {
        Preset->BuildResolvedFilters(this, ResolvedSimpleFilters, ResolvedAdvancedRules, ResolvedSortRules);
    }

    ResolvedSimpleFilters.Append(SimpleFilters);

    for (USessionFilterRule* Rule : AdvancedRules)
    {
        if (Rule)
        {
            ResolvedAdvancedRules.Add(DuplicateObject<USessionFilterRule>(Rule, this));
        }
    }

    for (USessionSortRule* Rule : SortRules)
    {
        if (Rule)
        {
            ResolvedSortRules.Add(DuplicateObject<USessionSortRule>(Rule, this));
        }
    }

    const FName TypeKey("SESSION_TYPE_KEY");
    const FString DesiredTypeName = NexusOnline::SessionTypeToName(DesiredType).ToString();
    const bool bHasTypeFilter = ResolvedSimpleFilters.ContainsByPredicate([TypeKey](const FSessionSearchFilter& Filter)
    {
        return Filter.Key == TypeKey;
    });

    if (!bHasTypeFilter)
    {
        FSessionSearchFilter& TypeFilter = ResolvedSimpleFilters.AddDefaulted_GetRef();
        TypeFilter.Key = TypeKey;
        TypeFilter.ValueType = ENexusSessionFilterValueType::String;
        TypeFilter.StringValue = DesiredTypeName;
        TypeFilter.ComparisonOp = EOnlineComparisonOp::Equals;
        TypeFilter.bApplyToQuery = true;
        TypeFilter.bApplyToResults = true;
    }
}

bool UAsyncTask_FindSessions::ApplyLocalFilters(const FOnlineSessionSearchResult& Result) const
{
    for (const FSessionSearchFilter& Filter : ResolvedSimpleFilters)
    {
        if (!Filter.Matches(Result))
        {
            UE_LOG(LogTemp, Verbose, TEXT("[NexusOnline|Filter] Simple filter rejected session %s with key %s"),
                *Result.GetSessionIdStr(), *Filter.Key.ToString());
            return false;
        }
    }

    for (USessionFilterRule* Rule : ResolvedAdvancedRules)
    {
        if (Rule && !Rule->PassesFilter(Result))
        {
            UE_LOG(LogTemp, Verbose, TEXT("[NexusOnline|Filter] Advanced rule %s rejected session %s"),
                *Rule->GetName(), *Result.GetSessionIdStr());
            return false;
        }
    }

    return true;
}

void UAsyncTask_FindSessions::ApplySortRules(TArray<FOnlineSessionSearchResult>& Results) const
{
    if (ResolvedSortRules.Num() == 0 || Results.Num() <= 1)
    {
        return;
    }

    TArray<USessionSortRule*> ActiveRules;
    ActiveRules.Reserve(ResolvedSortRules.Num());
    for (USessionSortRule* Rule : ResolvedSortRules)
    {
        if (Rule && Rule->bEnabled)
        {
            ActiveRules.Add(Rule);
        }
    }

    ActiveRules.Sort([](const USessionSortRule* const& A, const USessionSortRule* const& B)
    {
        return A->Priority < B->Priority;
    });

    if (ActiveRules.Num() == 0)
    {
        return;
    }

    Results.Sort([ActiveRules](const FOnlineSessionSearchResult& A, const FOnlineSessionSearchResult& B)
    {
        for (USessionSortRule* Rule : ActiveRules)
        {
            const ENexusSessionSortPreference Preference = Rule->Compare(A, B);
            if (Preference == ENexusSessionSortPreference::PreferFirst)
            {
                return true;
            }

            if (Preference == ENexusSessionSortPreference::PreferSecond)
            {
                return false;
            }
        }

        return false;
    });

    UE_LOG(LogTemp, Log, TEXT("[NexusOnline|Sort] Applied %d sort rules."), ActiveRules.Num());
}
