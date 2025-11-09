#pragma once
#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "NexusOnlineSubsystemManager.generated.h"


class IOnlineSubsystem;

UCLASS()
class NEXUSFRAMEWORK_API UNexusOnlineSubsystemManager : public UObject
{
	GENERATED_BODY()

public:
	/** Retourne le OnlineSubsystem actif (Steam, EOS, etc.) */
	static IOnlineSubsystem* GetSubsystem();

	/** Retourne l'interface de session (OnlineSessionInterface) */
	static IOnlineSessionPtr GetSessionInterface();
};
