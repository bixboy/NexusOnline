#include "UI/NexusChatChannelList.h"
#include "UI/NexusChatChannelEntry.h"
#include "Core/NexusChatSubsystem.h"
#include "UI/NexusChatWindow.h"
#include "Components/Button.h"

void UNexusChatChannelList::NativeConstruct()
{
	Super::NativeConstruct();

    if (ChannelListView)
    {
        ChannelListView->OnItemClicked().AddUObject(this, &UNexusChatChannelList::OnChannelItemClicked);
    }
    
    if (CloseButton)
    {
        CloseButton->OnClicked.AddDynamic(this, &UNexusChatChannelList::OnCloseClicked);
    }
}

void UNexusChatChannelList::Init(UNexusChatWindow* InChatWindow)
{
    ChatWindow = InChatWindow;
    RefreshList();
}

void UNexusChatChannelList::RefreshList()
{
    if (!ChannelListView) return;

    ChannelListView->ClearListItems();

    // 1. Add Public Channels
    // Hardcoded list or from config
    TArray<FName> PublicChannels = { FName("Global"), FName("Team"), FName("Party"), FName("Guild") };
    
    // Check Config if available?
    // For now use standard ones.
    
    for (const FName& Chan : PublicChannels)
    {
        UNexusChatChannelData* Data = NewObject<UNexusChatChannelData>(this);
        Data->ChannelName = Chan;
        Data->bIsPrivate = false;
        ChannelListView->AddItem(Data);
    }

    // 2. Add Private Whispers
    if (UWorld* World = GetWorld())
    {
        if (UNexusChatSubsystem* Subsystem = World->GetSubsystem<UNexusChatSubsystem>())
        {
            TSet<FString> Whispers = Subsystem->GetActiveWhisperTargets();
             
             // Filter out "System" and maybe "Self" (though Subsystem Logic tries to track only targets)
             // We also need to get "Self" name to be sure we don't list ourselves if logic adds it.
             // Can't easily get PlayerState name here without iterating, but Whisper list usually is "Others".
             
            for (const FString& TargetName : Whispers)
            {
                UNexusChatChannelData* Data = NewObject<UNexusChatChannelData>(this);
                Data->ChannelName = FName(*TargetName);
                Data->bIsPrivate = true;
                ChannelListView->AddItem(Data);
            }
        }
    }
}

void UNexusChatChannelList::OnChannelItemClicked(UObject* Item)
{
	if (UNexusChatChannelData* Data = Cast<UNexusChatChannelData>(Item))
	{
		if (ChatWindow)
		{
			ChatWindow->OpenChannel(Data->ChannelName, Data->bIsPrivate);
			
			// Hide myself
			SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UNexusChatChannelList::OnCloseClicked()
{
    SetVisibility(ESlateVisibility::Collapsed);
}
