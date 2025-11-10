#pragma once

#include "Filters/SessionSortRule.h"
#include "SessionSortRule_Ping.generated.h"

/**
 * Règle de tri par ping (croissant ou décroissant).
 */
UCLASS(BlueprintType)
class NEXUSFRAMEWORK_API USessionSortRule_Ping : public USessionSortRule
{
        GENERATED_BODY()

public:
        /** Tri croissant par défaut. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Sort")
        bool bAscending = true;

        virtual bool Compare(const FOnlineSessionSearchResult& A, const FOnlineSessionSearchResult& B) const override;
        virtual FString GetRuleDescription() const override;
};

