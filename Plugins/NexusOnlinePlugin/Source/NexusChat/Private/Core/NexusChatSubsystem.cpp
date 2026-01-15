#include "Core/NexusChatSubsystem.h"
#include "HAL/PlatformProcess.h"


void UNexusChatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	GlobalChatHistory.Empty();
	FilteredChannels.Empty();
	LinkHandlers.Empty();
}

void UNexusChatSubsystem::Deinitialize()
{
	LinkHandlers.Empty();
	Super::Deinitialize();
}

// ════════════════════════════════════════════════════════════════════════════════
// HISTORY MANAGEMENT
// ════════════════════════════════════════════════════════════════════════════════

void UNexusChatSubsystem::AddMessage(const FNexusChatMessage& Msg)
{
	GlobalChatHistory.Add(Msg);

	if (Msg.Channel == ENexusChatChannel::Whisper)
	{
		// If I sent it, add the target. If I received it, add the sender.
		// Note: We don't have direct access to "MyName" easily here without context, 
		// but usually the Subsystem exists on the client. 
		// Ideally, we just check if Sender is me or not, but for now we can rely on 
		// the UI to filter, OR just add both names to the "known" list if they aren't System.
		
		// A cleaner approach: The UI usually drives the "MyName" check. 
		// But let's try to infer or just add the "Other" party.
		// Actually, standard logic: Add the Sender (if it's not me) and Target (if it's not me).
		// Since we don't know "Me" here easily (Subsystem is UWorldSubsystem, GetLocalPlayer involves getting a helper),
		// we'll defer the "MyName" logic to determining who the "Other" is.
		// BUT, for purely "Recently messaged people", we can just add both and filter out "Self" in UI.
		// Or better: The `Msg` struct doesn't strictly say who "local player" is. 
		
		// Let's add them to the list. The UI will filter out "Self".
		if (Msg.SenderName != TEXT("System")) ActiveWhisperTargets.Add(Msg.SenderName);
		if (Msg.TargetName != TEXT("System")) ActiveWhisperTargets.Add(Msg.TargetName);
	}

	if (GlobalChatHistory.Num() > MaxHistorySize)
	{
		GlobalChatHistory.RemoveAt(0);
	}
}

TArray<FNexusChatMessage> UNexusChatSubsystem::GetFilteredHistory() const
{
	if (FilteredChannels.IsEmpty())
	{
		return GlobalChatHistory;
	}

	TArray<FNexusChatMessage> Result;
	Result.Reserve(GlobalChatHistory.Num());

	for (const FNexusChatMessage& Msg : GlobalChatHistory)
	{
		if (IsMessagePassesFilter(Msg))
		{
			Result.Add(Msg);
		}
	}

	return Result;
}

void UNexusChatSubsystem::AddHistoryFilter(FName ChannelName)
{
	FilteredChannels.Add(ChannelName);
}

void UNexusChatSubsystem::RemoveHistoryFilter(FName ChannelName)
{
	FilteredChannels.Remove(ChannelName);
}

void UNexusChatSubsystem::ClearHistoryFilters()
{
	FilteredChannels.Empty();
}

bool UNexusChatSubsystem::IsMessagePassesFilter(const FNexusChatMessage& Msg) const
{
	if (FilteredChannels.IsEmpty())
		return true;
    
	FName CheckName = Msg.ChannelName;
	if (CheckName.IsNone())
	{
		CheckName = FName(*UEnum::GetDisplayValueAsText(Msg.Channel).ToString());
	}

	if (bWhitelistMode)
	{
		// Special handling for Whispers in Private Tabs
		if (Msg.Channel == ENexusChatChannel::Whisper)
		{
			// If we are filtering for "Alice", we want messages WHERE (Sender=Alice OR Target=Alice)
			for (const FName& Filter : FilteredChannels)
			{
				if (Filter.IsEqual(FName(*Msg.SenderName)) || Filter.IsEqual(FName(*Msg.TargetName)))
				{
					return true;
				}
			}
		}
		
		return FilteredChannels.Contains(CheckName);
	}
	
	// Blacklist mode (default or inverted) - currently only Whitelist logic is actively used for tabs
	return !FilteredChannels.Contains(CheckName);
}

void UNexusChatSubsystem::SetWhitelistMode(bool bEnable)
{
	bWhitelistMode = bEnable;
}

// ════════════════════════════════════════════════════════════════════════════════
// LINK HANDLING
// ════════════════════════════════════════════════════════════════════════════════

void UNexusChatSubsystem::RegisterLinkHandler(const FString& LinkType, FLinkTypeHandler Handler)
{
	if (!LinkType.IsEmpty())
	{
		LinkHandlers.Add(LinkType, Handler);
	}
}

void UNexusChatSubsystem::UnregisterLinkHandler(const FString& LinkType)
{
	LinkHandlers.Remove(LinkType);
}

bool UNexusChatSubsystem::HasLinkHandler(const FString& LinkType) const
{
	return LinkHandlers.Contains(LinkType);
}

void UNexusChatSubsystem::HandleLinkClicked(const FString& LinkType, const FString& LinkData)
{
	OnAnyLinkClicked.Broadcast(LinkType, LinkData);

	if (LinkType == TEXT("player"))
	{
		HandlePlayerLink(LinkData);
	}
	else if (LinkType == TEXT("url"))
	{
		HandleUrlLink(LinkData);
	}
	else if (FLinkTypeHandler* Handler = LinkHandlers.Find(LinkType))
	{
		if (Handler->IsBound())
		{
			Handler->Execute(LinkData);
		}
	}
}

void UNexusChatSubsystem::HandleUrlLink(const FString& Url)
{
	if (bAutoOpenUrls && !Url.IsEmpty())
	{
		FString FinalUrl = Url;
		if (!FinalUrl.StartsWith(TEXT("http")))
		{
			FinalUrl = TEXT("http://") + FinalUrl;
		}
		
		FPlatformProcess::LaunchURL(*FinalUrl, nullptr, nullptr);
	}
}

void UNexusChatSubsystem::HandlePlayerLink(const FString& PlayerName)
{
	UE_LOG(LogTemp, Log, TEXT("[NexusChat] Player link clicked: %s"), *PlayerName);
}

void UNexusChatSubsystem::AddWhisperTarget(const FString& PlayerName)
{
	if (!PlayerName.IsEmpty() && PlayerName != "System")
	{
		ActiveWhisperTargets.Add(PlayerName);
	}
}

void UNexusChatSubsystem::RemoveWhisperTarget(const FString& PlayerName)
{
	ActiveWhisperTargets.Remove(PlayerName);
}