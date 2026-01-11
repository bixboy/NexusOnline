#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Types/OnlineSessionData.h"
#include "AsyncTask_DestroySession.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSessionDestroyed);

UCLASS()
class NEXUSFRAMEWORK_API UAsyncTask_DestroySession : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable, Category="Nexus|Online|Session")
	FOnSessionDestroyed OnSuccess;

	UPROPERTY(BlueprintAssignable, Category="Nexus|Online|Session")
	FOnSessionDestroyed OnFailure;
	
	/**
	 * Creates an asynchronous task that destroys a session of the specified type.
	 * 
	 * @param WorldContextObject Context (World or PlayerController) for async execution.
	 * @param SessionType Type of session to destroy (e.g., GameSession, PartySession).
	 */
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject"), Category="Nexus|Online|Session")
	static UAsyncTask_DestroySession* DestroySession(UObject* WorldContextObject, ENexusSessionType SessionType = ENexusSessionType::GameSession);

	virtual void Activate() override;

private:

	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	
	UPROPERTY()
	UObject* WorldContextObject = nullptr;
	
	ENexusSessionType TargetSessionType = ENexusSessionType::GameSession;
	
	FDelegateHandle DestroyDelegateHandle;
};
