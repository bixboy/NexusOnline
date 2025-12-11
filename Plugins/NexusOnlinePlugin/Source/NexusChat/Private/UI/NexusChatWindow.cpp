#include "UI/NexusChatWindow.h"
#include "Core/NexusChatSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

// ════════════════════════════════════════════════════════════════════════════════
// LIFECYCLE
// ════════════════════════════════════════════════════════════════════════════════

void UNexusChatWindow::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind to chat component
	if (APlayerController* PC = GetOwningPlayer())
	{
		ChatComponent = PC->FindComponentByClass<UNexusChatComponent>();
		if (ChatComponent)
		{
			ChatComponent->OnMessageReceived.AddDynamic(this, &UNexusChatWindow::HandleMessageReceived);
			
			// Populate existing history
			for (const FNexusChatMessage& Msg : ChatComponent->GetClientChatHistory())
			{
				HandleMessageReceived(Msg);
			}
		}
	}

	// Bind input events
	if (ChatInput)
	{
		ChatInput->OnTextCommitted.AddDynamic(this, &UNexusChatWindow::HandleTextCommitted);
		ChatInput->OnTextChanged.AddDynamic(this, &UNexusChatWindow::HandleChatTextChanged);
	}

	// Bind to subsystem for link events
	if (UWorld* World = GetWorld())
	{
		if (UNexusChatSubsystem* Subsystem = World->GetSubsystem<UNexusChatSubsystem>())
		{
			Subsystem->OnPlayerLinkClicked.AddDynamic(this, &UNexusChatWindow::HandlePlayerLinkClicked);
			Subsystem->OnUrlLinkClicked.AddDynamic(this, &UNexusChatWindow::HandleUrlLinkClicked);
			Subsystem->OnAnyLinkClicked.AddDynamic(this, &UNexusChatWindow::HandleAnyLinkClicked);
		}
	}
}

void UNexusChatWindow::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		if (UNexusChatSubsystem* Subsystem = World->GetSubsystem<UNexusChatSubsystem>())
		{
			Subsystem->OnPlayerLinkClicked.RemoveDynamic(this, &UNexusChatWindow::HandlePlayerLinkClicked);
			Subsystem->OnUrlLinkClicked.RemoveDynamic(this, &UNexusChatWindow::HandleUrlLinkClicked);
			Subsystem->OnAnyLinkClicked.RemoveDynamic(this, &UNexusChatWindow::HandleAnyLinkClicked);
		}
	}

	Super::NativeDestruct();
}

// ════════════════════════════════════════════════════════════════════════════════
// MESSAGE HANDLING
// ════════════════════════════════════════════════════════════════════════════════

void UNexusChatWindow::HandleMessageReceived(const FNexusChatMessage& Msg)
{
	if (!ChatScrollBox || !MessageRowClass)
	{
		return;
	}

	if (UNexusChatMessageRow* NewRow = CreateWidget<UNexusChatMessageRow>(this, MessageRowClass))
	{
		NewRow->InitWidget(Msg);
		ChatScrollBox->AddChild(NewRow);
		ChatScrollBox->ScrollToEnd();

		// Limit message count
		if (ChatScrollBox->GetChildrenCount() > MaxChatLines)
		{
			ChatScrollBox->RemoveChildAt(0);
		}
	}
}

void UNexusChatWindow::HandleTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod != ETextCommit::OnEnter || !ChatComponent || Text.IsEmpty())
	{
		return;
	}

	// Format and send message
	ENexusChatChannel Channel = ENexusChatChannel::Global;
	FString FormattedMessage = FormatOutgoingMessage(Text.ToString(), Channel);
	ChatComponent->SendChatMessage(FormattedMessage, Channel);

	if (ChatInput)
	{
		ChatInput->SetText(FText::GetEmpty());
	}
}

FString UNexusChatWindow::FormatOutgoingMessage_Implementation(const FString& RawMessage, ENexusChatChannel Channel)
{
	// Override in Blueprint to add custom formatting (links, emotes, etc.)
	return RawMessage;
}

// ════════════════════════════════════════════════════════════════════════════════
// INPUT & FOCUS
// ════════════════════════════════════════════════════════════════════════════════

void UNexusChatWindow::ToggleChatFocus(bool bFocus)
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	if (bFocus)
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(ChatInput ? ChatInput->TakeWidget() : TakeWidget());
		PC->SetInputMode(InputMode);
		PC->SetShowMouseCursor(true);
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
}

FReply UNexusChatWindow::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Tab for auto-completion
	if (ChatInput && ChatInput->HasKeyboardFocus() && InKeyEvent.GetKey() == EKeys::Tab)
	{
		HandleAutoCompletion();
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

// ════════════════════════════════════════════════════════════════════════════════
// AUTO-COMPLETION
// ════════════════════════════════════════════════════════════════════════════════

void UNexusChatWindow::HandleChatTextChanged(const FText& Text)
{
	if (!bIsAutoCompleting)
	{
		AutoCompleteMatches.Empty();
		CurrentMatchIndex = 0;
		AutoCompletePrefix.Empty();
	}
	else
	{
		bIsAutoCompleting = false;
	}
}

void UNexusChatWindow::HandleAutoCompletion()
{
	if (!ChatInput)
	{
		return;
	}

	FString CurrentText = ChatInput->GetText().ToString();
	if (CurrentText.IsEmpty())
	{
		return;
	}

	// Find last word being typed
	int32 LastSpaceIndex = CurrentText.FindLastCharByPredicate([](TCHAR C) { return FChar::IsWhitespace(C); });
	FString WordPrefix = CurrentText.RightChop(LastSpaceIndex + 1);
	
	if (WordPrefix.IsEmpty())
	{
		return;
	}

	// Build match list if needed
	if (AutoCompleteMatches.Num() == 0)
	{
		AutoCompletePrefix = WordPrefix;
		
		if (UWorld* World = GetWorld())
		{
			if (AGameStateBase* GameState = World->GetGameState())
			{
				for (APlayerState* PS : GameState->PlayerArray)
				{
					if (PS)
					{
						FString Name = PS->GetPlayerName();
						if (Name.StartsWith(AutoCompletePrefix, ESearchCase::IgnoreCase))
						{
							AutoCompleteMatches.Add(Name);
						}
					}
				}
			}
		}
		
		AutoCompleteMatches.Sort();
		CurrentMatchIndex = 0;
	}

	// Apply match
	if (AutoCompleteMatches.Num() > 0)
	{
		CurrentMatchIndex = CurrentMatchIndex % AutoCompleteMatches.Num();
		FString NewText = CurrentText.Left(LastSpaceIndex + 1) + AutoCompleteMatches[CurrentMatchIndex];
		
		bIsAutoCompleting = true;
		ChatInput->SetText(FText::FromString(NewText));
		CurrentMatchIndex++;
	}
}

// ════════════════════════════════════════════════════════════════════════════════
// LINK EVENT RELAYS
// ════════════════════════════════════════════════════════════════════════════════

void UNexusChatWindow::HandlePlayerLinkClicked(const FString& LinkType, const FString& LinkData)
{
	OnPlayerClicked.Broadcast(LinkType, LinkData);
}

void UNexusChatWindow::HandleUrlLinkClicked(const FString& LinkType, const FString& LinkData)
{
	OnUrlClicked.Broadcast(LinkType, LinkData);
}

void UNexusChatWindow::HandleAnyLinkClicked(const FString& LinkType, const FString& LinkData)
{
	OnAnyLinkClicked.Broadcast(LinkType, LinkData);
}
