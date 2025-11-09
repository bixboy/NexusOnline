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
	UPROPERTY(BlueprintAssignable)
	FOnSessionDestroyed OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnSessionDestroyed OnFailure;

	/** Détruit une session du type spécifié */
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject"), Category="Nexus|Online|Session")
	static UAsyncTask_DestroySession* DestroySession(UObject* WorldContextObject, ENexusSessionType SessionType = ENexusSessionType::GameSession);

	virtual void Activate() override;

private:
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	UPROPERTY()
	UObject* WorldContextObject;

	ENexusSessionType TargetSessionType;

	FDelegateHandle DestroyDelegateHandle;
};
