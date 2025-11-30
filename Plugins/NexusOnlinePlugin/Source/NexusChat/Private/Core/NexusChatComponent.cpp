#include "Core/NexusChatComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"

const float UNexusChatComponent::SpamCooldown = 0.5f;

UNexusChatComponent::UNexusChatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UNexusChatComponent::BeginPlay()
{
    Super::BeginPlay();
    RegisterCommands();
}

void UNexusChatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UNexusChatComponent, TeamId);
}

void UNexusChatComponent::SetTeamId(int32 NewTeamId)
{
	TeamId = NewTeamId;
}

// ──────────────────────────────────────────────
// SYSTEME DE COMMANDES
// ──────────────────────────────────────────────
void UNexusChatComponent::RegisterCommands()
{
    CommandHandlers.Add(TEXT("/quit"), FChatCommandDelegate::CreateUObject(this, &UNexusChatComponent::Cmd_Quit));
    CommandHandlers.Add(TEXT("/w"),    FChatCommandDelegate::CreateUObject(this, &UNexusChatComponent::Cmd_Whisper));
    CommandHandlers.Add(TEXT("/team"), FChatCommandDelegate::CreateUObject(this, &UNexusChatComponent::Cmd_Team));
    CommandHandlers.Add(TEXT("/r"),    FChatCommandDelegate::CreateUObject(this, &UNexusChatComponent::Cmd_Reply));
}

// --- Custom commands ---

void UNexusChatComponent::RegisterExternalCommand(const FString& Command, FChatCommandDelegate Callback)
{
	if (Command.IsEmpty())
		return;
    
	FString CleanCmd = Command.StartsWith("/") ? Command : "/" + Command;

	if (CommandHandlers.Contains(CleanCmd))
	{
		UE_LOG(LogTemp, Warning, TEXT("[NexusChat] Overriding existing command: %s"), *CleanCmd);
	}

	CommandHandlers.Add(CleanCmd, Callback);
}

void UNexusChatComponent::UnregisterExternalCommand(const FString& Command)
{
	FString CleanCmd = Command.StartsWith("/") ? Command : "/" + Command;
	CommandHandlers.Remove(CleanCmd);
}

// --- Base commands ---

bool UNexusChatComponent::ProcessSlashCommand(const FString& Content)
{
	if (!Content.StartsWith("/")) return false;

	FString Command;
	FString Params;
    
	if (!Content.Split(" ", &Command, &Params))
	{
		Command = Content;
	}
	
	if (FChatCommandDelegate* Handler = CommandHandlers.Find(Command))
	{
		Handler->ExecuteIfBound(Params);
		return true;
	}
	
	if (OnCustomCommand.IsBound())
	{
		OnCustomCommand.Broadcast(Command, Params);
		return true;
	}

	return false;
}

// --- Implémentation des commandes ---

void UNexusChatComponent::Cmd_Quit(const FString& Params)
{
    if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
    {
        PC->ConsoleCommand("quit");
    }
}

void UNexusChatComponent::Cmd_Whisper(const FString& Params)
{
    FString TargetName;
    FString MessageContent;
    
    if (Params.Split(" ", &TargetName, &MessageContent))
    {
        FNexusChatMessage Msg;
        Msg.Channel        = ENexusChatChannel::Whisper;
    	Msg.Timestamp      = FDateTime::UtcNow();
        Msg.TargetName     = TargetName;
        Msg.MessageContent = MessageContent;

        if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
        {
            if (PC->PlayerState)
            	Msg.SenderName = PC->PlayerState->GetPlayerName();
        }

        FilterProfanity(Msg.MessageContent);
        Msg.MessageContent = DecorateMessage(Msg.MessageContent, Msg.Channel);
        
        RouteMessage(Msg);
    }
}

void UNexusChatComponent::Cmd_Team(const FString& Params)
{
    FNexusChatMessage Msg;
    Msg.Channel = ENexusChatChannel::Team;
    Msg.MessageContent = Params;
    
    FDateTime CurrentTime = FDateTime::UtcNow();
    Msg.Timestamp = CurrentTime;
    
    if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
    {
        if (PC->PlayerState)
        {
            Msg.SenderName = PC->PlayerState->GetPlayerName();
        	Msg.SenderTeamId = TeamId;
        }
    }
    
    FilterProfanity(Msg.MessageContent);
    Msg.MessageContent = DecorateMessage(Msg.MessageContent, Msg.Channel);
    
    RouteMessage(Msg);
}

