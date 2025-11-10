#include "Filters/Rules/SessionFilterRule_KeyValue.h"

void USessionFilterRule_KeyValue::ConfigureSearchSettings(FOnlineSessionSearch& SearchSettings) const
{
    if (!bEnabled || !bApplyToSearchQuery || Key.IsNone())
		return;

    FSessionSearchFilter Filter;
    Filter.Key = Key;
    Filter.Value = ExpectedValue;
    Filter.ComparisonOp = Comparison;
    Filter.bApplyToQuerySettings = true;

    if (!Filter.ApplyToSearchSettings(SearchSettings))
    {
    	UE_LOG(LogNexusOnlineFilter, Verbose, TEXT("[NexusOnline|Filter] Unable to apply key/value filter %s"), *GetRuleDescription());
    }
}

bool USessionFilterRule_KeyValue::PassesFilter(const FOnlineSessionSearchResult& Result) const
{
    if (!bEnabled || Key.IsNone())
        return true;

    FSessionSearchFilter Filter;
    Filter.Key = Key;
    Filter.Value = ExpectedValue;
    Filter.ComparisonOp = Comparison;
    Filter.bApplyToQuerySettings = false;

    const bool bResult = Filter.MatchesResult(Result);
    if (!bResult)
    {
    	UE_LOG(LogNexusOnlineFilter, VeryVerbose, TEXT("[NexusOnline|Filter] Result filtered by %s"), *GetRuleDescription());
    }

    return bResult;
}

FString USessionFilterRule_KeyValue::GetRuleDescription() const
{
    const FString ValueString = ExpectedValue.ToDebugString();
    return FString::Printf(TEXT("%s %s %s"), *Key.ToString(), *StaticEnum<ENexusSessionComparisonOp>()->GetNameStringByValue(static_cast<int64>(Comparison)), *ValueString);
}

