#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NexusLinkHelpers.generated.h"

/**
 * Blueprint function library for creating clickable links in chat messages.
 * 
 * Link syntax: <link type="TYPE" data="DATA">Display Text</>
 * 
 * Built-in types:
 * - "player": Player name links (triggers OnPlayerLinkClicked)
 * - "url": Web links (auto-opens browser if enabled)
 * 
 * Custom types: Register handlers via UNexusLinkSubsystem::RegisterLinkHandler()
 */
UCLASS()
class NEXUSCHAT_API UNexusLinkHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ────────────────────────────────────────────
	// GENERIC LINK CREATION
	// ────────────────────────────────────────────

	/**
	 * Create a generic clickable link.
	 * @param Type The link type (e.g., "player", "url", "item", "quest")
	 * @param Data The data associated with the link (e.g., player name, URL, item ID)
	 * @param DisplayText The text to display for the link
	 * @return Formatted link string: <link type="TYPE" data="DATA">DisplayText</>
	 */
	UFUNCTION(BlueprintPure, Category = "NexusChat|Links", meta = (DisplayName = "Make Link"))
	static FString MakeLink(const FString& Type, const FString& Data, const FString& DisplayText);

	// ────────────────────────────────────────────
	// CONVENIENCE HELPERS
	// ────────────────────────────────────────────

	/**
	 * Create a player name link.
	 * @param PlayerName The full player name
	 * @param MaxDisplayLength Maximum characters to display (0 = no limit)
	 * @return Formatted player link
	 */
	UFUNCTION(BlueprintPure, Category = "NexusChat|Links", meta = (DisplayName = "Make Player Link"))
	static FString MakePlayerLink(const FString& PlayerName, int32 MaxDisplayLength = 15);

	/**
	 * Create a URL link.
	 * @param Url The URL to link to
	 * @param DisplayText Optional display text (uses URL if empty)
	 * @return Formatted URL link
	 */
	UFUNCTION(BlueprintPure, Category = "NexusChat|Links", meta = (DisplayName = "Make URL Link"))
	static FString MakeUrlLink(const FString& Url, const FString& DisplayText = TEXT(""));

	/**
	 * Automatically detect and format URLs in text.
	 * Converts http:// and https:// URLs into clickable links.
	 * @param Text The text to scan for URLs
	 * @return Text with URLs converted to clickable links
	 */
	UFUNCTION(BlueprintPure, Category = "NexusChat|Links", meta = (DisplayName = "Auto Format URLs"))
	static FString AutoFormatUrls(const FString& Text);

	// ────────────────────────────────────────────
	// CUSTOM LINK HELPERS (for plugin users)
	// ────────────────────────────────────────────

	/**
	 * Create an item link (convenience for game items).
	 * @param ItemId The item identifier
	 * @param ItemName The display name of the item
	 * @return Formatted item link
	 */
	UFUNCTION(BlueprintPure, Category = "NexusChat|Links", meta = (DisplayName = "Make Item Link"))
	static FString MakeItemLink(const FString& ItemId, const FString& ItemName);

	/**
	 * Create a quest link (convenience for quests).
	 * @param QuestId The quest identifier
	 * @param QuestName The display name of the quest
	 * @return Formatted quest link
	 */
	UFUNCTION(BlueprintPure, Category = "NexusChat|Links", meta = (DisplayName = "Make Quest Link"))
	static FString MakeQuestLink(const FString& QuestId, const FString& QuestName);

	/**
	 * Create a location link (convenience for map locations).
	 * @param LocationId The location identifier
	 * @param LocationName The display name of the location
	 * @return Formatted location link
	 */
	UFUNCTION(BlueprintPure, Category = "NexusChat|Links", meta = (DisplayName = "Make Location Link"))
	static FString MakeLocationLink(const FString& LocationId, const FString& LocationName);
};
