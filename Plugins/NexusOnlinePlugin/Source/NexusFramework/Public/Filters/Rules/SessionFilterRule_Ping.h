#pragma once

#include "Filters/SessionFilterRule.h"
#include "SessionFilterRule_Ping.generated.h"

/**
 * Règle de filtrage basée sur le ping maximum.
 */
UCLASS(BlueprintType)
class NEXUSFRAMEWORK_API USessionFilterRule_Ping : public USessionFilterRule
{
        GENERATED_BODY()

public:
        /** Ping maximum acceptable en millisecondes. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Filter")
        int32 MaxPing = 100;

        virtual bool PassesFilter(const FOnlineSessionSearchResult& Result) const override;
        virtual FString GetRuleDescription() const override;
};

