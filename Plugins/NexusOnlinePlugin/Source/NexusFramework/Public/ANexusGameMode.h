#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "ANexusGameMode.generated.h"


UCLASS()
class NEXUSFRAMEWORK_API AANexusGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
};
