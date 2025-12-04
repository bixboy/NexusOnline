#include "UI/NexusChatWindow.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

void UNexusChatWindow::NativeConstruct()
{
	Super::NativeConstruct();

	if (APlayerController* PC = GetOwningPlayer())
	{
		ChatComponent = PC->FindComponentByClass<UNexusChatComponent>();
		if (ChatComponent)
		{
			ChatComponent->OnMessageReceived.AddDynamic(this, &UNexusChatWindow::HandleMessageReceived);
			
			// Populate history
			for (const FNexusChatMessage& Msg : ChatComponent->GetClientChatHistory())
			{
				HandleMessageReceived(Msg);
			}
		}
	}

	if (ChatInput)
	{
		ChatInput->OnTextCommitted.AddDynamic(this, &UNexusChatWindow::HandleTextCommitted);
		ChatInput->OnTextChanged.AddDynamic(this, &UNexusChatWindow::HandleChatTextChanged);
	}
}

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

		if (ChatScrollBox->GetChildrenCount() > MaxChatLines)
		{
			ChatScrollBox->RemoveChildAt(0);
		}
	}
}

void UNexusChatWindow::HandleTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		if (ChatComponent && !Text.IsEmpty())
		{
			ChatComponent->SendChatMessage(Text.ToString(), ENexusChatChannel::Global);
			
			if (ChatInput)
			{
				ChatInput->SetText(FText::GetEmpty());
			}
		}
	}
}

void UNexusChatWindow::ToggleChatFocus(bool bFocus)
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (bFocus)
		{
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(ChatInput ? ChatInput->TakeWidget() : TakeWidget());
			PC->SetInputMode(InputMode);
			PC->SetShowMouseCursor(true);
		}
		else
		{
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->SetShowMouseCursor(false);
		}
	}
}

FReply UNexusChatWindow::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (ChatInput && ChatInput->HasKeyboardFocus())
	{
		if (InKeyEvent.GetKey() == EKeys::Tab)
		{
			HandleAutoCompletion();
			return FReply::Handled();
		}
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

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

	// Find the word being typed (last word)
	int32 CursorPos = CurrentText.Len(); // Ideally we'd get the cursor position, but for now assuming end of text
	// Note: UEditableTextBox doesn't expose cursor position easily in C++ without accessing the slate widget.
	// For simplicity, we'll assume we are auto-completing the last word if the cursor is at the end.
	
	int32 LastSpaceIndex = -1;
	for (int32 i = CurrentText.Len() - 1; i >= 0; --i)
	{
		if (FChar::IsWhitespace(CurrentText[i]))
		{
			LastSpaceIndex = i;
			break;
		}
	}

	FString WordPrefix = CurrentText.RightChop(LastSpaceIndex + 1);
	
	if (WordPrefix.IsEmpty())
	{
		return;
	}

	// If we are starting a new completion or the prefix changed significantly (user typed more)
	// Actually, the logic "if (!bIsAutoCompleting)" in TextChanged handles the reset.
	// So here we just check if we need to fetch matches.
	
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
						FString PlayerName = PS->GetPlayerName();
						if (PlayerName.StartsWith(AutoCompletePrefix, ESearchCase::IgnoreCase))
						{
							AutoCompleteMatches.Add(PlayerName);
						}
					}
				}
			}
		}
		
		AutoCompleteMatches.Sort();
		CurrentMatchIndex = 0;
	}

	if (AutoCompleteMatches.Num() > 0)
	{
		// Cycle matches
		if (CurrentMatchIndex >= AutoCompleteMatches.Num())
		{
			CurrentMatchIndex = 0;
		}

		FString MatchedName = AutoCompleteMatches[CurrentMatchIndex];
		
		// Replace the last word with the matched name
		FString NewText = CurrentText.Left(LastSpaceIndex + 1) + MatchedName;
		
		bIsAutoCompleting = true; // Set flag to ignore the next TextChanged event
		ChatInput->SetText(FText::FromString(NewText));
		
		// Move index for next tab press
		CurrentMatchIndex++;
	}
}
