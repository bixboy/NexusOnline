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
#include "Subsystems/NexusMigrationSubsystem.h"


AOnlineSessionManager::AOnlineSessionManager()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    PrimaryActorTick.bCanEverTick = false;

    PlayerCount = 0;
    LastNotifiedPlayerCount = 0;
    TrackedSessionName = NAME_GameSession;
}

void AOnlineSessionManager::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Warning, TEXT("[AOnlineSessionManager] BeginPlay Started. This: %p, World: %p"), this, GetWorld());

    // Cache this instance for O(1) Get() access, keyed by world for PIE support
    if (UWorld* World = GetWorld())
    {
        CachedInstances.Add(World, this);
    }

    if (HasAuthority())
    {
        BindSessionDelegates();

        GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
        {
            RefreshPlayerCount();
        }));

        GetWorldTimerManager().SetTimer(TimerHandle_Refresh, this, &AOnlineSessionManager::RefreshPlayerCount, 10.f, true);
    }
    else
    {
        if (PlayerCount > 0)
        {
            OnRep_PlayerCount();
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("[AOnlineSessionManager] BeginPlay Finished."));
}

void AOnlineSessionManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        TWeakObjectPtr<AOnlineSessionManager>* Found = CachedInstances.Find(World);
        if (Found && Found->Get() == this)
        {
            CachedInstances.Remove(World);
        }
    }

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
    DOREPLIFETIME(AOnlineSessionManager, NextHostUniqueId);
}

// ──────────────────────────────────────────────
// CLIENT UPDATES
// ──────────────────────────────────────────────

void AOnlineSessionManager::OnRep_PlayerCount()
{
    OnPlayerCountChanged.Broadcast(PlayerCount);
}

// ──────────────────────────────────────────────
// SERVER LOGIC
// ──────────────────────────────────────────────

void AOnlineSessionManager::ForceUpdatePlayerCount()
{
    if (HasAuthority())
    {
        RefreshPlayerCount();
    }
}

void AOnlineSessionManager::RefreshPlayerCount()
{
    UE_LOG(LogTemp, Verbose, TEXT("[AOnlineSessionManager] RefreshPlayerCount Called."));
    if (!HasAuthority())
		return;

	UWorld* World = GetWorld();
	if (!World)
		return;

	int32 SessionCount = 0;
	int32 GameStateCount = 0;

	// 1. Check Session Count
	if (IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(World))
	{
		if (FNamedOnlineSession* NamedSession = Session->GetNamedSession(TrackedSessionName))
		{
			SessionCount = NamedSession->RegisteredPlayers.Num();
			
			// Auto-register host fix (unchanged)
			if (SessionCount == 0 && !World->IsNetMode(NM_Client) && !World->IsNetMode(NM_DedicatedServer))
			{
				 if (IOnlineIdentityPtr Identity = NexusOnline::GetIdentityInterface(World))
				 {
					 if (TSharedPtr<const FUniqueNetId> LocalId = Identity->GetUniquePlayerId(0))
					 {
						 UE_LOG(LogTemp, Log, TEXT("[SessionManager] Auto-registering host..."));
						 TArray<FUniqueNetIdRef> Arr;
						 Arr.Add(LocalId.ToSharedRef());
						 Session->RegisterPlayers(TrackedSessionName, Arr, false);
						 return;
					 }
				 }
			}
		}
	}

	// 2. Check GameState Count
	if (World->GetGameState())
	{
		GameStateCount = World->GetGameState()->PlayerArray.Num();
	}

	// 3. Take logic MAX to be safe
	int32 NewCount = FMath::Max(SessionCount, GameStateCount);

	UE_LOG(LogTemp, Log, TEXT("[SessionManager] Refreshing. Session: %d, GameState: %d -> Final: %d"), SessionCount, GameStateCount, NewCount);

	if (PlayerCount != NewCount)
	{
		PlayerCount = NewCount;
		OnRep_PlayerCount();
	}
	
	UpdateHeir();
}

