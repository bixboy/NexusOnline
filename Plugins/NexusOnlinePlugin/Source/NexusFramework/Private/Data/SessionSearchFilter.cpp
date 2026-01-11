#include "Data/SessionSearchFilter.h"
#include "OnlineSessionSettings.h"
#include "Containers/UnrealString.h"

namespace
{
    FString VariantToString(const FVariantData& Data)
    {
        FString Out;

        switch (Data.GetType())
        {
        case EOnlineKeyValuePairDataType::String:
            Data.GetValue(Out);
            return Out;

        case EOnlineKeyValuePairDataType::Int32:
        {
            int32 IntVal = 0;
            Data.GetValue(IntVal);
            return LexToString(IntVal);
        }

        case EOnlineKeyValuePairDataType::Double:
        {
            double DVal = 0.0;
            Data.GetValue(DVal);
            return LexToString(DVal);
        }

        case EOnlineKeyValuePairDataType::Bool:
        {
            bool bVal = false;
            Data.GetValue(bVal);
            return bVal ? TEXT("true") : TEXT("false");
        }

        default:
            return TEXT("");
        }
    }

    bool EvaluateNumeric(double Left, double Right, EOnlineComparisonOp::Type Op)
    {
        switch (Op)
        {
        case EOnlineComparisonOp::Equals:
        	return FMath::IsNearlyEqual(Left, Right, KINDA_SMALL_NUMBER);
        	
        case EOnlineComparisonOp::NotEquals:
        	return !FMath::IsNearlyEqual(Left, Right, KINDA_SMALL_NUMBER);
        	
        case EOnlineComparisonOp::GreaterThan:
        	return Left > Right;
        	
        case EOnlineComparisonOp::GreaterThanEquals:
        	return Left >= Right;
        	
        case EOnlineComparisonOp::LessThan:
        	return Left < Right;
        	
        case EOnlineComparisonOp::LessThanEquals:
        	return Left <= Right;
        	
        default:
        	return false;
        }
    }

    bool EvaluateBool(bool Left, bool Right, EOnlineComparisonOp::Type Op)
    {
        switch (Op)
        {
        case EOnlineComparisonOp::Equals:
        	return Left == Right;
        	
        case EOnlineComparisonOp::NotEquals:
        	return Left != Right;
        	
        default:
        	return false;
        }
    }

