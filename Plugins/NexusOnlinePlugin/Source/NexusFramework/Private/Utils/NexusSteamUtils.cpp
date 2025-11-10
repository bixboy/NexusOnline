#include "Utils/NexusSteamUtils.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSubsystemTypes.h"
#include "Engine/Engine.h"

#ifndef LOG_STEAM
#define LOG_STEAM(Verbosity, Format, ...) UE_LOG(LogTemp, Verbosity, TEXT("[♟ NexusSteam] ") Format, ##__VA_ARGS__)
#endif

namespace
{
	struct FFriendsCacheEntry
	{
		TArray<FSteamFriendInfo> Friends;
		FDateTime LastUpdateUtc = FDateTime::MinValue();
	};

	TMap<FString, FFriendsCacheEntry> GFriendsCache;

	FString MakeFriendsCacheKey(FName ListName, int32 UserIndex)
	{
		return FString::Printf(TEXT("%s:%d"), *ListName.ToString(), UserIndex);
	}

	template<typename T>
	FORCEINLINE bool IsPtrValid(const T& Ptr) { return Ptr.IsValid(); }
}

// ────────────────────────────────────────────────
// Safe Getters
// ────────────────────────────────────────────────

IOnlineSubsystem* UNexusSteamUtils::GetOSS(UObject* WorldContextObject)
{
	if (!IsValid(WorldContextObject))
		return nullptr;

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? Online::GetSubsystem(World) : nullptr;
}

IOnlineIdentityPtr UNexusSteamUtils::GetIdentity(UObject* WorldContextObject)
{
	if (IOnlineSubsystem* OSS = GetOSS(WorldContextObject))
		return OSS->GetIdentityInterface();
	
	return nullptr;
}

IOnlineFriendsPtr UNexusSteamUtils::GetFriends(UObject* WorldContextObject)
{
	if (IOnlineSubsystem* OSS = GetOSS(WorldContextObject))
		return OSS->GetFriendsInterface();
	
	return nullptr;
}

IOnlineExternalUIPtr UNexusSteamUtils::GetExternalUI(UObject* WorldContextObject)
{
	if (IOnlineSubsystem* OSS = GetOSS(WorldContextObject))
		return OSS->GetExternalUIInterface();
	
	return nullptr;
}

IOnlinePresencePtr UNexusSteamUtils::GetPresence(UObject* WorldContextObject)
{
	if (IOnlineSubsystem* OSS = GetOSS(WorldContextObject))
		return OSS->GetPresenceInterface();
	
	return nullptr;
}

