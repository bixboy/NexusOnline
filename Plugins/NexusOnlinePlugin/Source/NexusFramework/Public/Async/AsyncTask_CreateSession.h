#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Types/OnlineSessionData.h"
#include "Data/SessionSearchFilter.h"
#include "Data/SessionFilterPreset.h"
#include "AsyncTask_CreateSession.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSessionCreated);


/**
 * Asynchronous Blueprint node that creates a new online session.
 */
UCLASS()
class NEXUSFRAMEWORK_API UAsyncTask_CreateSession : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintAssignable, Category="Nexus|Online|Session")
	FOnSessionCreated OnSuccess;

	UPROPERTY(BlueprintAssignable, Category="Nexus|Online|Session")
	FOnSessionCreated OnFailure;
	
	/**
	 * Creates a new session asynchronously.
	 *
	 * @param WorldContextObject   World or PlayerController context.
	 * @param SettingsData         Configuration for the session (map, name, max players, etc.).
	 * @param AdditionalSettings   Optional extra session filters/metadata (key/value pairs).
	 * @param Preset               Optional preset that merges reusable filters and sort rules.
	 */
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject", AutoCreateRefTerm="AdditionalSettings"), Category="Nexus|Online|Session")
	static UAsyncTask_CreateSession* CreateSession(UObject* WorldContextObject,
		const FSessionSettingsData& SettingsData,
		const TArray<FSessionSearchFilter>& AdditionalSettings,
		USessionFilterPreset* Preset
	);

	virtual void Activate() override;

private:

	// ───────────────────────────────
	// Internal Callback
	// ───────────────────────────────
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);

	
	// ───────────────────────────────
	// Internal Data
	// ───────────────────────────────
	UPROPERTY()
	UObject* WorldContextObject = nullptr;
	
	FSessionSettingsData Data;

	UPROPERTY()
	TArray<FSessionSearchFilter> SessionAdditionalSettings;
	
	UPROPERTY()
	TObjectPtr<USessionFilterPreset> SessionPreset;

	FDelegateHandle CreateDelegateHandle;
};
