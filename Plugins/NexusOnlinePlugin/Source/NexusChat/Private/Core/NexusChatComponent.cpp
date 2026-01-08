#include "Core/NexusChatComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "Types/NexusChatTypes.h"
#include "Core/NexusChatSubsystem.h"


const float UNexusChatComponent::SpamCooldown = 0.5f;

// ──────────────────────────────────────────────
// LIFECYCLE
// ──────────────────────────────────────────────

UNexusChatComponent::UNexusChatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UNexusChatComponent::BeginPlay()
{
    Super::BeginPlay();
    RegisterCommands();

    if (!GetOwner()->HasAuthority())
    {
        Server_RequestChatHistory();
    }
}

void UNexusChatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UNexusChatComponent, TeamId);
}

// ──────────────────────────────────────────────
// PUBLIC API
// ──────────────────────────────────────────────

void UNexusChatComponent::SendChatMessage(const FString& Content, ENexusChatChannel Channel)
{
	SendChatMessageCustom(Content, NAME_None, Channel);
}

void UNexusChatComponent::SendChatMessageCustom(const FString& Content, FName ChannelName, ENexusChatChannel Routing)
{
	if (Content.IsEmpty())
		return;
	
	Server_SendChatMessage(Content, Routing, ChannelName);
}

void UNexusChatComponent::SetTeamId(int32 NewTeamId)
{
    TeamId = NewTeamId;
}

void UNexusChatComponent::BroadcastGameLog(const FString& Content, FLinearColor LogColor)
{
    if (!GetOwner()->HasAuthority())
        return;

    if (Content.IsEmpty())
        return;
    
    FString FinalContent = DecorateMessage(Content, ENexusChatChannel::GameLog);

    FNexusChatMessage Msg;
    Msg.SenderName = TEXT("GAME");
    Msg.Channel = ENexusChatChannel::GameLog;
    Msg.Timestamp = FDateTime::UtcNow();
    Msg.MessageContent = FinalContent;

    RouteMessage(Msg);
}

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

// ──────────────────────────────────────────────
// NETWORK & LOGIC
// ──────────────────────────────────────────────

bool UNexusChatComponent::Server_SendChatMessage_Validate(const FString& Content, ENexusChatChannel Channel, FName ChannelName)
{
	return Content.Len() <= 512;
}

void UNexusChatComponent::Server_SendChatMessage_Implementation(const FString& Content, ENexusChatChannel Channel, FName ChannelName)
{
	const FDateTime CurrentTime = FDateTime::UtcNow();

	if ((CurrentTime - LastMessageTime).GetTotalSeconds() < SpamCooldown)
		return;
	
	LastMessageTime = CurrentTime;

	if (ProcessSlashCommand(Content))
		return;

	FString FinalContent = Content;
	FilterProfanity(FinalContent);
	
	FName EffectiveChannelName = ChannelName;
	if (EffectiveChannelName.IsNone())
	{
		EffectiveChannelName = FName(*UEnum::GetDisplayValueAsText(Channel).ToString());
	}

	FinalContent = DecorateMessage(FinalContent, Channel); 

	FNexusChatMessage Msg;
	Msg.MessageContent = FinalContent;
	Msg.Channel = Channel;
	Msg.ChannelName = EffectiveChannelName;
	Msg.Timestamp = CurrentTime;
	
	if (const APlayerController* PC = Cast<APlayerController>(GetOwner()))
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
    
    ClientChatHistory.Add(Message);
    if (ClientChatHistory.Num() > 100)
    {
        ClientChatHistory.RemoveAt(0);
    }
    
    OnMessageReceived.Broadcast(Message);
}

void UNexusChatComponent::Client_ReceiveChatMessages_Implementation(const TArray<FNexusChatMessage>& Messages)
{
    for (const FNexusChatMessage& Message : Messages)
    {
        if (Message.Channel == ENexusChatChannel::Whisper)
        {
            LastWhisperSender = Message.SenderName;
        }
    
        ClientChatHistory.Add(Message);
        if (ClientChatHistory.Num() > 100)
        {
            ClientChatHistory.RemoveAt(0);
        }
    
        OnMessageReceived.Broadcast(Message);   
    }
}

void UNexusChatComponent::Server_RequestChatHistory_Implementation()
{
    if (UWorld* World = GetWorld())
    {
        if (UNexusChatSubsystem* ChatSubsystem = World->GetSubsystem<UNexusChatSubsystem>())
        {
            Client_ReceiveChatMessages(ChatSubsystem->GetFilteredHistory());
        }
    }
}


