#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Types/NexusChatTypes.h"
#include "NexusChatInterface.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMessageReceived, const FNexusChatMessage&, Message);

UINTERFACE(MinimalAPI)
class UNexusChatInterface : public UInterface
{
	GENERATED_BODY()
};

class NEXUSCHAT_API INexusChatInterface
{
	GENERATED_BODY()

public:
	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
};
