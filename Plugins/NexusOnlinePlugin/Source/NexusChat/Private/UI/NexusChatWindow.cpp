#include "UI/NexusChatWindow.h"
#include "UI/NexusChatChannelList.h"
#include "Core/NexusChatConfig.h"
#include "Components/ListView.h"
#include "UI/NexusChatMessageObj.h"
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "UI/NexusChatMessageRow.h"
#include "UI/NexusChatTabButton.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
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

		if (ChannelListButton)
		{
			ChannelListButton->SetVisibility(ESlateVisibility::Visible);
			ChannelListButton->OnClicked.AddDynamic(this, &UNexusChatWindow::OnChannelListButtonClicked);
		}

		if (ChannelList)
		{
			// Initialize list and hide it by default? Or keep state.
			ChannelList->Init(this);
			ChannelList->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (NotificationToggle)
		{
			NotificationToggle->SetVisibility(ESlateVisibility::Visible);
			NotificationToggle->OnCheckStateChanged.AddDynamic(this, &UNexusChatWindow::OnNotificationToggled);
			
			// Set initial state from Config
			if (const UNexusChatConfig* Config = GetDefault<UNexusChatConfig>()) 
			{
				NotificationToggle->SetIsChecked(Config->bEnableNotifications);
			}
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
	if (!ChatListView || !MessageRowClass)
		return;

	// Dynamic Tab Creation for Whispers (Ensure this runs before filtering!)
	if (bEnableChatTabs && Msg.Channel == ENexusChatChannel::Whisper)
	{
		if (APlayerController* PC = GetOwningPlayer()) 
		{
			FString MyName = PC->PlayerState ? PC->PlayerState->GetPlayerName() : "";
			FString OtherParty = (Msg.SenderName == MyName) ? Msg.TargetName : Msg.SenderName;
			
			if (!OtherParty.IsEmpty() && OtherParty != "System")
			{
				GetOrCreatePrivateTab(FName(*OtherParty));
			}
		}
	}

	if (UNexusChatSubsystem* Subsystem = GetWorld()->GetSubsystem<UNexusChatSubsystem>())
	{
		if (!Subsystem->IsMessagePassesFilter(Msg))
			return;
	}

	if (UNexusChatMessageObj* MessageObj = NewObject<UNexusChatMessageObj>(this))
	{
		MessageObj->Message = Msg;
		ChatListView->AddItem(MessageObj);
		ChatListView->ScrollIndexIntoView(ChatListView->GetNumItems() - 1);

		if (ChatListView->GetNumItems() > MaxChatLines)
		{
			if (UObject* OldObj = ChatListView->GetItemAt(0))
			{
				ChatListView->RemoveItem(OldObj);
			}
		}
	}

	// Notification Sound Logic
	if (UNexusChatConfig* Config = GetMutableDefault<UNexusChatConfig>())
	{
		if (Config->bEnableNotifications && Config->NotificationSound)
		{
			if (APlayerController* PC = GetOwningPlayer())
			{
				FString MyName = PC->PlayerState ? PC->PlayerState->GetPlayerName() : "";
				if (Msg.SenderName != MyName)
				{
					UGameplayStatics::PlaySound2D(this, Config->NotificationSound);
				}
			}
		}
	}
}

void UNexusChatWindow::OnTabClicked(FName ChannelName, bool bIsGeneral)
{
	SelectTab(ChannelName, bIsGeneral);
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
			TargetRouting = ENexusChatChannel::Custom;
			TargetChannelName = ActiveChannelName;
		}
	}

	FString FormattedMessage = FormatOutgoingMessage(Text.ToString(), TargetRouting);

	// Auto-Reply Logic for Whisper Tab
	if (TargetRouting == ENexusChatChannel::Whisper && !FormattedMessage.StartsWith(TEXT("/")))
	{
		FormattedMessage = TEXT("/r ") + FormattedMessage;
	}
	// Auto-Command for Dynamic Private Tabs
	else if (PrivateMessageTabs.Contains(ActiveChannelName) && !FormattedMessage.StartsWith(TEXT("/")))
	{
		FormattedMessage = FString::Printf(TEXT("/w %s %s"), *ActiveChannelName.ToString(), *FormattedMessage);
	}
	// Auto-Command for Dynamic Private Tabs
	else if (PrivateMessageTabs.Contains(ActiveChannelName) && !FormattedMessage.StartsWith(TEXT("/")))
	{
		FormattedMessage = FString::Printf(TEXT("/w %s %s"), *ActiveChannelName.ToString(), *FormattedMessage);
	}
	
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
			// Fixed: Skip null player states, process valid ones
			if (!PS)
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

