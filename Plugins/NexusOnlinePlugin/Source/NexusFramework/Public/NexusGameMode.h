#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Interfaces/INexusSessionHandler.h"
#include "NexusGameMode.generated.h"

class APlayerController;


UCLASS()
class NEXUSFRAMEWORK_API ANexusGameMode : public AGameModeBase, public INexusSessionHandler
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
};