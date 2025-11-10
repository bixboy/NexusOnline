#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "NexusGameInstance.generated.h"


UCLASS()
class NEXUSFRAMEWORK_API UNexusGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

private:
	void LogOnlineSubsystemStatus();
};
