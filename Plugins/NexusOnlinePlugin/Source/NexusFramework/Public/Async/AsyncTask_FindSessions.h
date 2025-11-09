#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Types/OnlineSessionData.h"
#include "AsyncTask_FindSessions.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFindSessionsCompleted, bool, bWasSuccessful, const TArray<FOnlineSessionSearchResultData>&, Results);

UCLASS()
class NEXUSFRAMEWORK_API UAsyncTask_FindSessions : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnFindSessionsCompleted OnCompleted;

	/** Lance la recherche de sessions selon le type */
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject"), Category="Nexus|Online|Session")
	static UAsyncTask_FindSessions* FindSessions(UObject* WorldContextObject, ENexusSessionType SessionType, int32 MaxResults = 50);

	virtual void Activate() override;

private:
	void OnFindSessionsComplete(bool bWasSuccessful);

	UPROPERTY()
	UObject* WorldContextObject;

	FDelegateHandle FindSessionsHandle;
	
	TSharedPtr<FOnlineSessionSearch> SearchSettings;
	
	ENexusSessionType DesiredType;
};