void UNexusChatComponent::RouteMessage(const FNexusChatMessage& Msg)
{
    const UWorld* World = GetWorld();
    if (!World)
        return;

    const AGameStateBase* GameState = World->GetGameState();
    if (!GameState)
        return;

    for (APlayerState* PS : GameState->PlayerArray)
    {
        if (!PS)
            continue;

        const APlayerController* PC = Cast<APlayerController>(PS->GetOwner());
        if (!PC)
            continue;

        if (UNexusChatComponent* ChatComp = PC->FindComponentByClass<UNexusChatComponent>())
        {
            bool bShouldSend = false;

            switch (Msg.Channel)
            {
                case ENexusChatChannel::Global:
                case ENexusChatChannel::System:
                case ENexusChatChannel::GameLog:
                    bShouldSend = true;
                    break;
                
                case ENexusChatChannel::Team:
                    if (ChatComp->GetTeamId() == Msg.SenderTeamId)
                    {
                        bShouldSend = true;
                    }
                    break;
                
                case ENexusChatChannel::Party:
                    bShouldSend = true;
                    break;
                
                case ENexusChatChannel::Whisper:
                    if (PS->GetPlayerName() == Msg.TargetName || PS->GetPlayerName() == Msg.SenderName)
                    {
                        bShouldSend = true;
                    }
                    break;
            }

            if (bShouldSend)
            {
                ChatComp->Client_ReceiveChatMessage(Msg);
            }
        }
    }

    if (Msg.Channel == ENexusChatChannel::Global || Msg.Channel == ENexusChatChannel::GameLog)
    {
        if (UNexusChatSubsystem* ChatSubsystem = World->GetSubsystem<UNexusChatSubsystem>())
        {
            ChatSubsystem->AddMessage(Msg);
        }
    }
}

// ──────────────────────────────────────────────
// COMMAND SYSTEM
// ──────────────────────────────────────────────

void UNexusChatComponent::RegisterCommands()
{
    CommandHandlers.Add(TEXT("/quit"), FChatCommandDelegate::CreateUObject(this, &UNexusChatComponent::Cmd_Quit));
    CommandHandlers.Add(TEXT("/w"),    FChatCommandDelegate::CreateUObject(this, &UNexusChatComponent::Cmd_Whisper));
    CommandHandlers.Add(TEXT("/team"), FChatCommandDelegate::CreateUObject(this, &UNexusChatComponent::Cmd_Team));
    CommandHandlers.Add(TEXT("/r"),    FChatCommandDelegate::CreateUObject(this, &UNexusChatComponent::Cmd_Reply));
}

bool UNexusChatComponent::ProcessSlashCommand(const FString& Content)
{
    if (!Content.StartsWith("/"))
        return false;

    FString Command;
    FString Params;
    
    if (!Content.Split(" ", &Command, &Params))
    {
        Command = Content;
    }
    
    if (const FChatCommandDelegate* Handler = CommandHandlers.Find(Command))
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
        Msg.Channel = ENexusChatChannel::Whisper;
        Msg.Timestamp = FDateTime::UtcNow();
        Msg.TargetName = TargetName;
        
        FilterProfanity(MessageContent);
        Msg.MessageContent = DecorateMessage(MessageContent, Msg.Channel);

        if (const APlayerController* PC = Cast<APlayerController>(GetOwner()))
        {
            if (PC->PlayerState)
                Msg.SenderName = PC->PlayerState->GetPlayerName();
        }
        
        RouteMessage(Msg);
    }
}

void UNexusChatComponent::Cmd_Team(const FString& Params)
{
    FNexusChatMessage Msg;
    Msg.Channel = ENexusChatChannel::Team;
    
    FString Content = Params;
    FilterProfanity(Content);
    Msg.MessageContent = DecorateMessage(Content, Msg.Channel);
    
    Msg.Timestamp = FDateTime::UtcNow();
    
    if (const APlayerController* PC = Cast<APlayerController>(GetOwner()))
    {
        if (PC->PlayerState)
        {
            Msg.SenderName = PC->PlayerState->GetPlayerName();
            Msg.SenderTeamId = TeamId;
        }
    }
    
    RouteMessage(Msg);
}

void UNexusChatComponent::Cmd_Reply(const FString& Params)
{
    if (LastWhisperSender.IsEmpty())
    {
        FNexusChatMessage SysMsg;
        SysMsg.Channel = ENexusChatChannel::System;
        SysMsg.MessageContent = DecorateMessage("No one to reply to.", ENexusChatChannel::System);
        
        Client_ReceiveChatMessage(SysMsg);
        return;
    }

    const FString FullParams = FString::Printf(TEXT("%s %s"), *LastWhisperSender, *Params);
    Cmd_Whisper(FullParams);
}

// ──────────────────────────────────────────────
// UTILS
// ──────────────────────────────────────────────

void UNexusChatComponent::FilterProfanity(FString& Message)
{
    // 1. Sanitization
    Message = Message.Replace(TEXT("<"), TEXT("&lt;"));
    Message = Message.Replace(TEXT(">"), TEXT("&gt;"));

    // 2. Profanity
    const FString BadWord = "badword";
    if (Message.Contains(BadWord))
    {
        Message = Message.Replace(*BadWord, TEXT("****"));
    }
}

FString UNexusChatComponent::DecorateMessage(const FString& Message, ENexusChatChannel Channel) const
{
    FString Prefix;

    switch (Channel)
    {
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

        case ENexusChatChannel::GameLog:
            Prefix = "[GameLog] ";
            break;
        
        case ENexusChatChannel::Global: default:
            break;
    }

    return Prefix + Message;
}