#include "NexusFramework.h"

#define LOCTEXT_NAMESPACE "FNexusFrameworkModule"

void FNexusFrameworkModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("[NexusOnlineFramework] Module started"));
}

void FNexusFrameworkModule::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("[NexusOnlineFramework] Module shutdown"));
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FNexusFrameworkModule, NexusFramework)