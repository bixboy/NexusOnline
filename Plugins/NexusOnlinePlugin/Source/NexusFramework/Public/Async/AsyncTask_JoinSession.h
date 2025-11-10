#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Types/OnlineSessionData.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "AsyncTask_JoinSession.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJoinSessionSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJoinSessionFailure);

/**
 * Asynchronous Blueprint node to join an online session.
 */
UCLASS()
class NEXUSFRAMEWORK_API UAsyncTask_JoinSession : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable, Category="Nexus|Online|Session")
	FOnJoinSessionSuccess OnSuccess;

	UPROPERTY(BlueprintAssignable, Category="Nexus|Online|Session")
	FOnJoinSessionFailure OnFailure;

	// ───────────────────────────────
	// Factory Method
	// ───────────────────────────────
	
	/**
	 * Creates a join session async node.
	 * @param WorldContextObject World or PlayerController context.
	 * @param SessionResult Result data from FindSessions (contains raw session info).
	 * @param SessionType Type of session (GameSession, PartySession, etc.).
	 */
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject"), Category="Nexus|Online|Session")
	static UAsyncTask_JoinSession* JoinSession(UObject* WorldContextObject,
		const FOnlineSessionSearchResultData& SessionResult,
		ENexusSessionType SessionType = ENexusSessionType::GameSession
	);

	virtual void Activate() override;

private:

	// ───────────────────────────────
	// Internal Callbacks
	// ───────────────────────────────
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	// ───────────────────────────────
	// Internal Data
	// ───────────────────────────────
	
	UPROPERTY()
	UObject* WorldContextObject = nullptr;
	
	FOnlineSessionSearchResult RawResult;
	
	ENexusSessionType DesiredType = ENexusSessionType::GameSession;

	FDelegateHandle JoinDelegateHandle;
};
