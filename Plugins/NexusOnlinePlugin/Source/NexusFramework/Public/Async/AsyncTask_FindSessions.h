#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Types/OnlineSessionData.h"

class USessionFilterRule;
class USessionSortRule;
class USessionFilterPreset;
struct FSessionSearchFilter;

#include "AsyncTask_FindSessions.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFindSessionsCompleted, bool, bWasSuccessful, const TArray<FOnlineSessionSearchResultData>&, Results);

UCLASS()
class NEXUSFRAMEWORK_API UAsyncTask_FindSessions : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FOnFindSessionsCompleted OnCompleted;

    UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject", AutoCreateRefTerm="SimpleFilters,AdvancedRules,SortRules"), Category="Nexus|Online|Session")
    static UAsyncTask_FindSessions* FindSessions(UObject* WorldContextObject, ENexusSessionType SessionType,
        int32 MaxResults,
        const TArray<FSessionSearchFilter>& SimpleFilters,
        const TArray<USessionFilterRule*>& AdvancedRules,
        const TArray<USessionSortRule*>& SortRules,
        USessionFilterPreset* Preset
    );

    virtual void Activate() override;

private:
    void OnFindSessionsComplete(bool bWasSuccessful);
    void ProcessSearchResults(const TArray<FOnlineSessionSearchResult>& InResults, bool bUpdateCache = true);
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
