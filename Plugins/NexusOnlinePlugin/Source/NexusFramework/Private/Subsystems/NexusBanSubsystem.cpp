#include "Subsystems/NexusBanSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"


void UNexusBanSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadBanList();
	CleanupExpiredBans();
}

void UNexusBanSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

// ──────────────────────────────────────────────
// PERSISTENCE
// ──────────────────────────────────────────────

void UNexusBanSubsystem::LoadBanList()
{
	CachedBanSave = Cast<UNexusBanSave>(UGameplayStatics::LoadGameFromSlot(BAN_SAVE_SLOT, 0));
	if (!CachedBanSave)
	{
		CachedBanSave = Cast<UNexusBanSave>(UGameplayStatics::CreateSaveGameObject(UNexusBanSave::StaticClass()));
	}
}

void UNexusBanSubsystem::SaveBanList()
{
	if (IsValid(CachedBanSave))
	{
		UGameplayStatics::SaveGameToSlot(CachedBanSave, BAN_SAVE_SLOT, 0);
	}
}

void UNexusBanSubsystem::CleanupExpiredBans()
{
	if (!IsValid(CachedBanSave))
		return;

	const FDateTime Now = FDateTime::UtcNow();
	TArray<FString> ExpiredIds;

	for (const auto& Pair : CachedBanSave->BannedPlayers)
	{
		if (Pair.Value.ExpirationTimestamp != FDateTime::MaxValue() && Now >= Pair.Value.ExpirationTimestamp)
		{
			ExpiredIds.Add(Pair.Key);
		}
	}

	if (ExpiredIds.Num() > 0)
	{
		for (const FString& Id : ExpiredIds)
		{
			CachedBanSave->BannedPlayers.Remove(Id);
			UE_LOG(LogTemp, Log, TEXT("[NexusBanSubsystem] Cleaned up expired ban for player %s."), *Id);
		}
		SaveBanList();
	}
}

// ──────────────────────────────────────────────
// PUBLIC API
// ──────────────────────────────────────────────

bool UNexusBanSubsystem::IsBanned(const FString& PlayerId, FString& OutReason, FDateTime& OutExpiration) const
{
	if (!IsValid(CachedBanSave) || PlayerId.IsEmpty())
		return false;

	const FNexusBanInfo* BanInfo = CachedBanSave->BannedPlayers.Find(PlayerId);
	if (!BanInfo)
		return false;

	const FDateTime Now = FDateTime::UtcNow();
	if (Now >= BanInfo->ExpirationTimestamp && BanInfo->ExpirationTimestamp != FDateTime::MaxValue())
	{
		// Expired — will be cleaned up next time
		return false;
	}

	OutReason = BanInfo->Reason;
	OutExpiration = BanInfo->ExpirationTimestamp;
	return true;
}

bool UNexusBanSubsystem::BanPlayer(const FString& PlayerId, const FText& Reason)
{
	return TempBanPlayer(PlayerId, -1.0f, Reason);
}

bool UNexusBanSubsystem::TempBanPlayer(const FString& PlayerId, float DurationHours, const FText& Reason)
{
	if (PlayerId.IsEmpty() || !IsValid(CachedBanSave))
		return false;

	FNexusBanInfo NewBan;
	NewBan.PlayerId = PlayerId;
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

	CachedBanSave->BannedPlayers.Add(PlayerId, NewBan);
	SaveBanList();

	UE_LOG(LogTemp, Log, TEXT("[NexusBanSubsystem] Banned player %s. Expiration: %s"), *PlayerId, *NewBan.ExpirationTimestamp.ToString());

	OnPlayerBanned.Broadcast(PlayerId, NewBan.Reason);

	// Auto-Kick if online
	if (UWorld* World = GetWorld())
	{
		if (const AGameStateBase* GS = World->GetGameState())
		{
			for (APlayerState* PS : GS->PlayerArray)
			{
				if (PS && PS->GetUniqueId().ToString() == PlayerId)
				{
					if (APlayerController* PC = PS->GetPlayerController())
					{
						KickPlayer(PC, Reason);
					}
					break;
				}
			}
		}
	}

	return true;
}

bool UNexusBanSubsystem::KickPlayer(APlayerController* Target, const FText& Reason)
{
	if (!Target) return false;

	// Note: We can check if it's local controller to prevent kicking host, 
	// but sometimes we might want to kick a local player in split screen?
	// For now, let's keep the safety against Listen Server Host.
	if (Target->IsLocalController())
	{
		UE_LOG(LogTemp, Warning, TEXT("[NexusBanSubsystem] Cannot kick Host/LocalPlayer."));
		return false;
	}

	Target->ClientReturnToMainMenuWithTextReason(Reason);
	
	if (UWorld* World = GetWorld())
	{
		if (AGameModeBase* GM = World->GetAuthGameMode())
		{
			if (GM->GameSession)
			{
				GM->GameSession->KickPlayer(Target, Reason);
			}
		}
	}
	
	UE_LOG(LogTemp, Log, TEXT("[NexusBanSubsystem] Kicked player %s. Reason: %s"), *Target->GetName(), *Reason.ToString());
	return true;
}

bool UNexusBanSubsystem::UnbanPlayer(const FString& PlayerId)
{
	if (PlayerId.IsEmpty() || !IsValid(CachedBanSave))
		return false;

	if (CachedBanSave->BannedPlayers.Remove(PlayerId) > 0)
	{
		SaveBanList();
		UE_LOG(LogTemp, Log, TEXT("[NexusBanSubsystem] Unbanned player %s."), *PlayerId);
		OnPlayerUnbanned.Broadcast(PlayerId);
		return true;
	}

	return false;
}

TMap<FString, FNexusBanInfo> UNexusBanSubsystem::GetAllBans() const
{
	if (IsValid(CachedBanSave))
	{
		return CachedBanSave->BannedPlayers;
	}
	return {};
}
