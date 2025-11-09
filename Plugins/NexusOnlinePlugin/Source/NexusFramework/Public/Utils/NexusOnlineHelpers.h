#pragma once

#include "CoreMinimal.h"
#include "Delegates/DelegateCombinations.h"
#include "OnlineSubsystemUtils.h"
#include "Types/OnlineSessionData.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNexusOnline, Log, All);

class UWorld;

namespace NexusOnline
{
/** Diffusion globale lorsqu'un changement de joueurs est détecté. */
DECLARE_MULTICAST_DELEGATE_FiveParams(FOnNexusSessionPlayersChanged, UWorld* /*World*/, ENexusSessionType /*SessionType*/, FName /*SessionName*/, int32 /*CurrentPlayers*/, int32 /*MaxPlayers*/);

/** Conversion d’un type de session (enum) vers son FName interne. */
static FORCEINLINE FName SessionTypeToName(ENexusSessionType Type)
{
switch (Type)
{
case ENexusSessionType::PartySession:
return FName(TEXT("PartySession"));

case ENexusSessionType::SpectatorSession:
return FName(TEXT("SpectatorSession"));

case ENexusSessionType::CustomSession:
return FName(TEXT("CustomSession"));

default:
return FName(TEXT("GameSession"));
}
}

/** Conversion inverse FName → enum. */
static FORCEINLINE ENexusSessionType NameToSessionType(const FName& Name)
{
if (Name == TEXT("PartySession"))
{
return ENexusSessionType::PartySession;
}

if (Name == TEXT("SpectatorSession"))
{
return ENexusSessionType::SpectatorSession;
}

if (Name == TEXT("CustomSession"))
{
return ENexusSessionType::CustomSession;
}

return ENexusSessionType::GameSession;
}

/** Check si le subsystem est disponible. */
static FORCEINLINE bool IsSubsystemAvailable(UWorld* World)
{
return (Online::GetSubsystem(World) != nullptr);
}

/** Retourne l'interface session. */
static FORCEINLINE IOnlineSessionPtr GetSessionInterface(UWorld* World)
{
if (IOnlineSubsystem* Subsystem = Online::GetSubsystem(World))
{
return Subsystem->GetSessionInterface();
}

return nullptr;
}

/** Accès à l'événement global. */
NEXUSFRAMEWORK_API FOnNexusSessionPlayersChanged& OnSessionPlayersChanged();

/** Récupère les compteurs de joueurs actuels et max. */
NEXUSFRAMEWORK_API bool ResolveSessionPlayerCounts(UWorld* World, ENexusSessionType SessionType, int32& OutCurrentPlayers, int32& OutMaxPlayers, FName& OutSessionName);

/** Diffuse l'état courant auprès des abonnés. */
NEXUSFRAMEWORK_API void BroadcastSessionPlayersChanged(UWorld* World, ENexusSessionType SessionType);

/** Active la surveillance des changements de session (join/leave). */
NEXUSFRAMEWORK_API void BeginTrackingSession(UWorld* World);

/** Désactive la surveillance des changements de session. */
NEXUSFRAMEWORK_API void StopTrackingSession(UWorld* World);
}
