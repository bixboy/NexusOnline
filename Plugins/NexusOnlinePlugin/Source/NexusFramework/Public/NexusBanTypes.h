#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "NexusBanTypes.generated.h"

USTRUCT(BlueprintType)
struct FNexusBanInfo
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Nexus|Admin")
	FString PlayerId;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Nexus|Admin")
	FString Reason;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Nexus|Admin")
	FDateTime BanTimestamp;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Nexus|Admin")
	FDateTime ExpirationTimestamp;
};

UCLASS()
class NEXUSFRAMEWORK_API UNexusBanSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Nexus|Admin")
	TMap<FString, FNexusBanInfo> BannedPlayers;
};
