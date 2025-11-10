#include "ANexusGameMode.h"
#include "EngineUtils.h"
#include "Managers/OnlineSessionManager.h"
#include "TimerManager.h"

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
