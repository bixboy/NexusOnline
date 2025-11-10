#include "Filters/Rules/SessionSortRule_Ping.h"
#include "OnlineSessionSettings.h"


bool USessionSortRule_Ping::Compare(const FOnlineSessionSearchResult& A, const FOnlineSessionSearchResult& B) const
{
    if (!bEnabled)
    {
        return false;
    }

    if (bAscending)
    {
        return A.PingInMs < B.PingInMs;
    }

    return A.PingInMs > B.PingInMs;
}

FString USessionSortRule_Ping::GetRuleDescription() const
{
    return FString::Printf(TEXT("Ping %s"), bAscending ? TEXT("ASC") : TEXT("DESC"));
}

