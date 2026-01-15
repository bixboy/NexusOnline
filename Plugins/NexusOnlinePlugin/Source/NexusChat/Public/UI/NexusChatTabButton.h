#pragma once
#include "CoreMinimal.h"
#include "Components/Button.h"
#include "NexusChatTabButton.generated.h"

class UNexusChatWindow;


UCLASS()
class NEXUSCHAT_API UNexusChatTabButton : public UButton
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TWeakObjectPtr<UNexusChatWindow> MainWindow;

	FName ChannelName;
	
	bool bIsGeneral;

	void Init(UNexusChatWindow* Window, FName InChannelName, bool bInIsGeneral);

private:
	
	UFUNCTION()
	void HandleClick();
};
