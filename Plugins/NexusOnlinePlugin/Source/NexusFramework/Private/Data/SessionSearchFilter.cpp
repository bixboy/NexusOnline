#include "Data/SessionSearchFilter.h"

#include "OnlineSessionSettings.h"
#include "Containers/UnrealString.h"

namespace
{
        FString VariantToString(const FVariantData& Data)
        {
                FString Out;
                if (Data.GetValue(Out))
                {
                        return Out;
                }

                int32 IntValue = 0;
                if (Data.GetValue(IntValue))
                {
                        return LexToString(IntValue);
                }

                double DoubleValue = 0.0;
                if (Data.GetValue(DoubleValue))
                {
                        return LexToString(DoubleValue);
                }

                bool bBoolValue = false;
                if (Data.GetValue(bBoolValue))
                {
                        return bBoolValue ? TEXT("true") : TEXT("false");
                }

                return FString();
        }

        bool EvaluateNumeric(double Left, double Right, EOnlineComparisonOp::Type Op)
        {
                switch (Op)
                {
                case EOnlineComparisonOp::Equals:              return FMath::IsNearlyEqual(Left, Right, KINDA_SMALL_NUMBER);
                case EOnlineComparisonOp::NotEquals:           return !FMath::IsNearlyEqual(Left, Right, KINDA_SMALL_NUMBER);
                case EOnlineComparisonOp::GreaterThan:         return Left > Right;
                case EOnlineComparisonOp::GreaterThanEquals:   return Left >= Right;
                case EOnlineComparisonOp::LessThan:            return Left < Right;
                case EOnlineComparisonOp::LessThanEquals:      return Left <= Right;
                default:                                      return false;
                }
        }

        bool EvaluateBool(bool Left, bool Right, EOnlineComparisonOp::Type Op)
        {
                switch (Op)
                {
                case EOnlineComparisonOp::Equals:     return Left == Right;
                case EOnlineComparisonOp::NotEquals:  return Left != Right;
                default:                              return false;
                }
        }

        bool EvaluateString(const FString& Left, const FString& Right, EOnlineComparisonOp::Type Op)
        {
                switch (Op)
                {
                case EOnlineComparisonOp::Equals:     return Left.Equals(Right, ESearchCase::IgnoreCase);
                case EOnlineComparisonOp::NotEquals:  return !Left.Equals(Right, ESearchCase::IgnoreCase);
                default:                              return false;
                }
        }
}

bool FSessionFilterValue::ToVariantData(FVariantData& OutData) const
{
        switch (Type)
        {
        case ENexusSessionFilterValueType::String:
                OutData.SetValue(StringValue);
                return true;
        case ENexusSessionFilterValueType::Int32:
                OutData.SetValue(IntValue);
                return true;
        case ENexusSessionFilterValueType::Float:
                OutData.SetValue(static_cast<double>(FloatValue));
                return true;
        case ENexusSessionFilterValueType::Bool:
                OutData.SetValue(bBoolValue);
                return true;
        default:
                return false;
        }
}

bool FSessionFilterValue::CompareVariant(const FVariantData& OtherData, EOnlineComparisonOp::Type Op) const
{
        if (Op == EOnlineComparisonOp::Exists)
        {
                return OtherData.GetType() != EOnlineKeyValuePairDataType::Empty;
        }

        switch (Type)
        {
        case ENexusSessionFilterValueType::String:
        {
                const FString Other = VariantToString(OtherData);
                return EvaluateString(Other, StringValue, Op);
        }
        case ENexusSessionFilterValueType::Int32:
        {
                int32 IntValueOther = 0;
                if (OtherData.GetValue(IntValueOther))
                {
                        return EvaluateNumeric(static_cast<double>(IntValueOther), static_cast<double>(IntValue), Op);
                }

                double DoubleOther = 0.0;
                if (OtherData.GetValue(DoubleOther))
                {
                        return EvaluateNumeric(DoubleOther, static_cast<double>(IntValue), Op);
                }

                const FString Other = VariantToString(OtherData);
                if (!Other.IsEmpty())
                {
                        return EvaluateNumeric(FCString::Atod(*Other), static_cast<double>(IntValue), Op);
                }

                return Op == EOnlineComparisonOp::NotEquals;
        }
        case ENexusSessionFilterValueType::Float:
        {
                double DoubleOther = 0.0;
                if (!OtherData.GetValue(DoubleOther))
                {
                        int32 IntValueOther = 0;
                        if (OtherData.GetValue(IntValueOther))
                        {
                                DoubleOther = static_cast<double>(IntValueOther);
                        }
                        else
                        {
                                const FString Other = VariantToString(OtherData);
                                if (!Other.IsEmpty())
                                {
                                        DoubleOther = FCString::Atod(*Other);
                                }
                                else
                                {
                                        return Op == EOnlineComparisonOp::NotEquals;
                                }
                        }
                }

                return EvaluateNumeric(DoubleOther, static_cast<double>(FloatValue), Op);
        }
        case ENexusSessionFilterValueType::Bool:
        {
                bool bOther = false;
                if (OtherData.GetValue(bOther))
                {
                        return EvaluateBool(bOther, bBoolValue, Op);
                }

                int32 IntValueOther = 0;
                if (OtherData.GetValue(IntValueOther))
                {
                        return EvaluateBool(IntValueOther != 0, bBoolValue, Op);
                }

                const FString Other = VariantToString(OtherData);
                if (!Other.IsEmpty())
                {
                        const bool bFromString = Other.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Other == TEXT("1");
                        return EvaluateBool(bFromString, bBoolValue, Op);
                }

                return Op == EOnlineComparisonOp::NotEquals;
        }
        default:
                break;
        }

        return false;
}

