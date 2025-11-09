#include "Blueprint/NexusOnlineBlueprintLibrary.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Utils/NexusOnlineHelpers.h"

bool UNexusOnlineBlueprintLibrary::GetSessionPlayerCounts(UObject* WorldContextObject, ENexusSessionType SessionType, int32& CurrentPlayers, int32& MaxPlayers)
{
        CurrentPlayers = 0;
        MaxPlayers = 0;

        if (!IsValid(WorldContextObject))
        {
                UE_LOG(LogTemp, Warning, TEXT("[NexusOnline] GetSessionPlayerCounts called with invalid WorldContextObject."));
                return false;
        }

        UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
        if (!World)
        {
                UE_LOG(LogTemp, Warning, TEXT("[NexusOnline] GetSessionPlayerCounts could not resolve world."));
                return false;
        }

        IOnlineSessionPtr SessionInterface = NexusOnline::GetSessionInterface(World);
        if (!SessionInterface.IsValid())
        {
                UE_LOG(LogTemp, Warning, TEXT("[NexusOnline] GetSessionPlayerCounts could not access session interface."));
                return false;
        }

        const FName SessionName = NexusOnline::SessionTypeToName(SessionType);
        if (FNamedOnlineSession* NamedSession = SessionInterface->GetNamedSession(SessionName))
        {
                MaxPlayers = NamedSession->SessionSettings.NumPublicConnections;

                const int32 OccupiedFromConnections = MaxPlayers - NamedSession->NumOpenPublicConnections;
                const int32 OccupiedFromRegistrations = NamedSession->RegisteredPlayers.Num();

                if (OccupiedFromRegistrations > 0)
                {
                        CurrentPlayers = OccupiedFromRegistrations;
                }
                else
                {
                        CurrentPlayers = OccupiedFromConnections;
                }

                CurrentPlayers = FMath::Clamp(CurrentPlayers, 0, MaxPlayers > 0 ? MaxPlayers : CurrentPlayers);
                return true;
        }

        UE_LOG(LogTemp, Verbose, TEXT("[NexusOnline] GetSessionPlayerCounts did not find an active session named '%s'."), *SessionName.ToString());
        return false;
}
