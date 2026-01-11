#include "Core/NexusChatComponent.h"
#include "Core/NexusChatConfig.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "Types/NexusChatTypes.h"
#include "Core/NexusChatSubsystem.h"

const float UNexusChatComponent::DefaultSpamCooldown = 0.5f;

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
    DOREPLIFETIME(UNexusChatComponent, PartyId);
    DOREPLIFETIME(UNexusChatComponent, ChatConfig);
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

    if (Content.StartsWith(TEXT("/")))
    {
        if (ProcessSlashCommand(Content))
        {
            return;
        }
    }

    const FDateTime Now = FDateTime::Now();
    float Cooldown = ChatConfig ? ChatConfig->SpamCooldown : DefaultSpamCooldown;

    if ((Now - LastMessageTime).GetTotalSeconds() < Cooldown)
    {
        return;
    }

    LastMessageTime = Now;
    Server_SendChatMessage(Content, Routing, ChannelName);
}

void UNexusChatComponent::SetTeamId(int32 NewTeamId)
{
    if (GetOwner()->HasAuthority())
    {
        TeamId = NewTeamId;
    }
}

void UNexusChatComponent::SetPartyId(int32 NewPartyId)
{
    if (GetOwner()->HasAuthority())
    {
        PartyId = NewPartyId;
    }
}

void UNexusChatComponent::RegisterBlueprintCommand(FString CommandName)
{
    if (CommandName.IsEmpty())
        return;

    FString CleanCmd = CommandName.StartsWith(TEXT("/")) ? CommandName : TEXT("/") + CommandName;
    RegisterInternalCommand(CleanCmd, FChatCommandDelegate::CreateWeakLambda(this, [this, CleanCmd](const FString& Params)
    {
        OnCustomCommand.Broadcast(CleanCmd, Params);
    }));
    
    UE_LOG(LogTemp, Log, TEXT("[NexusChat] Registered BP command: %s"), *CleanCmd);
}

// ──────────────────────────────────────────────
// RESEAU (SERVER)
// ──────────────────────────────────────────────

bool UNexusChatComponent::Server_SendChatMessage_Validate(const FString& Content, ENexusChatChannel Channel, FName ChannelName)
{
    return Content.Len() < 512;
}

void UNexusChatComponent::Server_SendChatMessage_Implementation(const FString& Content, ENexusChatChannel Channel, FName ChannelName)
{
    APlayerController* PC = Cast<APlayerController>(GetOwner());
    if (!PC || !PC->PlayerState)
        return;

    FString ProcessedContent = Content;
    
    FilterProfanity(ProcessedContent);

    FNexusChatMessage Msg;
    Msg.SenderName = PC->PlayerState->GetPlayerName();
    Msg.MessageContent = ProcessedContent;
    Msg.Channel = Channel;
    Msg.ChannelName = ChannelName;
    Msg.SenderTeamId = TeamId;
    Msg.SenderPartyId = PartyId;
    Msg.Timestamp = FDateTime::Now();
    Msg.TargetName = ChannelName.IsNone() ? "" : ChannelName.ToString();

    if (UWorld* World = GetWorld())
    {
        if (UNexusChatSubsystem* ChatSubsystem = World->GetSubsystem<UNexusChatSubsystem>())
        {
            ChatSubsystem->AddMessage(Msg);
        }
    }

    RouteMessage(Msg);
}

