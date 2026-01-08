#pragma once
#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Types/NexusChatTypes.h"
#include "NexusChatTabButton.generated.h"

class UNexusChatWindow;


UCLASS()
class NEXUSCHAT_API UNexusChatTabButton : public UButton
{
	GENERATED_BODY()

public:
	// The channel name this button activates
	FName ChannelName; // Custom channel name
	
	// True if this is the "General" (all channels) tab
	bool bIsGeneral;
	
	// Weak reference to the main window to avoid circles
	UPROPERTY()
	TWeakObjectPtr<UNexusChatWindow> MainWindow;

	/** Initialize the button with its context */
	void Init(UNexusChatWindow* Window, FName InChannelName, bool bInIsGeneral);

private:
	UFUNCTION()
	void HandleClick();
};
