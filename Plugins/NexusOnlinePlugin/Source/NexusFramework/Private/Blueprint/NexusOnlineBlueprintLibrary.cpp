#include "Blueprint/NexusOnlineBlueprintLibrary.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Utils/NexusOnlineHelpers.h"

bool UNexusOnlineBlueprintLibrary::GetSessionPlayerCounts(UObject* WorldContextObject, ENexusSessionType SessionType, int32& CurrentPlayers, int32& MaxPlayers)
{
        CurrentPlayers = 0;
        MaxPlayers = 0;

        if (!IsValid(WorldContextObject))
        {
                UE_LOG(LogNexusOnline, Warning, TEXT("GetSessionPlayerCounts called with invalid WorldContextObject."));
                return false;
        }

        UWorld* World = WorldContextObject->GetWorld();
        if (!World && GEngine)
        {
                World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
        }

        if (!World)
        {
                UE_LOG(LogNexusOnline, Warning, TEXT("GetSessionPlayerCounts could not resolve a valid world."));
                return false;
        }

        FName SessionName = NAME_None;
        const bool bHasSession = NexusOnline::ResolveSessionPlayerCounts(World, SessionType, CurrentPlayers, MaxPlayers, SessionName);

        if (!bHasSession)
        {
                UE_LOG(LogNexusOnline, Verbose, TEXT("GetSessionPlayerCounts: no active session for '%s'."), *NexusOnline::SessionTypeToName(SessionType).ToString());
        }

        return bHasSession;
}
