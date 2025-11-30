#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/NexusChatTypes.h"
#include "Core/NexusChatComponent.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "UI/NexusChatMessageRow.h"
#include "NexusChatWindow.generated.h"

UCLASS()
class NEXUSCHAT_API UNexusChatWindow : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	UScrollBox* ChatScrollBox;

	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* ChatInput;

	UPROPERTY(EditDefaultsOnly, Category = "NexusChat")
	TSubclassOf<UNexusChatMessageRow> MessageRowClass;

	UPROPERTY(EditDefaultsOnly, Category = "NexusChat")
	int32 MaxChatLines = 100;

	UFUNCTION(BlueprintCallable, Category = "NexusChat")
	void ToggleChatFocus(bool bFocus);

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleMessageReceived(const FNexusChatMessage& Msg);

	UFUNCTION()
	void HandleTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

private:
	UPROPERTY()
	UNexusChatComponent* ChatComponent;
};
