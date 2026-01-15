#include "Subsystems/NexusMigrationSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "Async/AsyncTask_CreateSession.h"
#include "Async/AsyncTask_FindSessions.h"
#include "Async/AsyncTask_JoinSession.h"
#include "Utils/NexusOnlineHelpers.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Configs/NexusMigrationConfig.h"


void UNexusMigrationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &UNexusMigrationSubsystem::OnNetworkFailure);
	}
}

void UNexusMigrationSubsystem::Deinitialize()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
	}
	
	Super::Deinitialize();
}

void UNexusMigrationSubsystem::CacheSessionSettings(const FSessionSettingsData& InSettings)
{
	CachedSessionSettings = InSettings;
	if (CachedSessionSettings.MigrationSessionID.IsEmpty())
	{
		CachedSessionSettings.MigrationSessionID = NexusOnline::GenerateRandomSessionId(16);
	}
	
	UE_LOG(LogTemp, Log, TEXT("[NexusMigration] Session cached. MigrationID: %s"), *CachedSessionSettings.MigrationSessionID);
}

void UNexusMigrationSubsystem::OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	const UNexusMigrationConfig* Config = GetDefault<UNexusMigrationConfig>();
	if (!Config || !Config->bEnableMigration)
		return;

	if (!CachedSessionSettings.bAllowHostMigration)
		return;
	
	if (bIsMigrating)
		return; 

	if (bIntentionalLeave)
	{
		UE_LOG(LogTemp, Log, TEXT("[NexusMigration] Network Failure ignored due to Intentional Leave."));
		return;
	}

	if (FailureType == ENetworkFailure::ConnectionLost || FailureType == ENetworkFailure::ConnectionTimeout)
	{
		if (World && World->GetNetMode() == NM_Client)
		{
			UE_LOG(LogTemp, Warning, TEXT("[NexusMigration] Network Failure detected. Attempting Migration..."));
			bIsMigrating = true;
			HandleSessionRecovery();
		}
	}
}

void UNexusMigrationSubsystem::HandleSessionRecovery()
{
	bIntentionalLeave = false; // Reset for safety

	if (CachedNextHostId.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[NexusMigration] No Heir elected. Returning to menu."));
		bIsMigrating = false;
		return;
	}

	FString MyId;
	if (IOnlineIdentityPtr Identity = NexusOnline::GetIdentityInterface(GetWorld()))
	{
		if (TSharedPtr<const FUniqueNetId> LocalId = Identity->GetUniquePlayerId(0))
		{
			MyId = LocalId->ToString();
		}
	}

	if (MyId == CachedNextHostId)
	{
		UE_LOG(LogTemp, Log, TEXT("[NexusMigration] I AM THE NEW HOST! Starting recovery..."));
		StartHostRecovery();
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[NexusMigration] Waiting for new host (%s)..."), *CachedNextHostId);
		StartClientRecovery();
	}
}

// ──────────────────────────────────────────────
// HOST RECOVERY
// ──────────────────────────────────────────────
void UNexusMigrationSubsystem::StartHostRecovery()
{
	IOnlineSessionPtr SessionInt = NexusOnline::GetSessionInterface(GetWorld());
	if (SessionInt.IsValid())
	{
		SessionInt->DestroySession(NexusOnline::SessionTypeToName(CachedSessionSettings.SessionType));
	}
	
	FSessionSearchFilter MigrationFilter;
	MigrationFilter.Key = FName("MIGRATION_ID_KEY");
	MigrationFilter.Value.Type = ENexusSessionFilterValueType::String;
	MigrationFilter.Value.StringValue = CachedSessionSettings.MigrationSessionID;

	TArray<FSessionSearchFilter> ExtraSettings;
	ExtraSettings.Add(MigrationFilter);

	UE_LOG(LogTemp, Log, TEXT("[NexusMigration] StartHostRecovery. Map: %s, ID: %s"), *CachedSessionSettings.MapName, *CachedSessionSettings.MigrationSessionID);

	CurrentCreateTask = UAsyncTask_CreateSession::CreateSession(GetWorld(), CachedSessionSettings, ExtraSettings, true, nullptr);

	if (CurrentCreateTask)
	{
		CurrentCreateTask->OnSuccess.AddDynamic(this, &UNexusMigrationSubsystem::OnRecoveryCreateSuccess);
		CurrentCreateTask->Activate();
	}
}

void UNexusMigrationSubsystem::OnRecoveryCreateSuccess()
{
	UE_LOG(LogTemp, Log, TEXT("[NexusMigration] Host Recovery Successful. Session recreated."));
	bIsMigrating = false;
	CurrentCreateTask = nullptr;
}