void UNexusChatWindow::OpenChannel(FName ChannelName, bool bIsPrivate)
{
	// 1. Handle Global
	if (ChannelName == FName("Global"))
	{
		SelectTab(ChannelName, true);
		return;
	}

	// 2. Handle Existing
	if (ChannelTabButtons.Contains(ChannelName))
	{
		SelectTab(ChannelName, false);
		return;
	}

	// 3. Create New
	if (bIsPrivate)
	{
		GetOrCreatePrivateTab(ChannelName); // Adds to PrivateMessageTabs and UI
	}
	else
	{
		// Generic Public Tab (Team, Party, etc.)
		UNexusChatTabButton* NewTab = CreateTabButton(FText::FromName(ChannelName), ChannelName, false);
		if (NewTab)
		{
			if (TabContainer) TabContainer->AddChild(NewTab);
			ChannelTabButtons.Add(ChannelName, NewTab);
		}
	}

	// 4. Select it
	SelectTab(ChannelName, false);
}

void UNexusChatWindow::SelectTab(FName ChannelName, bool bIsGeneral)
{
	UNexusChatSubsystem* Subsystem = GetWorld()->GetSubsystem<UNexusChatSubsystem>();
	if (!Subsystem || !ChatComponent)
		return;

	ActiveChannelName = ChannelName;
	bIsGeneralTabActive = bIsGeneral;

	UpdateTabStyles();

	Subsystem->ClearHistoryFilters();
	if (bIsGeneral)
	{
		Subsystem->SetWhitelistMode(false); 
	}
	else
	{
		Subsystem->SetWhitelistMode(true);
		Subsystem->AddHistoryFilter(ChannelName);
	}

	if (ChatListView)
	{
		ChatListView->ClearListItems();
		
		TArray<FNexusChatMessage> Messages = ChatComponent->GetClientChatHistory();
		for (const FNexusChatMessage& Msg : Messages)
		{
			HandleMessageReceived(Msg);
		}
		
		ChatListView->ScrollIndexIntoView(ChatListView->GetNumItems() - 1);
	}
}

void UNexusChatWindow::OnChannelListButtonClicked()
{
	if (ChannelList)
	{
		if (ChannelList->GetVisibility() == ESlateVisibility::Collapsed || ChannelList->GetVisibility() == ESlateVisibility::Hidden)
		{
			ChannelList->SetVisibility(ESlateVisibility::Visible);
			ChannelList->RefreshList();
		}
		else
		{
			ChannelList->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UNexusChatWindow::OnNotificationToggled(bool bIsChecked)
{
	if (UNexusChatConfig* Config = GetMutableDefault<UNexusChatConfig>())
	{
		Config->bEnableNotifications = bIsChecked;
	}
}

UNexusChatTabButton* UNexusChatWindow::CreateTabButton(const FText& Label, FName ChannelName, bool bIsGeneral)
{
	UTextBlock* Text = NewObject<UTextBlock>(this);
	Text->SetText(Label);
	Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Text->SetFont(FCoreStyle::Get().GetFontStyle("NormalFont"));

	UNexusChatTabButton* Button = NewObject<UNexusChatTabButton>(this);
	Button->AddChild(Text);
	Button->Init(this, ChannelName, bIsGeneral);
	
	return Button;
}

void UNexusChatWindow::UpdateTabStyles()
{
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

UNexusChatTabButton* UNexusChatWindow::GetOrCreatePrivateTab(FName PlayerName)
{
	if (ChannelTabButtons.Contains(PlayerName))
	{
		return ChannelTabButtons[PlayerName];
	}

	if (!TabContainer)
		return nullptr;

	UNexusChatTabButton* NewTab = CreateTabButton(FText::FromName(PlayerName), PlayerName, false);
	if (NewTab)
	{
		TabContainer->AddChild(NewTab);
		ChannelTabButtons.Add(PlayerName, NewTab);
		PrivateMessageTabs.Add(PlayerName);
	}
	
	return NewTab;
}
