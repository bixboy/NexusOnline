#include "ANexusGameMode.h"
#include "EngineUtils.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Managers/OnlineSessionManager.h"


void AANexusGameMode::BeginPlay()
{
    Super::BeginPlay();
    if (!HasAuthority()) return;

    UWorld* World = GetWorld();
    if (!ensure(World))
        return;

    // -- Spawn manager
    AOnlineSessionManager* ExistingManager = nullptr;
    for (TActorIterator<AOnlineSessionManager> It(World); It; ++It)
    {
        ExistingManager = *It; break;
    }

    if (!ExistingManager)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Params.Name = FName(TEXT("OnlineSessionManagerRuntime"));
        ExistingManager = World->SpawnActor<AOnlineSessionManager>(AOnlineSessionManager::StaticClass(), FTransform::Identity, Params);
        if (ExistingManager)
        {
            ExistingManager->TrackedSessionName = NAME_GameSession;
            ExistingManager->SetReplicates(true);
            ExistingManager->SetActorHiddenInGame(true);
            UE_LOG(LogTemp, Log, TEXT("[NexusOnline] ✅ OnlineSessionManager spawned in GameMode."));
        }
    }

    // -- Register host first
    GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
    {
        TryRegisterHost();

        // Delay StartSession slightly until host is registered
        GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
        {
            if (IOnlineSessionPtr Session = Online::GetSessionInterface(GetWorld()); Session.IsValid())
            {
                const FName SessionName = NAME_GameSession;
                const EOnlineSessionState::Type State = Session->GetSessionState(SessionName);

                if (State == EOnlineSessionState::Pending)
                {
                    const bool bOk = Session->StartSession(SessionName);
                    UE_LOG(LogTemp, Log, TEXT("[NexusOnline] ✅ StartSession('%s') after host register -> %s"), *SessionName.ToString(), bOk ? TEXT("OK") : TEXT("FAIL"));
                }
            }
        }));
    }));
}


void AANexusGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	if (!ErrorMessage.IsEmpty())
		return;

	if (!HasAuthority())
		return;

	UWorld* World = GetWorld();
	if (!World)
		return;

	IOnlineSessionPtr Session = Online::GetSessionInterface(World);
	if (!Session.IsValid())
		return;

	const FName SessionName = NAME_GameSession;
	FNamedOnlineSession* NamedSession = Session->GetNamedSession(SessionName);
	if (!NamedSession)
		return;

	const bool bHasOpenPublicConnections = NamedSession->NumOpenPublicConnections > 0;
	const bool bHasOpenPrivateConnections = NamedSession->NumOpenPrivateConnections > 0;
	if (bHasOpenPublicConnections || bHasOpenPrivateConnections)
		return;

	const int32 MaxPublic = NamedSession->SessionSettings.NumPublicConnections;
	const int32 MaxPrivate = NamedSession->SessionSettings.NumPrivateConnections;
	const int32 TotalMax = MaxPublic + MaxPrivate;

	UE_LOG(LogTemp, Warning, TEXT("[NexusOnline] Rejecting login for '%s': session '%s' is full (Max=%d)."),
			UniqueId.IsValid() ? *UniqueId->ToString() : TEXT("Unknown"),
			*SessionName.ToString(),
			TotalMax);

	ErrorMessage = TEXT("SessionFull");
}


void AANexusGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	IOnlineSessionPtr Session = Online::GetSessionInterface(GetWorld());
	if (Session.IsValid() && NewPlayer && NewPlayer->PlayerState)
	{
		FUniqueNetIdRepl Id = NewPlayer->PlayerState->GetUniqueId();
		if (Id.IsValid())
		{
			Session->RegisterPlayer(NAME_GameSession, *Id, /*bWasInvited*/ false);
			Session->UpdateSession(NAME_GameSession, Session->GetNamedSession(NAME_GameSession)->SessionSettings, /*bShouldRefreshOnlineData*/ true);
		}
	}
}

void AANexusGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	
	IOnlineSessionPtr Session = Online::GetSessionInterface(GetWorld());
	if (Session.IsValid())
	{
		if (APlayerState* PS = Exiting ? Exiting->GetPlayerState<APlayerState>() : nullptr)
		{
			FUniqueNetIdRepl Id = PS->GetUniqueId(); if (Id.IsValid())
			{
				Session->UnregisterPlayer(NAME_GameSession, *Id);
				Session->UpdateSession(NAME_GameSession, Session->GetNamedSession(NAME_GameSession)->SessionSettings, true);
			}
		}
	}
}


// ---- récupérer l'UniqueId de l’hôte ----
bool AANexusGameMode::GetHostUniqueId(FUniqueNetIdRepl& OutId) const
{
    if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
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

    if (ULocalPlayer* LP = GetWorld() ? GetWorld()->GetFirstLocalPlayerFromController() : nullptr)
    {
        const FUniqueNetIdPtr Pref = LP->GetPreferredUniqueNetId().GetUniqueNetId();
        if (Pref.IsValid())
        {
            OutId = FUniqueNetIdRepl(Pref.ToSharedRef());
            return true;
        }
    }

    if (IOnlineSubsystem* OSS = Online::GetSubsystem(GetWorld()))
    {
        if (IOnlineIdentityPtr Identity = OSS->GetIdentityInterface())
        {
            FUniqueNetIdPtr Id = Identity->GetUniquePlayerId(0);
            if (Id.IsValid())
            {
                OutId = FUniqueNetIdRepl(Id.ToSharedRef());
                return true;
            }
        }
    }

    return false;
}

// ---- register l’hôte s’il ne l’est pas déjà ----
void AANexusGameMode::TryRegisterHost()
{
	IOnlineSessionPtr Session = Online::GetSessionInterface(GetWorld());
	if (!Session.IsValid())
		return;

	FNamedOnlineSession* Named = Session->GetNamedSession(NAME_GameSession);
	if (!Named)
		return;

	if (GetNumPlayers() > 0)
	{
		FUniqueNetIdRepl HostId;
		if (GetHostUniqueId(HostId))
		{
			Session->RegisterPlayer(NAME_GameSession, *HostId, false);
			Session->UpdateSession(NAME_GameSession, Named->SessionSettings, true);
			UE_LOG(LogTemp, Log, TEXT("[NexusOnline] ✅ Host registered into session (%d max players)."),
				Named->SessionSettings.NumPublicConnections);
		}
		else
		{
			GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]{ TryRegisterHost(); }));
		}
	}
}

