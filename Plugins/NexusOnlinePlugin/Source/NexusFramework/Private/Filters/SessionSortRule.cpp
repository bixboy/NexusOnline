#include "Filters/SessionSortRule.h"

void USessionSortRule::ConfigureSearchSettings(FOnlineSessionSearch& SearchSettings) const
{
    (void)SearchSettings;
}

bool USessionSortRule::Compare(const FOnlineSessionSearchResult& A, const FOnlineSessionSearchResult& B) const
{
    (void)A;
    (void)B;
    return false;
}

FString USessionSortRule::GetRuleDescription() const
{
    return GetClass() ? GetClass()->GetName() : TEXT("SessionSortRule");
}