FString FSessionFilterValue::ToDebugString() const
{
        switch (Type)
        {
        case ENexusSessionFilterValueType::String:
                return StringValue;
        case ENexusSessionFilterValueType::Int32:
                return LexToString(IntValue);
        case ENexusSessionFilterValueType::Float:
                return LexToString(FloatValue);
        case ENexusSessionFilterValueType::Bool:
                return bBoolValue ? TEXT("true") : TEXT("false");
        default:
                break;
        }

        return TEXT("<invalid>");
}

EOnlineComparisonOp::Type FSessionSearchFilter::ToOnlineOp() const
{
        switch (ComparisonOp)
        {
        case ENexusSessionComparisonOp::Equals:            return EOnlineComparisonOp::Equals;
        case ENexusSessionComparisonOp::NotEquals:         return EOnlineComparisonOp::NotEquals;
        case ENexusSessionComparisonOp::GreaterThan:       return EOnlineComparisonOp::GreaterThan;
        case ENexusSessionComparisonOp::GreaterThanEquals: return EOnlineComparisonOp::GreaterThanEquals;
        case ENexusSessionComparisonOp::LessThan:          return EOnlineComparisonOp::LessThan;
        case ENexusSessionComparisonOp::LessThanEquals:    return EOnlineComparisonOp::LessThanEquals;
        case ENexusSessionComparisonOp::Exists:            return EOnlineComparisonOp::Exists;
        default:                                           return EOnlineComparisonOp::Equals;
        }
}

bool FSessionSearchFilter::ApplyToSearchSettings(FOnlineSessionSearch& Search) const
{
        if (!bApplyToQuerySettings || Key.IsNone())
        {
                return false;
        }

        const EOnlineComparisonOp::Type Op = ToOnlineOp();
        switch (Value.Type)
        {
        case ENexusSessionFilterValueType::String:
                Search.QuerySettings.Set(Key, Value.StringValue, Op);
                return true;
        case ENexusSessionFilterValueType::Int32:
                Search.QuerySettings.Set(Key, Value.IntValue, Op);
                return true;
        case ENexusSessionFilterValueType::Float:
                Search.QuerySettings.Set(Key, static_cast<double>(Value.FloatValue), Op);
                return true;
        case ENexusSessionFilterValueType::Bool:
                Search.QuerySettings.Set(Key, Value.bBoolValue, Op);
                return true;
        default:
                break;
        }

        return false;
}

bool FSessionSearchFilter::MatchesResult(const FOnlineSessionSearchResult& Result) const
{
        if (Key.IsNone())
        {
                return true;
        }

        if (ComparisonOp == ENexusSessionComparisonOp::Exists)
        {
                const bool bHasValue = Result.Session.SessionSettings.Settings.Contains(Key);
                return bHasValue;
        }

        const FOnlineSessionSetting* Setting = Result.Session.SessionSettings.Settings.Find(Key);
        if (!Setting)
        {
                return ComparisonOp == ENexusSessionComparisonOp::NotEquals;
        }

        return Value.CompareVariant(Setting->Data, ToOnlineOp());
}

void NexusSessionFilterUtils::ApplyFiltersToSettings(const TArray<FSessionSearchFilter>& Filters, FOnlineSessionSearch& SearchSettings)
{
        for (const FSessionSearchFilter& Filter : Filters)
        {
                Filter.ApplyToSearchSettings(SearchSettings);
        }
}

void NexusSessionFilterUtils::ApplyFiltersToSettings(const TArray<FSessionSearchFilter>& Filters, FOnlineSessionSettings& SessionSettings)
{
        for (const FSessionSearchFilter& Filter : Filters)
        {
                if (Filter.Key.IsNone())
                {
                        continue;
                }

                FVariantData Variant;
                if (!Filter.Value.ToVariantData(Variant))
                {
                        continue;
                }

                SessionSettings.Set(Filter.Key, Variant, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
        }
}

bool NexusSessionFilterUtils::PassesAllFilters(const TArray<FSessionSearchFilter>& Filters, const FOnlineSessionSearchResult& Result)
{
        for (const FSessionSearchFilter& Filter : Filters)
        {
                if (!Filter.MatchesResult(Result))
                {
                        return false;
                }
        }

        return true;
}

