#include "Runtime/Managers/OnlineSessionManager.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Net/UnrealNetwork.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemTypes.h"
#include "Runtime/Launch/Resources/Version.h"
#include "GameFramework/OnlineSessionNames.h"
#include "Utils/NexusOnlineHelpers.h"

namespace
{
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
template <typename SessionType>
auto ClearParticipantsChangeDelegateImpl(SessionType& Session, const FDelegateHandle& Handle, int)
    -> decltype(Session.ClearOnSessionParticipantsChangeDelegate_Handle(Handle), void())
{
    Session.ClearOnSessionParticipantsChangeDelegate_Handle(Handle);
}

template <typename SessionType>
auto ClearParticipantsChangeDelegateImpl(SessionType& Session, const FDelegateHandle& Handle, long)
    -> decltype(Session.UnregisterOnSessionParticipantsChangeDelegate(Handle), void())
{
    Session.UnregisterOnSessionParticipantsChangeDelegate(Handle);
}

template <typename SessionType>
void ClearParticipantsChangeDelegateImpl(SessionType&, const FDelegateHandle&, ...)
{
}

template <typename SessionType>
void ClearParticipantsChangeDelegate(SessionType& Session, const FDelegateHandle& Handle)
{
    ClearParticipantsChangeDelegateImpl(Session, Handle, 0);
}
#endif // ENGINE VERSION GUARD
}

AOnlineSessionManager::AOnlineSessionManager()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(false);
    PrimaryActorTick.bCanEverTick = false;
    PlayerCount = 0;
    LastNotifiedPlayerCount = INDEX_NONE;
    TrackedSessionName = NAME_None;
}

void AOnlineSessionManager::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        BindSessionDelegates();
        RefreshPlayerCount();
    }
    else
    {
        LastNotifiedPlayerCount = PlayerCount;
        OnRep_PlayerCount();
        ForceUpdatePlayerCount();
    }
}

void AOnlineSessionManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (HasAuthority())
    {
        UnbindSessionDelegates();
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
    {
        return nullptr;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        if (!GEngine)
        {
            return nullptr;
        }

        World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
    }

    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<AOnlineSessionManager> It(World); It; ++It)
    {
        return *It;
    }

    return nullptr;
}

void AOnlineSessionManager::RefreshPlayerCount()
{
    if (!HasAuthority())
    {
        return;
    }

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
    {
        return;
    }

    if (NewCount > PreviousCount)
    {
        // Broadcast joined event without a specific id (fallback).
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
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
            if (!ParticipantsChangedDelegateHandle.IsValid())
            {
                ParticipantsChangedDelegateHandle = Session->AddOnSessionParticipantsChangeDelegate_Handle(
                    FOnSessionParticipantsChangeDelegate::CreateUObject(this, &AOnlineSessionManager::HandleParticipantsChanged));
            }
#endif
        }
    }
}

void AOnlineSessionManager::UnbindSessionDelegates()
{
    if (UWorld* World = GetWorld())
    {
        if (IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World))
        {
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
            if (ParticipantsChangedDelegateHandle.IsValid())
            {
                ClearParticipantsChangeDelegate(*Session, ParticipantsChangedDelegateHandle);
                ParticipantsChangedDelegateHandle = FDelegateHandle();
            }
#endif
        }
    }
}

void AOnlineSessionManager::HandleParticipantsChanged(FName SessionName, const FUniqueNetId& PlayerId, bool bJoined)
{
    if (!HasAuthority())
    {
        return;
    }

    const FName EffectiveSessionName = TrackedSessionName.IsNone() ? NAME_GameSession : TrackedSessionName;
    if (!EffectiveSessionName.IsNone() && SessionName != EffectiveSessionName)
    {
        return;
    }

    const FString PlayerIdString = PlayerId.ToString();

    RefreshPlayerCount();

    if (bJoined)
    {
        OnPlayerJoined.Broadcast(PlayerIdString);
    }
    else
    {
        OnPlayerLeft.Broadcast(PlayerIdString);
    }
}
