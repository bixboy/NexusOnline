#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Async/AsyncTask_FindSessions.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "WBP_NexusOnlineDebug.generated.h"

/**
 * Debug utility widget for testing Nexus Online session features.
 */
UCLASS()
class NEXUSFRAMEWORK_API UWBP_NexusOnlineDebug : public UUserWidget
{
	GENERATED_BODY()

public:

	// ───────────────────────────────
	// UI References
	// ───────────────────────────────

	UPROPERTY(meta = (BindWidget))
	UButton* Button_Create;

	UPROPERTY(meta = (BindWidget))
	UButton* Button_Join;

	UPROPERTY(meta = (BindWidget))
	UButton* Button_Leave;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_SessionInfo;

	// ───────────────────────────────
	// Runtime Data
	// ───────────────────────────────

	/** List of sessions found during last search. */
	UPROPERTY(BlueprintReadOnly, Category = "Nexus|Online")
	TArray<FOnlineSessionSearchResultData> FoundSessions;

	/** Index of the selected session to join (default = 0). */
	UPROPERTY(BlueprintReadOnly, Category = "Nexus|Online")
	int32 SelectedSessionIndex = 0;

	/** Current active session name (or “No session”). */
	UPROPERTY(BlueprintReadOnly, Category = "Nexus|Online")
	FString CurrentSessionName = TEXT("No session");

	/** Current session ID. */
	UPROPERTY(BlueprintReadOnly, Category="Nexus|Online")
	FString CurrentSessionId = TEXT("N/A");

	/** Number of players currently connected. */
	UPROPERTY(BlueprintReadOnly, Category = "Nexus|Online")
	int32 CurrentPlayers = 0;

	/** Maximum number of players allowed in the session. */
	UPROPERTY(BlueprintReadOnly, Category = "Nexus|Online")
	int32 MaxPlayers = 0;

	UPROPERTY(EditAnywhere, Category = "Nexus|Online")
	FString MapName = TEXT("TestMap");

	UPROPERTY(EditAnywhere, Category = "Nexus|Online")
	bool IsLan = false;

	
	virtual void NativeConstruct() override;

	// ───────────────────────────────
	// Button Callbacks
	// ───────────────────────────────
	UFUNCTION()
	void OnCreateClicked();
	
	UFUNCTION()
	void OnJoinClicked();
	
	UFUNCTION()
	void OnLeaveClicked();

	// ───────────────────────────────
	// Async Task Callbacks
	// ───────────────────────────────
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

	// ───────────────────────────────
	// Session Information
	// ───────────────────────────────
	UFUNCTION()
	void UpdateSessionInfo();

	UFUNCTION(BlueprintCallable, Category = "Nexus|Online")
	FText GetSessionInfoText();
};
