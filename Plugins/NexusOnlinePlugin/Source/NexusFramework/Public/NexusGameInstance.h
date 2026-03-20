#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "NexusGameInstance.generated.h"

class UOnlineSession;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnNexusNetworkFailure, UWorld*, World, UNetDriver*, NetDriver, ENetworkFailure::Type, FailureType, const FString&, ErrorString);

UCLASS()
class NEXUSFRAMEWORK_API UNexusGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	virtual TSubclassOf<UOnlineSession> GetOnlineSessionClass() override;

	UPROPERTY(BlueprintAssignable, Category = "Nexus|Network")
	FOnNexusNetworkFailure OnNetworkFailure;

private:
	void LogOnlineSubsystemStatus();
};
