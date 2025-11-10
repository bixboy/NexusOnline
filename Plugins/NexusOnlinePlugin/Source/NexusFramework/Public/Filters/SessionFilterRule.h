#pragma once

#include "CoreMinimal.h"
#include "Types/OnlineSessionData.h"
#include "SessionFilterRule.generated.h"

class FOnlineSessionSearchResult;

/**
 * Abstract base class for complex session filtering logic. Users can inherit in C++ or Blueprints.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class NEXUSFRAMEWORK_API USessionFilterRule : public UObject
{
    GENERATED_BODY()

public:
    /** If disabled the rule will be ignored. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Online|Session")
    bool bEnabled = true;

    /**
     * Evaluates this rule against the provided session search result.
     * Returning true keeps the result, false removes it from the list.
     */
    bool PassesFilter(const FOnlineSessionSearchResult& Result) const;

protected:
    /** Override in derived classes for pure C++ rules. */
    virtual bool NativePassesFilter(const FOnlineSessionSearchResult& Result) const;

    /** Blueprint-native hook for custom logic. */
    UFUNCTION(BlueprintNativeEvent, Category = "Nexus|Online|Session")
    bool K2_PassesFilter(const FOnlineSessionSearchResultData& SessionData) const;

    /** Utility used to convert an online search result into the lightweight structure shared with Blueprints. */
    static FOnlineSessionSearchResultData ConvertToBlueprintData(const FOnlineSessionSearchResult& Result);
};

/**
 * Filter rule comparing a session key with a configured value.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class NEXUSFRAMEWORK_API USessionFilterRule_KeyValue : public USessionFilterRule
{
    GENERATED_BODY()

public:
    /** Key inside the session settings to read. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Online|Session")
    FName Key = NAME_None;

    /** Expected comparison operator. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Online|Session")
    TEnumAsByte<EOnlineComparisonOp::Type> ComparisonOp = EOnlineComparisonOp::Equals;

    /** Value used for the comparison. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Online|Session")
    FString ExpectedValue;

protected:
    virtual bool NativePassesFilter(const FOnlineSessionSearchResult& Result) const override;
};

/**
 * Filter rule rejecting sessions that exceed the configured ping.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class NEXUSFRAMEWORK_API USessionFilterRule_Ping : public USessionFilterRule
{
    GENERATED_BODY()

public:
    /** Maximum accepted ping (in milliseconds). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Online|Session")
    int32 MaxPing = 100;

protected:
    virtual bool NativePassesFilter(const FOnlineSessionSearchResult& Result) const override;
};