void UNexusChatComponent::RouteMessage(const FNexusChatMessage& Msg)
{
    APlayerController* SenderPC = Cast<APlayerController>(GetOwner());
    UWorld* World = GetWorld();
    if (!World || !SenderPC)
        return;

    TArray<APlayerController*> Recipients;
    bool bHandledCustom = false;

    // ─────────────────────────────────────────────────────────────────
    // A. ROUTING HYBRIDE
    // ─────────────────────────────────────────────────────────────────
    if (Msg.Channel != ENexusChatChannel::Global && Msg.Channel != ENexusChatChannel::System && Msg.Channel != ENexusChatChannel::GameLog)
    {
        if (OnRoutingQuery.IsBound())
        {
            Recipients = OnRoutingQuery.Execute(SenderPC, FName(*Msg.TargetName));
            bHandledCustom = true;
        }
        else 
        {
            TArray<APlayerController*> BPRecipients = ResolveCustomRecipients(SenderPC, FName(*Msg.TargetName));
            if (!BPRecipients.IsEmpty())
            {
                Recipients = BPRecipients;
                bHandledCustom = true;
            }
        }
    }

    // ─────────────────────────────────────────────────────────────────
    // B. ROUTING STANDARD (Fallback)
    // ─────────────────────────────────────────────────────────────────
    if (!bHandledCustom)
    {
        AGameStateBase* GS = World->GetGameState();
        if (!GS)
            return;

        for (APlayerState* PS : GS->PlayerArray)
        {
            if (!PS)
                continue;
            
            APlayerController* TargetPC = Cast<APlayerController>(PS->GetOwner());
            if (!TargetPC)
                continue;

            bool bShouldReceive = false;

            switch (Msg.Channel)
            {
                case ENexusChatChannel::Global:
                case ENexusChatChannel::System:
                case ENexusChatChannel::GameLog:
                case ENexusChatChannel::Custom:
                    bShouldReceive = true; 
                    break;

                case ENexusChatChannel::Team:
                {
                    UNexusChatComponent* TargetComp = TargetPC->FindComponentByClass<UNexusChatComponent>();
                    if (TargetComp && TargetComp->GetTeamId() == Msg.SenderTeamId)
                    {
                        bShouldReceive = true;
                    }
                    break;
                }

                case ENexusChatChannel::Party:
                {
                    UNexusChatComponent* TargetComp = TargetPC->FindComponentByClass<UNexusChatComponent>();
                    if (TargetComp && TargetComp->GetPartyId() == Msg.SenderPartyId)
                    {
                        bShouldReceive = true;
                    }
                    break;
                }

                case ENexusChatChannel::Whisper:
                {
                    // For Whisper, target is specified in Msg.ChannelName
                    // Also sender should see their own sent whispers
                    FString TargetName = Msg.ChannelName.ToString();
                    FString TargetPlayerName = TargetPC->PlayerState->GetPlayerName();
                    
                    if (TargetPlayerName == TargetName || TargetPC == SenderPC)
                    {
                        bShouldReceive = true;
                    }
                    break;
                }
                
                default: break;
            }

            if (bShouldReceive)
            {
                Recipients.AddUnique(TargetPC);
            }
        }
    }

    // ─────────────────────────────────────────────────────────────────
    // C. ENVOI FINAL
    // ─────────────────────────────────────────────────────────────────
    for (APlayerController* Target : Recipients)
    {
        if (UNexusChatComponent* TargetComp = Target->FindComponentByClass<UNexusChatComponent>())
        {
            TargetComp->Client_ReceiveChatMessage(Msg);
        }
    }
}

// ──────────────────────────────────────────────
// RESEAU (CLIENT)
// ──────────────────────────────────────────────

void UNexusChatComponent::Client_ReceiveChatMessage_Implementation(const FNexusChatMessage& Message)
{
    if (Message.Channel == ENexusChatChannel::Whisper)
    {
        LastWhisperSender = Message.SenderName;
    }
    
    ClientChatHistory.Add(Message);

    OnMessageReceived.Broadcast(Message);
}

void UNexusChatComponent::Server_RequestChatHistory_Implementation()
{
    if (UWorld* World = GetWorld())
    {
        if (UNexusChatSubsystem* ChatSubsystem = World->GetSubsystem<UNexusChatSubsystem>())
        {
            const TArray<FNexusChatMessage>& History = ChatSubsystem->GetHistory();
            if (History.Num() > 0)
            {
                Client_ReceiveChatHistory(History);
            }
        }
    }
}

void UNexusChatComponent::Client_ReceiveChatHistory_Implementation(const TArray<FNexusChatMessage>& History)
{
    ClientChatHistory = History;
    OnChatHistoryReceived.Broadcast(ClientChatHistory);
}

// ──────────────────────────────────────────────
// UTILS & LOGIC
// ──────────────────────────────────────────────

