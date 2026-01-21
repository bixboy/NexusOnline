#pragma once
#include "CoreMinimal.h"
#include "NetworkToolTypes.generated.h"


// Type de serveur
UENUM(BlueprintType)
enum class EServerEnvironment : uint8
{
	LocalHost,      // 127.0.0.1
	Lan,            // Réseau local
	Remote_OVH,     // Serveur dédié distant
	Remote_AWS      // Cloud
};

// Profil d'un serveur
USTRUCT(BlueprintType)
struct FServerProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Config")
	FString ProfileName;

	UPROPERTY(EditAnywhere, Category = "Config")
	EServerEnvironment EnvType;

	UPROPERTY(EditAnywhere, Category = "Network")
	FString IPAddress;

	UPROPERTY(EditAnywhere, Category = "Network")
	int32 Port = 7777;

	UPROPERTY(EditAnywhere, Category = "Config")
	FString MapName;

	UPROPERTY(EditAnywhere, Category = "Config")
	FString ExtraArgs;

	// Pour l'API HTTP (Start/Stop)
	UPROPERTY(EditAnywhere, Category = "Remote Control")
	FString ApiUrl_Start; 

	UPROPERTY(EditAnywhere, Category = "Remote Control")
	FString ApiUrl_Stop;
    
	UPROPERTY(EditAnywhere, Category = "Remote Control")
	FString AuthToken;
};

// Profil de simulation réseau
USTRUCT(BlueprintType)
struct FNetworkEmulationProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FString Name;

	UPROPERTY(EditAnywhere)
	int32 PktLag = 0; // en ms

	UPROPERTY(EditAnywhere)
	int32 PktLoss = 0; // en %
};