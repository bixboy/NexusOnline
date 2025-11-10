#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Types/OnlineSessionData.h"
#include "AsyncTask_FindSessionById.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFindSessionByIdCompleted, bool, bWasSuccessful, FOnlineSessionSearchResultData, Result);

/**
 * 🔍 Find a session using its unique Session ID .
 */
UCLASS()
class NEXUSFRAMEWORK_API UAsyncTask_FindSessionById : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnFindSessionByIdCompleted OnCompleted;

	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject"), Category="Nexus|Online|Session")
	static UAsyncTask_FindSessionById* FindSessionById(UObject* WorldContextObject, const FString& SessionId);

	virtual void Activate() override;

private:
	void OnFindSessionsComplete(bool bWasSuccessful);

	UPROPERTY()
	UObject* WorldContextObject;
	
	FString TargetSessionId;
	
	TSharedPtr<FOnlineSessionSearch> SearchSettings;
};