IOnlineSessionPtr UNexusSteamUtils::GetSession(UObject* WorldContextObject)
{
	if (IOnlineSubsystem* OSS = GetOSS(WorldContextObject))
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
// Core
// ────────────────────────────────────────────────

void UNexusSteamUtils::BuildFriendInfo(const TSharedRef<FOnlineFriend>& InFriend, FSteamFriendInfo& OutInfo)
{
	OutInfo.DisplayName = InFriend->GetDisplayName().IsEmpty() ? TEXT("[Unknown Name]") : InFriend->GetDisplayName();
	OutInfo.RealName    = InFriend->GetRealName().IsEmpty()    ? TEXT("[Unknown RealName]") : InFriend->GetRealName();
	OutInfo.UniqueId    = InFriend->GetUserId();

	const FOnlineUserPresence& Presence = InFriend->GetPresence();
	OutInfo.PresenceStatus = Presence.Status.StatusStr.IsEmpty() ? TEXT("[No Status]") : Presence.Status.StatusStr;
	OutInfo.bIsOnline   = Presence.bIsOnline;
	OutInfo.bIsPlaying  = Presence.bIsPlaying || Presence.bIsPlayingThisGame;
	OutInfo.bIsJoinable = Presence.bIsJoinable;
}

void UNexusSteamUtils::FillFriendsFromOSS(UObject* WorldContextObject, TArray<FSteamFriendInfo>& OutFriends, int32 UserIndex, FName ListName)
{
	OutFriends.Reset();

	IOnlineFriendsPtr Friends = GetFriends(WorldContextObject);
	if (!IsPtrValid(Friends))
	{
		LOG_STEAM(Warning, TEXT("Friends interface not available."));
		return;
	}

	TArray<TSharedRef<FOnlineFriend>> FriendList;
	if (!Friends->GetFriendsList(UserIndex, ListName.ToString(), FriendList))
	{
		LOG_STEAM(Verbose, TEXT("Friends list not cached yet for list '%s' (UserIndex=%d)."), *ListName.ToString(), UserIndex);
		return;
	}

	OutFriends.Reserve(FriendList.Num());
	for (const TSharedRef<FOnlineFriend>& F : FriendList)
	{
		if (!F->GetUserId().ToSharedPtr())
		{
			LOG_STEAM(Warning, TEXT("Friend entry has invalid UserId. Skipping."));
			continue;
		}

		FSteamFriendInfo Info;
		BuildFriendInfo(F, Info);
		OutFriends.Add(MoveTemp(Info));
	}
}

// ────────────────────────────────────────────────
// Public API
// ────────────────────────────────────────────────

FString UNexusSteamUtils::GetLocalSteamName(UObject* WorldContextObject, int32 UserIndex)
{
	IOnlineIdentityPtr Identity = GetIdentity(WorldContextObject);
	if (!IsPtrValid(Identity))
		return FString();

	FString Nick = Identity->GetPlayerNickname(UserIndex);
	if (!Nick.IsEmpty())
		return Nick;

	if (TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(UserIndex))
	{
		Nick = Identity->GetPlayerNickname(*UserId);
	}
	
	return Nick;
}

FString UNexusSteamUtils::GetLocalSteamID(UObject* WorldContextObject, int32 UserIndex)
{
	if (TSharedPtr<const FUniqueNetId> Id = GetLocalUserId(WorldContextObject, UserIndex))
		return Id->ToString();
	return FString();
}

void UNexusSteamUtils::GetSteamFriends(UObject* WorldContextObject, TArray<FSteamFriendInfo>& OutFriends, int32 UserIndex)
{
	FillFriendsFromOSS(WorldContextObject, OutFriends, UserIndex, TEXT("default"));
}

void UNexusSteamUtils::ReadSteamFriends(UObject* WorldContextObject, const FOnSteamFriendsLoaded& OnCompleted, int32 UserIndex, FName ListName)
{
	IOnlineFriendsPtr Friends = GetFriends(WorldContextObject);
	if (!IsPtrValid(Friends))
	{
		LOG_STEAM(Warning, TEXT("ReadSteamFriends: Friends interface not available."));
		OnCompleted.ExecuteIfBound(false);
		return;
	}

	TWeakObjectPtr WeakContext(WorldContextObject);

	Friends->ReadFriendsList( UserIndex, ListName.ToString(), FOnReadFriendsListComplete::CreateLambda
		([WeakContext, OnCompleted, UserIndex, ListName](int32 InLocalUserNum, bool bWasSuccessful, const FString& ListStr, const FString& ErrorStr)
			{
				if (!WeakContext.IsValid() || !bWasSuccessful)
				{
					if (!ErrorStr.IsEmpty())
					{
						LOG_STEAM(Warning, TEXT("ReadFriendsList failed: %s"), *ErrorStr);
					}
					OnCompleted.ExecuteIfBound(false);
					return;
				}

				TArray<FSteamFriendInfo> Fresh;
				FillFriendsFromOSS(WeakContext.Get(), Fresh, UserIndex, ListName);

				const FString Key = MakeFriendsCacheKey(ListName, UserIndex);
				FFriendsCacheEntry& Entry = GFriendsCache.FindOrAdd(Key);
				Entry.Friends = MoveTemp(Fresh);
				Entry.LastUpdateUtc = FDateTime::UtcNow();

				OnCompleted.ExecuteIfBound(true);
			}
		)
	);
}

void UNexusSteamUtils::GetSteamFriendsCached(UObject* WorldContextObject, TArray<FSteamFriendInfo>& OutFriends, float RefreshDelaySeconds, int32 UserIndex, FName ListName)
{
	const FString Key = MakeFriendsCacheKey(ListName, UserIndex);
	if (const FFriendsCacheEntry* Entry = GFriendsCache.Find(Key))
	{
		const double Age = (FDateTime::UtcNow() - Entry->LastUpdateUtc).GetTotalSeconds();
		if (Age <= RefreshDelaySeconds)
		{
			OutFriends = Entry->Friends;
			return;
		}
	}

	FillFriendsFromOSS(WorldContextObject, OutFriends, UserIndex, ListName);

	FFriendsCacheEntry& NewEntry = GFriendsCache.FindOrAdd(Key);
	NewEntry.Friends = OutFriends;
	NewEntry.LastUpdateUtc = FDateTime::UtcNow();
}

bool UNexusSteamUtils::InviteFriendToSession(UObject* WorldContextObject, const FUniqueNetIdRepl& FriendId, FName SessionName, int32 UserIndex)
{
	IOnlineSessionPtr Session = GetSession(WorldContextObject);
	if (!IsPtrValid(Session))
	{
		LOG_STEAM(Warning, TEXT("InviteFriendToSession: Session interface not available."));
		return false;
	}

	if (!FriendId.IsValid())
	{
		LOG_STEAM(Warning, TEXT("InviteFriendToSession: FriendId is invalid."));
		return false;
	}

	FNamedOnlineSession* Named = Session->GetNamedSession(SessionName);
	if (!Named)
	{
		LOG_STEAM(Warning, TEXT("InviteFriendToSession: Session '%s' not found."), *SessionName.ToString());
		return false;
	}

	const TSharedPtr<const FUniqueNetId> LocalId = GetLocalUserId(WorldContextObject, UserIndex);
	if (!LocalId.IsValid())
	{
		LOG_STEAM(Warning, TEXT("InviteFriendToSession: Local user id invalid (UserIndex=%d)."), UserIndex);
		return false;
	}

	bool bOk = Session->SendSessionInviteToFriend(UserIndex, SessionName, *FriendId.GetUniqueNetId());


	LOG_STEAM(Log, TEXT("InviteFriendToSession(%s): %s"), *SessionName.ToString(), bOk ? TEXT("OK") : TEXT("FAILED"));
	return bOk;
}

bool UNexusSteamUtils::ShowInviteOverlay(UObject* WorldContextObject, FName SessionName, int32 UserIndex)
{
	IOnlineExternalUIPtr ExtUI = GetExternalUI(WorldContextObject);
	if (!IsPtrValid(ExtUI))
	{
		LOG_STEAM(Warning, TEXT("ShowInviteOverlay: External UI interface not available."));
		return false;
	}

	const bool bOk = ExtUI->ShowInviteUI(UserIndex, SessionName);
	LOG_STEAM(Log, TEXT("ShowInviteOverlay(%s): %s"), *SessionName.ToString(), bOk ? TEXT("OK") : TEXT("FAILED"));
	
	return bOk;
}

bool UNexusSteamUtils::ShowProfileOverlay(UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId, int32 UserIndex)
{
	IOnlineExternalUIPtr ExtUI = GetExternalUI(WorldContextObject);
	if (!IsPtrValid(ExtUI))
	{
		LOG_STEAM(Warning, TEXT("ShowProfileOverlay: External UI interface not available."));
		return false;
	}

	if (!PlayerId.IsValid())
	{
		LOG_STEAM(Warning, TEXT("ShowProfileOverlay: PlayerId invalid."));
		return false;
	}

	const TSharedPtr<const FUniqueNetId> LocalId = GetLocalUserId(WorldContextObject, UserIndex);
	if (!LocalId.IsValid())
	{
		LOG_STEAM(Warning, TEXT("ShowProfileOverlay: LocalId invalid for UserIndex %d."), UserIndex);
		return false;
	}

	const bool bOk = ExtUI->ShowProfileUI(*LocalId, *PlayerId.GetUniqueNetId(), FOnProfileUIClosedDelegate());
	LOG_STEAM(Log, TEXT("ShowProfileOverlay: %s"), bOk ? TEXT("OK") : TEXT("FAILED"));
	
	return bOk;
}


bool UNexusSteamUtils::IsSteamActive(UObject* WorldContextObject)
{
	IOnlineSubsystem* OSS = GetOSS(WorldContextObject);
	if (!OSS) return false;

	const FName SubsystemName = OSS->GetSubsystemName();
	const bool bActive = (SubsystemName == STEAM_SUBSYSTEM);
	
	LOG_STEAM(Verbose, TEXT("IsSteamActive? %s (Subsystem=%s)"), bActive ? TEXT("Yes") : TEXT("No"), *SubsystemName.ToString());
	return bActive;
}

bool UNexusSteamUtils::GetLocalPresence(UObject* WorldContextObject, FSteamPresence& OutPresence, int32 UserIndex)
{
	OutPresence = FSteamPresence{};

	IOnlinePresencePtr Presence = GetPresence(WorldContextObject);
	IOnlineIdentityPtr Identity = GetIdentity(WorldContextObject);

	if (!IsPtrValid(Identity))
	{
		LOG_STEAM(Warning, TEXT("GetLocalPresence: Identity interface not available."));
		return false;
	}

	const TSharedPtr<const FUniqueNetId> LocalId = Identity->GetUniquePlayerId(UserIndex);
	if (!LocalId.IsValid())
	{
		LOG_STEAM(Warning, TEXT("GetLocalPresence: Local user id invalid (UserIndex=%d)."), UserIndex);
		return false;
	}
	
	if (IsPtrValid(Presence))
	{
		TSharedPtr<FOnlineUserPresence> Cached;
		const EOnlineCachedResult::Type Result = Presence->GetCachedPresence(*LocalId, Cached);

		if (Result == EOnlineCachedResult::Success && Cached.IsValid())
		{
			const FOnlineUserPresence& P = *Cached.Get();
			OutPresence.StatusText = P.Status.StatusStr;
			OutPresence.bIsOnline = P.bIsOnline;
			OutPresence.bIsPlaying = P.bIsPlaying || P.bIsPlayingThisGame;
			OutPresence.bIsJoinable = P.bIsJoinable;
			return true;
		}
	}

	const ELoginStatus::Type Status = Identity->GetLoginStatus(UserIndex);
	OutPresence.bIsOnline = (Status == ELoginStatus::Type::LoggedIn);
	OutPresence.bIsPlaying = false;
	OutPresence.bIsJoinable = false;
	OutPresence.StatusText = OutPresence.bIsOnline ? TEXT("Online") : TEXT("Offline");

	return true;
}
