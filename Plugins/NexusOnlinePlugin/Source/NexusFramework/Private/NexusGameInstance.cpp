#include "NexusGameInstance.h"
#include "NexusOnlineSession.h"
#include "Subsystems/NexusMigrationSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Configs/NexusMigrationConfig.h"
#include "Kismet/GameplayStatics.h"


void UNexusGameInstance::Init()
{
	Super::Init();

	UE_LOG(LogTemp, Display, TEXT("[GameInstance] Init called."));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("✅ GameInstance initialized"));
	
	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
		GEngine->OnNetworkFailure().AddUObject(this, &UNexusGameInstance::HandleNetworkFailure);
	}

	LogOnlineSubsystemStatus();
}

void UNexusGameInstance::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	if (OnNetworkFailure.IsBound())
	{
		OnNetworkFailure.Broadcast(World, NetDriver, FailureType, ErrorString);
	}

	if (UNexusMigrationSubsystem* MigSub = GetSubsystem<UNexusMigrationSubsystem>())
	{
		if (MigSub->ShouldSuppressNetworkFailure(World, NetDriver, FailureType, ErrorString))
		{
			UE_LOG(LogTemp, Warning, TEXT("[NexusGameInstance] Suppressing Network Failure Handling due to active Host Migration/Recovery rules. Error: %s"), *ErrorString);
			return;
		}
	}
	
	UE_LOG(LogTemp, Error, TEXT("[NexusGameInstance] Network Failure: %s. Returning to MainMenu..."), *ErrorString);
	
	if (World)
	{
		FName TargetMap = FName("MainMenu");
		if (const UNexusMigrationConfig* Config = GetDefault<UNexusMigrationConfig>())
		{
			if (!Config->MainMenuMap.IsEmpty())
			{
				TargetMap = FName(*Config->MainMenuMap);
			}
		}
		UGameplayStatics::OpenLevel(World, TargetMap);
	}
}

TSubclassOf<UOnlineSession> UNexusGameInstance::GetOnlineSessionClass()
{
	return UNexusOnlineSession::StaticClass();
}

void UNexusGameInstance::LogOnlineSubsystemStatus()
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ No OnlineSubsystem loaded!"));
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("❌ No OnlineSubsystem loaded"));
		return;
	}

	FString SubsystemName = Subsystem->GetSubsystemName().ToString();
	UE_LOG(LogTemp, Display, TEXT("✅ OnlineSubsystem: %s"), *SubsystemName);
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan, FString::Printf(TEXT("✅ Subsystem: %s"), *SubsystemName));

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ Session interface is NULL for subsystem %s"), *SubsystemName);
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, TEXT("⚠️ SessionInterface is null"));
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("✅ Session interface valid for subsystem %s"), *SubsystemName);
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("✅ SessionInterface valid"));
	}
}

