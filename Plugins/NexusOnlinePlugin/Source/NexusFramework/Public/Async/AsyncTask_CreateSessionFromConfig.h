#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "AsyncTask_CreateSessionFromConfig.generated.h"

class UNexusSessionConfig;
class USessionFilterPreset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSessionConfigCreated);

/**
 * Creates a Multiplayer Session using a Nexus Session Config DataAsset.
 * This is the modular, preferred way to create sessions.
 */
UCLASS(meta=(DisplayName="Create Session From Config"))
class NEXUSFRAMEWORK_API UAsyncTask_CreateSessionFromConfig : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintAssignable, Category="Nexus|Online|Session")
	FOnSessionConfigCreated OnSuccess;

	UPROPERTY(BlueprintAssignable, Category="Nexus|Online|Session")
	FOnSessionConfigCreated OnFailure;
	
	/**
	 * Creates a session using a Configuration Asset.
	 * 
	 * @param SessionConfig        The DataAsset containing all session settings.
	 * @param bAutoTravel          If true, automatically travels to the Map defined in the Config.
	 */
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject"), Category="Nexus|Online|Session")
	static UAsyncTask_CreateSessionFromConfig* CreateSessionFromConfig(
	   UObject* WorldContextObject, 
	   UNexusSessionConfig* SessionConfig,
	   bool bAutoTravel = true
	);

	virtual void Activate() override;

private:
	
	/** Step 1: Callback when/if old session is destroyed */
	void OnOldSessionDestroyed(FName SessionName, bool bWasSuccessful);

	/** Step 2: Main Logic */
	void CreateSessionInternal();

	/** Step 3: Completion */
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);

	UPROPERTY()
	UObject* WorldContextObject = nullptr;
	
	UPROPERTY()
	UNexusSessionConfig* SessionConfig = nullptr;

	bool bShouldAutoTravel = true;

	FDelegateHandle CreateDelegateHandle;
	FDelegateHandle DestroyDelegateHandle;
};
