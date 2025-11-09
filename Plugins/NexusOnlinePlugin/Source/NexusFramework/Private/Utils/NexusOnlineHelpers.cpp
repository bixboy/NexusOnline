// Copyright Nexus Online

#include "Utils/NexusOnlineHelpers.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

DEFINE_LOG_CATEGORY(LogNexusOnline);

namespace NexusOnline
{
namespace
{
struct FSessionMonitor
{
TWeakObjectPtr<UWorld> World;
FDelegateHandle RegisterPlayersHandle;
FDelegateHandle UnregisterPlayersHandle;
};

static TArray<FSessionMonitor> GSessionMonitors;
static FOnNexusSessionPlayersChanged GSessionPlayersChanged;

FSessionMonitor* FindMonitor(UWorld* World)
{
for (FSessionMonitor& Monitor : GSessionMonitors)
{
if (Monitor.World.Get() == World)
{
return &Monitor;
}
}

return nullptr;
}

void PruneInvalidMonitors()
{
GSessionMonitors.RemoveAllSwap([](const FSessionMonitor& Monitor)
{
return !Monitor.World.IsValid();
});
}
}

FOnNexusSessionPlayersChanged& OnSessionPlayersChanged()
{
return GSessionPlayersChanged;
}

bool ResolveSessionPlayerCounts(UWorld* World, ENexusSessionType SessionType, int32& OutCurrentPlayers, int32& OutMaxPlayers, FName& OutSessionName)
{
OutCurrentPlayers = 0;
OutMaxPlayers = 0;
OutSessionName = SessionTypeToName(SessionType);

if (!IsValid(World))
{
UE_LOG(LogNexusOnline, Verbose, TEXT("ResolveSessionPlayerCounts failed: world is invalid."));
return false;
}

const IOnlineSessionPtr SessionInterface = GetSessionInterface(World);
if (!SessionInterface.IsValid())
{
UE_LOG(LogNexusOnline, Verbose, TEXT("ResolveSessionPlayerCounts failed: session interface unavailable."));
return false;
}

const FName InternalSessionName = SessionTypeToName(SessionType);
FNamedOnlineSession* NamedSession = SessionInterface->GetNamedSession(InternalSessionName);
if (!NamedSession)
{
UE_LOG(LogNexusOnline, Verbose, TEXT("ResolveSessionPlayerCounts: no active session named '%s'."), *InternalSessionName.ToString());
return false;
}

OutSessionName = NamedSession->SessionName;

const int32 SessionMax = NamedSession->Session.SessionSettings.NumPublicConnections;
const int32 LegacyMax = NamedSession->SessionSettings.NumPublicConnections;
OutMaxPlayers = SessionMax > 0 ? SessionMax : LegacyMax;

const int32 OccupiedFromSession = SessionMax > 0 ? (SessionMax - NamedSession->Session.NumOpenPublicConnections) : 0;
const int32 OccupiedFromLegacy = LegacyMax > 0 ? (LegacyMax - NamedSession->NumOpenPublicConnections) : 0;
const int32 OccupiedFromRegistrations = NamedSession->RegisteredPlayers.Num();

int32 ObservedPlayers = FMath::Max(OccupiedFromSession, OccupiedFromLegacy);
ObservedPlayers = FMath::Max(ObservedPlayers, OccupiedFromRegistrations);

if (AGameStateBase* GameState = World->GetGameState())
{
ObservedPlayers = FMath::Max(ObservedPlayers, GameState->PlayerArray.Num());

if (OutMaxPlayers <= 0 && GameState->PlayerArray.Num() > 0)
{
OutMaxPlayers = GameState->PlayerArray.Num();
}
}

OutMaxPlayers = FMath::Max(OutMaxPlayers, OccupiedFromRegistrations);
OutCurrentPlayers = (OutMaxPlayers > 0)
? FMath::Clamp(ObservedPlayers, 0, OutMaxPlayers)
: ObservedPlayers;

return true;
}

void BroadcastSessionPlayersChanged(UWorld* World, ENexusSessionType SessionType)
{
int32 CurrentPlayers = 0;
int32 MaxPlayers = 0;
FName SessionName = SessionTypeToName(SessionType);
const bool bHasActiveSession = ResolveSessionPlayerCounts(World, SessionType, CurrentPlayers, MaxPlayers, SessionName);

if (!bHasActiveSession)
{
SessionName = NAME_None;
}

const FString SessionLabel = SessionName.IsNone() ? FString(TEXT("<None>")) : SessionName.ToString();

UE_LOG(LogNexusOnline, Log, TEXT("Session players update: %s (%d/%d, Active=%s)"),
*SessionLabel, CurrentPlayers, MaxPlayers, bHasActiveSession ? TEXT("true") : TEXT("false"));

GSessionPlayersChanged.Broadcast(World, SessionType, SessionName, CurrentPlayers, MaxPlayers);
}

void BeginTrackingSession(UWorld* World)
{
if (!IsValid(World))
{
return;
}

PruneInvalidMonitors();

IOnlineSessionPtr SessionInterface = GetSessionInterface(World);
if (!SessionInterface.IsValid())
{
return;
}

FSessionMonitor* Monitor = FindMonitor(World);
if (!Monitor)
{
Monitor = &GSessionMonitors.AddDefaulted_GetRef();
Monitor->World = World;
}

TWeakObjectPtr<UWorld> WeakWorld = World;

if (!Monitor->RegisterPlayersHandle.IsValid())
{
Monitor->RegisterPlayersHandle = SessionInterface->AddOnRegisterPlayersCompleteDelegate_Handle(
FOnRegisterPlayersCompleteDelegate::CreateLambda([WeakWorld](FName SessionName, const TArray<FUniqueNetIdRef>&, bool)
{
if (UWorld* PinnedWorld = WeakWorld.Get())
{
BroadcastSessionPlayersChanged(PinnedWorld, NameToSessionType(SessionName));
}
})
);
}

if (!Monitor->UnregisterPlayersHandle.IsValid())
{
Monitor->UnregisterPlayersHandle = SessionInterface->AddOnUnregisterPlayersCompleteDelegate_Handle(
FOnUnregisterPlayersCompleteDelegate::CreateLambda([WeakWorld](FName SessionName, const TArray<FUniqueNetIdRef>&, bool)
{
if (UWorld* PinnedWorld = WeakWorld.Get())
{
BroadcastSessionPlayersChanged(PinnedWorld, NameToSessionType(SessionName));
}
})
);
}
}

void StopTrackingSession(UWorld* World)
{
if (!IsValid(World))
{
return;
}

IOnlineSessionPtr SessionInterface = GetSessionInterface(World);
if (FSessionMonitor* Monitor = FindMonitor(World))
{
if (SessionInterface.IsValid())
{
if (Monitor->RegisterPlayersHandle.IsValid())
{
SessionInterface->ClearOnRegisterPlayersCompleteDelegate_Handle(Monitor->RegisterPlayersHandle);
Monitor->RegisterPlayersHandle = FDelegateHandle();
}

if (Monitor->UnregisterPlayersHandle.IsValid())
{
SessionInterface->ClearOnUnregisterPlayersCompleteDelegate_Handle(Monitor->UnregisterPlayersHandle);
Monitor->UnregisterPlayersHandle = FDelegateHandle();
}
}

Monitor->World.Reset();
}

PruneInvalidMonitors();
}
}
