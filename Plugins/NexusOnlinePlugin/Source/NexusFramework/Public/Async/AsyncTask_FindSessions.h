#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Types/OnlineSessionData.h"
#include "Data/SessionSearchFilter.h"
#include "Data/SessionFilterPreset.h"
#include "Filters/SessionFilterRule.h"
#include "Filters/SessionSortRule.h"
#include "AsyncTask_FindSessions.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFindSessionsCompleted, bool, bWasSuccessful, const TArray<FOnlineSessionSearchResultData>&, Results);

UCLASS()
class NEXUSFRAMEWORK_API UAsyncTask_FindSessions : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FOnFindSessionsCompleted OnCompleted;

    /** Lance la recherche de sessions selon le type */
    UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject"), Category="Nexus|Online|Session")
    static UAsyncTask_FindSessions* FindSessions(
        UObject* WorldContextObject,
        ENexusSessionType SessionType,
        int32 MaxResults = 50,
        const TArray<FSessionSearchFilter>& SimpleFilters = TArray<FSessionSearchFilter>(),
        const TArray<USessionFilterRule*>& AdvancedRules = TArray<USessionFilterRule*>(),
        const TArray<USessionSortRule*>& SortRules = TArray<USessionSortRule*>(),
        USessionFilterPreset* Preset = nullptr);
    /**
     * Exemple Blueprint:
     * - SimpleFilters: (Key="REGION_KEY", StringValue="EU"), (Key="MAP_NAME_KEY", StringValue="Arena01")
     * - AdvancedRules: Instancier USessionFilterRule_Ping (MaxPing=80)
     * - SortRules: Instancier USessionSortRule_Ping
     */

    virtual void Activate() override;

private:
    void OnFindSessionsComplete(bool bWasSuccessful);

    UPROPERTY()
    UObject* WorldContextObject = nullptr;

    FDelegateHandle FindSessionsHandle;

    TSharedPtr<FOnlineSessionSearch> SearchSettings;

    ENexusSessionType DesiredType;

    UPROPERTY()
    TArray<FSessionSearchFilter> ResolvedSimpleFilters;

    UPROPERTY()
    TArray<TObjectPtr<USessionFilterRule>> ResolvedAdvancedRules;

    UPROPERTY()
    TArray<TObjectPtr<USessionSortRule>> ResolvedSortRules;

    void ResolvePresetFilters(USessionFilterPreset* Preset, const TArray<FSessionSearchFilter>& SimpleFilters, const TArray<USessionFilterRule*>& AdvancedRules, const TArray<USessionSortRule*>& SortRules);

    bool ApplyLocalFilters(const FOnlineSessionSearchResult& Result) const;

    void ApplySortRules(TArray<FOnlineSessionSearchResult>& Results) const;
};
