#include "UI/NexusChatWindow.h"
#include "GameFramework/PlayerController.h"

void UNexusChatWindow::NativeConstruct()
{
	Super::NativeConstruct();

	if (APlayerController* PC = GetOwningPlayer())
	{
		ChatComponent = PC->FindComponentByClass<UNexusChatComponent>();
		if (ChatComponent)
		{
			ChatComponent->OnMessageReceived.AddDynamic(this, &UNexusChatWindow::HandleMessageReceived);
		}
	}

	if (ChatInput)
	{
		ChatInput->OnTextCommitted.AddDynamic(this, &UNexusChatWindow::HandleTextCommitted);
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
