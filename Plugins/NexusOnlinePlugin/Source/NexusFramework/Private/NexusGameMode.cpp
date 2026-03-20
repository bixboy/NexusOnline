#include "NexusGameMode.h"

#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/NexusBanSubsystem.h"
#include "Managers/OnlineSessionManager.h"
#include "Utils/NexusOnlineHelpers.h"


ANexusGameMode::ANexusGameMode()
{
	// Optionnel : Configurer des defaults ici
}

void ANexusGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
		return;

	UWorld* World = GetWorld();
	if (!ensure(World))
		return;
	
	INexusSessionHandler::InitializeNexusSession(this, NAME_GameSession);

	// Register Host via Manager
	if (AOnlineSessionManager* Manager = AOnlineSessionManager::Get(this))
	{
		Manager->RegisterLocalHost();
	}
	else
	{
		// Spawn it if missing? Usually it's spawned by Subsystem or GameState via logic?
		// For now we assume InitializeNexusSession might have spawned it or it's implicitly spawned.
		// Actually, AOnlineSessionManager::Spawn is a static. 
		// If we want to guarantee it exists:
		AOnlineSessionManager::Spawn(this)->RegisterLocalHost();
	}
}

// ──────────────────────────────────────────────
// LOGIN FLOW
// ──────────────────────────────────────────────

void ANexusGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	if (!ErrorMessage.IsEmpty())
		return;

	// ──────────────────────────────────────────────
	// BAN CHECK (via Subsystem — cached, O(1))
	// ──────────────────────────────────────────────
	if (UniqueId.IsValid())
	{
		if (UNexusBanSubsystem* BanSub = GetGameInstance()->GetSubsystem<UNexusBanSubsystem>())
		{
			FString BanReason;
			FDateTime BanExpiration;
			if (BanSub->IsBanned(UniqueId.ToString(), BanReason, BanExpiration))
			{
				if (BanExpiration == FDateTime::MaxValue())
				{
					ErrorMessage = FString::Printf(TEXT("You are banned: %s"), *BanReason);
				}
				else
				{
					const FTimespan Remaining = BanExpiration - FDateTime::UtcNow();
					ErrorMessage = FString::Printf(TEXT("TempBan remaining: %d minutes. Reason: %s"), (int32)Remaining.GetTotalMinutes(), *BanReason);
				}
				UE_LOG(LogTemp, Warning, TEXT("[NexusGameMode] Rejecting banned player %s: %s"), *UniqueId.ToString(), *ErrorMessage);
				return;
			}
		}
	}

	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(GetWorld());
	if (!Session.IsValid())
		return;

	FNamedOnlineSession* NamedSession = Session->GetNamedSession(NAME_GameSession);
	if (!NamedSession)
		return;

	const int32 MaxPublic = NamedSession->SessionSettings.NumPublicConnections;
	const int32 MaxPrivate = NamedSession->SessionSettings.NumPrivateConnections;
	
	const bool bHasSpace = (NamedSession->NumOpenPublicConnections > 0) || (NamedSession->NumOpenPrivateConnections > 0);
	if (!bHasSpace)
	{
		UE_LOG(LogTemp, Warning, TEXT("[NexusGameMode] Rejecting login: Session Full (%d/%d)."), 
			(MaxPublic + MaxPrivate) - (NamedSession->NumOpenPublicConnections + NamedSession->NumOpenPrivateConnections),
			(MaxPublic + MaxPrivate));
		
		ErrorMessage = TEXT("SessionFull");
	}
}

void ANexusGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	INexusSessionHandler::RegisterPlayerWithSession(this, NewPlayer);
}

void ANexusGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	INexusSessionHandler::UnregisterPlayerFromSession(this, Exiting);
}