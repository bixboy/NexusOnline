#pragma once
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "NexusMigrationConfig.generated.h"


UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Nexus Migration"))
class NEXUSFRAMEWORK_API UNexusMigrationConfig : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UNexusMigrationConfig();

    UPROPERTY(Config, EditAnywhere, Category="General")
    bool bEnableMigration;

    UPROPERTY(Config, EditAnywhere, Category="Recovery", meta=(ClampMin=1, ClampMax=50))
    int32 MaxMigrationRetries;

    UPROPERTY(Config, EditAnywhere, Category="Recovery", meta=(ClampMin=1.0f))
    float ClientSearchInterval;

    UPROPERTY(Config, EditAnywhere, Category="Recovery", meta=(ClampMin=1.0f))
    float ClientRetryDelay;

    UPROPERTY(Config, EditAnywhere, Category="Recovery", meta=(ClampMin=1.0f))
    float JoinFailureDelay;
};
