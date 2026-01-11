#include "NexusGameMode.h"

#include "NexusBanTypes.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Managers/OnlineSessionManager.h"
#include "Utils/NexusOnlineHelpers.h"
#include "Kismet/GameplayStatics.h"


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
	
	AOnlineSessionManager::Spawn(this, NAME_GameSession);
	UE_LOG(LogTemp, Log, TEXT("[NexusGameMode] OnlineSessionManager initialized."));

	RegisterHostRetries = 0;
	TryRegisterHost();
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
	// BAN CHECK
	// ──────────────────────────────────────────────
	if (UniqueId.IsValid())
	{
		if (UNexusBanSave* BanSave = Cast<UNexusBanSave>(UGameplayStatics::LoadGameFromSlot("NexusBanList", 0)))
		{
			FString IdStr = UniqueId.ToString();
			if (FNexusBanInfo* BanInfo = BanSave->BannedPlayers.Find(IdStr))
			{
				FDateTime Now = FDateTime::UtcNow();
				if (Now < BanInfo->ExpirationTimestamp)
				{
					// Banned
					if (BanInfo->ExpirationTimestamp == FDateTime::MaxValue())
					{
						ErrorMessage = FString::Printf(TEXT("You are banned: %s"), *BanInfo->Reason);
					}
					else
					{
						FTimespan Remaining = BanInfo->ExpirationTimestamp - Now;
						ErrorMessage = FString::Printf(TEXT("TempBan remaining: %d minutes. Reason: %s"), (int32)Remaining.GetTotalMinutes(), *BanInfo->Reason);
					}

					UE_LOG(LogTemp, Warning, TEXT("[NexusGameMode] Rejecting banned player %s: %s"), *IdStr, *ErrorMessage);
					return;
				}
				else
				{
					// Expired, Unban locally
					BanSave->BannedPlayers.Remove(IdStr);
					UGameplayStatics::SaveGameToSlot(BanSave, "NexusBanList", 0);
					UE_LOG(LogTemp, Log, TEXT("[NexusGameMode] Ban expired for player %s. Unbanning."), *IdStr);
				}
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
	
	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(GetWorld());
	if (Session.IsValid() && NewPlayer && NewPlayer->PlayerState)
	{
		FUniqueNetIdRepl Id = NewPlayer->PlayerState->GetUniqueId();
		
		if (Id.IsValid())
		{
			Session->RegisterPlayer(NAME_GameSession, *Id, false);
			
			if (FNamedOnlineSession* Named = Session->GetNamedSession(NAME_GameSession))
			{
				Session->UpdateSession(NAME_GameSession, Named->SessionSettings, true);
			}
		}
	}
}

void ANexusGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	
	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(GetWorld());
	if (Session.IsValid())
	{
		if (APlayerState* PS = Exiting ? Exiting->GetPlayerState<APlayerState>() : nullptr)
		{
			FUniqueNetIdRepl Id = PS->GetUniqueId();
			if (Id.IsValid())
			{
				Session->UnregisterPlayer(NAME_GameSession, *Id);
				
				if (FNamedOnlineSession* Named = Session->GetNamedSession(NAME_GameSession))
				{
					Session->UpdateSession(NAME_GameSession, Named->SessionSettings, true);
				}
			}
		}
	}
}

// ──────────────────────────────────────────────
// HOST REGISTRATION UTILS
// ──────────────────────────────────────────────

void ANexusGameMode::TryRegisterHost()
{
	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(GetWorld());
	if (!Session.IsValid())
		return;

	FNamedOnlineSession* Named = Session->GetNamedSession(NAME_GameSession);
	if (!Named)
		return;

	if (IsRunningDedicatedServer())
		return;

	FUniqueNetIdRepl HostId;
	if (GetHostUniqueId(HostId))
	{
		Session->RegisterPlayer(NAME_GameSession, *HostId, false);
		if (Session->GetSessionState(NAME_GameSession) == EOnlineSessionState::Pending)
		{
			Session->StartSession(NAME_GameSession);
			UE_LOG(LogTemp, Log, TEXT("[NexusGameMode] Session Started after Host Registration."));
		}

		Session->UpdateSession(NAME_GameSession, Named->SessionSettings, true);
		UE_LOG(LogTemp, Log, TEXT("[NexusGameMode] Host Registered: %s"), *HostId.ToString());
		
		GetWorldTimerManager().ClearTimer(TimerHandle_RetryRegister);
	}
	else
	{
		RegisterHostRetries++;
		if (RegisterHostRetries < MAX_REGISTER_RETRIES)
		{
			UE_LOG(LogTemp, Verbose, TEXT("[NexusGameMode] Host ID not ready, retrying... (%d/%d)"), RegisterHostRetries, MAX_REGISTER_RETRIES);
			GetWorldTimerManager().SetTimer(TimerHandle_RetryRegister, this, &ANexusGameMode::TryRegisterHost, 0.5f, false);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[NexusGameMode] Failed to register host after %d retries. Session might not be visible correctly."), MAX_REGISTER_RETRIES);
		}
	}
}

bool ANexusGameMode::GetHostUniqueId(FUniqueNetIdRepl& OutId) const
{
	UWorld* World = GetWorld();
	if (!World)
		return false;

	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		if (APlayerState* PS = PC->PlayerState)
		{
			FUniqueNetIdRepl Id = PS->GetUniqueId();
			if (Id.IsValid())
			{
				OutId = Id;
				return true;
			}
		}
	}

	if (ULocalPlayer* LP = World->GetFirstLocalPlayerFromController())
	{
		FUniqueNetIdRepl Id = LP->GetPreferredUniqueNetId();
		if (Id.IsValid())
		{
			OutId = Id;
			return true;
		}
	}

	if (IOnlineIdentityPtr Identity = NexusOnline::GetIdentityInterface(World))
	{
		if (TSharedPtr<const FUniqueNetId> Id = Identity->GetUniquePlayerId(0))
		{
			OutId = FUniqueNetIdRepl(Id);
			return true;
		}
	}

	return false;
}

