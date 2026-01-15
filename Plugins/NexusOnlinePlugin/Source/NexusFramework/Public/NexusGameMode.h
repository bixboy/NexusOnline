#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "NexusGameMode.generated.h"

class APlayerController;


UCLASS()
class NEXUSFRAMEWORK_API ANexusGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ANexusGameMode();

	// ──────────────────────────────────────────────
	// Lifecycle
	// ──────────────────────────────────────────────
	virtual void BeginPlay() override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	// ──────────────────────────────────────────────
	// Admin Actions
	// ──────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Nexus|Admin")
	bool KickPlayer(APlayerController* Target, const FText& Reason);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Nexus|Admin")
	bool BanPlayer(APlayerController* Target, const FText& Reason);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Nexus|Admin")
	bool TempBanPlayer(APlayerController* Target, float DurationHours, const FText& Reason);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Nexus|Admin")
	bool UnbanPlayer(const FString& PlayerId);

private:
	// ──────────────────────────────────────────────
	// Host Registration Logic
	// ──────────────────────────────────────────────
	
	void TryRegisterHost();

	bool GetHostUniqueId(FUniqueNetIdRepl& OutId) const;

	FTimerHandle TimerHandle_RetryRegister;
	
	int32 RegisterHostRetries = 0;
	const int32 MAX_REGISTER_RETRIES = 10;
};