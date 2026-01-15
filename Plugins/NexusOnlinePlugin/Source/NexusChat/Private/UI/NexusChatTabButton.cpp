#include "UI/NexusChatTabButton.h"
#include "UI/NexusChatWindow.h"


void UNexusChatTabButton::Init(UNexusChatWindow* Window, FName InChannelName, bool bInIsGeneral)
{
	MainWindow = Window;
	ChannelName = InChannelName;
	bIsGeneral = bInIsGeneral;
	
	OnClicked.RemoveDynamic(this, &UNexusChatTabButton::HandleClick);
	OnClicked.AddDynamic(this, &UNexusChatTabButton::HandleClick);
}

void UNexusChatTabButton::HandleClick()
{
	if (MainWindow.IsValid())
	{
		MainWindow->OnTabClicked(ChannelName, bIsGeneral);
	}
}
