#pragma once
#include "CoreMinimal.h"
#include "Types/NexusChatTypes.h"
#include "NexusChatMessageObj.generated.h"


UCLASS(BlueprintType)
class NEXUSCHAT_API UNexusChatMessageObj : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "NexusChat")
	FNexusChatMessage Message;
};
