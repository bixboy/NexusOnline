#include "Filters/SessionSortRule.h"
#include "OnlineSessionSettings.h"

ENexusSessionSortPreference USessionSortRule::Compare(const FOnlineSessionSearchResult& First, const FOnlineSessionSearchResult& Second) const
{
    if (!bEnabled)
    {
        return ENexusSessionSortPreference::NoPreference;
    }

    ENexusSessionSortPreference NativeResult = NativeCompare(First, Second);
    if (NativeResult != ENexusSessionSortPreference::NoPreference)
    {
        return NativeResult;
    }

    return const_cast<USessionSortRule*>(this)->K2_Compare(ConvertToBlueprintData(First), ConvertToBlueprintData(Second));
}

ENexusSessionSortPreference USessionSortRule::NativeCompare(const FOnlineSessionSearchResult& First, const FOnlineSessionSearchResult& Second) const
{
    return const_cast<USessionSortRule*>(this)->K2_Compare(ConvertToBlueprintData(First), ConvertToBlueprintData(Second));
}

ENexusSessionSortPreference USessionSortRule::K2_Compare_Implementation(const FOnlineSessionSearchResultData& /*First*/, const FOnlineSessionSearchResultData& /*Second*/) const
{
    return ENexusSessionSortPreference::NoPreference;
}

FOnlineSessionSearchResultData USessionSortRule::ConvertToBlueprintData(const FOnlineSessionSearchResult& Result)
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

ENexusSessionSortPreference USessionSortRule_Ping::NativeCompare(const FOnlineSessionSearchResult& First, const FOnlineSessionSearchResult& Second) const
{
    if (First.PingInMs < Second.PingInMs)
    {
        return ENexusSessionSortPreference::PreferFirst;
    }

    if (First.PingInMs > Second.PingInMs)
    {
        return ENexusSessionSortPreference::PreferSecond;
    }

    return ENexusSessionSortPreference::NoPreference;
}
