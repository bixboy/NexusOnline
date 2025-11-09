#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Async/AsyncTask_FindSessions.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "WBP_NexusOnlineDebug.generated.h"


UCLASS()
class NEXUSFRAMEWORK_API UWBP_NexusOnlineDebug : public UUserWidget
{
	GENERATED_BODY()

public:

	//  Widgets reliés dans l’éditeur (BindWidget)
	UPROPERTY(meta=(BindWidget))
	UButton* Button_Create;

	UPROPERTY(meta=(BindWidget))
	UButton* Button_Join;

	UPROPERTY(meta=(BindWidget))
	UButton* Button_Leave;

	UPROPERTY(meta=(BindWidget))
	UTextBlock* Text_SessionInfo;

	//  Données
	UPROPERTY(BlueprintReadOnly, Category="Nexus|Online")
	TArray<FOnlineSessionSearchResultData> FoundSessions;

	UPROPERTY(BlueprintReadOnly, Category="Nexus|Online")
	int32 SelectedSessionIndex = 0;

UPROPERTY(BlueprintReadOnly, Category="Nexus|Online")
FString CurrentSessionName = TEXT("No session active");

	UPROPERTY(BlueprintReadOnly, Category="Nexus|Online")
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category="Nexus|Online")
	int32 MaxPlayers = 0;

//  Overrides
virtual void NativeConstruct() override;
        virtual void NativeDestruct() override;

//  Boutons
UFUNCTION()
void OnCreateClicked();

	UFUNCTION()
	void OnJoinClicked();

	UFUNCTION()
	void OnLeaveClicked();

	//  Callbacks
	UFUNCTION()
	void OnCreateSuccess();

	UFUNCTION()
	void OnCreateFailure();

	UFUNCTION()
	void OnFindSessionsCompleted(bool bWasSuccessful, const TArray<FOnlineSessionSearchResultData>& Results);

	UFUNCTION()
	void OnJoinSuccess();

	UFUNCTION()
	void OnJoinFailure();

	UFUNCTION()
	void OnLeaveSuccess();

UFUNCTION()
void OnLeaveFailure();

//  UI helpers
UFUNCTION(BlueprintCallable, Category="Nexus|Online")
FText GetSessionInfoText() const;

private:
void HandleSessionPlayersChanged(UWorld* InWorld, ENexusSessionType SessionType, FName SessionName, int32 CurrentPlayersValue, int32 MaxPlayersValue);
void ApplyActiveSession(const FString& SessionLabel, int32 CurrentPlayersValue, int32 MaxPlayersValue);
void ApplyNoSession();

        FDelegateHandle PlayersChangedHandle;
};
