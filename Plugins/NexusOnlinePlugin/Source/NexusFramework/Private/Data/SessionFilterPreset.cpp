#include "Data/SessionFilterPreset.h"

void USessionFilterPreset::BuildResolvedFilters(UObject* Outer, TArray<FSessionSearchFilter>& OutSimpleFilters, TArray<TObjectPtr<USessionFilterRule>>& OutFilterRules, TArray<TObjectPtr<USessionSortRule>>& OutSortRules) const
{
    OutSimpleFilters.Append(SimpleFilters);

    for (USessionFilterRule* Rule : FilterRules)
    {
        if (Rule)
        {
            OutFilterRules.Add(DuplicateObject<USessionFilterRule>(Rule, Outer));
        }
    }

    for (USessionSortRule* Rule : SortRules)
    {
        if (Rule)
        {
            OutSortRules.Add(DuplicateObject<USessionSortRule>(Rule, Outer));
        }
    }
}