    bool EvaluateString(const FString& Left, const FString& Right, EOnlineComparisonOp::Type Op)
    {
        switch (Op)
        {
        case EOnlineComparisonOp::Equals:
        	return Left.Equals(Right, ESearchCase::IgnoreCase);
        	
        case EOnlineComparisonOp::NotEquals:
        	return !Left.Equals(Right, ESearchCase::IgnoreCase);
        	
        default:
        	return false;
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
    if (Op == static_cast<EOnlineComparisonOp::Type>(255))
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
        int32 IntVal = 0;
        if (OtherData.GetType() == EOnlineKeyValuePairDataType::Int32)
        {
            OtherData.GetValue(IntVal);
            return EvaluateNumeric(IntVal, IntValue, Op);
        }

        const FString Other = VariantToString(OtherData);
        if (!Other.IsEmpty())
        {
            return EvaluateNumeric(FCString::Atod(*Other), IntValue, Op);
        }

        return Op == EOnlineComparisonOp::NotEquals;
    }

    case ENexusSessionFilterValueType::Float:
    {
        double DoubleOther = 0.0;
        if (OtherData.GetType() == EOnlineKeyValuePairDataType::Double)
        {
            OtherData.GetValue(DoubleOther);
        }
        else
        {
            const FString Other = VariantToString(OtherData);
            DoubleOther = FCString::Atod(*Other);
        }

        return EvaluateNumeric(DoubleOther, static_cast<double>(FloatValue), Op);
    }

    case ENexusSessionFilterValueType::Bool:
    {
        bool bOther = false;
        if (OtherData.GetType() == EOnlineKeyValuePairDataType::Bool)
        {
            OtherData.GetValue(bOther);
            return EvaluateBool(bOther, bBoolValue, Op);
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
        return false;
    }
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
    	
    default: return TEXT("<invalid>");
    }
}

EOnlineComparisonOp::Type FSessionSearchFilter::ToOnlineOp() const
{
    switch (ComparisonOp)
    {
    case ENexusSessionComparisonOp::Equals:
    	return EOnlineComparisonOp::Equals;
    	
    case ENexusSessionComparisonOp::NotEquals:
    	return EOnlineComparisonOp::NotEquals;
    	
    case ENexusSessionComparisonOp::GreaterThan:
    	return EOnlineComparisonOp::GreaterThan;
    	
    case ENexusSessionComparisonOp::GreaterThanEquals:
    	return EOnlineComparisonOp::GreaterThanEquals;
    	
    case ENexusSessionComparisonOp::LessThan:
    	return EOnlineComparisonOp::LessThan;
    	
    case ENexusSessionComparisonOp::LessThanEquals:
    	return EOnlineComparisonOp::LessThanEquals;
    	
    case ENexusSessionComparisonOp::Exists:
    	return static_cast<EOnlineComparisonOp::Type>(255);
    	
    default:
    	return EOnlineComparisonOp::Equals;
    }
}

bool FSessionSearchFilter::ApplyToSearchSettings(FOnlineSessionSearch& Search) const
{
	if (!bApplyToQuerySettings || Key.IsNone())
		return false;

	if (ComparisonOp == ENexusSessionComparisonOp::Exists)
		return false; 

	const EOnlineComparisonOp::Type Op = ToOnlineOp();
	
    switch (Value.Type)
    {
    	
    case ENexusSessionFilterValueType::String:
    	Search.QuerySettings.Set(Key, Value.StringValue, Op);
    	break;
    	
    case ENexusSessionFilterValueType::Int32:
    	Search.QuerySettings.Set(Key, Value.IntValue, Op);
    	break;
    	
    case ENexusSessionFilterValueType::Float:
    	Search.QuerySettings.Set(Key, static_cast<double>(Value.FloatValue), Op);
    	break;
    	
    case ENexusSessionFilterValueType::Bool:
    	Search.QuerySettings.Set(Key, Value.bBoolValue, Op);
    	break;
    	
    }

    return true;
}

bool FSessionSearchFilter::MatchesResult(const FOnlineSessionSearchResult& Result) const
{
    if (Key.IsNone())
        return true;

    if (ComparisonOp == ENexusSessionComparisonOp::Exists)
    {
        return Result.Session.SessionSettings.Settings.Contains(Key);
    }

    const FOnlineSessionSetting* Setting = Result.Session.SessionSettings.Settings.Find(Key);
    if (!Setting)
        return ComparisonOp == ENexusSessionComparisonOp::NotEquals;

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
			continue;

		const FSessionFilterValue& Val = Filter.Value;

		switch (Val.Type)
		{
		case ENexusSessionFilterValueType::String:
			SessionSettings.Set(Filter.Key, Val.StringValue, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
			break;

		case ENexusSessionFilterValueType::Int32:
			SessionSettings.Set(Filter.Key, Val.IntValue, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
			break;

		case ENexusSessionFilterValueType::Float:
			SessionSettings.Set(Filter.Key, static_cast<double>(Val.FloatValue), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
			break;

		case ENexusSessionFilterValueType::Bool:
			SessionSettings.Set(Filter.Key, Val.bBoolValue, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
			break;

		default:
			break;
		}
	}
}


bool NexusSessionFilterUtils::PassesAllFilters(const TArray<FSessionSearchFilter>& Filters, const FOnlineSessionSearchResult& Result)
{
    for (const FSessionSearchFilter& Filter : Filters)
    {
        if (!Filter.MatchesResult(Result))
            return false;
    }
	
    return true;
}
