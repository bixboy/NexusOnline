#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Types/OnlineSessionData.h"
#include "AsyncTask_FindSessions.generated.h"

class USessionFilterRule;
class USessionSortRule;
class USessionFilterPreset;
struct FSessionSearchFilter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFindSessionsCompleted, bool, bWasSuccessful, const TArray<FOnlineSessionSearchResultData>&, Results);

/**
 * Asynchronous Blueprint node to find online sessions.
 * Supports advanced filtering, sorting, and presets.
 */
UCLASS()
class NEXUSFRAMEWORK_API UAsyncTask_FindSessions : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:

	// ───────────────────────────────
	// Delegates
	// ───────────────────────────────
	UPROPERTY(BlueprintAssignable, Category="Nexus|Online|Session")
	FOnFindSessionsCompleted OnCompleted;

	// ───────────────────────────────
	// Factory Method
	// ───────────────────────────────

	/**
	 * Creates a Find session async node.
	 * @param WorldContextObject Reference to the world or player calling this async node.
	 *
	 * @param SessionType Type of session to search for (e.g. GameSession, Lobby, PartySession).
	 * .
	 * @param MaxResults Maximum number of results to return from the search.
	 * .
	 * @param SimpleFilters Basic filters (key/value pairs) applied directly to query settings.
	 * .
	 * @param AdvancedRules Complex filtering logic (Blueprint subclasses of USessionFilterRule).
	 * .
	 * @param SortRules Optional sorting rules to order results (Blueprint subclasses of USessionSortRule).
	 * .
	 * @param Preset Optional preset combining reusable filter and sort configurations.
	 */
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject", AutoCreateRefTerm="SimpleFilters,AdvancedRules,SortRules"), Category="Nexus|Online|Session")
	static UAsyncTask_FindSessions* FindSessions(
		UObject* WorldContextObject,
		ENexusSessionType SessionType,
		int32 MaxResults,
		const TArray<FSessionSearchFilter>& SimpleFilters,
		const TArray<USessionFilterRule*>& AdvancedRules,
		const TArray<USessionSortRule*>& SortRules,
		USessionFilterPreset* Preset
	);

	virtual void Activate() override;

private:

	// ───────────────────────────────
	// Internal Flow
	// ───────────────────────────────
	
	void OnFindSessionsComplete(bool bWasSuccessful);
	void ProcessSearchResults(const TArray<FOnlineSessionSearchResult>& InResults);

	// ───────────────────────────────
	// Filter & Sorting Utilities
	// ───────────────────────────────
	
	void RebuildResolvedFilters();
	void ApplyQueryFilters();

	// ───────────────────────────────
	// Context & Session Data
	// ───────────────────────────────
	
	UPROPERTY()
	UObject* WorldContextObject = nullptr;
	
	TSharedPtr<FOnlineSessionSearch> SearchSettings;
	
	FDelegateHandle FindSessionsHandle;
	
	ENexusSessionType DesiredType = ENexusSessionType::GameSession;

	// ───────────────────────────────
	// User-defined configuration
	// ───────────────────────────────
	
	UPROPERTY()
	TArray<FSessionSearchFilter> UserSimpleFilters;
	
	UPROPERTY()
	TArray<TObjectPtr<USessionFilterRule>> UserAdvancedRules;
	
	UPROPERTY()
	TArray<TObjectPtr<USessionSortRule>> UserSortRules;
	
	UPROPERTY()
	TObjectPtr<USessionFilterPreset> UserPreset;

	// ───────────────────────────────
	// Resolved filters
	// ───────────────────────────────
	UPROPERTY()
	TArray<FSessionSearchFilter> ResolvedSimpleFilters;
	
	UPROPERTY()
	TArray<TObjectPtr<USessionFilterRule>> ResolvedAdvancedRules;
	
	UPROPERTY()
	TArray<TObjectPtr<USessionSortRule>> ResolvedSortRules;
};
