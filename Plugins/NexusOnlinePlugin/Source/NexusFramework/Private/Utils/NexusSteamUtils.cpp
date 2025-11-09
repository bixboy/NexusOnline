#include "Utils/NexusSteamUtils.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "Interfaces/OnlinePresenceInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystemTypes.h"

// ────────────────────────────────────────────────
// Log Helper
// ────────────────────────────────────────────────
#ifndef LOG_STEAM
#define LOG_STEAM(Verbosity, Format, ...) UE_LOG(LogTemp, Verbosity, TEXT("[♟ NexusSteam] ") Format, ##__VA_ARGS__)
#endif

// ────────────────────────────────────────────────
// Cache FriendList
// ────────────────────────────────────────────────
namespace
{
	struct FFriendsCacheEntry
	{
		TArray<FSteamFriendInfo> Friends;
		FDateTime LastUpdateUtc = FDateTime::MinValue();
	};

	static TMap<FString, FFriendsCacheEntry> GFriendsCache;

	static FString MakeFriendsCacheKey(FName ListName, int32 UserIndex)
	{
		return FString::Printf(TEXT("%s:%d"), *ListName.ToString(), UserIndex);
	}
}

// ────────────────────────────────────────────────
// Helpers
// ────────────────────────────────────────────────
IOnlineSubsystem* UNexusSteamUtils::GetOSS(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	return Online::GetSubsystem(World);
}

IOnlineIdentityPtr UNexusSteamUtils::GetIdentity(UObject* WorldContextObject)
{
	if (auto* OSS = GetOSS(WorldContextObject))
		return OSS->GetIdentityInterface();
	return nullptr;
}

IOnlineFriendsPtr UNexusSteamUtils::GetFriends(UObject* WorldContextObject)
{
	if (auto* OSS = GetOSS(WorldContextObject))
		return OSS->GetFriendsInterface();
	return nullptr;
}

IOnlineExternalUIPtr UNexusSteamUtils::GetExternalUI(UObject* WorldContextObject)
{
	if (auto* OSS = GetOSS(WorldContextObject))
		return OSS->GetExternalUIInterface();
	return nullptr;
}

IOnlinePresencePtr UNexusSteamUtils::GetPresence(UObject* WorldContextObject)
{
	if (auto* OSS = GetOSS(WorldContextObject))
		return OSS->GetPresenceInterface();
	return nullptr;
}

IOnlineSessionPtr UNexusSteamUtils::GetSession(UObject* WorldContextObject)
{
	if (auto* OSS = GetOSS(WorldContextObject))
		return OSS->GetSessionInterface();
	return nullptr;
}

TSharedPtr<const FUniqueNetId> UNexusSteamUtils::GetLocalUserId(UObject* WorldContextObject, int32 UserIndex)
{
	if (IOnlineIdentityPtr Identity = GetIdentity(WorldContextObject))
		return Identity->GetUniquePlayerId(UserIndex);
	return nullptr;
}

// ────────────────────────────────────────────────
// Friend Info Build
// ────────────────────────────────────────────────
void UNexusSteamUtils::BuildFriendInfo(const TSharedRef<FOnlineFriend>& InFriend, FSteamFriendInfo& OutInfo)
{
	// ✅ Strings sécurisées — GetDisplayName() et GetRealName() peuvent être vides
	const FString SafeDisplay = InFriend->GetDisplayName().IsEmpty()
		? TEXT("[Unknown Name]")
		: InFriend->GetDisplayName();

	const FString SafeReal = InFriend->GetRealName().IsEmpty()
		? TEXT("[Unknown RealName]")
		: InFriend->GetRealName();

	OutInfo.DisplayName = SafeDisplay;
	OutInfo.RealName    = SafeReal;
	OutInfo.UniqueId    = InFriend->GetUserId();

	// ✅ Sécurise aussi la présence
	const FOnlineUserPresence& Presence = InFriend->GetPresence();

#if (ENGINE_MAJOR_VERSION >= 5)
	OutInfo.PresenceStatus = Presence.Status.StatusStr.IsEmpty()
		? TEXT("[No Status]")
		: Presence.Status.StatusStr;
#else
	OutInfo.PresenceStatus = Presence.StatusStr.IsEmpty()
		? TEXT("[No Status]")
		: Presence.StatusStr;
#endif

	OutInfo.bIsOnline   = Presence.bIsOnline;
	OutInfo.bIsPlaying  = Presence.bIsPlaying || Presence.bIsPlayingThisGame;
	OutInfo.bIsJoinable = Presence.bIsJoinable;

#if !(UE_BUILD_SHIPPING)
	FString DebugMsg = FString::Printf(
		TEXT("Friend: %s | Online:%d | Playing:%d | Joinable:%d"),
		*OutInfo.DisplayName,
		OutInfo.bIsOnline,
		OutInfo.bIsPlaying,
		OutInfo.bIsJoinable
	);
	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, DebugMsg);
