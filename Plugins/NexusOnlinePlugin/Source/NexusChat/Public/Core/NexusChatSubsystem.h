#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Types/NexusChatTypes.h"
#include "NexusChatSubsystem.generated.h"

/** Delegate for when any link is clicked */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatLinkClicked, const FString&, LinkType, const FString&, LinkData);

/** Delegate for specific link type handlers */
DECLARE_DYNAMIC_DELEGATE_OneParam(FLinkTypeHandler, const FString&, LinkData);

/**
 * Central subsystem for chat functionality.
 * Handles: message history, history filters, and clickable link events.
 * 
 * Access via: GetWorld()->GetSubsystem<UNexusChatSubsystem>()
 */
UCLASS()
class NEXUSCHAT_API UNexusChatSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ────────────────────────────────────────────
	// HISTORY
	// ────────────────────────────────────────────

	void AddMessage(const FNexusChatMessage& Msg);

	const TArray<FNexusChatMessage>& GetHistory() const { return GlobalChatHistory; }

	// --- History Filters ---
	UFUNCTION(BlueprintCallable, Category = "NexusChat|History")
	void AddHistoryFilter(ENexusChatChannel Channel);

	UFUNCTION(BlueprintCallable, Category = "NexusChat|History")
	void RemoveHistoryFilter(ENexusChatChannel Channel);

	UFUNCTION(BlueprintCallable, Category = "NexusChat|History")
	void ClearHistoryFilters();

	UFUNCTION(BlueprintPure, Category = "NexusChat|History")
	bool IsChannelFiltered(ENexusChatChannel Channel) const;

	TArray<FNexusChatMessage> GetFilteredHistory() const;

	// ────────────────────────────────────────────
	// LINK EVENTS
	// ────────────────────────────────────────────

	/** Called when ANY link is clicked. Useful for logging or global handling. */
	UPROPERTY(BlueprintAssignable, Category = "NexusChat|Links")
	FOnChatLinkClicked OnAnyLinkClicked;

	/** Called specifically when a player link is clicked (type="player") */
	UPROPERTY(BlueprintAssignable, Category = "NexusChat|Links")
	FOnChatLinkClicked OnPlayerLinkClicked;

	/** Called specifically when a URL link is clicked (type="url") */
	UPROPERTY(BlueprintAssignable, Category = "NexusChat|Links")
	FOnChatLinkClicked OnUrlLinkClicked;

	// ────────────────────────────────────────────
	// LINK HANDLER REGISTRATION
	// ────────────────────────────────────────────

	/** Register a custom handler for a specific link type */
	UFUNCTION(BlueprintCallable, Category = "NexusChat|Links")
	void RegisterLinkHandler(const FString& LinkType, FLinkTypeHandler Handler);

	/** Unregister a handler for a specific link type */
	UFUNCTION(BlueprintCallable, Category = "NexusChat|Links")
	void UnregisterLinkHandler(const FString& LinkType);

	/** Check if a handler is registered for a link type */
	UFUNCTION(BlueprintPure, Category = "NexusChat|Links")
	bool HasLinkHandler(const FString& LinkType) const;

	// ────────────────────────────────────────────
	// LINK SETTINGS
	// ────────────────────────────────────────────

	/** If true, URL links will automatically open in the default browser */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusChat|Links")
	bool bAutoOpenUrls = true;

	// ────────────────────────────────────────────
	// INTERNAL (called by decorator)
	// ────────────────────────────────────────────

	/** Called internally when a link is clicked. Do not call directly. */
	void HandleLinkClicked(const FString& LinkType, const FString& LinkData);

private:
	// --- History ---
	UPROPERTY()
	TArray<FNexusChatMessage> GlobalChatHistory;

	UPROPERTY()
	TSet<ENexusChatChannel> FilteredChannels;

	int32 MaxHistorySize = 50;

	// --- Links ---
	TMap<FString, FLinkTypeHandler> LinkHandlers;

	void HandlePlayerLink(const FString& PlayerName);
	void HandleUrlLink(const FString& Url);
};
