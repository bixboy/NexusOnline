#pragma once
#include "CoreMinimal.h"
#include "NetworkToolTypes.h"
#include "NetworkToolSettings.generated.h"


UCLASS(Config = EditorPerProjectUserSettings, DefaultConfig, meta = (DisplayName = "Network Tool Config"), Category = "NexusLinker")
class NEXUSLINKER_API UNetworkToolSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// Liste extensible de tes serveurs
	UPROPERTY(Config, EditAnywhere, Category = "Profiles")
	TArray<FServerProfile> ServerProfiles;

	// Liste extensible de tes comptes auto-login
	UPROPERTY(Config, EditAnywhere, Category = "Profiles")
	TArray<FString> AutoLoginAccounts;
    
	// Liste extensible des presets de lag
	UPROPERTY(Config, EditAnywhere, Category = "Simulation")
	TArray<FNetworkEmulationProfile> LagPresets;
};