#endif
}


// ────────────────────────────────────────────────
// Public API
// ────────────────────────────────────────────────
bool UNexusSteamUtils::IsSteamActive(UObject* WorldContextObject)
{
	if (auto* OSS = GetOSS(WorldContextObject))
	{
		const FName Name = OSS->GetSubsystemName();
		if (Name.ToString().Equals(TEXT("STEAM"), ESearchCase::IgnoreCase))
			return true;
		LOG_STEAM(Warning, TEXT("Active subsystem is not Steam (%s)."), *Name.ToString());
	}
	else
	{
		LOG_STEAM(Warning, TEXT("No OnlineSubsystem active."));
	}
	return false;
}

bool UNexusSteamUtils::GetLocalPresence(UObject* WorldContextObject, FSteamPresence& OutPresence, int32 UserIndex)
{
	OutPresence = FSteamPresence{};
	IOnlinePresencePtr Presence = GetPresence(WorldContextObject);
	if (!Presence.IsValid())
		return false;

	if (TSharedPtr<const FUniqueNetId> LocalId = GetLocalUserId(WorldContextObject, UserIndex))
	{
		TSharedPtr<FOnlineUserPresence> RawPresence;
		if (Presence->GetCachedPresence(*LocalId, RawPresence) && RawPresence.IsValid())
		{
#if (ENGINE_MAJOR_VERSION >= 5)
			OutPresence.StatusText = RawPresence->Status.StatusStr;
#else
			OutPresence.StatusText = RawPresence->StatusStr;
#endif
			OutPresence.bIsOnline   = RawPresence->bIsOnline;
			OutPresence.bIsPlaying  = RawPresence->bIsPlaying || RawPresence->bIsPlayingThisGame;
			OutPresence.bIsJoinable = RawPresence->bIsJoinable;
			return true;
		}
	}
	return false;
}

FString UNexusSteamUtils::GetLocalSteamName(UObject* WorldContextObject, int32 UserIndex)
{
	if (IOnlineIdentityPtr Identity = GetIdentity(WorldContextObject))
	{
		if (TSharedPtr<const FUniqueNetId> LocalId = Identity->GetUniquePlayerId(UserIndex))
		{
			FSteamPresence Dummy;
			GetLocalPresence(WorldContextObject, Dummy, UserIndex);

			const FString Nick = Identity->GetPlayerNickname(*LocalId);
			if (!Nick.IsEmpty())
				return Nick;
		}
	}
	return TEXT("Unknown");
}

FString UNexusSteamUtils::GetLocalSteamID(UObject* WorldContextObject, int32 UserIndex)
{
	if (TSharedPtr<const FUniqueNetId> Id = GetLocalUserId(WorldContextObject, UserIndex))
		return Id->ToString();
	return TEXT("0");
}

void UNexusSteamUtils::FillFriendsFromCacheOrOSS(UObject* WorldContextObject, TArray<FSteamFriendInfo>& OutFriends, int32 UserIndex, FName ListName)
{
	OutFriends.Reset();
	IOnlineFriendsPtr Friends = GetFriends(WorldContextObject);
	if (!Friends.IsValid())
		return;

	TArray<TSharedRef<FOnlineFriend>> FriendList;
	if (!Friends->GetFriendsList(UserIndex, ListName.ToString(), FriendList))
		return;

	OutFriends.Reserve(FriendList.Num());
	for (const TSharedRef<FOnlineFriend>& F : FriendList)
	{
		FSteamFriendInfo Info;
		BuildFriendInfo(F, Info);
		OutFriends.Add(MoveTemp(Info));
	}
}

