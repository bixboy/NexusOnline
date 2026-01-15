#include "UI/NexusChatContextMenu.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"

void UNexusChatContextMenu::NativeConstruct()
{
	Super::NativeConstruct();
	
	SetVisibility(ESlateVisibility::Collapsed);
}

FReply UNexusChatContextMenu::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return FReply::Handled();
}

void UNexusChatContextMenu::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusLost(InFocusEvent);
	
	// Optionnel : Si le focus part ailleurs (ex: clic dans le vide), on ferme.
	// Attention : cliquer sur un bouton enfant fait parfois perdre le focus du conteneur,
	// Hide(); 
}

void UNexusChatContextMenu::ShowForPlayer(const FString& PlayerName, FVector2D ScreenPosition)
{
	TargetPlayerName = PlayerName;

	SetVisibility(ESlateVisibility::Visible);
	ForceLayoutPrepass();

	UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(this);
	
	if (CanvasSlot)
	{
		FVector2D ViewportSize = FVector2D::ZeroVector;
		if (GEngine && GEngine->GameViewport)
		{
			GEngine->GameViewport->GetViewportSize(ViewportSize);
		}

		FVector2D MenuSize = GetDesiredSize();
		FVector2D FinalPosition = ScreenPosition;

		if (FinalPosition.X + MenuSize.X > ViewportSize.X)
		{
			FinalPosition.X -= MenuSize.X;
		}

		if (FinalPosition.Y + MenuSize.Y > ViewportSize.Y)
		{
			FinalPosition.Y -= MenuSize.Y;
		}

		CanvasSlot->SetPosition(FinalPosition);
		
		CanvasSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		CanvasSlot->SetAutoSize(true);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[NexusChat] ContextMenu must be placed inside a CanvasPanel to work properly!"));
	}

	SetFocus();
}

void UNexusChatContextMenu::Hide()
{
	TargetPlayerName.Empty();
	SetVisibility(ESlateVisibility::Collapsed);
}

// ──────────────────────────────────────────────
// IMPLEMENTATION DES ACTIONS
// ──────────────────────────────────────────────

void UNexusChatContextMenu::ActionWhisper_Implementation()
{
	if (!TargetPlayerName.IsEmpty())
		OnWhisperRequested.Broadcast(TargetPlayerName);
	
	Hide();
}

void UNexusChatContextMenu::ActionAddFriend_Implementation()
{
	if (!TargetPlayerName.IsEmpty())
		OnAddFriendRequested.Broadcast(TargetPlayerName);
	
	Hide();
}

void UNexusChatContextMenu::ActionBlock_Implementation()
{
	if (!TargetPlayerName.IsEmpty())
		OnBlockRequested.Broadcast(TargetPlayerName);
	
	Hide();
}

void UNexusChatContextMenu::ActionInviteToParty_Implementation()
{
	if (!TargetPlayerName.IsEmpty())
		OnInviteToPartyRequested.Broadcast(TargetPlayerName);
	
	Hide();
}

void UNexusChatContextMenu::ActionViewProfile_Implementation()
{
	if (!TargetPlayerName.IsEmpty())
		OnViewProfileRequested.Broadcast(TargetPlayerName);
	
	Hide();
}