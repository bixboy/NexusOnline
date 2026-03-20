#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "INexusSessionHandler.generated.h"

class APlayerController;

UINTERFACE(MinimalAPI, Blueprintable)
class UNexusSessionHandler : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for GameModes that want to use NexusOnline session features
 * without inheriting from ANexusGameMode.
 *
 * Implement this on your custom GameMode to hook into the Nexus session lifecycle.
 * The default implementations are no-ops, so you only override what you need.
 */
class NEXUSFRAMEWORK_API INexusSessionHandler
{
	GENERATED_BODY()

public:

	// ──────────────────────────────────────────────
	// Session Lifecycle Hooks
	// ──────────────────────────────────────────────

	/** Called during PreLogin. Return a non-empty string to reject the player with that error message. */
	virtual FString OnNexusPreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId)
	{
		return FString();
	}

	/** Called after a player has fully logged in and been registered with the session. */
	virtual void OnNexusPostLogin(APlayerController* NewPlayer) {}

	/** Called when a player logs out and is unregistered from the session. */
	virtual void OnNexusLogout(AController* Exiting) {}

	// ──────────────────────────────────────────────
	// Static Helpers — call from ANY GameMode
	// ──────────────────────────────────────────────

	/** Spawns an OnlineSessionManager if one doesn't already exist in the world. */
	static void InitializeNexusSession(UObject* WorldContextObject, FName SessionName = NAME_GameSession);

	/** Registers a player with the active session. Call from PostLogin. */
	static void RegisterPlayerWithSession(UObject* WorldContextObject, APlayerController* Player, FName SessionName = NAME_GameSession);

	/** Unregisters a player from the active session. Call from Logout. */
	static void UnregisterPlayerFromSession(UObject* WorldContextObject, AController* Exiting, FName SessionName = NAME_GameSession);
};
