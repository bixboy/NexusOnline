#pragma once

#include "CoreMinimal.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemTypes.h"
#include "SessionSearchFilter.generated.h"

class FVariantData;
class FOnlineSessionSearch;
class FOnlineSessionSearchResult;
class FOnlineSessionSettings;

/**
 * Type de valeur pour les filtres de session simples.
 */
UENUM(BlueprintType)
enum class ENexusSessionFilterValueType : uint8
{
    String  UMETA(DisplayName="String"),
    Int32   UMETA(DisplayName="Int32"),
    Float   UMETA(DisplayName="Float"),
    Bool    UMETA(DisplayName="Bool")
};

/**
 * Opérations de comparaison Blueprint exposées.
 */
UENUM(BlueprintType)
enum class ENexusSessionComparisonOp : uint8
{
    Equals              UMETA(DisplayName="=="),
    NotEquals           UMETA(DisplayName="!="),
    GreaterThan         UMETA(DisplayName=">"),
    GreaterThanEquals   UMETA(DisplayName=">="),
    LessThan            UMETA(DisplayName="<"),
    LessThanEquals      UMETA(DisplayName="<="),
    Exists              UMETA(DisplayName="Exists")
};

/**
 * Valeur Blueprint configurable utilisée par FSessionSearchFilter.
 */
USTRUCT(BlueprintType)
struct NEXUSFRAMEWORK_API FSessionFilterValue
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session")
    ENexusSessionFilterValueType Type = ENexusSessionFilterValueType::String;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session", meta=(EditCondition="Type==ENexusSessionFilterValueType::String"))
    FString StringValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session", meta=(EditCondition="Type==ENexusSessionFilterValueType::Int32"))
    int32 IntValue = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session", meta=(EditCondition="Type==ENexusSessionFilterValueType::Float"))
    float FloatValue = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session", meta=(EditCondition="Type==ENexusSessionFilterValueType::Bool"))
    bool bBoolValue = false;

    bool ToVariantData(FVariantData& OutData) const;
    bool CompareVariant(const FVariantData& OtherData, EOnlineComparisonOp::Type Op) const;
    FString ToDebugString() const;
};

/**
 * Filtre simple configuré côté Blueprint, appliqué aux QuerySettings et en post-filtrage.
 */
USTRUCT(BlueprintType)
struct NEXUSFRAMEWORK_API FSessionSearchFilter
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session")
    FName Key = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session")
    FSessionFilterValue Value;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session")
    ENexusSessionComparisonOp ComparisonOp = ENexusSessionComparisonOp::Equals;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session")
    bool bApplyToQuerySettings = true;

    bool ApplyToSearchSettings(FOnlineSessionSearch& Search) const;
    bool MatchesResult(const FOnlineSessionSearchResult& Result) const;
    EOnlineComparisonOp::Type ToOnlineOp() const;
};

namespace NexusSessionFilterUtils
{
    void ApplyFiltersToSettings(const TArray<FSessionSearchFilter>& Filters, FOnlineSessionSearch& SearchSettings);
    void ApplyFiltersToSettings(const TArray<FSessionSearchFilter>& Filters, FOnlineSessionSettings& SessionSettings);
    bool PassesAllFilters(const TArray<FSessionSearchFilter>& Filters, const FOnlineSessionSearchResult& Result);
}
