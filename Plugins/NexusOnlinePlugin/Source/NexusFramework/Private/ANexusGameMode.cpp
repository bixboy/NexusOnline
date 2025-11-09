// Fill out your copyright notice in the Description page of Project Settings.


#include "ANexusGameMode.h"

#include "EngineUtils.h"
#include "Managers/OnlineSessionManager.h"

void AANexusGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		AOnlineSessionManager* ExistingManager = nullptr;
		for (TActorIterator<AOnlineSessionManager> It(GetWorld()); It; ++It)
		{
			ExistingManager = *It;
			break;
		}

		if (!ExistingManager)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ExistingManager = GetWorld()->SpawnActor<AOnlineSessionManager>(AOnlineSessionManager::StaticClass(), FTransform::Identity, Params);
			
			if (ExistingManager)
			{
				ExistingManager->TrackedSessionName = NAME_GameSession;
				ExistingManager->SetReplicates(true);
				ExistingManager->SetActorHiddenInGame(true);
				UE_LOG(LogTemp, Log, TEXT("[NexusOnline] ✅ OnlineSessionManager spawned in GameMode."));
			}
		}
	}
}
