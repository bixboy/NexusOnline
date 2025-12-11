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

	if (GlobalChatHistory.Num() > MaxHistorySize)
	{
		GlobalChatHistory.RemoveAt(0);
	}
}

void UNexusChatSubsystem::AddHistoryFilter(ENexusChatChannel Channel)
{
	FilteredChannels.Add(Channel);
}

void UNexusChatSubsystem::RemoveHistoryFilter(ENexusChatChannel Channel)
{
	FilteredChannels.Remove(Channel);
}

void UNexusChatSubsystem::ClearHistoryFilters()
{
	FilteredChannels.Empty();
}

bool UNexusChatSubsystem::IsChannelFiltered(ENexusChatChannel Channel) const
{
	return FilteredChannels.Contains(Channel);
}

TArray<FNexusChatMessage> UNexusChatSubsystem::GetFilteredHistory() const
{
	TArray<FNexusChatMessage> Result;
	Result.Reserve(GlobalChatHistory.Num());
	
	for (const FNexusChatMessage& Msg : GlobalChatHistory)
	{
		if (!FilteredChannels.Contains(Msg.Channel))
		{
			Result.Add(Msg);
		}
	}
	return Result;
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
	// Always broadcast the global event first
	OnAnyLinkClicked.Broadcast(LinkType, LinkData);

	// Route to appropriate handler
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

void UNexusChatSubsystem::HandlePlayerLink(const FString& PlayerName)
{
	OnPlayerLinkClicked.Broadcast(TEXT("player"), PlayerName);
}

void UNexusChatSubsystem::HandleUrlLink(const FString& Url)
{
	OnUrlLinkClicked.Broadcast(TEXT("url"), Url);
	
	if (bAutoOpenUrls)
	{
		FPlatformProcess::LaunchURL(*Url, nullptr, nullptr);
	}
}
