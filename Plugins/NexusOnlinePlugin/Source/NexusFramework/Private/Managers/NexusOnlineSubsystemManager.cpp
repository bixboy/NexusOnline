#include "NexusFramework/Public/Managers/NexusOnlineSubsystemManager.h"
#include "../../../../../../../../Versions/UE_5.6/Engine/Plugins/Online/OnlineSubsystem/Source/Public/OnlineSubsystem.h"
#include "../../../../../../../../Versions/UE_5.6/Engine/Plugins/Online/OnlineSubsystemUtils/Source/OnlineSubsystemUtils/Public/OnlineSubsystemUtils.h"

IOnlineSubsystem* UNexusOnlineSubsystemManager::GetSubsystem()
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObjectChecked(GEngine) : nullptr;
	if (!World)
		return nullptr;

	return Online::GetSubsystem(World);
}

IOnlineSessionPtr UNexusOnlineSubsystemManager::GetSessionInterface()
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObjectChecked(GEngine) : nullptr;
	if (!World)
		return nullptr;

	if (IOnlineSubsystem* Subsystem = Online::GetSubsystem(World))
	{
		return Subsystem->GetSessionInterface();
	}

	return nullptr;
}
