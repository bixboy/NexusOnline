#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Types/NexusChatTypes.h"
#include "NexusChatSubsystem.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatLinkClicked, const FString&, LinkType, const FString&, LinkData);
DECLARE_DYNAMIC_DELEGATE_OneParam(FLinkTypeHandler, const FString&, LinkData);


UCLASS()
class NEXUSCHAT_API UNexusChatSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "NexusChat")
	void AddMessage(const FNexusChatMessage& Msg);

	// ====== History ======
	
	const TArray<FNexusChatMessage>& GetHistory() const { return GlobalChatHistory; }

	UFUNCTION(BlueprintPure, Category = "NexusChat")
	TArray<FNexusChatMessage> GetFilteredHistory() const;

	UFUNCTION(BlueprintCallable, Category = "NexusChat")
	void AddHistoryFilter(FName ChannelName);

	UFUNCTION(BlueprintCallable, Category = "NexusChat")
	void RemoveHistoryFilter(FName ChannelName);

	UFUNCTION(BlueprintCallable, Category = "NexusChat")
	void ClearHistoryFilters();

	// ====== Filters ======
	
	UFUNCTION(BlueprintPure, Category = "NexusChat")
	bool IsMessagePassesFilter(const FNexusChatMessage& Msg) const;

	UFUNCTION(BlueprintCallable, Category = "NexusChat")
	void SetWhitelistMode(bool bEnable);

	// ====== Hyper Link ======
	
	UFUNCTION(BlueprintCallable, Category = "NexusChat|Links")
	void RegisterLinkHandler(const FString& LinkType, FLinkTypeHandler Handler);

	UFUNCTION(BlueprintCallable, Category = "NexusChat|Links")
	void UnregisterLinkHandler(const FString& LinkType);

	UFUNCTION()
	void HandleLinkClicked(const FString& LinkType, const FString& LinkData);

	UFUNCTION(BlueprintPure, Category = "NexusChat|Links")
	bool HasLinkHandler(const FString& LinkType) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusChat|Links")
	bool bAutoOpenUrls = true;

	UPROPERTY(BlueprintAssignable, Category = "NexusChat|Links")
	FOnChatLinkClicked OnAnyLinkClicked;

private:
	void HandleUrlLink(const FString& Url);
	void HandlePlayerLink(const FString& PlayerName);

	UPROPERTY()
	TArray<FNexusChatMessage> GlobalChatHistory;

	UPROPERTY()
	TSet<FName> FilteredChannels;

	UPROPERTY()
	TMap<FString, FLinkTypeHandler> LinkHandlers;

	bool bWhitelistMode = false;
	int32 MaxHistorySize = 50;

	UPROPERTY()
	TSet<FString> ActiveWhisperTargets;

public:
	UFUNCTION(BlueprintCallable, Category = "NexusChat")
	void AddWhisperTarget(const FString& PlayerName);

	UFUNCTION(BlueprintCallable, Category = "NexusChat")
	void RemoveWhisperTarget(const FString& PlayerName);

	UFUNCTION(BlueprintPure, Category = "NexusChat")
	TSet<FString> GetActiveWhisperTargets() const { return ActiveWhisperTargets; }
};