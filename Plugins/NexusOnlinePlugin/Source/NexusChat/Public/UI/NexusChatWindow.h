#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/NexusChatTypes.h"
#include "Core/NexusChatComponent.h"
#include "Core/NexusChatSubsystem.h"
#include "Components/ListView.h"
#include "Components/EditableTextBox.h"
#include "Components/CheckBox.h"
#include "UI/NexusChatMessageRow.h"
#include "NexusChatWindow.generated.h"


class UNexusChatTabButton;
class UNexusChatChannelList;


UCLASS()
class NEXUSCHAT_API UNexusChatWindow : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly)
	UListView* ChatListView;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly)
	UEditableTextBox* ChatInput;

	UPROPERTY(EditDefaultsOnly, Category = "NexusChat")
	TSubclassOf<UNexusChatMessageRow> MessageRowClass;

	UPROPERTY(EditDefaultsOnly, Category = "NexusChat")
	int32 MaxChatLines = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusChat")
	bool bEnableChatTabs = true;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	class UHorizontalBox* TabContainer;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	class UButton* AddChannelButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	class UButton* ChannelListButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	class UCheckBox* NotificationToggle;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	class UNexusChatChannelList* ChannelList;

	// ────────────────────────────────────────────
	// TAB SYSTEM
	// ────────────────────────────────────────────

	/** List of channels to cycle through when adding a new tab via the UI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusChat")
	TArray<FName> AvailableChannels;

	UFUNCTION(BlueprintCallable, Category = "NexusChat")
	void SelectTab(FName ChannelName, bool bIsGeneral);

	UFUNCTION(BlueprintCallable, Category = "NexusChat")
	void OpenChannel(FName ChannelName, bool bIsPrivate);

	UFUNCTION()
	void OnChannelListButtonClicked();

	UFUNCTION()
	void OnNotificationToggled(bool bIsChecked);

	UFUNCTION()
	void OnTabClicked(FName ChannelName, bool bIsGeneral);

	// ────────────────────────────────────────────
	// LINK CLICK EVENTS
	// ────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "NexusChat|Links")
	FOnChatLinkClicked OnPlayerClicked;

	UPROPERTY(BlueprintAssignable, Category = "NexusChat|Links")
	FOnChatLinkClicked OnUrlClicked;

	UPROPERTY(BlueprintAssignable, Category = "NexusChat|Links")
	FOnChatLinkClicked OnAnyLinkClicked;

	// ────────────────────────────────────────────
	// MESSAGE FORMATTING
	// ────────────────────────────────────────────
	
	UFUNCTION(BlueprintNativeEvent, Category = "NexusChat")
	FString FormatOutgoingMessage(const FString& RawMessage, ENexusChatChannel Channel);
	virtual FString FormatOutgoingMessage_Implementation(const FString& RawMessage, ENexusChatChannel Channel);

	UFUNCTION(BlueprintCallable, Category = "NexusChat")
	void ToggleChatFocus(bool bFocus);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleMessageReceived(const FNexusChatMessage& Msg);

	UFUNCTION()
	void HandleTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void HandlePlayerLinkClicked(const FString& LinkType, const FString& LinkData);

	UFUNCTION()
	void HandleUrlLinkClicked(const FString& LinkType, const FString& LinkData);

	UFUNCTION()
	void HandleAnyLinkClicked(const FString& LinkType, const FString& LinkData);

private:
	UPROPERTY()
	UNexusChatComponent* ChatComponent;
	
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UFUNCTION()
	void HandleChatTextChanged(const FText& Text);

	void HandleAutoCompletion();

	TArray<FString> AutoCompleteMatches;
	int32 CurrentMatchIndex = 0;
	FString AutoCompletePrefix;
	bool bIsAutoCompleting = false;

	bool bIsGeneralTabActive = true;
	FName ActiveChannelName = NAME_None;

	/** Set of channels that are private conversations (Player Names) */
	UPROPERTY()
	TSet<FName> PrivateMessageTabs;

	/** We need to keep references to the dynamically created tab buttons to style them selected/unselected */
	UPROPERTY()
	TMap<FName, UNexusChatTabButton*> ChannelTabButtons;
	
	UPROPERTY()
	UNexusChatTabButton* GeneralTabButton = nullptr;

	void UpdateTabStyles();
	
	UNexusChatTabButton* CreateTabButton(const FText& Label, FName ChannelName, bool bIsGeneral);
	UNexusChatTabButton* GetOrCreatePrivateTab(FName PlayerName);
};
