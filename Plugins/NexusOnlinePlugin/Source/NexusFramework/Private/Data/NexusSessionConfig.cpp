#include "Data/NexusSessionConfig.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemTypes.h"
#include "Utils/NexusOnlineHelpers.h"

void UNexusSessionConfig::ApplyToOnlineSettings(FOnlineSessionSettings& OutSettings) const
{
	// 1. Basic Settings mapping
	OutSettings.NumPublicConnections = SessionSettings.bIsPrivate ? 0 : SessionSettings.MaxPlayers;
	OutSettings.NumPrivateConnections = SessionSettings.bIsPrivate ? SessionSettings.MaxPlayers : 0;
	OutSettings.bIsLANMatch = SessionSettings.bIsLAN;
	OutSettings.bUsesPresence = SessionSettings.bUsesPresence;
	OutSettings.bAllowJoinInProgress = SessionSettings.bAllowJoinInProgress;
	OutSettings.bAllowInvites = SessionSettings.bAllowInvites;
	OutSettings.bAntiCheatProtected = SessionSettings.bUseAntiCheat;
	OutSettings.bIsDedicated = SessionSettings.bIsDedicated;
	OutSettings.bShouldAdvertise = !SessionSettings.bIsPrivate;
	OutSettings.bAllowJoinViaPresence = SessionSettings.bUsesPresence;
	OutSettings.bAllowJoinViaPresenceFriendsOnly = SessionSettings.bFriendsOnly;

	// 2. Apply Metadata (Map, Mode, Name...)
	const auto AdvertisementType = SessionSettings.bIsLAN ? EOnlineDataAdvertisementType::ViaOnlineService : EOnlineDataAdvertisementType::ViaOnlineServiceAndPing;

	OutSettings.Set(TEXT("SESSION_DISPLAY_NAME"), SessionSettings.SessionName, AdvertisementType);
	OutSettings.Set(TEXT("MAP_NAME_KEY"), SessionSettings.MapName, AdvertisementType);
	OutSettings.Set(TEXT("GAME_MODE_KEY"), SessionSettings.GameMode, AdvertisementType);
	
	// Convert Enum to String for generic storage
	OutSettings.Set(TEXT("SESSION_TYPE_KEY"), NexusOnline::SessionTypeToName(SessionSettings.SessionType).ToString(), AdvertisementType);

	// ID Logic (if needed)
	if (!SessionSettings.SessionId.IsEmpty())
	{
		OutSettings.Set(TEXT("SESSION_ID_KEY"), SessionSettings.SessionId, AdvertisementType);
	}

	// 3. Apply Preset Metadata if available
	if (SearchMetadata)
	{
		// We use the preset to apply *extra* settings or tags that might be defined there
		// Currently NexusSessionFilterUtils::ApplyFiltersToSettings is mainly for Search, 
		// but we could extend it to apply "Default Metadata" to the session itself.
		// For now, let's just leave it as a hook for future expansion.
	}
}

void UNexusSessionConfig::ModifySessionSettings(FOnlineSessionSettings& OutSettings) const
{
	// Base implementation applies the struct data.
	ApplyToOnlineSettings(OutSettings);
}
