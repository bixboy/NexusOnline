#pragma once
#include "CoreMinimal.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Types/OnlineSessionData.h"
#include "AsyncTask_JoinSession.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJoinSessionSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJoinSessionFailure);

UCLASS()
class NEXUSFRAMEWORK_API UAsyncTask_JoinSession : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnJoinSessionSuccess OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnJoinSessionFailure OnFailure;

	/** Rejoint une session existante */
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject"), Category="Nexus|Online|Session")
	static UAsyncTask_JoinSession* JoinSession(UObject* WorldContextObject, const FOnlineSessionSearchResultData& SessionResult, ENexusSessionType SessionType = ENexusSessionType::GameSession);

	virtual void Activate() override;

private:
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	UPROPERTY()
	UObject* WorldContextObject;

	FOnlineSessionSearchResult RawResult;

	ENexusSessionType DesiredType;

	FDelegateHandle JoinDelegateHandle;
};
