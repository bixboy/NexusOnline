#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Types/NexusChatTypes.h"
#include "NexusChatSubsystem.generated.h"

/**
 * Subsystem to handle persistent chat history for the world.
 */
UCLASS()
class NEXUSCHAT_API UNexusChatSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void AddMessage(const FNexusChatMessage& Msg);

	const TArray<FNexusChatMessage>& GetHistory() const { return GlobalChatHistory; }

private:
	UPROPERTY()
	TArray<FNexusChatMessage> GlobalChatHistory;

	int32 MaxHistorySize = 50;
};
