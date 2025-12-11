#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NexusChatContextMenu.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnContextMenuAction, const FString&, PlayerName);

/**
 * Context menu widget for player interactions in chat.
 * Create a Blueprint child of this class to customize the UI.
 */
UCLASS(Blueprintable)
class NEXUSCHAT_API UNexusChatContextMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The player name this menu is targeting */
	UPROPERTY(BlueprintReadOnly, Category = "NexusChat|ContextMenu")
	FString TargetPlayerName;

	// --- Events ---

	/** Called when Whisper action is triggered */
	UPROPERTY(BlueprintAssignable, Category = "NexusChat|ContextMenu")
	FOnContextMenuAction OnWhisperRequested;

	/** Called when Add Friend action is triggered */
	UPROPERTY(BlueprintAssignable, Category = "NexusChat|ContextMenu")
	FOnContextMenuAction OnAddFriendRequested;

	/** Called when Block action is triggered */
	UPROPERTY(BlueprintAssignable, Category = "NexusChat|ContextMenu")
	FOnContextMenuAction OnBlockRequested;

	/** Called when Invite to Party action is triggered */
	UPROPERTY(BlueprintAssignable, Category = "NexusChat|ContextMenu")
	FOnContextMenuAction OnInviteToPartyRequested;

	/** Called when View Profile action is triggered */
	UPROPERTY(BlueprintAssignable, Category = "NexusChat|ContextMenu")
	FOnContextMenuAction OnViewProfileRequested;

	// --- Public API ---

	/** Show the context menu for a specific player at the given screen position */
	UFUNCTION(BlueprintCallable, Category = "NexusChat|ContextMenu")
	void ShowForPlayer(const FString& PlayerName, FVector2D ScreenPosition);

	/** Hide the context menu */
	UFUNCTION(BlueprintCallable, Category = "NexusChat|ContextMenu")
	void Hide();

	// --- Default Actions (can be overridden in Blueprint) ---

	/** Default whisper action - broadcasts event */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NexusChat|ContextMenu")
	void ActionWhisper();
	virtual void ActionWhisper_Implementation();

	/** Default add friend action - broadcasts event */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NexusChat|ContextMenu")
	void ActionAddFriend();
	virtual void ActionAddFriend_Implementation();

	/** Default block action - broadcasts event */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NexusChat|ContextMenu")
	void ActionBlock();
	virtual void ActionBlock_Implementation();

	/** Default invite to party action - broadcasts event */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NexusChat|ContextMenu")
	void ActionInviteToParty();
	virtual void ActionInviteToParty_Implementation();

	/** Default view profile action - broadcasts event */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NexusChat|ContextMenu")
	void ActionViewProfile();
	virtual void ActionViewProfile_Implementation();

protected:
	virtual void NativeConstruct() override;
	
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
};
