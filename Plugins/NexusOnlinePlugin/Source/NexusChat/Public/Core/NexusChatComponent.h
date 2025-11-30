#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/NexusChatTypes.h"
#include "NexusChatInterface.h"
#include "NexusChatComponent.generated.h"


DECLARE_DELEGATE_OneParam(FChatCommandDelegate, const FString& /*Params*/);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCustomCommand, const FString&, Command, const FString&, Params);


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEXUSCHAT_API UNexusChatComponent : public UActorComponent
{
	GENERATED_BODY()

public: 
	UNexusChatComponent();

	UPROPERTY(BlueprintAssignable, Category = "NexusChat")
	FOnMessageReceived OnMessageReceived;

	UFUNCTION(BlueprintCallable, Category = "NexusChat")
	void SendChatMessage(const FString& Content, ENexusChatChannel Channel);

	UFUNCTION(BlueprintCallable, Category = "NexusChat")
	void SetTeamId(int32 NewTeamId);

	UFUNCTION(BlueprintPure, Category = "NexusChat")
	int32 GetTeamId() const { return TeamId; }

	// --- Custom Commands ---
	
	UPROPERTY(BlueprintAssignable, Category = "NexusChat|Events")
	FOnCustomCommand OnCustomCommand;

	void RegisterExternalCommand(const FString& Command, FChatCommandDelegate Callback);

	void UnregisterExternalCommand(const FString& Command);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendChatMessage(const FString& Content, ENexusChatChannel Channel);

	UFUNCTION(Client, Reliable)
	void Client_ReceiveChatMessage(const FNexusChatMessage& Message);

	virtual void RouteMessage(const FNexusChatMessage& Msg);

	void FilterProfanity(FString& Message);
	
	FString DecorateMessage(const FString& Message, ENexusChatChannel Channel);

	// --- Base Commands ---
	
	void RegisterCommands();
	bool ProcessSlashCommand(const FString& Content);
    
	void Cmd_Quit(const FString& Params);
	void Cmd_Whisper(const FString& Params);
	void Cmd_Team(const FString& Params);
	void Cmd_Reply(const FString& Params);

private:
	
	UPROPERTY()
	FDateTime LastMessageTime;

	UPROPERTY()
	FString LastWhisperSender;

	static const float SpamCooldown;

	TMap<FString, FChatCommandDelegate> CommandHandlers;

	UPROPERTY(Replicated)
	int32 TeamId = -1;
};
