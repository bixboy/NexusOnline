#include "UI/NexusChatWindow.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "UI/NexusChatMessageRow.h"
#include "UI/NexusChatTabButton.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"


// ════════════════════════════════════════════════════════════════════════════════
// LIFECYCLE
// ════════════════════════════════════════════════════════════════════════════════

void UNexusChatWindow::NativeConstruct()
{
	Super::NativeConstruct();

	if (APlayerController* PC = GetOwningPlayer())
	{
		ChatComponent = PC->FindComponentByClass<UNexusChatComponent>();
		if (ChatComponent)
		{
			ChatComponent->OnMessageReceived.AddDynamic(this, &UNexusChatWindow::HandleMessageReceived);
			
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

	if (UWorld* World = GetWorld())
	{
		if (UNexusChatSubsystem* Subsystem = World->GetSubsystem<UNexusChatSubsystem>())
		{
			Subsystem->OnPlayerLinkClicked.AddDynamic(this, &UNexusChatWindow::HandlePlayerLinkClicked);
			Subsystem->OnUrlLinkClicked.AddDynamic(this, &UNexusChatWindow::HandleUrlLinkClicked);
			Subsystem->OnAnyLinkClicked.AddDynamic(this, &UNexusChatWindow::HandleAnyLinkClicked);
		}
	}

	if (bEnableChatTabs)
	{
		if (TabContainer)
		{
			TabContainer->SetVisibility(ESlateVisibility::Visible);
			TabContainer->ClearChildren();

			GeneralTabButton = CreateTabButton(FText::FromString("General"), FName("Global"), true);
			if (GeneralTabButton)
			{
				TabContainer->AddChild(GeneralTabButton);
			}
		}

		if (AddChannelButton)
		{
			AddChannelButton->SetVisibility(ESlateVisibility::Visible);
			AddChannelButton->OnClicked.AddDynamic(this, &UNexusChatWindow::OnAddChannelClicked);
		}

		SelectTab(FName("Global"), true);
	}
	else
	{
		if (TabContainer)
			TabContainer->SetVisibility(ESlateVisibility::Collapsed);
		
		if (AddChannelButton)
			AddChannelButton->SetVisibility(ESlateVisibility::Collapsed);
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
		return;

	if (UNexusChatSubsystem* Subsystem = GetWorld()->GetSubsystem<UNexusChatSubsystem>())
	{
		if (!Subsystem->IsMessagePassesFilter(Msg))
		{
			return;
		}
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
	if (CommitMethod != ETextCommit::OnEnter || !ChatComponent || Text.IsEmpty())
		return;

	ENexusChatChannel TargetRouting = ENexusChatChannel::Global;
	FName TargetChannelName = NAME_None;

	if (!bIsGeneralTabActive)
	{
		bool bFoundEnum = false;
		if (const UEnum* EnumPtr = StaticEnum<ENexusChatChannel>())
		{
			for(int32 i = 0; i < EnumPtr->NumEnums(); i++)
			{
				if (EnumPtr->HasMetaData(TEXT("Hidden"), i))
					continue;

				int64 Val = EnumPtr->GetValueByIndex(i);
				if (Val == static_cast<int64>(ENexusChatChannel::Global))
					continue;

				if (FName(*EnumPtr->GetDisplayNameTextByValue(Val).ToString()) == ActiveChannelName)
				{
					TargetRouting = static_cast<ENexusChatChannel>(Val);
					TargetChannelName = NAME_None;
					bFoundEnum = true;
					break;
				}
			}
		}

		if (!bFoundEnum)
		{
			TargetRouting = ENexusChatChannel::Global;
			TargetChannelName = ActiveChannelName;
		}
	}

	FString FormattedMessage = FormatOutgoingMessage(Text.ToString(), TargetRouting);
	
	if (TargetChannelName.IsNone())
	{
		ChatComponent->SendChatMessage(FormattedMessage, TargetRouting);
	}
	else
	{
		ChatComponent->SendChatMessageCustom(FormattedMessage, TargetChannelName, TargetRouting);
	}

	if (ChatInput)
	{
		ChatInput->SetText(FText::GetEmpty());
		ToggleChatFocus(true);
	}
}

FString UNexusChatWindow::FormatOutgoingMessage_Implementation(const FString& RawMessage, ENexusChatChannel Channel)
{
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
		return;

	FString CurrentText = ChatInput->GetText().ToString();
	if (CurrentText.IsEmpty())
		return;

	int32 LastSpaceIndex = CurrentText.FindLastCharByPredicate([](TCHAR C) { return FChar::IsWhitespace(C); });
	FString WordPrefix = CurrentText.RightChop(LastSpaceIndex + 1);
	
	if (WordPrefix.IsEmpty())
		return;

	if (AutoCompleteMatches.Num() == 0)
	{
		AutoCompletePrefix = WordPrefix;

		UWorld* World = GetWorld();
		if (!World)
			 return;

		AGameStateBase* GameState = World->GetGameState();
		if (!GameState)
			return;
		
		for (APlayerState* PS : GameState->PlayerArray)
		{
			if (PS)
				continue;
			
			FString Name = PS->GetPlayerName();
			if (Name.StartsWith(AutoCompletePrefix, ESearchCase::IgnoreCase))
			{
				AutoCompleteMatches.Add(Name);
			}
		}
		
		AutoCompleteMatches.Sort();
		CurrentMatchIndex = 0;
	}

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

// ════════════════════════════════════════════════════════════════════════════════
// TAB SYSTEM
// ════════════════════════════════════════════════════════════════════════════════

void UNexusChatWindow::SelectTab(FName ChannelName, bool bIsGeneral)
{
	UNexusChatSubsystem* Subsystem = GetWorld()->GetSubsystem<UNexusChatSubsystem>();
	if (!Subsystem || !ChatComponent)
	{
		return;
	}

	ActiveChannelName = ChannelName;
	bIsGeneralTabActive = bIsGeneral;

	UpdateTabStyles();

	// Update filters
	Subsystem->ClearHistoryFilters();
	
	if (bIsGeneral)
	{
		// General Tab: Show everything, no whitelist/blacklist (or blacklisted spam if any)
		Subsystem->SetWhitelistMode(false); 
	}
	else
	{
		// Specific Channel Tab: Show ONLY this channel
		Subsystem->SetWhitelistMode(true);
		Subsystem->AddHistoryFilter(ChannelName);
	}

	// Refresh UI
	if (ChatScrollBox)
	{
		ChatScrollBox->ClearChildren();
		
		// Re-add messages from LOCAL COMPONENT HISTORY which contains everything (Party, Team, etc.)
		// The Subsystem only contains Global messages.
		TArray<FNexusChatMessage> Messages = ChatComponent->GetClientChatHistory();
		for (const FNexusChatMessage& Msg : Messages)
		{
			// We reuse HandleMessageReceived but we must ensure it respects the filter now
			// To avoid duplicating logic, we modify HandleMessageReceived to check filter.
			HandleMessageReceived(Msg);
		}
		
		ChatScrollBox->ScrollToEnd();
	}
}

void UNexusChatWindow::OnAddChannelClicked()
{
	// If user hasn't configured AvailableChannels, populate defaults
	if (AvailableChannels.Num() == 0)
	{
		const TArray<ENexusChatChannel> DefaultChannels = { 
			ENexusChatChannel::Team, 
			ENexusChatChannel::Party, 
			ENexusChatChannel::Whisper, 
			ENexusChatChannel::System 
		};
		
		for (ENexusChatChannel Chan : DefaultChannels)
		{
			AvailableChannels.Add(FName(*UEnum::GetDisplayValueAsText(Chan).ToString()));
		}
		// Add some custom examples if empty (for testing/showcase)
		AvailableChannels.Add(FName("Trade"));
		AvailableChannels.Add(FName("Guild"));
	}

	for (FName ChanName : AvailableChannels)
	{
		if (!ChannelTabButtons.Contains(ChanName))
		{
			// Create this tab
			UNexusChatTabButton* NewTab = CreateTabButton(FText::FromName(ChanName), ChanName, false);
			
			if (NewTab && TabContainer)
			{
				TabContainer->AddChild(NewTab);
				ChannelTabButtons.Add(ChanName, NewTab);
				
				// Auto select
				SelectTab(ChanName, false);
			}
			
			break; // Add one at a time
		}
	}
}

void UNexusChatWindow::OnTabClicked(FName ChannelName, bool bIsGeneral)
{
	SelectTab(ChannelName, bIsGeneral);
}

UNexusChatTabButton* UNexusChatWindow::CreateTabButton(const FText& Label, FName ChannelName, bool bIsGeneral)
{
	UNexusChatTabButton* Button = NewObject<UNexusChatTabButton>(this);
	
	UTextBlock* Text = NewObject<UTextBlock>(this);
	Text->SetText(Label);
	// Basic styling
	Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Text->SetFont(FCoreStyle::Get().GetFontStyle("NormalFont"));
	
	Button->AddChild(Text);
	
	// Init internal data and connection
	Button->Init(this, ChannelName, bIsGeneral);
	
	return Button;
}

void UNexusChatWindow::UpdateTabStyles()
{
	// Opacity 1.0 for active, 0.5 for inactive
	const FLinearColor ActiveColor(1.0f, 1.0f, 1.0f, 1.0f);
	const FLinearColor InactiveColor(1.0f, 1.0f, 1.0f, 0.5f);

	if (GeneralTabButton)
	{
		GeneralTabButton->SetBackgroundColor(bIsGeneralTabActive ? ActiveColor : InactiveColor);
	}

	for (auto& Elem : ChannelTabButtons)
	{
		if (UButton* Btn = Elem.Value)
		{
			bool bIsMe = (!bIsGeneralTabActive && ActiveChannelName == Elem.Key);
			Btn->SetBackgroundColor(bIsMe ? ActiveColor : InactiveColor);
		}
	}
}
