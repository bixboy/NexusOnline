#pragma once

#include "CoreMinimal.h"
#include "Types/OnlineSessionData.h"
#include "SessionSortRule.generated.h"

class FOnlineSessionSearchResult;

UENUM(BlueprintType)
enum class ENexusSessionSortPreference : uint8
{
    NoPreference UMETA(DisplayName = "No Preference"),
    PreferFirst  UMETA(DisplayName = "Prefer First"),
    PreferSecond UMETA(DisplayName = "Prefer Second")
};

/**
 * Base class for session sorting logic.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class NEXUSFRAMEWORK_API USessionSortRule : public UObject
{
    GENERATED_BODY()

public:
    /** Lower priorities are evaluated first. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Online|Session")
    int32 Priority = 0;

    /** If false this rule is skipped. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nexus|Online|Session")
    bool bEnabled = true;

    ENexusSessionSortPreference Compare(const FOnlineSessionSearchResult& First, const FOnlineSessionSearchResult& Second) const;

protected:
    virtual ENexusSessionSortPreference NativeCompare(const FOnlineSessionSearchResult& First, const FOnlineSessionSearchResult& Second) const;

    UFUNCTION(BlueprintNativeEvent, Category = "Nexus|Online|Session")
    ENexusSessionSortPreference K2_Compare(const FOnlineSessionSearchResultData& First, const FOnlineSessionSearchResultData& Second) const;

    static FOnlineSessionSearchResultData ConvertToBlueprintData(const FOnlineSessionSearchResult& Result);
};

/**
 * Sort sessions by ping in ascending order.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class NEXUSFRAMEWORK_API USessionSortRule_Ping : public USessionSortRule
{
    GENERATED_BODY()

protected:
    virtual ENexusSessionSortPreference NativeCompare(const FOnlineSessionSearchResult& First, const FOnlineSessionSearchResult& Second) const override;
};
