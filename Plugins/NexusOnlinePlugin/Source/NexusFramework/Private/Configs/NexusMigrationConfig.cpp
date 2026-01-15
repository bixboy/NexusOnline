#include "Configs/NexusMigrationConfig.h"

UNexusMigrationConfig::UNexusMigrationConfig()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("Nexus Migration");

	bEnableMigration = true;
	MaxMigrationRetries = 15;
	ClientSearchInterval = 5.0f;
    ClientRetryDelay = 3.0f;
    JoinFailureDelay = 2.0f;
}