void UNexusChatComponent::FilterProfanity(FString& Message)
{
    // 1. Sécurité HTML (Anti-Injection)
    Message = Message.Replace(TEXT("<"), TEXT("&lt;"));
    Message = Message.Replace(TEXT(">"), TEXT("&gt;"));

    // 2. Censure (Config)
    if (ChatConfig)
    {
        for (const FString& BadWord : ChatConfig->BannedWords)
        {
            if (Message.Contains(BadWord))
            {
                 FString Mask;
                 for(int32 i=0; i<BadWord.Len(); ++i) Mask.AppendChar('*');
                 Message = Message.Replace(*BadWord, *Mask, ESearchCase::IgnoreCase);
            }
        }
    }
    // Test
    else 
    {
        const FString DefaultBad = TEXT("badword");
        if (Message.Contains(DefaultBad))
            Message = Message.Replace(*DefaultBad, TEXT("****"));
    }
}

FString UNexusChatComponent::DecorateMessage(const FString& Message, ENexusChatChannel Channel) const
{
    // Note: Cette fonction est maintenant purement utilitaire pour le Client.
    // Elle utilise la Config pour retourner le préfixe textuel si besoin.
    
    if (ChatConfig)
    {
        if (const FText* FoundPrefix = ChatConfig->ChannelPrefixes.Find(Channel))
        {
            return FoundPrefix->ToString();
        }
    }
    return FString();
}

TArray<APlayerController*> UNexusChatComponent::ResolveCustomRecipients_Implementation(APlayerController* Sender, FName ChannelName)
{
    return TArray<APlayerController*>();
}

// ──────────────────────────────────────────────
// COMMAND SYSTEM
// ──────────────────────────────────────────────

void UNexusChatComponent::RegisterCommands()
{
    RegisterInternalCommand(TEXT("/quit"), FChatCommandDelegate::CreateUObject(this, &UNexusChatComponent::Cmd_Quit));
    RegisterInternalCommand(TEXT("/w"), FChatCommandDelegate::CreateUObject(this, &UNexusChatComponent::Cmd_Whisper));
    RegisterInternalCommand(TEXT("/whisper"), FChatCommandDelegate::CreateUObject(this, &UNexusChatComponent::Cmd_Whisper));
    RegisterInternalCommand(TEXT("/r"), FChatCommandDelegate::CreateUObject(this, &UNexusChatComponent::Cmd_Reply));
    RegisterInternalCommand(TEXT("/reply"), FChatCommandDelegate::CreateUObject(this, &UNexusChatComponent::Cmd_Reply));
    RegisterInternalCommand(TEXT("/team"), FChatCommandDelegate::CreateUObject(this, &UNexusChatComponent::Cmd_Team));
}

void UNexusChatComponent::RegisterInternalCommand(const FString& Command, FChatCommandDelegate Callback)
{
    CommandHandlers.Add(Command, Callback);
}

bool UNexusChatComponent::ProcessSlashCommand(const FString& Content)
{
    FString Cmd, Params;

    // 1. On sépare la commande des paramètres
    if (Content.Split(TEXT(" "), &Cmd, &Params))
    {
        // Cmd contient "/w", Params contient "Player Hello"
    }
    else
    {
        Cmd = Content;
        Params.Empty();
    }

    // 2. Mets en minuscule
    Cmd = Cmd.ToLower();

    // 3. On cherche et on exécute
    if (FChatCommandDelegate* Handler = CommandHandlers.Find(Cmd))
    {
        Handler->ExecuteIfBound(Params);
        return true;
    }

    return false;
}

// --- Built-in Commands implementations ---

void UNexusChatComponent::Cmd_Quit(const FString& Params)
{
    if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
    {
        PC->ConsoleCommand("quit");
    }
}

void UNexusChatComponent::Cmd_Whisper(const FString& Params)
{
    FString TargetName, MsgContent;
    if (Params.Split(TEXT(" "), &TargetName, &MsgContent))
    {
        Server_SendChatMessage(MsgContent, ENexusChatChannel::Whisper, FName(*TargetName));
    }
    else
    {
        Client_ReceiveChatMessage(FNexusChatMessage::MakeSystem("Usage: /w <PlayerName> <Message>"));
    }
}

void UNexusChatComponent::Cmd_Team(const FString& Params)
{
    Server_SendChatMessage(Params, ENexusChatChannel::Team);
}

void UNexusChatComponent::Cmd_Reply(const FString& Params)
{
    if (LastWhisperSender.IsEmpty())
    {
        Client_ReceiveChatMessage(FNexusChatMessage::MakeSystem("No one to reply to."));
        return;
    }
    Cmd_Whisper(FString::Printf(TEXT("%s %s"), *LastWhisperSender, *Params));
}