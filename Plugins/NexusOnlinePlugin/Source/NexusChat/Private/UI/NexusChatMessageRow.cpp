#include "UI/NexusChatMessageRow.h"


void UNexusChatMessageRow::NativeConstruct()
{
	Super::NativeConstruct();

	if (ChannelStyles.IsEmpty())
	{
		ChannelStyles.Add(ENexusChatChannel::Global, "Global");
		ChannelStyles.Add(ENexusChatChannel::Team, "Team");
		ChannelStyles.Add(ENexusChatChannel::Party, "Party");
		ChannelStyles.Add(ENexusChatChannel::Whisper, "Whisper");
		ChannelStyles.Add(ENexusChatChannel::System, "System");
	}
}

void UNexusChatMessageRow::InitWidget_Implementation(const FNexusChatMessage& Message)
{
	if (!MessageText)
		return;

	FName StyleName = "Default";
	if (ChannelStyles.Contains(Message.Channel))
	{
		StyleName = ChannelStyles[Message.Channel];
	}

	FString FullText;
	if (Message.Channel == ENexusChatChannel::System)
	{
		FullText = FString::Printf(TEXT("<%s>%s</>"), *StyleName.ToString(), *Message.MessageContent);
	}
	else
	{
		FString SenderName = Message.SenderName;
		if (SenderName.Len() > 10)
		{
			SenderName = SenderName.Left(10) + "...";
		}
		FullText = FString::Printf(TEXT("<%s>%s: %s</>"), *StyleName.ToString(), *SenderName, *Message.MessageContent);
	}

	MessageText->SetText(FText::FromString(FullText));
}