// ──────────────────────────────────────────────
// ADMIN ACTIONS
// ──────────────────────────────────────────────

bool ANexusGameMode::KickPlayer(APlayerController* Target, const FText& Reason)
{
	if (!Target) return false;

	if (Target->IsLocalController())
	{
		UE_LOG(LogTemp, Warning, TEXT("[NexusGameMode] Cannot kick Host/LocalPlayer."));
		return false;
	}

	Target->ClientReturnToMainMenuWithTextReason(Reason);
	
	if (GameSession)
	{
		GameSession->KickPlayer(Target, Reason);
	}
	
	UE_LOG(LogTemp, Log, TEXT("[NexusGameMode] Kicked player %s. Reason: %s"), *Target->GetName(), *Reason.ToString());
	return true;
}

bool ANexusGameMode::BanPlayer(APlayerController* Target, const FText& Reason)
{
	return TempBanPlayer(Target, -1.0f, Reason);
}

bool ANexusGameMode::TempBanPlayer(APlayerController* Target, float DurationHours, const FText& Reason)
{
	if (!Target || !Target->PlayerState) return false;

	if (Target->IsLocalController())
	{
		UE_LOG(LogTemp, Warning, TEXT("[NexusGameMode] Cannot ban Host/LocalPlayer."));
		return false;
	}

	FUniqueNetIdRepl UniqueId = Target->PlayerState->GetUniqueId();
	if (!UniqueId.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[NexusGameMode] Cannot ban player with invalid UniqueNetId."));
		return false;
	}

	FString IdStr = UniqueId.ToString();

	UNexusBanSave* BanSave = Cast<UNexusBanSave>(UGameplayStatics::LoadGameFromSlot("NexusBanList", 0));
	if (!BanSave)
	{
		BanSave = Cast<UNexusBanSave>(UGameplayStatics::CreateSaveGameObject(UNexusBanSave::StaticClass()));
	}

	FNexusBanInfo NewBan;
	NewBan.PlayerId = IdStr;
	NewBan.Reason = Reason.ToString();
	NewBan.BanTimestamp = FDateTime::UtcNow();

	if (DurationHours < 0)
	{
		NewBan.ExpirationTimestamp = FDateTime::MaxValue();
	}
	else
	{
		NewBan.ExpirationTimestamp = NewBan.BanTimestamp + FTimespan::FromHours(DurationHours);
	}

	BanSave->BannedPlayers.Add(IdStr, NewBan);
	UGameplayStatics::SaveGameToSlot(BanSave, "NexusBanList", 0);

	UE_LOG(LogTemp, Log, TEXT("[NexusGameMode] Banned player %s (%s). Expiration: %s"), *Target->GetName(), *IdStr, *NewBan.ExpirationTimestamp.ToString());

	KickPlayer(Target, Reason);

	return true;
}

bool ANexusGameMode::UnbanPlayer(const FString& PlayerId)
{
	UNexusBanSave* BanSave = Cast<UNexusBanSave>(UGameplayStatics::LoadGameFromSlot("NexusBanList", 0));
	if (BanSave)
	{
		if (BanSave->BannedPlayers.Remove(PlayerId) > 0)
		{
			UGameplayStatics::SaveGameToSlot(BanSave, "NexusBanList", 0);
			UE_LOG(LogTemp, Log, TEXT("[NexusGameMode] Unbanned player %s."), *PlayerId);
			return true;
		}
	}
	else
	{
		// Try to see if save exists even if load failed? No, Load creates null if not found.
		// If null, no bans exist.
	}
	
	return false;
}