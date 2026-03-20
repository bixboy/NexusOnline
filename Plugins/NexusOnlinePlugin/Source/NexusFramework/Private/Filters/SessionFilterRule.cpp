#include "Filters/SessionFilterRule.h"

#include "OnlineSessionSettings.h"


void USessionFilterRule::ConfigureSearchSettings(FOnlineSessionSearch& SearchSettings) const
{
    (void)SearchSettings;
}

bool USessionFilterRule::PassesFilter(const FOnlineSessionSearchResult& Result) const
{
    (void)Result;
    return true;
}

FString USessionFilterRule::GetRuleDescription() const
{
    return GetClass() ? GetClass()->GetName() : TEXT("SessionFilterRule");
}