void UNexusChatComponent::Cmd_Reply(const FString& Params)
{
    if (LastWhisperSender.IsEmpty())
    {
        FNexusChatMessage SysMsg;
        SysMsg.Channel = ENexusChatChannel::System;
        SysMsg.MessageContent = "No one to reply to.";
    	
        Client_ReceiveChatMessage(SysMsg);
        return;
    }

    FString FullParams = FString::Printf(TEXT("%s %s"), *LastWhisperSender, *Params);
    Cmd_Whisper(FullParams);
}


// ──────────────────────────────────────────────
// FONCTIONS RESEAU
// ──────────────────────────────────────────────
void UNexusChatComponent::SendChatMessage(const FString& Content, ENexusChatChannel Channel)
{
    if (Content.IsEmpty())
    	return;
	
    Server_SendChatMessage(Content, Channel);
}

bool UNexusChatComponent::Server_SendChatMessage_Validate(const FString& Content, ENexusChatChannel Channel)
{
    return Content.Len() <= 512;
}

void UNexusChatComponent::Server_SendChatMessage_Implementation(const FString& Content, ENexusChatChannel Channel)
{
    FDateTime CurrentTime = FDateTime::UtcNow();
    if ((CurrentTime - LastMessageTime).GetTotalSeconds() < SpamCooldown)
    	return;
	
    LastMessageTime = CurrentTime;

	// Commande
    if (ProcessSlashCommand(Content))
    	return;

	// Message
    FString FinalContent = Content;
    FilterProfanity(FinalContent);
    FinalContent = DecorateMessage(FinalContent, Channel);

    FNexusChatMessage Msg;
    Msg.MessageContent = FinalContent;
    Msg.Channel = Channel;
    Msg.Timestamp = CurrentTime;
    
    if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
    {
        if (PC->PlayerState)
        {
            Msg.SenderName = PC->PlayerState->GetPlayerName();
        	Msg.SenderTeamId = TeamId;
        }
    }

    RouteMessage(Msg);
}

void UNexusChatComponent::Client_ReceiveChatMessage_Implementation(const FNexusChatMessage& Message)
{
    if (Message.Channel == ENexusChatChannel::Whisper)
    {
        LastWhisperSender = Message.SenderName;
    }
    OnMessageReceived.Broadcast(Message);
}

void UNexusChatComponent::RouteMessage(const FNexusChatMessage& Msg)
{
    UWorld* World = GetWorld();
    if (!World)
    	return;

    AGameStateBase* GameState = World->GetGameState();
    if (!GameState)
    	return;

    for (APlayerState* PS : GameState->PlayerArray)
    {
        if (!PS)
        	continue;

        APlayerController* PC = Cast<APlayerController>(PS->GetOwner());
        if (!PC)
        	continue;

        if (UNexusChatComponent* ChatComp = PC->FindComponentByClass<UNexusChatComponent>())
        {
            bool bShouldSend = false;

            switch (Msg.Channel)
            {
            case ENexusChatChannel::Global:
            	bShouldSend = true;
            	break;
            	
            case ENexusChatChannel::System:
                bShouldSend = true;
                break;
            	
            case ENexusChatChannel::Team:
            	if (ChatComp->GetTeamId() == Msg.SenderTeamId)
            	{
            		bShouldSend = true;
            	}
                break;
            	
            case ENexusChatChannel::Party:
                bShouldSend = true; // Placeholder
                break;
            	
            case ENexusChatChannel::Whisper:
                if (PS->GetPlayerName() == Msg.TargetName || PS->GetPlayerName() == Msg.SenderName)
					bShouldSend = true;
            	
                break;
            }

            if (bShouldSend)
            {
                ChatComp->Client_ReceiveChatMessage(Msg);
            }
        }
    }
}

void UNexusChatComponent::FilterProfanity(FString& Message)
{
    const FString BadWord = "badword";
    if (Message.Contains(BadWord))
    {
        Message = Message.Replace(*BadWord, TEXT("****"));
    }
}

FString UNexusChatComponent::DecorateMessage(const FString& Message, ENexusChatChannel Channel)
{
    FString Prefix;

    switch (Channel)
    {
    case ENexusChatChannel::Global:
    	// No prefix for global
    	break;
    	
    case ENexusChatChannel::Team:
    	Prefix = "[TEAM] ";
    	break;
    	
    case ENexusChatChannel::Party:
    	Prefix = "[PARTY] ";
    	break;
    	
    case ENexusChatChannel::Whisper:
    	Prefix = "[WHISPER] ";
    	break;
    	
    case ENexusChatChannel::System:
    	Prefix = "[SYSTEM] ";
    	break;
    }

    return Prefix + Message;
}
