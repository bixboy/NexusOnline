#pragma once

#include "CoreMinimal.h"
#include "Components/RichTextBlockDecorator.h"
#include "NexusHyperlinkDecorator.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerLinkClicked, const FString&, PlayerName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUrlLinkClicked, const FString&, Url);

/**
 * Custom decorator for handling clickable links in chat messages.
 * Supports player names (player:Name) and URLs (url:http://...)
 */
UCLASS()
class NEXUSCHAT_API UNexusHyperlinkDecorator : public URichTextBlockDecorator
{
	GENERATED_BODY()

public:
	UNexusHyperlinkDecorator(const FObjectInitializer& ObjectInitializer);

	virtual TSharedPtr<ITextDecorator> CreateDecorator(URichTextBlock* InOwner) override;

	// --- Events for customization ---
	
	/** Called when a player name link is clicked */
	UPROPERTY(BlueprintAssignable, Category = "NexusChat|Links")
	FOnPlayerLinkClicked OnPlayerClicked;

	/** Called when a URL link is clicked */
	UPROPERTY(BlueprintAssignable, Category = "NexusChat|Links")
	FOnUrlLinkClicked OnUrlClicked;

	/** Handle player link click - broadcasts the event */
	void HandlePlayerClick(const FString& PlayerName);

	/** Handle URL link click - opens browser and broadcasts event */
	void HandleUrlClick(const FString& Url);

	// --- Static Helpers ---

	/** Format a player name as a clickable link: <a id="player" href="PlayerName">DisplayName</a> */
	UFUNCTION(BlueprintCallable, Category = "NexusChat|Links")
	static FString MakePlayerLink(const FString& PlayerName, int32 MaxDisplayLength = 15);

	/** Format a URL as a clickable link: <a id="url" href="URL">DisplayText</a> */
	UFUNCTION(BlueprintCallable, Category = "NexusChat|Links")
	static FString MakeUrlLink(const FString& Url, const FString& DisplayText = TEXT(""));

	/** Detect URLs in text and automatically format them as clickable links */
	UFUNCTION(BlueprintCallable, Category = "NexusChat|Links")
	static FString AutoFormatUrls(const FString& Text);
};
