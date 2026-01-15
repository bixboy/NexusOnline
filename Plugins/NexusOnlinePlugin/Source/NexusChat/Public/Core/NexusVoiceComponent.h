#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/NexusChatTypes.h"
#include "NexusVoiceComponent.generated.h"

class UNexusChatComponent;
class UNexusVoiceConfig;


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEXUSCHAT_API UNexusVoiceComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UNexusVoiceComponent();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusVoice")
	TObjectPtr<UNexusVoiceConfig> VoiceConfig;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnChatReceived(const FNexusChatMessage& Message);

	void Speak(const FString& Text, int32 Rate, int32 Pitch);

private:
	UPROPERTY()
	UNexusChatComponent* ChatComponent;

	void* VoiceInterface = nullptr; 
};