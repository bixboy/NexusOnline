#include "UI/NexusChatMessageRow.h"
#include "Core/NexusLinkHelpers.h"

void UNexusChatMessageRow::NativeConstruct()
{
	Super::NativeConstruct();

	// Initialize default channel styles if not set
	if (ChannelStyles.IsEmpty())
	{
		ChannelStyles.Add(ENexusChatChannel::Global, "Global");
		ChannelStyles.Add(ENexusChatChannel::Team, "Team");
		ChannelStyles.Add(ENexusChatChannel::Party, "Party");
		ChannelStyles.Add(ENexusChatChannel::Whisper, "Whisper");
		ChannelStyles.Add(ENexusChatChannel::System, "System");
		ChannelStyles.Add(ENexusChatChannel::GameLog, "GameLog");
	}
}

void UNexusChatMessageRow::HandlePlayerLinkClicked(const FString& PlayerName)
{
	OnPlayerClicked.Broadcast(PlayerName);
}

void UNexusChatMessageRow::InitWidget_Implementation(const FNexusChatMessage& Message)
{
	if (!MessageText)
	{
		return;
	}

	// Get style for this channel
	FName StyleName = ChannelStyles.Contains(Message.Channel) 
		? ChannelStyles[Message.Channel] 
		: FName("Default");

	// Decode HTML entities (may be escaped during network transfer)
	FString Content = Message.MessageContent;
	Content = Content.Replace(TEXT("&lt;"), TEXT("<"));
	Content = Content.Replace(TEXT("&gt;"), TEXT(">"));
	Content = Content.Replace(TEXT("&amp;"), TEXT("&"));
	Content = Content.Replace(TEXT("&quot;"), TEXT("\""));

	// Auto-format URLs first (converts URLs to <link> tags)
	FString FormattedContent = UNexusLinkHelpers::AutoFormatUrls(Content);
	
	// Check if content has links AFTER formatting
	const bool bHasLinks = FormattedContent.Contains(TEXT("<link"));
	const bool bIsSystemMessage = (Message.Channel == ENexusChatChannel::System || 
	                               Message.Channel == ENexusChatChannel::GameLog);

	// Build the final text
	FString FullText;
	
	if (bIsSystemMessage)
	{
		// System messages: no player name
		if (bHasLinks)
		{
			FullText = FormattedContent;
		}
		else
		{
			FullText = FString::Printf(TEXT("<%s>%s</>"), *StyleName.ToString(), *FormattedContent);
		}
	}
	else
	{
		// Player messages: clickable name + content
		FString PlayerLink = UNexusLinkHelpers::MakePlayerLink(Message.SenderName, 15);
		
		if (bHasLinks)
		{
			// Has links - don't wrap in style tag (would break link parsing)
			FullText = FString::Printf(TEXT("%s: %s"), *PlayerLink, *FormattedContent);
		}
		else
		{
			// No links - safe to wrap in style tag
			FullText = FString::Printf(TEXT("%s: <%s>%s</>"), *PlayerLink, *StyleName.ToString(), *FormattedContent);
		}
	}

	MessageText->SetText(FText::FromString(FullText));
}
