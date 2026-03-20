#include "NexusOnlineSession.h"
#include "Subsystems/NexusMigrationSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void UNexusOnlineSession::HandleDisconnect(UWorld* World, UNetDriver* NetDriver)
{
	if (World)
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UNexusMigrationSubsystem* MigSub = GI->GetSubsystem<UNexusMigrationSubsystem>())
			{
				if (MigSub->IsMigrating())
				{
					UE_LOG(LogTemp, Log, TEXT("[NexusOnlineSession] Suppressing HandleDisconnect — migration in progress."));
					return;
				}
			}
		}
	}

	// Not migrating — fall back to default UE behavior
	Super::HandleDisconnect(World, NetDriver);
}
