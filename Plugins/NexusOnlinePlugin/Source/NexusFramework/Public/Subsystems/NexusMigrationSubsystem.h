#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Types/OnlineSessionData.h"
#include "NexusMigrationSubsystem.generated.h"

class UAsyncTask_CreateSession;
class UAsyncTask_FindSessions;
class UAsyncTask_JoinSession;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMigrationStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMigrationFailed, FString, Reason);

UCLASS()
class NEXUSFRAMEWORK_API UNexusMigrationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="Nexus|Migration")
	void CacheSessionSettings(const FSessionSettingsData& InSettings);

	void SetCachedNextHostId(const FString& NewHeirId) { CachedNextHostId = NewHeirId; }

	UFUNCTION(BlueprintPure, Category="Nexus|Migration")
	bool IsMigrating() const { return bIsMigrating; }

	UPROPERTY(BlueprintAssignable, Category="Nexus|Migration")
	FOnMigrationStarted OnMigrationStarted;

	UPROPERTY(BlueprintAssignable, Category="Nexus|Migration")
	FOnMigrationFailed OnMigrationFailed;

protected:
	UPROPERTY(Transient)
	FSessionSettingsData CachedSessionSettings;

	FString CachedNextHostId;
	bool bIsMigrating = false;
	bool bIntentionalLeave = false;

public:
	UFUNCTION(BlueprintCallable, Category="Nexus|Migration")
	void SetIntentionalLeave(bool bEnable) { bIntentionalLeave = bEnable; }

protected:

	int32 MigrationRetries = 0;

	UPROPERTY()
	UAsyncTask_CreateSession* CurrentCreateTask;

	UPROPERTY()
	UAsyncTask_FindSessions* CurrentFindTask;

	UPROPERTY()
	UAsyncTask_JoinSession* CurrentJoinTask;

	void OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleSessionRecovery();
	void StartHostRecovery();
	
	void StartClientRecovery();
	void PerformClientSearch();

	// Callbacks
	UFUNCTION() void OnRecoveryCreateSuccess();
	UFUNCTION() void OnRecoveryFindComplete(bool bWasSuccessful, const TArray<struct FOnlineSessionSearchResultData>& Results);
	UFUNCTION() void OnRecoveryJoinSuccess();
	UFUNCTION() void OnRecoveryJoinFailure();
};