void UNexusSteamUtils::GetSteamFriends(UObject* WorldContextObject, TArray<FSteamFriendInfo>& OutFriends, int32 UserIndex)
{
	FillFriendsFromCacheOrOSS(WorldContextObject, OutFriends, UserIndex, TEXT("default"));
}

void UNexusSteamUtils::ReadSteamFriends(UObject* WorldContextObject, const FOnSteamFriendsLoaded& OnCompleted, int32 UserIndex, FName ListName)
{
	IOnlineFriendsPtr Friends = GetFriends(WorldContextObject);
	if (!Friends.IsValid())
	{
		OnCompleted.ExecuteIfBound(false);
		return;
	}

	Friends->ReadFriendsList(UserIndex, ListName.ToString(),
		FOnReadFriendsListComplete::CreateLambda([WorldContextObject, OnCompleted, UserIndex, ListName](int32, bool bSuccess, const FString&, const FString&)
		{
			if (!bSuccess)
			{
				OnCompleted.ExecuteIfBound(false);
				return;
			}

			TArray<FSteamFriendInfo> Fresh;
			UNexusSteamUtils::FillFriendsFromCacheOrOSS(WorldContextObject, Fresh, UserIndex, ListName);

			const FString Key = MakeFriendsCacheKey(ListName, UserIndex);
			FFriendsCacheEntry& Entry = GFriendsCache.FindOrAdd(Key);
			Entry.Friends = MoveTemp(Fresh);
			Entry.LastUpdateUtc = FDateTime::UtcNow();

			OnCompleted.ExecuteIfBound(true);
		})
	);
}

void UNexusSteamUtils::GetSteamFriendsCached(UObject* WorldContextObject, TArray<FSteamFriendInfo>& OutFriends, float RefreshDelaySeconds, int32 UserIndex, FName ListName)
{
	const FString Key = MakeFriendsCacheKey(ListName, UserIndex);
	const FFriendsCacheEntry* Entry = GFriendsCache.Find(Key);

	const bool bFresh = (Entry != nullptr) && (FDateTime::UtcNow() - Entry->LastUpdateUtc).GetTotalSeconds() <= RefreshDelaySeconds;
	if (bFresh)
	{
		OutFriends = Entry->Friends;
		return;
	}

	FillFriendsFromCacheOrOSS(WorldContextObject, OutFriends, UserIndex, ListName);
	FFriendsCacheEntry& NewEntry = GFriendsCache.FindOrAdd(Key);
	NewEntry.Friends = OutFriends;
	NewEntry.LastUpdateUtc = FDateTime::UtcNow();
}

bool UNexusSteamUtils::InviteFriendToSession(UObject* WorldContextObject, const FUniqueNetIdRepl& FriendId, FName SessionName, int32 UserIndex)
{
	IOnlineSessionPtr Session = GetSession(WorldContextObject);
	IOnlineIdentityPtr Identity = GetIdentity(WorldContextObject);

	if (!Session.IsValid() || !Identity.IsValid())
		return false;

	TSharedPtr<const FUniqueNetId> LocalId = Identity->GetUniquePlayerId(UserIndex);
	if (!LocalId.IsValid() || !FriendId.IsValid())
		return false;

	return Session->SendSessionInviteToFriend(*LocalId, SessionName, *FriendId);
}

bool UNexusSteamUtils::ShowInviteOverlay(UObject* WorldContextObject, FName SessionName, int32 UserIndex)
{
	IOnlineExternalUIPtr UI = GetExternalUI(WorldContextObject);
	return UI.IsValid() ? UI->ShowInviteUI(UserIndex, SessionName) : false;
}

bool UNexusSteamUtils::ShowProfileOverlay(UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId, int32 UserIndex)
{
	IOnlineExternalUIPtr UI = GetExternalUI(WorldContextObject);
	if (!UI.IsValid() || !PlayerId.IsValid())
		return false;

	IOnlineIdentityPtr Identity = GetIdentity(WorldContextObject);
	if (!Identity.IsValid())
		return false;

	TSharedPtr<const FUniqueNetId> LocalUserId = Identity->GetUniquePlayerId(UserIndex);
	if (!LocalUserId.IsValid())
		return false;

	return UI->ShowProfileUI(*LocalUserId, *PlayerId, FOnProfileUIClosedDelegate());
}


