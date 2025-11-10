#pragma once

#include "Filters/SessionFilterRule.h"
#include "Data/SessionSearchFilter.h"
#include "SessionFilterRule_KeyValue.generated.h"

/**
 * Règle de filtrage basée sur une paire clé/valeur.
 */
UCLASS(BlueprintType)
class NEXUSFRAMEWORK_API USessionFilterRule_KeyValue : public USessionFilterRule
{
        GENERATED_BODY()

public:
        /** Clé à comparer. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Filter")
        FName Key = NAME_None;

        /** Valeur attendue. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Filter")
        FSessionFilterValue ExpectedValue;

        /** Opérateur de comparaison. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Filter")
        ENexusSessionComparisonOp Comparison = ENexusSessionComparisonOp::Equals;

        virtual void ConfigureSearchSettings(FOnlineSessionSearch& SearchSettings) const override;
        virtual bool PassesFilter(const FOnlineSessionSearchResult& Result) const override;
        virtual FString GetRuleDescription() const override;
};

