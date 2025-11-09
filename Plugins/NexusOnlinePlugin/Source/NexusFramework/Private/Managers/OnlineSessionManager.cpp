#include "Managers/OnlineSessionManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Utils/NexusOnlineHelpers.h"


AOnlineSessionManager::AOnlineSessionManager()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    PrimaryActorTick.bCanEverTick = false;

    PlayerCount = 0;
    LastNotifiedPlayerCount = INDEX_NONE;
    TrackedSessionName = NAME_None;
}

void AOnlineSessionManager::BeginPlay()
{
    Super::BeginPlay();
	
	UE_LOG(LogTemp, Log, TEXT("[SessionManager] BeginPlay (Authority: %s)"), HasAuthority() ? TEXT("Server") : TEXT("Client"));
	
    if (HasAuthority())
    {
        BindSessionDelegates();
        RefreshPlayerCount();

        GetWorldTimerManager().SetTimer
    	(
            TimerHandle_Refresh, this, &AOnlineSessionManager::RefreshPlayerCount, 5.f, true
        );
    }
    else
    {
        LastNotifiedPlayerCount = PlayerCount;
        OnRep_PlayerCount();
    }
}

void AOnlineSessionManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (HasAuthority())
    {
        UnbindSessionDelegates();
        GetWorldTimerManager().ClearTimer(TimerHandle_Refresh);
    }

    Super::EndPlay(EndPlayReason);
}

void AOnlineSessionManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AOnlineSessionManager, PlayerCount);
    DOREPLIFETIME(AOnlineSessionManager, TrackedSessionName);
}

void AOnlineSessionManager::OnRep_PlayerCount()
{
    const int32 Previous = (LastNotifiedPlayerCount == INDEX_NONE) ? PlayerCount : LastNotifiedPlayerCount;
    BroadcastPlayerCountChange(Previous, PlayerCount, true);
    LastNotifiedPlayerCount = PlayerCount;
}

void AOnlineSessionManager::ForceUpdatePlayerCount()
{
    if (HasAuthority())
    {
        RefreshPlayerCount();
    }
    else
    {
        Server_UpdatePlayerCount();
    }
}

void AOnlineSessionManager::Server_UpdatePlayerCount_Implementation()
{
    RefreshPlayerCount();
}

TArray<FString> AOnlineSessionManager::GetPlayerList() const
{
    TArray<FString> Players;

    if (const UWorld* World = GetWorld())
    {
        if (IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(const_cast<UWorld*>(World)))
        {
            const FName EffectiveSessionName = TrackedSessionName.IsNone() ? NAME_GameSession : TrackedSessionName;
            if (FNamedOnlineSession* NamedSession = Session->GetNamedSession(EffectiveSessionName))
            {
                Players.Reserve(NamedSession->RegisteredPlayers.Num());
                for (const FUniqueNetIdRef& PlayerId : NamedSession->RegisteredPlayers)
                {
                    Players.Add(PlayerId->ToString());
                }
            }
        }
    }

    return Players;
}

AOnlineSessionManager* AOnlineSessionManager::Get(UObject* WorldContextObject)
{
    if (!WorldContextObject)
        return nullptr;

    UWorld* World = WorldContextObject->GetWorld();
    if (!World && GEngine)
        World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);

    if (!World)
        return nullptr;

    for (TActorIterator<AOnlineSessionManager> It(World); It; ++It)
        return *It;

    return nullptr;
}

void AOnlineSessionManager::RefreshPlayerCount()
{
    if (!HasAuthority())
        return;

    int32 NewCount = 0;

    if (UWorld* World = GetWorld())
    {
        if (IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World))
        {
            const FName EffectiveSessionName = TrackedSessionName.IsNone() ? NAME_GameSession : TrackedSessionName;
            if (FNamedOnlineSession* NamedSession = Session->GetNamedSession(EffectiveSessionName))
            {
                NewCount = NamedSession->RegisteredPlayers.Num();
            }
        }
    }

    if (PlayerCount != NewCount)
    {
        const int32 Previous = PlayerCount;
        PlayerCount = NewCount;
        BroadcastPlayerCountChange(Previous, PlayerCount, false);
        LastNotifiedPlayerCount = PlayerCount;
    }
}

void AOnlineSessionManager::BroadcastPlayerCountChange(int32 PreviousCount, int32 NewCount, bool bAllowFallbackEvents)
{
    OnPlayerCountChanged.Broadcast(NewCount);

    if (!bAllowFallbackEvents)
        return;

    if (NewCount > PreviousCount)
    {
        OnPlayerJoined.Broadcast(TEXT(""));
    }
    else if (NewCount < PreviousCount)
    {
        OnPlayerLeft.Broadcast(TEXT(""));
    }
}

void AOnlineSessionManager::BindSessionDelegates()
{
    if (UWorld* World = GetWorld())
    {
        if (IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World))
        {
            // Player join
            RegisterPlayersHandle = Session->AddOnRegisterPlayersCompleteDelegate_Handle
        	(
                FOnRegisterPlayersCompleteDelegate::CreateUObject(this, &AOnlineSessionManager::OnPlayersRegistered)
            );

            // Player leave
            UnregisterPlayersHandle = Session->AddOnUnregisterPlayersCompleteDelegate_Handle
        	(
                FOnUnregisterPlayersCompleteDelegate::CreateUObject(this, &AOnlineSessionManager::OnPlayersUnregistered)
            );
        }
    }
}

void AOnlineSessionManager::UnbindSessionDelegates()
{
    if (UWorld* World = GetWorld())
    {
        if (IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World))
        {
            if (RegisterPlayersHandle.IsValid())
            {
                Session->ClearOnRegisterPlayersCompleteDelegate_Handle(RegisterPlayersHandle);
                RegisterPlayersHandle.Reset();
            }

            if (UnregisterPlayersHandle.IsValid())
            {
                Session->ClearOnUnregisterPlayersCompleteDelegate_Handle(UnregisterPlayersHandle);
                UnregisterPlayersHandle.Reset();
            }
        }
    }
}

void AOnlineSessionManager::OnPlayersRegistered(FName SessionName, const TArray<FUniqueNetIdRef>& Players, bool bWasSuccessful)
{
    if (!HasAuthority())
        return;

    RefreshPlayerCount();

    for (const FUniqueNetIdRef& Id : Players)
        OnPlayerJoined.Broadcast(Id->ToString());
}

void AOnlineSessionManager::OnPlayersUnregistered(FName SessionName, const TArray<FUniqueNetIdRef>& Players, bool bWasSuccessful)
{
    if (!HasAuthority())
        return;

    RefreshPlayerCount();

    for (const FUniqueNetIdRef& Id : Players)
        OnPlayerLeft.Broadcast(Id->ToString());
}
