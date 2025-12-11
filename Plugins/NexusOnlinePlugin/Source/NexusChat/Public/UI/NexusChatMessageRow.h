#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/NexusChatTypes.h"
#include "Components/RichTextBlock.h"
#include "NexusChatMessageRow.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChatPlayerClicked, const FString&, PlayerName);

/**
 * Widget representing a single chat message row.
 * Displays formatted text with clickable links for player names and URLs.
 * 
 * Use NexusRichTextBlock as MessageText for link support.
 */
UCLASS()
class NEXUSCHAT_API UNexusChatMessageRow : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The rich text widget displaying the message. Use NexusRichTextBlock for link support. */
	UPROPERTY(meta = (BindWidget), BlueprintReadWrite)
	URichTextBlock* MessageText;

	/** Maps chat channels to RichTextBlock style names (from your Text Style Set DataTable) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusChat")
	TMap<ENexusChatChannel, FName> ChannelStyles;

	/** Fired when a player name link is clicked */
	UPROPERTY(BlueprintAssignable, Category = "NexusChat|Events")
	FOnChatPlayerClicked OnPlayerClicked;

	virtual void NativeConstruct() override;
	
	/** Initialize the widget with a chat message. Override to customize display. */
	UFUNCTION(BlueprintNativeEvent, Category = "NexusChat")
	void InitWidget(const FNexusChatMessage& Message);
	virtual void InitWidget_Implementation(const FNexusChatMessage& Message);

protected:
	UFUNCTION()
	void HandlePlayerLinkClicked(const FString& PlayerName);
};
