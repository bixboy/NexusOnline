#pragma once

#include "CoreMinimal.h"
#include "GameFramework/OnlineSession.h"
#include "NexusOnlineSession.generated.h"

class UNexusMigrationSubsystem;

/**
 * Custom OnlineSession that suppresses UE's default HandleDisconnect
 * (which navigates to ?closed) when host migration is in progress.
 *
 * Without this, UEngine::HandleDisconnect races with migration's
 * auto-travel to ?listen and wins, killing the new listen server.
 */
UCLASS()
class NEXUSFRAMEWORK_API UNexusOnlineSession : public UOnlineSession
{
	GENERATED_BODY()

public:
	virtual void HandleDisconnect(UWorld* World, UNetDriver* NetDriver) override;
};
