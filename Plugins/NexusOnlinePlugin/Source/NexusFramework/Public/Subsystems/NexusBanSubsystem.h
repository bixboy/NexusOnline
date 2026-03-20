#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NexusBanTypes.h"
#include "NexusBanSubsystem.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerBanned, const FString&, PlayerId, const FString&, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerUnbanned, const FString&, PlayerId);


/**
 * Centralized Ban Management Subsystem.
 * Caches the ban list in memory and provides O(1) ban checks.
 * Any GameMode can reference this subsystem without inheriting from ANexusGameMode.
 */
UCLASS()
class NEXUSFRAMEWORK_API UNexusBanSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ──────────────────────────────────────────────
	// Public API
	// ──────────────────────────────────────────────

	/** Check if a player is currently banned. Returns true if banned. */
	UFUNCTION(BlueprintCallable, Category="Nexus|Admin")
	bool IsBanned(const FString& PlayerId, FString& OutReason, FDateTime& OutExpiration) const;

	/** Permanent ban. Returns false if the player is already banned or the ID is invalid. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Nexus|Admin")
	bool BanPlayer(const FString& PlayerId, const FText& Reason);

	/** Temporary ban. Returns false if the player is already banned or the ID is invalid. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Nexus|Admin")
	bool TempBanPlayer(const FString& PlayerId, float DurationHours, const FText& Reason);

	/** Remove a ban. Returns false if the player wasn't banned. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Nexus|Admin")
	bool UnbanPlayer(const FString& PlayerId);

	/** Kicks a player from the server. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Nexus|Admin")
	bool KickPlayer(APlayerController* Target, const FText& Reason);

	/** Get all currently active bans. */
	UFUNCTION(BlueprintCallable, Category="Nexus|Admin")
	TMap<FString, FNexusBanInfo> GetAllBans() const;

	// ──────────────────────────────────────────────
	// Events
	// ──────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category="Nexus|Admin")
	FOnPlayerBanned OnPlayerBanned;

	UPROPERTY(BlueprintAssignable, Category="Nexus|Admin")
	FOnPlayerUnbanned OnPlayerUnbanned;

private:
	void LoadBanList();
	void SaveBanList();
	void CleanupExpiredBans();

	UPROPERTY(Transient)
	TObjectPtr<UNexusBanSave> CachedBanSave;

	static constexpr const TCHAR* BAN_SAVE_SLOT = TEXT("NexusBanList");
};
