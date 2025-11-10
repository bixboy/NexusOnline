#include "Filters/SessionFilterRule_Ping.h"

#include "OnlineSessionSettings.h"

bool USessionFilterRule_Ping::PassesFilter(const FOnlineSessionSearchResult& Result) const
{
        if (!bEnabled)
        {
                return true;
        }

        const bool bValid = Result.PingInMs <= MaxPing;
        if (!bValid)
        {
                UE_LOG(LogNexusOnlineFilter, VeryVerbose, TEXT("[NexusOnline|Filter] Rejecting session with ping %d (max %d)"), Result.PingInMs, MaxPing);
        }

        return bValid;
}

FString USessionFilterRule_Ping::GetRuleDescription() const
{
        return FString::Printf(TEXT("Ping <= %d"), MaxPing);
}

