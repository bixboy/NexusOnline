#include "UI/NexusChatContextMenu.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"

void UNexusChatContextMenu::NativeConstruct()
{
	Super::NativeConstruct();
}

FReply UNexusChatContextMenu::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Consume click to prevent it going through the menu
	return FReply::Handled();
}

void UNexusChatContextMenu::ShowForPlayer(const FString& PlayerName, FVector2D ScreenPosition)
{
	TargetPlayerName = PlayerName;
	
	// Position the menu at the click location
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		CanvasSlot->SetPosition(ScreenPosition);
	}
	
	SetVisibility(ESlateVisibility::Visible);
}

void UNexusChatContextMenu::Hide()
{
	TargetPlayerName.Empty();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UNexusChatContextMenu::ActionWhisper_Implementation()
{
	OnWhisperRequested.Broadcast(TargetPlayerName);
	Hide();
}

void UNexusChatContextMenu::ActionAddFriend_Implementation()
{
	OnAddFriendRequested.Broadcast(TargetPlayerName);
	Hide();
}

void UNexusChatContextMenu::ActionBlock_Implementation()
{
	OnBlockRequested.Broadcast(TargetPlayerName);
	Hide();
}

void UNexusChatContextMenu::ActionInviteToParty_Implementation()
{
	OnInviteToPartyRequested.Broadcast(TargetPlayerName);
	Hide();
}

void UNexusChatContextMenu::ActionViewProfile_Implementation()
{
	OnViewProfileRequested.Broadcast(TargetPlayerName);
	Hide();
}
