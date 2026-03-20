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

	/** Returns true if the network failure should be ignored (e.g. during migration). */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Migration")
	bool ShouldSuppressNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	UPROPERTY(BlueprintAssignable, Category="Nexus|Migration")
	FOnMigrationStarted OnMigrationStarted;

	UPROPERTY(BlueprintAssignable, Category="Nexus|Migration")
	FOnMigrationFailed OnMigrationFailed;

protected:
	UPROPERTY(Transient)
	FSessionSettingsData CachedSessionSettings;

	FString CachedNextHostId;
	bool bIsMigrating = false;
	bool bRecoveringHost = false; // Pending Host Recovery State
	bool bIntentionalLeave = false;
	int32 MigrationGeneration = 0;

public:
	UFUNCTION(BlueprintCallable, Category="Nexus|Migration")
	void SetIntentionalLeave(bool bEnable) { bIntentionalLeave = bEnable; }

protected:

	int32 MigrationRetries = 0;

	FString GetEffectiveMigrationId() const;
    
    // Timestamp of the last migration failure (Travel or Network) to prevent loops
    double LastMigrationFailureTime = 0.0;
    
	UPROPERTY()
	UAsyncTask_CreateSession* CurrentCreateTask;

	UPROPERTY()
	UAsyncTask_FindSessions* CurrentFindTask;

	UPROPERTY()
	UAsyncTask_JoinSession* CurrentJoinTask;

public:
	void OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void OnTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);
	bool HandleNetworkFailureMigration(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleSessionRecovery();
	void StartHostRecovery();
	void FinishHostRecovery(UWorld* World);
	void OnMapLoadComplete(UWorld* World);
	
	void StartClientRecovery();
	void PerformClientSearch();

	// Callbacks
	UFUNCTION() void OnRecoveryCreateSuccess();
	UFUNCTION() void OnRecoveryFindComplete(bool bWasSuccessful, const TArray<struct FOnlineSessionSearchResultData>& Results);
	UFUNCTION() void OnRecoveryJoinSuccess();
	UFUNCTION() void OnRecoveryJoinFailure();
};