void AOnlineSessionManager::UpdateHeir()
{
    UE_LOG(LogTemp, Warning, TEXT("[AOnlineSessionManager] UpdateHeir Called."));
    if (!HasAuthority())
        return;

    IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(GetWorld());
    if (!Session)
        return;

    FNamedOnlineSession* NamedSession = Session->GetNamedSession(TrackedSessionName);
    if (!NamedSession)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AOnlineSessionManager] UpdateHeir: NamedSession not found."));
        return;
    }

    // Get Host ID
    TSharedPtr<const FUniqueNetId> HostId;
    if (IOnlineIdentityPtr Identity = NexusOnline::GetIdentityInterface(GetWorld()))
    {
        HostId = Identity->GetUniquePlayerId(0);
    }

    FString BestHeirId;

    // Cache GameState players for O(N) lookup
    TSet<FString> ActivePlayers;
    if (AGameStateBase* GS = GetWorld()->GetGameState())
    {
        for (APlayerState* PS : GS->PlayerArray)
        {
            if (PS && PS->GetUniqueId().IsValid())
            {
                ActivePlayers.Add(PS->GetUniqueId().ToString());
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[AOnlineSessionManager] Checking %d Registered Players..."), NamedSession->RegisteredPlayers.Num());

	for (const FUniqueNetIdRef& PlayerId : NamedSession->RegisteredPlayers)
	{
        FString CandidateId = PlayerId->ToString();
        UE_LOG(LogTemp, Warning, TEXT("  - Candidate: %s"), *CandidateId);

		// Skip Host
		if (HostId.IsValid() && *PlayerId == *HostId)
        {
            UE_LOG(LogTemp, Warning, TEXT("    -> Is Host. Skipping."));
			continue;
        }

		// Skip invalid
		if (!PlayerId->IsValid())
			continue;

        // Cross-reference with GameState to ensure they haven't timed out/disconnected
        if (ActivePlayers.Contains(CandidateId))
        {
		    BestHeirId = CandidateId;
		    UE_LOG(LogTemp, Warning, TEXT("[SessionManager] Heir elected via Session+GameState: %s"), *BestHeirId);
		    break;
        }
	}

	// Fallback: Check GameState if Session list didn't yield an heir
	if (BestHeirId.IsEmpty() && GetWorld()->GetGameState())
	{
        UE_LOG(LogTemp, Warning, TEXT("[AOnlineSessionManager] Using Fallback GameState Loop..."));
		for (APlayerState* PS : GetWorld()->GetGameState()->PlayerArray)
		{
			if (!PS) continue;
			
			// Skip Host (Local Player) using HostId captured earlier
			if (HostId.IsValid() && PS->GetUniqueId().IsValid() && *PS->GetUniqueId() == *HostId)
				continue;

			if (PS->GetUniqueId().IsValid())
			{
				BestHeirId = PS->GetUniqueId().ToString();
				UE_LOG(LogTemp, Warning, TEXT("[SessionManager] Heir elected via GameState (Fallback): %s"), *BestHeirId);
				break;
			}
		}
	}

    if (NextHostUniqueId != BestHeirId)
    {
        NextHostUniqueId = BestHeirId;
        UE_LOG(LogTemp, Warning, TEXT("[AOnlineSessionManager] New Heir Selected: %s"), *NextHostUniqueId);
        
        // Cache on Server (for local migration subsystem awareness)
        if (UGameInstance* GI = GetGameInstance())
        {
             if (UNexusMigrationSubsystem* Subsystem = GI->GetSubsystem<UNexusMigrationSubsystem>())
             {
                 Subsystem->SetCachedNextHostId(NextHostUniqueId);
             }
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("[AOnlineSessionManager] UpdateHeir Finished."));
}

void AOnlineSessionManager::OnRep_NextHostUniqueId()
{
    // Cache on Client (local)
    if (UGameInstance* GI = GetGameInstance())
    {
         if (UNexusMigrationSubsystem* Subsystem = GI->GetSubsystem<UNexusMigrationSubsystem>())
         {
             Subsystem->SetCachedNextHostId(NextHostUniqueId);
         }
    }
}

// ──────────────────────────────────────────────
// EVENTS & DELEGATES
// ──────────────────────────────────────────────

void AOnlineSessionManager::BindSessionDelegates()
{
    if (IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(GetWorld()))
    {
        RegisterPlayersHandle = Session->AddOnRegisterPlayersCompleteDelegate_Handle(FOnRegisterPlayersCompleteDelegate::CreateUObject(this,
            &AOnlineSessionManager::OnPlayersRegistered));

        UnregisterPlayersHandle = Session->AddOnUnregisterPlayersCompleteDelegate_Handle(FOnUnregisterPlayersCompleteDelegate::CreateUObject(this,
            &AOnlineSessionManager::OnPlayersUnregistered));
    }
}

void AOnlineSessionManager::UnbindSessionDelegates()
{
    if (IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(GetWorld()))
    {
        if (RegisterPlayersHandle.IsValid())
            Session->ClearOnRegisterPlayersCompleteDelegate_Handle(RegisterPlayersHandle);
        
        if (UnregisterPlayersHandle.IsValid())
            Session->ClearOnUnregisterPlayersCompleteDelegate_Handle(UnregisterPlayersHandle);
    }
}

void AOnlineSessionManager::OnPlayersRegistered(FName SessionName, const TArray<FUniqueNetIdRef>& Players, bool bWasSuccessful)
{
    if (SessionName != TrackedSessionName)
        return;
    
    RefreshPlayerCount();

    if (bWasSuccessful)
    {
        for (const FUniqueNetIdRef& Id : Players)
        {
            FString DisplayName = GetPlayerNameFromId(*Id);
            OnPlayerJoined.Broadcast(DisplayName);
        }
    }
}

void AOnlineSessionManager::OnPlayersUnregistered(FName SessionName, const TArray<FUniqueNetIdRef>& Players, bool bWasSuccessful)
{
    if (SessionName != TrackedSessionName)
        return;

    RefreshPlayerCount();

    if (bWasSuccessful)
    {
        for (const FUniqueNetIdRef& Id : Players)
        {
             FString DisplayName = GetPlayerNameFromId(*Id);
             OnPlayerLeft.Broadcast(DisplayName);
        }
    }
}

// ──────────────────────────────────────────────
// UTILS
// ──────────────────────────────────────────────

TArray<FString> AOnlineSessionManager::GetPlayerList() const
{
    TArray<FString> PlayerNames;
    const UWorld* World = GetWorld();
    if (!World)
        return PlayerNames;

    if (const AGameStateBase* GameState = World->GetGameState())
    {
        for (APlayerState* PS : GameState->PlayerArray)
        {
            if (PS)
                PlayerNames.Add(PS->GetPlayerName());
        }
        
        if (PlayerNames.Num() > 0)
            return PlayerNames;
    }

    if (IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(const_cast<UWorld*>(World)))
    {
        if (FNamedOnlineSession* NamedSession = Session->GetNamedSession(TrackedSessionName))
        {
            for (const FUniqueNetIdRef& Id : NamedSession->RegisteredPlayers)
            {
                PlayerNames.Add(Id->ToString());
            }
        }
    }

    return PlayerNames;
}

FString AOnlineSessionManager::GetPlayerNameFromId(const FUniqueNetId& PlayerId) const
{
    if (const UWorld* World = GetWorld())
    {
        if (const AGameStateBase* GameState = World->GetGameState())
        {
            for (APlayerState* PS : GameState->PlayerArray)
            {
                if (PS && PS->GetUniqueId() == PlayerId)
                {
                    return PS->GetPlayerName();
                }
            }
        }
    }
    
    return PlayerId.ToString();
}

// Static per-world cache
TMap<TWeakObjectPtr<UWorld>, TWeakObjectPtr<AOnlineSessionManager>> AOnlineSessionManager::CachedInstances;

AOnlineSessionManager* AOnlineSessionManager::Get(UObject* WorldContextObject)
{
    if (!WorldContextObject)
        return nullptr;
    
    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
        return nullptr;

    // O(1) cached path — per-world
    if (TWeakObjectPtr<AOnlineSessionManager>* Found = CachedInstances.Find(World))
    {
        if (Found->IsValid())
        {
            return Found->Get();
        }
        CachedInstances.Remove(World);
    }

    // Fallback: TActorIterator (after level transitions when cache is stale)
    TActorIterator<AOnlineSessionManager> It(World);
    if (It)
    {
        CachedInstances.Add(World, *It);
        return *It;
    }
    
    return nullptr;
}

AOnlineSessionManager* AOnlineSessionManager::Spawn(UObject* WorldContextObject, FName InSessionName)
{
    if (!WorldContextObject)
        return nullptr;
    
    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
        return nullptr;
    
    if (AOnlineSessionManager* Existing = Get(WorldContextObject))
        return Existing;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
    AOnlineSessionManager* Manager = World->SpawnActor<AOnlineSessionManager>(AOnlineSessionManager::StaticClass(), Params);
    if (Manager)
    {
        Manager->TrackedSessionName = InSessionName;
        CachedInstances.Add(World, Manager);
    }
    
    return Manager;
}

// ──────────────────────────────────────────────
// HOST REGISTRATION
// ──────────────────────────────────────────────

void AOnlineSessionManager::RegisterLocalHost()
{
	if (!HasAuthority()) 
		return;

	RegisterHostRetries = 0;
	TryRegisterHost();
}

void AOnlineSessionManager::TryRegisterHost()
{
	IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(GetWorld());
	if (!Session.IsValid())
		return;

	FNamedOnlineSession* Named = Session->GetNamedSession(TrackedSessionName);
	if (!Named)
		return;

	if (IsRunningDedicatedServer())
		return;

	FUniqueNetIdRepl HostId;
	if (GetHostUniqueId(HostId))
	{
		Session->RegisterPlayer(TrackedSessionName, *HostId, false);
		if (Session->GetSessionState(TrackedSessionName) == EOnlineSessionState::Pending)
		{
			Session->StartSession(TrackedSessionName);
			UE_LOG(LogTemp, Log, TEXT("[SessionManager] Session Started after Host Registration."));
		}

		Session->UpdateSession(TrackedSessionName, Named->SessionSettings, true);
		UE_LOG(LogTemp, Log, TEXT("[SessionManager] Host Registered: %s"), *HostId.ToString());
		
		GetWorldTimerManager().ClearTimer(TimerHandle_RetryRegister);
	}
	else
	{
		RegisterHostRetries++;
		if (RegisterHostRetries < MAX_REGISTER_RETRIES)
		{
			UE_LOG(LogTemp, Verbose, TEXT("[SessionManager] Host ID not ready, retrying... (%d/%d)"), RegisterHostRetries, MAX_REGISTER_RETRIES);
			GetWorldTimerManager().SetTimer(TimerHandle_RetryRegister, this, &AOnlineSessionManager::TryRegisterHost, 0.5f, false);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[SessionManager] Failed to register host after %d retries. Session might not be visible correctly."), MAX_REGISTER_RETRIES);
		}
	}
}

bool AOnlineSessionManager::GetHostUniqueId(FUniqueNetIdRepl& OutId) const
{
	UWorld* World = GetWorld();
	if (!World)
		return false;

	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		if (APlayerState* PS = PC->PlayerState)
		{
			FUniqueNetIdRepl Id = PS->GetUniqueId();
			if (Id.IsValid())
			{
				OutId = Id;
				return true;
			}
		}
	}

	if (ULocalPlayer* LP = World->GetFirstLocalPlayerFromController())
	{
		FUniqueNetIdRepl Id = LP->GetPreferredUniqueNetId();
		if (Id.IsValid())
		{
			OutId = Id;
			return true;
		}
	}

	if (IOnlineIdentityPtr Identity = NexusOnline::GetIdentityInterface(World))
	{
		if (TSharedPtr<const FUniqueNetId> Id = Identity->GetUniquePlayerId(0))
		{
			OutId = FUniqueNetIdRepl(Id);
			return true;
		}
	}

	return false;
}