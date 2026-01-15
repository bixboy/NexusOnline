#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Types/NexusChatTypes.h"
#include "NexusChatConfig.generated.h"


UCLASS(BlueprintType)
class NEXUSCHAT_API UNexusChatConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UNexusChatConfig();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NexusChat|General")
	float SpamCooldown = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NexusChat|Moderation")
	TArray<FString> BannedWords;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NexusChat|Channels")
	TMap<ENexusChatChannel, FText> ChannelPrefixes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NexusChat|Channels")
	TMap<ENexusChatChannel, FLinearColor> ChannelColors;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NexusChat|Channels")
	TMap<FName, FLinearColor> CustomChannelColors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusChat|Notifications")
	bool bEnableNotifications = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusChat|Notifications")
	USoundBase* NotificationSound;
};
