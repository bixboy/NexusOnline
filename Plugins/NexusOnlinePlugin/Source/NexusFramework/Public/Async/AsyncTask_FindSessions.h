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

        /**
         * Lance la recherche de sessions selon le type et les filtres fournis.
         *
         * Exemple C++ :
         * @code
         *      FSessionSearchFilter RegionFilter;
         *      RegionFilter.Key = FName("REGION_KEY");
         *      RegionFilter.Value.Type = ENexusSessionFilterValueType::String;
         *      RegionFilter.Value.StringValue = TEXT("EU");
         *      RegionFilter.ComparisonOp = ENexusSessionComparisonOp::Equals;
         *
         *      TArray<FSessionSearchFilter> SimpleFilters;
         *      SimpleFilters.Add(RegionFilter);
         *
         *      auto* PingRule = NewObject<USessionFilterRule_Ping>(this);
         *      PingRule->MaxPing = 80;
         *
         *      UAsyncTask_FindSessions::FindSessions(this, ENexusSessionType::GameSession, 50, SimpleFilters,
         *              { PingRule },
         *              { NewObject<USessionSortRule_Ping>(this) });
         * @endcode
         */
        UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject", AutoCreateRefTerm="SimpleFilters,AdvancedRules,SortRules"), Category="Nexus|Online|Session")
        static UAsyncTask_FindSessions* FindSessions(UObject* WorldContextObject, ENexusSessionType SessionType, int32 MaxResults = 50, const TArray<FSessionSearchFilter>& SimpleFilters = TArray<FSessionSearchFilter>(), const TArray<USessionFilterRule*>& AdvancedRules = TArray<USessionFilterRule*>(), const TArray<USessionSortRule*>& SortRules = TArray<USessionSortRule*>(), USessionFilterPreset* Preset = nullptr);

	virtual void Activate() override;

private:
        void OnFindSessionsComplete(bool bWasSuccessful);
        void ProcessSearchResults(const TArray<FOnlineSessionSearchResult>& InResults);
        void RebuildResolvedFilters();
        void ApplyQueryFilters();

        UPROPERTY()
        UObject* WorldContextObject;

        FDelegateHandle FindSessionsHandle;

        TSharedPtr<FOnlineSessionSearch> SearchSettings;

        ENexusSessionType DesiredType;

        UPROPERTY()
        TArray<FSessionSearchFilter> UserSimpleFilters;

        UPROPERTY()
        TArray<TObjectPtr<USessionFilterRule>> UserAdvancedRules;

        UPROPERTY()
        TArray<TObjectPtr<USessionSortRule>> UserSortRules;

        UPROPERTY()
        TObjectPtr<USessionFilterPreset> UserPreset;

        UPROPERTY()
        TArray<FSessionSearchFilter> ResolvedSimpleFilters;

        UPROPERTY()
        TArray<TObjectPtr<USessionFilterRule>> ResolvedAdvancedRules;

        UPROPERTY()
        TArray<TObjectPtr<USessionSortRule>> ResolvedSortRules;
};
