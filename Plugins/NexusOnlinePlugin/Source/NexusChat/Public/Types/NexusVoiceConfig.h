#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Types/NexusChatTypes.h"
#include "NexusVoiceConfig.generated.h"


UCLASS(BlueprintType, Const)
class NEXUSCHAT_API UNexusVoiceConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Voice Settings")
	int32 BaseRate = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Voice Settings")
	int32 BasePitch = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Voice Settings")
	int32 Volume = 100;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Voice Behavior")
	bool bRandomizeVoicePerPlayer = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Voice Behavior")
	bool bHearOwnVoice = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Voice Filters")
	TSet<ENexusChatChannel> AllowedChannels;
};