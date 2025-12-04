#include "Core/NexusChatSubsystem.h"

void UNexusChatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	GlobalChatHistory.Empty();
}

void UNexusChatSubsystem::AddMessage(const FNexusChatMessage& Msg)
{
	GlobalChatHistory.Add(Msg);

	if (GlobalChatHistory.Num() > MaxHistorySize)
	{
		GlobalChatHistory.RemoveAt(0);
	}
}
