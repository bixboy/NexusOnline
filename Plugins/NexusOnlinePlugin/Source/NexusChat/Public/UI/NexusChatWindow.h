#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/NexusChatTypes.h"
#include "Core/NexusChatComponent.h"
#include "Core/NexusChatSubsystem.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "UI/NexusChatMessageRow.h"
#include "NexusChatWindow.generated.h"

UCLASS()
class NEXUSCHAT_API UNexusChatWindow : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly)
	UScrollBox* ChatScrollBox;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly)
	UEditableTextBox* ChatInput;

	UPROPERTY(EditDefaultsOnly, Category = "NexusChat")
	TSubclassOf<UNexusChatMessageRow> MessageRowClass;

	UPROPERTY(EditDefaultsOnly, Category = "NexusChat")
	int32 MaxChatLines = 100;

	// ────────────────────────────────────────────
	// LINK CLICK EVENTS (bind these in Blueprint!)
	// ────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "NexusChat|Links")
	FOnChatLinkClicked OnPlayerClicked;

	UPROPERTY(BlueprintAssignable, Category = "NexusChat|Links")
	FOnChatLinkClicked OnUrlClicked;

	UPROPERTY(BlueprintAssignable, Category = "NexusChat|Links")
	FOnChatLinkClicked OnAnyLinkClicked;

	
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

	// Link event handlers (bound to subsystem)
	UFUNCTION()
	void HandlePlayerLinkClicked(const FString& LinkType, const FString& LinkData);

	UFUNCTION()
	void HandleUrlLinkClicked(const FString& LinkType, const FString& LinkData);

	UFUNCTION()
	void HandleAnyLinkClicked(const FString& LinkType, const FString& LinkData);

private:
	UPROPERTY()
	UNexusChatComponent* ChatComponent;

	// --- Auto Completion ---

	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UFUNCTION()
	void HandleChatTextChanged(const FText& Text);

	void HandleAutoCompletion();

	TArray<FString> AutoCompleteMatches;
	int32 CurrentMatchIndex = 0;
	FString AutoCompletePrefix;
	bool bIsAutoCompleting = false;
};