// ──────────────────────────────────────────────
// CLIENT RECOVERY
// ──────────────────────────────────────────────
void UNexusMigrationSubsystem::StartClientRecovery()
{
	MigrationRetries = 0;

	float SearchInterval = 5.0f;
	if (const UNexusMigrationConfig* Config = GetDefault<UNexusMigrationConfig>())
	{
		SearchInterval = Config->ClientSearchInterval;
	}

	FTimerHandle RetryHandle;
	GetWorld()->GetTimerManager().SetTimer(RetryHandle, this, &UNexusMigrationSubsystem::PerformClientSearch, SearchInterval, false);
}

void UNexusMigrationSubsystem::PerformClientSearch()
{
	FSessionSearchFilter MigrationFilter;
	MigrationFilter.Key = FName("MIGRATION_ID_KEY");
	MigrationFilter.Value.Type = ENexusSessionFilterValueType::String;
	MigrationFilter.Value.StringValue = CachedSessionSettings.MigrationSessionID;
	MigrationFilter.ComparisonOp = ENexusSessionComparisonOp::Equals;

	TArray<FSessionSearchFilter> SimpleFilters;
	SimpleFilters.Add(MigrationFilter);

	CurrentFindTask = UAsyncTask_FindSessions::FindSessions(
		GetWorld(), 
		CachedSessionSettings.SessionType, 
		20, 
		false, 
		SimpleFilters,
		{}, {}, nullptr
	);

	if (CurrentFindTask)
	{
		CurrentFindTask->OnCompleted.AddDynamic(this, &UNexusMigrationSubsystem::OnRecoveryFindComplete);
		CurrentFindTask->Activate();
	}
}

void UNexusMigrationSubsystem::OnRecoveryFindComplete(bool bWasSuccessful, const TArray<FOnlineSessionSearchResultData>& Results)
{
	const UNexusMigrationConfig* Config = GetDefault<UNexusMigrationConfig>();
	const int32 MaxRetries = Config ? Config->MaxMigrationRetries : 15;
	const float RetryDelay = Config ? Config->ClientRetryDelay : 3.0f;

	if (bWasSuccessful && Results.Num() > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[NexusMigration] FOUND MATCH! Joining..."));

		CurrentJoinTask = UAsyncTask_JoinSession::JoinSession(GetWorld(), Results[0], true, CachedSessionSettings.SessionType);

		if (CurrentJoinTask)
		{
			CurrentJoinTask->OnSuccess.AddDynamic(this, &UNexusMigrationSubsystem::OnRecoveryJoinSuccess);
			CurrentJoinTask->OnFailure.AddDynamic(this, &UNexusMigrationSubsystem::OnRecoveryJoinFailure);
			CurrentJoinTask->Activate();
		}
	}
	else
	{
		MigrationRetries++;
		// Use Config Variable
		if (MigrationRetries >= MaxRetries)
		{
			UE_LOG(LogTemp, Error, TEXT("[NexusMigration] Timeout after %d attempts."), MigrationRetries);
			bIsMigrating = false;
			OnMigrationFailed.Broadcast(TEXT("Migration Timed Out"));
			UGameplayStatics::OpenLevel(GetWorld(), FName("MainMenu"));
			return;
		}

		UE_LOG(LogTemp, Warning, TEXT("[NexusMigration] Match not found (%d/%d). Retrying in %.1fs..."), MigrationRetries, MaxRetries, RetryDelay);
		
		FTimerHandle RetryHandle;
		GetWorld()->GetTimerManager().SetTimer(RetryHandle, this, &UNexusMigrationSubsystem::PerformClientSearch, RetryDelay, false);
	}
}

void UNexusMigrationSubsystem::OnRecoveryJoinSuccess()
{
	UE_LOG(LogTemp, Log, TEXT("[NexusMigration] Client Recovery Successful. Joined new host."));
	bIsMigrating = false;
	CurrentJoinTask = nullptr;
}

void UNexusMigrationSubsystem::OnRecoveryJoinFailure()
{
	UE_LOG(LogTemp, Error, TEXT("[NexusMigration] Failed to join recovered session. Retrying..."));
	
	float FailureDelay = 2.0f;
	if (const UNexusMigrationConfig* Config = GetDefault<UNexusMigrationConfig>())
	{
		FailureDelay = Config->JoinFailureDelay;
	}

	FTimerHandle RetryHandle;
	GetWorld()->GetTimerManager().SetTimer(RetryHandle, this, &UNexusMigrationSubsystem::PerformClientSearch, FailureDelay, false);
}