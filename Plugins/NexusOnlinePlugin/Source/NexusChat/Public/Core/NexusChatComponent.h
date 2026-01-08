#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/NexusChatTypes.h"
#include "NexusChatInterface.h"
#include "NexusChatComponent.generated.h"


class APlayerController;


DECLARE_DELEGATE_OneParam(FChatCommandDelegate, const FString& /*Params*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCustomCommand, const FString&, Command, const FString&, Params);


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEXUSCHAT_API UNexusChatComponent : public UActorComponent
{
    GENERATED_BODY()

public: 
    UNexusChatComponent();

    // --- Events ---
    UPROPERTY(BlueprintAssignable, Category = "NexusChat|Events")
    FOnMessageReceived OnMessageReceived;

    UPROPERTY(BlueprintAssignable, Category = "NexusChat|Events")
    FOnCustomCommand OnCustomCommand;

    // --- Public API ---
    UFUNCTION(BlueprintCallable, Category = "NexusChat")
    void SendChatMessage(const FString& Content, ENexusChatChannel Channel);

    /** Send a message to a specific custom channel (or standard one with overridden name) */
    UFUNCTION(BlueprintCallable, Category = "NexusChat")
    void SendChatMessageCustom(const FString& Content, FName ChannelName, ENexusChatChannel Routing = ENexusChatChannel::Global);

    UFUNCTION(BlueprintCallable, Category = "NexusChat")
    void SetTeamId(int32 NewTeamId);

    UFUNCTION(BlueprintPure, Category = "NexusChat")
    int32 GetTeamId() const { return TeamId; }

    UFUNCTION(BlueprintPure, Category = "NexusChat")
    const TArray<FNexusChatMessage>& GetClientChatHistory() const { return ClientChatHistory; }

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NexusChat")
    void BroadcastGameLog(const FString& Content, FLinearColor LogColor = FLinearColor::White);

    void RegisterExternalCommand(const FString& Command, FChatCommandDelegate Callback);
    void UnregisterExternalCommand(const FString& Command);

protected:
    // --- Lifecycle ---
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    	// --- Network RPCs ---
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendChatMessage(const FString& Content, ENexusChatChannel Channel, FName ChannelName = NAME_None);

	UFUNCTION(Client, Reliable)
	void Client_ReceiveChatMessage(const FNexusChatMessage& Message);

    UFUNCTION(Client, Reliable)
    void Client_ReceiveChatMessages(const TArray<FNexusChatMessage>& Messages);

    UFUNCTION(Server, Reliable)
    void Server_RequestChatHistory();

    // --- Internal Logic ---
    virtual void RouteMessage(const FNexusChatMessage& Msg);
    void FilterProfanity(FString& Message);
    FString DecorateMessage(const FString& Message, ENexusChatChannel Channel) const;

    // --- Command System ---
    void RegisterCommands();
    bool ProcessSlashCommand(const FString& Content);

    // Built-in Commands
    void Cmd_Quit(const FString& Params);
    void Cmd_Whisper(const FString& Params);
    void Cmd_Team(const FString& Params);
    void Cmd_Reply(const FString& Params);

private:
    // --- State ---
    UPROPERTY()
    FDateTime LastMessageTime;

    UPROPERTY()
    FString LastWhisperSender;

    UPROPERTY(Replicated)
    int32 TeamId = -1;

    TMap<FString, FChatCommandDelegate> CommandHandlers;

    // --- Constants ---
    static const float SpamCooldown;

    UPROPERTY()
    TArray<FNexusChatMessage> ClientChatHistory;
};