#include "ANexusGameMode.h"
#include "EngineUtils.h"
#include "OnlineSubsystemUtils.h"
#include "Managers/OnlineSessionManager.h"
#include "TimerManager.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/OnlineSessionInterface.h"

void AANexusGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
		return;

	UWorld* World = GetWorld();
	if (!ensure(World))
		return;

	AOnlineSessionManager* ExistingManager = nullptr;
	for (TActorIterator<AOnlineSessionManager> It(World); It; ++It)
	{
		ExistingManager = *It;
		break;
	}

	if (!ExistingManager)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.Name = FName(TEXT("OnlineSessionManagerRuntime"));
		Params.bNoFail = true;

		ExistingManager = World->SpawnActor<AOnlineSessionManager>(
			AOnlineSessionManager::StaticClass(),
			FTransform::Identity,
			Params
		);

		if (ExistingManager)
		{
			ExistingManager->TrackedSessionName = NAME_GameSession;
			ExistingManager->SetReplicates(true);
			ExistingManager->SetActorHiddenInGame(true);

			UE_LOG(LogTemp, Log, TEXT("[NexusOnline] ✅ OnlineSessionManager spawned in GameMode."));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[NexusOnline] ❌ Failed to spawn OnlineSessionManager!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[NexusOnline] Using existing OnlineSessionManager: %s"), *ExistingManager->GetName());
	}

	if (ExistingManager)
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateWeakLambda(ExistingManager, [ExistingManager]()
			{
				if (ExistingManager->HasAuthority())
					ExistingManager->ForceUpdatePlayerCount();
			})
		);
	}
}

void AANexusGameMode::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);

	IOnlineSessionPtr Session = Online::GetSessionInterface(GetWorld());
	if (Session.IsValid())
	{
		FUniqueNetIdRepl PlayerId = NewPlayer->PlayerState->GetUniqueId();
		if (PlayerId.IsValid())
		{
			Session->RegisterPlayer(NAME_GameSession, *PlayerId, false);
			UE_LOG(LogTemp, Log, TEXT("✅ Registered player %s to session."), *PlayerId->ToString());
		}
	}
}

void AANexusGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	IOnlineSessionPtr Session = Online::GetSessionInterface(GetWorld());
	if (Session.IsValid())
	{
		if (APlayerState* PS = Exiting->GetPlayerState<APlayerState>())
		{
			FUniqueNetIdRepl PlayerId = PS->GetUniqueId();
			if (PlayerId.IsValid())
			{
				Session->UnregisterPlayer(NAME_GameSession, *PlayerId);
				UE_LOG(LogTemp, Log, TEXT("Unregistered player %s from session."), *PlayerId->ToString());
			}
		}
	}
}
