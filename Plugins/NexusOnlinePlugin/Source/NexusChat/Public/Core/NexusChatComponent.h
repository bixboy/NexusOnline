#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/NexusChatTypes.h"
#include "NexusChatInterface.h"
#include "NexusChatComponent.generated.h"

class UNexusChatConfig;
class APlayerController;


DECLARE_DELEGATE_RetVal_TwoParams(TArray<APlayerController*>, FNexusChatRoutingDelegate, APlayerController* /*Sender*/, FName /*ChannelName*/);

DECLARE_DELEGATE_OneParam(FChatCommandDelegate, const FString& /*Params*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCustomCommand, const FString&, Command, const FString&, Params);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChatHistoryReceived, const TArray<FNexusChatMessage>&, History);


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEXUSCHAT_API UNexusChatComponent : public UActorComponent
{
    GENERATED_BODY()

public: 
    UNexusChatComponent();
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "NexusChat")
    TObjectPtr<UNexusChatConfig> ChatConfig;
    
    
    UPROPERTY(BlueprintAssignable, Category = "NexusChat|Events")
    FOnMessageReceived OnMessageReceived;

    UPROPERTY(BlueprintAssignable, Category = "NexusChat|Events")
    FOnChatHistoryReceived OnChatHistoryReceived;

    UPROPERTY(BlueprintAssignable, Category = "NexusChat|Events")
    FOnCustomCommand OnCustomCommand;
    
    FNexusChatRoutingDelegate OnRoutingQuery;

    // ─────────────────────────────────────────────────────────────────
    // API PUBLIQUE
    // ─────────────────────────────────────────────────────────────────
    UFUNCTION(BlueprintCallable, Category = "NexusChat")
    void SendChatMessage(const FString& Content, ENexusChatChannel Channel);

    UFUNCTION(BlueprintCallable, Category = "NexusChat")
    void SendChatMessageCustom(const FString& Content, FName ChannelName, ENexusChatChannel Routing = ENexusChatChannel::Global);

    UFUNCTION(BlueprintCallable, Category = "NexusChat")
    void SetTeamId(int32 NewTeamId);

    UFUNCTION(BlueprintCallable, Category = "NexusChat")
    void SetPartyId(int32 NewPartyId);

    UFUNCTION(BlueprintPure, Category = "NexusChat")
    int32 GetTeamId() const { return TeamId; }

    UFUNCTION(BlueprintPure, Category = "NexusChat")
    int32 GetPartyId() const { return PartyId; }

    UFUNCTION(BlueprintPure, Category = "NexusChat")
    const TArray<FNexusChatMessage>& GetClientChatHistory() const { return ClientChatHistory; }

    UFUNCTION(BlueprintCallable, Category = "NexusChat")
    void RegisterBlueprintCommand(FString CommandName);

protected:
    // ─────────────────────────────────────────────────────────────────
    // LIFECYCLE & RESEAU
    // ─────────────────────────────────────────────────────────────────
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_SendChatMessage(const FString& Content, ENexusChatChannel Channel, FName ChannelName = NAME_None);

    UFUNCTION(Client, Reliable)
    void Client_ReceiveChatMessage(const FNexusChatMessage& Message);

    UFUNCTION(Client, Reliable)
    void Client_ReceiveChatHistory(const TArray<FNexusChatMessage>& History);

    UFUNCTION(Server, Reliable)
    void Server_RequestChatHistory();

    // ─────────────────────────────────────────────────────────────────
    // LOGIQUE INTERNE
    // ─────────────────────────────────────────────────────────────────
    virtual void RouteMessage(const FNexusChatMessage& Msg);
    void FilterProfanity(FString& Message);
    
    FString DecorateMessage(const FString& Message, ENexusChatChannel Channel) const;

    // ─────────────────────────────────────────────────────────────────
    // ROUTING BLUEPRINT (Fallback)
    // ─────────────────────────────────────────────────────────────────

    UFUNCTION(BlueprintNativeEvent, Category = "NexusChat|Routing")
    TArray<APlayerController*> ResolveCustomRecipients(APlayerController* Sender, FName ChannelName);
    virtual TArray<APlayerController*> ResolveCustomRecipients_Implementation(APlayerController* Sender, FName ChannelName);

    // ─────────────────────────────────────────────────────────────────
    // COMMANDES
    // ─────────────────────────────────────────────────────────────────
    void RegisterCommands();
    bool ProcessSlashCommand(const FString& Content);
    void Cmd_Quit(const FString& Params);
    void Cmd_Whisper(const FString& Params);
    void Cmd_Team(const FString& Params);
    void Cmd_Reply(const FString& Params);

    void RegisterInternalCommand(const FString& Command, FChatCommandDelegate Callback);

private:
    UPROPERTY()
    FDateTime LastMessageTime;

    UPROPERTY()
    FString LastWhisperSender;

    UPROPERTY(Replicated)
    int32 TeamId = -1;

    UPROPERTY(Replicated)
    int32 PartyId = -1;

    TMap<FString, FChatCommandDelegate> CommandHandlers;

    TArray<FNexusChatMessage> ClientChatHistory;

    static const float DefaultSpamCooldown;
};