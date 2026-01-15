#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NexusChatContextMenu.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnContextMenuAction, const FString&, PlayerName);


UCLASS(Blueprintable)
class NEXUSCHAT_API UNexusChatContextMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "NexusChat|ContextMenu")
	FString TargetPlayerName;

	// ──────────────────────────────────────────────
	// EVENTS
	// ──────────────────────────────────────────────
	
	UPROPERTY(BlueprintAssignable, Category = "NexusChat|ContextMenu")
	FOnContextMenuAction OnWhisperRequested;

	UPROPERTY(BlueprintAssignable, Category = "NexusChat|ContextMenu")
	FOnContextMenuAction OnAddFriendRequested;

	UPROPERTY(BlueprintAssignable, Category = "NexusChat|ContextMenu")
	FOnContextMenuAction OnBlockRequested;

	UPROPERTY(BlueprintAssignable, Category = "NexusChat|ContextMenu")
	FOnContextMenuAction OnInviteToPartyRequested;

	UPROPERTY(BlueprintAssignable, Category = "NexusChat|ContextMenu")
	FOnContextMenuAction OnViewProfileRequested;

	// ──────────────────────────────────────────────
	// API PUBLIQUE
	// ──────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "NexusChat|ContextMenu")
	void ShowForPlayer(const FString& PlayerName, FVector2D ScreenPosition);

	UFUNCTION(BlueprintCallable, Category = "NexusChat|ContextMenu")
	void Hide();

	// ──────────────────────────────────────────────
	// ACTIONS
	// ──────────────────────────────────────────────

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NexusChat|ContextMenu")
	void ActionWhisper();
	virtual void ActionWhisper_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NexusChat|ContextMenu")
	void ActionAddFriend();
	virtual void ActionAddFriend_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NexusChat|ContextMenu")
	void ActionBlock();
	virtual void ActionBlock_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NexusChat|ContextMenu")
	void ActionInviteToParty();
	virtual void ActionInviteToParty_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NexusChat|ContextMenu")
	void ActionViewProfile();
	virtual void ActionViewProfile_Implementation();

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;
};