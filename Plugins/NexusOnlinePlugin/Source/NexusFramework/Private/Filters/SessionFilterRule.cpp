#include "Filters/SessionFilterRule.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemTypes.h"

bool USessionFilterRule::PassesFilter(const FOnlineSessionSearchResult& Result) const
{
    if (!bEnabled)
    {
        return true;
    }

    return NativePassesFilter(Result);
}

bool USessionFilterRule::NativePassesFilter(const FOnlineSessionSearchResult& Result) const
{
    return const_cast<USessionFilterRule*>(this)->K2_PassesFilter(ConvertToBlueprintData(Result));
}

bool USessionFilterRule::K2_PassesFilter_Implementation(const FOnlineSessionSearchResultData& /*SessionData*/) const
{
    return true;
}

FOnlineSessionSearchResultData USessionFilterRule::ConvertToBlueprintData(const FOnlineSessionSearchResult& Result)
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

bool USessionFilterRule_KeyValue::NativePassesFilter(const FOnlineSessionSearchResult& Result) const
{
    if (Key.IsNone())
    {
        return true;
    }

    const FOnlineSessionSetting* Setting = Result.Session.SessionSettings.Settings.Find(Key);
    if (!Setting)
    {
        // If the value is missing we only keep the session when the rule explicitly checks for inequality.
        return ComparisonOp == EOnlineComparisonOp::NotEquals;
    }

    const FString CurrentValue = Setting->Data.ToString();
    const FString FilterValue = ExpectedValue;

    const double CurrentNumber = FCString::Atod(*CurrentValue);
    const double FilterNumber = FCString::Atod(*FilterValue);

    switch (ComparisonOp)
    {
    case EOnlineComparisonOp::Equals:
        return CurrentValue.Equals(FilterValue, ESearchCase::IgnoreCase);
    case EOnlineComparisonOp::NotEquals:
        return !CurrentValue.Equals(FilterValue, ESearchCase::IgnoreCase);
    case EOnlineComparisonOp::GreaterThan:
        return CurrentNumber > FilterNumber;
    case EOnlineComparisonOp::GreaterThanEquals:
        return CurrentNumber >= FilterNumber;
    case EOnlineComparisonOp::LessThan:
        return CurrentNumber < FilterNumber;
    case EOnlineComparisonOp::LessThanEquals:
        return CurrentNumber <= FilterNumber;
    default:
        return true;
    }
}

bool USessionFilterRule_Ping::NativePassesFilter(const FOnlineSessionSearchResult& Result) const
{
    return Result.PingInMs <= MaxPing;
}
