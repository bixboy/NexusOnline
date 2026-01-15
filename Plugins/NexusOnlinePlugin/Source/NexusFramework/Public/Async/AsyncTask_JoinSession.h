#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Types/OnlineSessionData.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "AsyncTask_JoinSession.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJoinSessionSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJoinSessionFailure);


UCLASS()
class NEXUSFRAMEWORK_API UAsyncTask_JoinSession : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable, Category="Nexus|Online|Session")
	FOnJoinSessionSuccess OnSuccess;

	UPROPERTY(BlueprintAssignable, Category="Nexus|Online|Session")
	FOnJoinSessionFailure OnFailure;

	/**
	 * Tente de rejoindre une session trouvée.
	 * * @param SessionResult Le résultat brut obtenu via FindSessions.
	 * @param bAutoTravel Si VRAI, lance le ClientTravel automatiquement vers le serveur.
	 * @param SessionType Le type de session interne (GameSession généralement).
	 */
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject"), Category="Nexus|Online|Session")
	static UAsyncTask_JoinSession* JoinSession(UObject* WorldContextObject, const FOnlineSessionSearchResultData& SessionResult, bool bAutoTravel = true,
		ENexusSessionType SessionType = ENexusSessionType::GameSession);

	virtual void Activate() override;

private:
	
	void OnOldSessionDestroyed(FName SessionName, bool bWasSuccessful);

	void JoinSessionInternal();

	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	
	
	UPROPERTY()
	UObject* WorldContextObject = nullptr;
	
	FOnlineSessionSearchResult RawResult;
	
	ENexusSessionType DesiredType = ENexusSessionType::GameSession;
	
	bool bShouldAutoTravel = true;

	FDelegateHandle JoinDelegateHandle;
	FDelegateHandle DestroyDelegateHandle;
};