#include "Data/SessionSearchFilter.h"
#include "OnlineSessionSettings.h"

FVariantData FSessionSearchFilter::ToVariantData() const
{
    FVariantData Variant;
    switch (ValueType)
    {
    case ENexusSessionFilterValueType::String:
        Variant.SetValue(StringValue);
        break;
    case ENexusSessionFilterValueType::Int32:
        Variant.SetValue(IntValue);
        break;
    case ENexusSessionFilterValueType::Float:
        Variant.SetValue(FloatValue);
        break;
    case ENexusSessionFilterValueType::Boolean:
        Variant.SetValue(bBoolValue);
        break;
    default:
        Variant.SetValue(StringValue);
        break;
    }
    return Variant;
}

void FSessionSearchFilter::ApplyToQuerySettings(FOnlineSessionSearch& SearchSettings) const
{
    if (!bApplyToQuery || Key.IsNone())
    {
        return;
    }

    const FOnlineSessionSearchParam Param(ToVariantData(), ComparisonOp);
    SearchSettings.QuerySettings.Set(Key, Param);
}

bool FSessionSearchFilter::Matches(const FOnlineSessionSearchResult& Result) const
{
    if (!bApplyToResults || Key.IsNone())
    {
        return true;
    }

    const FOnlineSessionSetting* Setting = Result.Session.SessionSettings.Settings.Find(Key);
    if (!Setting)
    {
        return ComparisonOp == EOnlineComparisonOp::NotEquals;
    }

    return EvaluateComparison(Setting->Data);
}

bool FSessionSearchFilter::EvaluateComparison(const FVariantData& Value) const
{
    const FString FilterString = ToVariantData().ToString();
    const FString ValueString = Value.ToString();

    const double FilterNumber = FCString::Atod(*FilterString);
    const double ValueNumber = FCString::Atod(*ValueString);

    switch (ComparisonOp)
    {
    case EOnlineComparisonOp::Equals:
        return ValueString.Equals(FilterString, ESearchCase::IgnoreCase);
    case EOnlineComparisonOp::NotEquals:
        return !ValueString.Equals(FilterString, ESearchCase::IgnoreCase);
    case EOnlineComparisonOp::GreaterThan:
        return ValueNumber > FilterNumber;
    case EOnlineComparisonOp::GreaterThanEquals:
        return ValueNumber >= FilterNumber;
    case EOnlineComparisonOp::LessThan:
        return ValueNumber < FilterNumber;
    case EOnlineComparisonOp::LessThanEquals:
        return ValueNumber <= FilterNumber;
    default:
        return true;
    }
}
