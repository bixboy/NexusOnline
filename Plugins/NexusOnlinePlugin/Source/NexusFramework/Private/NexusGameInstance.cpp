#include "NexusGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"


void UNexusGameInstance::Init()
{
	Super::Init();

	UE_LOG(LogTemp, Display, TEXT("[GameInstance] Init called."));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("✅ GameInstance initialized"));

	LogOnlineSubsystemStatus();
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
