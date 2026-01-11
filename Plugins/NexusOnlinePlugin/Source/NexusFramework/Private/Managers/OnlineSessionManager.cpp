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
    if (!HasAuthority())
        return;

    IOnlineSessionPtr Session = NexusOnline::GetSessionInterface(GetWorld());
    if (!Session)
        return;

    FNamedOnlineSession* NamedSession = Session->GetNamedSession(TrackedSessionName);
    if (!NamedSession)
        return;

    // Get Host ID
    TSharedPtr<const FUniqueNetId> HostId;
    if (IOnlineIdentityPtr Identity = NexusOnline::GetIdentityInterface(GetWorld()))
    {
        HostId = Identity->GetUniquePlayerId(0);
    }

    FString BestHeirId;

    // Iterate through registered players to find the oldest non-host client
    // RegisteredPlayers order is usually preservation order of joining? 
    // If not, we might need a better metric, but for now simple iteration is fine.
    	// Iterate through registered players to find the oldest non-host client
	for (const FUniqueNetIdRef& PlayerId : NamedSession->RegisteredPlayers)
	{
		// Skip Host
		if (HostId.IsValid() && *PlayerId == *HostId)
			continue;

		// Skip invalid
		if (!PlayerId->IsValid())
			continue;

		// Found our heir (first valid non-host player)
		BestHeirId = PlayerId->ToString();
		UE_LOG(LogTemp, Log, TEXT("[SessionManager] Heir elected via Session: %s"), *BestHeirId);
		break;
	}

	// Fallback: Check GameState if Session list didn't yield an heir (common in PIE or broken sessions)
	if (BestHeirId.IsEmpty() && GetWorld()->GetGameState())
	{
		for (APlayerState* PS : GetWorld()->GetGameState()->PlayerArray)
		{
			if (!PS) continue;
			
			// Skip Local Player (Host)
			if (PS->GetPlayerController() && PS->GetPlayerController()->IsLocalController())
				continue;

			// Check ID validity
			if (PS->GetUniqueId().IsValid())
			{
				BestHeirId = PS->GetUniqueId().ToString();
				UE_LOG(LogTemp, Warning, TEXT("[SessionManager] Heir elected via GameState: %s"), *BestHeirId);
				break;
			}
		}
	}

    if (NextHostUniqueId != BestHeirId)
    {
        NextHostUniqueId = BestHeirId;
        
        // Cache on Server (local)
        if (UGameInstance* GI = GetGameInstance())
        {
             if (UNexusMigrationSubsystem* Subsystem = GI->GetSubsystem<UNexusMigrationSubsystem>())
             {
                 Subsystem->SetCachedNextHostId(NextHostUniqueId);
             }
        }
    }
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

AOnlineSessionManager* AOnlineSessionManager::Get(UObject* WorldContextObject)
{
    if (!WorldContextObject)
        return nullptr;
    
    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
        return nullptr;

    TActorIterator<AOnlineSessionManager> It(World);
    if (It)
    {
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
    }
    
    return Manager;
}