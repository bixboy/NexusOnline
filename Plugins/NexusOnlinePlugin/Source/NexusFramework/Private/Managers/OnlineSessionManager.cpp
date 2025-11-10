#include "Managers/OnlineSessionManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
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

    UE_LOG(LogTemp, Log, TEXT("[SessionManager] BeginPlay (Authority: %s)"),
        HasAuthority() ? TEXT("Server") : TEXT("Client"));

    if (HasAuthority())
    {
        BindSessionDelegates();

        // 🔧 IMPORTANT: retarder le premier refresh d’1 tick pour laisser GameState/PlayerState/Session se créer
        GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
        {
            RefreshPlayerCount();
        }));

        // Refresh périodique
        GetWorldTimerManager().SetTimer(
            TimerHandle_Refresh, this, &AOnlineSessionManager::RefreshPlayerCount, 5.f, true);
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

    const UWorld* World = GetWorld();
    if (!World) return Players;

    if (IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(const_cast<UWorld*>(World)))
    {
        const FName EffectiveSessionName = TrackedSessionName.IsNone() ? NAME_GameSession : TrackedSessionName;
        if (FNamedOnlineSession* NamedSession = Session->GetNamedSession(EffectiveSessionName))
        {
            Players.Reserve(NamedSession->RegisteredPlayers.Num());
            for (const FUniqueNetIdRef& PlayerId : NamedSession->RegisteredPlayers)
            {
                // 🔒 Même si FUniqueNetIdRef est un SharedRef (non nul par contrat),
                // on protège au cas où une entrée corrompue se glisse (OSS exotiques / editor).
                if (PlayerId.ToSharedPtr())
                {
                    Players.Add(PlayerId->ToString());
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("[SessionManager] Invalid RegisteredPlayer entry."));
                }
            }
            return Players;
        }
    }

    // Fallback côté GameState (Standalone / avant RegisterPlayers)
    if (const AGameStateBase* GameState = World->GetGameState())
    {
        Players.Reserve(GameState->PlayerArray.Num());
        for (const APlayerState* PS : GameState->PlayerArray)
        {
            if (!PS) continue;

            const FUniqueNetIdRepl& IdRepl = PS->GetUniqueId();
            if (IdRepl.IsValid())
                Players.Add(IdRepl->ToString());
            else
                Players.Add(PS->GetPlayerName());
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

	UWorld* World = GetWorld();
	if (!World)
		return;

	int32 NewCount = 0;
	bool bHasValidSession = false;

	if (IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World))
	{
		const FName EffectiveSessionName = TrackedSessionName.IsNone() ? NAME_GameSession : TrackedSessionName;
		if (FNamedOnlineSession* NamedSession = Session->GetNamedSession(EffectiveSessionName))
		{
			bHasValidSession = true;
			NewCount = NamedSession->RegisteredPlayers.Num();

			if (NewCount == 0 && !World->IsNetMode(NM_Client))
			{
				if (IOnlineIdentityPtr Identity = NexusOnline::GetIdentityInterface(World))
				{
					if (TSharedPtr<const FUniqueNetId> LocalId = Identity->GetUniquePlayerId(0); LocalId.IsValid())
					{
						TArray<FUniqueNetIdRef> Arr;
						Arr.Add(LocalId.ToSharedRef());
						Session->RegisterPlayers(EffectiveSessionName, Arr, false);
						UE_LOG(LogTemp, Log, TEXT("[SessionManager] Auto-register host for active session."));
					}
				}
			}
		}
	}

	if (!bHasValidSession)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[SessionManager] No active session, PlayerCount forced to 0."));
		NewCount = 0;
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
    UWorld* World = GetWorld();
    if (!World) return;

    if (IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World))
    {
        RegisterPlayersHandle = Session->AddOnRegisterPlayersCompleteDelegate_Handle(
            FOnRegisterPlayersCompleteDelegate::CreateUObject(this, &AOnlineSessionManager::OnPlayersRegistered));

        UnregisterPlayersHandle = Session->AddOnUnregisterPlayersCompleteDelegate_Handle(
            FOnUnregisterPlayersCompleteDelegate::CreateUObject(this, &AOnlineSessionManager::OnPlayersUnregistered));
    }
}

void AOnlineSessionManager::UnbindSessionDelegates()
{
    UWorld* World = GetWorld();
    if (!World) return;

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

void AOnlineSessionManager::OnPlayersRegistered(FName SessionName, const TArray<FUniqueNetIdRef>& Players, bool bWasSuccessful)
{
    if (!HasAuthority()) return;

    RefreshPlayerCount();

    if (!bWasSuccessful) return;

    for (const FUniqueNetIdRef& Id : Players)
    {
        if (Id.ToSharedPtr())
            OnPlayerJoined.Broadcast(Id->ToString());
        else
            OnPlayerJoined.Broadcast(TEXT(""));
    }
}

void AOnlineSessionManager::OnPlayersUnregistered(FName SessionName, const TArray<FUniqueNetIdRef>& Players, bool bWasSuccessful)
{
    if (!HasAuthority()) return;

    RefreshPlayerCount();

    if (!bWasSuccessful) return;

    for (const FUniqueNetIdRef& Id : Players)
    {
        if (Id.ToSharedPtr())
            OnPlayerLeft.Broadcast(Id->ToString());
        else
            OnPlayerLeft.Broadcast(TEXT(""));
    }
}
