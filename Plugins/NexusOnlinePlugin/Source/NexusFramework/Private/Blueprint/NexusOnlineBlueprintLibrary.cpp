#include "Blueprint/NexusOnlineBlueprintLibrary.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
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
                const int32 ConnectionsMax = NamedSession->Session.SessionSettings.NumPublicConnections;
                const int32 LegacyMax = NamedSession->SessionSettings.NumPublicConnections;
                MaxPlayers = ConnectionsMax > 0 ? ConnectionsMax : LegacyMax;

                const int32 OccupiedFromSessionConnections = ConnectionsMax > 0
                        ? (ConnectionsMax - NamedSession->Session.NumOpenPublicConnections)
                        : 0;
                const int32 OccupiedFromLegacyConnections = LegacyMax > 0
                        ? (LegacyMax - NamedSession->NumOpenPublicConnections)
                        : 0;
                const int32 OccupiedFromRegistrations = NamedSession->RegisteredPlayers.Num();

                int32 ObservedPlayers = FMath::Max(OccupiedFromSessionConnections, OccupiedFromLegacyConnections);
                ObservedPlayers = FMath::Max(ObservedPlayers, OccupiedFromRegistrations);

                if (AGameStateBase* GameState = World->GetGameState())
                {
                        ObservedPlayers = FMath::Max(ObservedPlayers, GameState->PlayerArray.Num());
                        if (MaxPlayers <= 0 && GameState->PlayerArray.Num() > 0)
                        {
                                MaxPlayers = FMath::Max(MaxPlayers, ObservedPlayers);
                        }
                }

                CurrentPlayers = ObservedPlayers;
                CurrentPlayers = FMath::Clamp(CurrentPlayers, 0, MaxPlayers > 0 ? MaxPlayers : CurrentPlayers);
                return true;
        }

        UE_LOG(LogTemp, Verbose, TEXT("[NexusOnline] GetSessionPlayerCounts did not find an active session named '%s'."), *SessionName.ToString());
        return false;
}
