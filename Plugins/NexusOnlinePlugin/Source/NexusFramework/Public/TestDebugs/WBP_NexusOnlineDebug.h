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

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_SessionId;

	// ───────────────────────────────
	// Runtime Data
	// ───────────────────────────────
	
	UPROPERTY(BlueprintReadOnly, Category = "Nexus|Online")
	TArray<FOnlineSessionSearchResultData> FoundSessions;

	UPROPERTY(EditAnywhere, Category = "Nexus|Online")
	FString MapName = TEXT("TestMap");

	UPROPERTY(EditAnywhere, Category = "Nexus|Online")
	bool IsLan = false;

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ───────────────────────────────
	// Callbacks UI
	// ───────────────────────────────
	UFUNCTION() void OnCreateClicked();
	UFUNCTION() void OnJoinClicked();
	UFUNCTION() void OnLeaveClicked();

	// ───────────────────────────────
	// Callbacks Async Tasks
	// ───────────────────────────────
	UFUNCTION() void OnCreateSuccess();
	UFUNCTION() void OnCreateFailure();

	UFUNCTION() void OnFindSessionsCompleted(bool bWasSuccessful, const TArray<FOnlineSessionSearchResultData>& Results);

	UFUNCTION() void OnJoinSuccess();
	UFUNCTION() void OnJoinFailure();

	UFUNCTION() void OnLeaveSuccess();
	UFUNCTION() void OnLeaveFailure();

	// ───────────────────────────────
	// Live Updates (Manager)
	// ───────────────────────────────
	
	void UpdateSessionDisplay();

	UFUNCTION()
	void OnPlayerCountChanged(int32 NewCount);

private:
	bool bIsBoundToManager = false;
	TWeakObjectPtr<class AOnlineSessionManager> CachedManager;
	void TryBindToSessionManager();
};