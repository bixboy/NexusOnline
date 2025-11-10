#pragma once

#include "CoreMinimal.h"
#include "OnlineSessionSettings.h"
#include "SessionSearchFilter.generated.h"

/**
 * Supported data types for lightweight query filters that can be authored from Blueprints.
 */
UENUM(BlueprintType)
enum class ENexusSessionFilterValueType : uint8
{
    String      UMETA(DisplayName = "String"),
    Int32       UMETA(DisplayName = "Integer"),
    Float       UMETA(DisplayName = "Float"),
    Boolean     UMETA(DisplayName = "Boolean")
};

/**
 * Simple key/value filter that can be pushed to the backend query or evaluated locally once results are received.
 */
USTRUCT(BlueprintType)
struct FSessionSearchFilter
{
    GENERATED_BODY()

public:
    /** Key used either for backend query or local filtering. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Online|Session")
    FName Key = NAME_None;

    /** Type of the value stored in this filter. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Online|Session")
    ENexusSessionFilterValueType ValueType = ENexusSessionFilterValueType::String;

    /** String value used when ValueType equals String. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Online|Session")
    FString StringValue;

    /** Integer value used when ValueType equals Int32. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Online|Session")
    int32 IntValue = 0;

    /** Float value used when ValueType equals Float. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Online|Session")
    float FloatValue = 0.f;

    /** Boolean value used when ValueType equals Boolean. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Online|Session")
    bool bBoolValue = false;

    /** Comparison operator used for both query and local filtering. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Online|Session")
    TEnumAsByte<EOnlineComparisonOp::Type> ComparisonOp = EOnlineComparisonOp::Equals;

    /** If true the filter is translated to a backend query parameter. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Online|Session")
    bool bApplyToQuery = true;

    /** If true the filter is validated once the search results are received. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Online|Session")
    bool bApplyToResults = true;

    /** Converts the stored data into a variant used by the online subsystem. */
    FVariantData ToVariantData() const;

    /** Applies the filter to the provided search configuration. */
    void ApplyToQuerySettings(FOnlineSessionSearch& SearchSettings) const;

    /** Returns true if the given session search result matches the filter when evaluated locally. */
    bool Matches(const FOnlineSessionSearchResult& Result) const;

private:
    bool EvaluateComparison(const FVariantData& Value) const;
};
