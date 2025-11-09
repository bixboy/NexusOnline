#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Types/OnlineSessionData.h"
#include "AsyncTask_CreateSession.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSessionCreated);

UCLASS()
class NEXUSFRAMEWORK_API UAsyncTask_CreateSession : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnSessionCreated OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnSessionCreated OnFailure;

	/** Crée une session à partir d'une structure de paramètres */
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject"), Category="Nexus|Online|Session")
	static UAsyncTask_CreateSession* CreateSession(UObject* WorldContextObject, const FSessionSettingsData& SettingsData);

	virtual void Activate() override;

private:
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);

	UPROPERTY()
	UObject* WorldContextObject;

	FSessionSettingsData Data;
	
	FDelegateHandle CreateDelegateHandle;
};
