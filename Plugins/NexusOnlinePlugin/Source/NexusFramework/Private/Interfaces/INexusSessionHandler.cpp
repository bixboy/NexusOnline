#include "Interfaces/INexusSessionHandler.h"
#include "Managers/OnlineSessionManager.h"
#include "Utils/NexusOnlineHelpers.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/OnlineSessionInterface.h"


void INexusSessionHandler::InitializeNexusSession(UObject* WorldContextObject, FName SessionName)
{
	if (!WorldContextObject)
		return;

    UE_LOG(LogTemp, Log, TEXT("[NexusSessionHandler] Requesting OnlineSessionManager Spawn..."));
	AOnlineSessionManager::Spawn(WorldContextObject, SessionName);
	UE_LOG(LogTemp, Log, TEXT("[NexusSessionHandler] OnlineSessionManager initialized for session '%s'."), *SessionName.ToString());
}

void INexusSessionHandler::RegisterPlayerWithSession(UObject* WorldContextObject, APlayerController* Player, FName SessionName)
{
	if (!Player || !Player->PlayerState)
		return;

	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(WorldContextObject);
	if (!Session.IsValid())
		return;

	FUniqueNetIdRepl Id = Player->PlayerState->GetUniqueId();
	if (Id.IsValid())
	{
		Session->RegisterPlayer(SessionName, *Id, false);

		if (FNamedOnlineSession* Named = Session->GetNamedSession(SessionName))
		{
			Session->UpdateSession(SessionName, Named->SessionSettings, true);
		}
	}
}

void INexusSessionHandler::UnregisterPlayerFromSession(UObject* WorldContextObject, AController* Exiting, FName SessionName)
{
	if (!Exiting)
		return;

	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(WorldContextObject);
	if (!Session.IsValid())
		return;

	if (APlayerState* PS = Exiting->GetPlayerState<APlayerState>())
	{
		FUniqueNetIdRepl Id = PS->GetUniqueId();
		if (Id.IsValid())
		{
			Session->UnregisterPlayer(SessionName, *Id);

			if (FNamedOnlineSession* Named = Session->GetNamedSession(SessionName))
			{
				Session->UpdateSession(SessionName, Named->SessionSettings, true);
			}
		}
	}
}
