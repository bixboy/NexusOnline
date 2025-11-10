#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/SessionSearchFilter.h"
#include "Filters/SessionFilterRule.h"
#include "Filters/SessionSortRule.h"
#include "SessionFilterPreset.generated.h"

/**
 * Data asset that bundles together reusable search presets.
 */
UCLASS(BlueprintType)
class NEXUSFRAMEWORK_API USessionFilterPreset : public UDataAsset
{
    GENERATED_BODY()

public:
    /** Lightweight filters that can be mapped to query settings. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nexus|Online|Session")
    TArray<FSessionSearchFilter> SimpleFilters;

    /** Instanced complex rules evaluated locally. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "Nexus|Online|Session")
    TArray<TObjectPtr<USessionFilterRule>> FilterRules;

    /** Sorting rules applied after all filtering has been completed. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "Nexus|Online|Session")
    TArray<TObjectPtr<USessionSortRule>> SortRules;

    /** Copies all rules and filters into the provided arrays using the supplied outer. */
    void BuildResolvedFilters(UObject* Outer, TArray<FSessionSearchFilter>& OutSimpleFilters, TArray<TObjectPtr<USessionFilterRule>>& OutFilterRules, TArray<TObjectPtr<USessionSortRule>>& OutSortRules) const;
    
    /**
     * Exemple d'utilisation : créer un DataAsset contenant
     * - SimpleFilters : REGION_KEY == "EU"
     * - FilterRules : USessionFilterRule_Ping (MaxPing=120)
     * - SortRules : USessionSortRule_Ping
     * Puis fournir ce preset au FindSessions.
     */
};
