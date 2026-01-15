#include "Utils/NexusSteamUtils.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSubsystemTypes.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Engine/Engine.h"

#ifndef LOG_STEAM
#define LOG_STEAM(Verbosity, Format, ...) UE_LOG(LogTemp, Verbosity, TEXT("[♟ NexusSteam] ") Format, ##__VA_ARGS__)
#endif

// ────────────────────────────────────────────────
// Helpers Internes
// ────────────────────────────────────────────────

static IOnlineSubsystem* GetOSS(UObject* WorldContextObject)
{
	if (!IsValid(WorldContextObject))
		return nullptr;
	
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? Online::GetSubsystem(World) : nullptr;
}

// ────────────────────────────────────────────────
// Core Logic
// ────────────────────────────────────────────────

void UNexusSteamUtils::BuildFriendInfo(const TSharedRef<FOnlineFriend>& InFriend, FSteamFriendInfo& OutInfo)
{
	OutInfo.DisplayName = InFriend->GetDisplayName().IsEmpty() ? TEXT("Unknown") : InFriend->GetDisplayName();
	OutInfo.RealName    = InFriend->GetRealName();
	OutInfo.UniqueId    = FUniqueNetIdRepl(InFriend->GetUserId());

	const FOnlineUserPresence& Presence = InFriend->GetPresence();
	OutInfo.PresenceStatus = Presence.Status.StatusStr;
	OutInfo.bIsOnline   = Presence.bIsOnline;
	OutInfo.bIsPlaying  = Presence.bIsPlaying || Presence.bIsPlayingThisGame;
	OutInfo.bIsJoinable = Presence.bIsJoinable;
}

void UNexusSteamUtils::FillFriendsFromOSS(UObject* WorldContextObject, TArray<FSteamFriendInfo>& OutFriends, int32 UserIndex, FName ListName)
{
	OutFriends.Reset();

	IOnlineSubsystem* OSS = GetOSS(WorldContextObject);
	if (!OSS)
		return;
	
	IOnlineFriendsPtr Friends = OSS->GetFriendsInterface();
	if (!Friends.IsValid())
		return;

	TArray<TSharedRef<FOnlineFriend>> FriendList;
	if (Friends->GetFriendsList(UserIndex, ListName.ToString(), FriendList))
	{
		OutFriends.Reserve(FriendList.Num());
		for (const TSharedRef<FOnlineFriend>& F : FriendList)
		{
			FSteamFriendInfo Info;
			BuildFriendInfo(F, Info);
			OutFriends.Add(MoveTemp(Info));
		}
	}
}

// ────────────────────────────────────────────────
// Public API
// ────────────────────────────────────────────────

FString UNexusSteamUtils::GetLocalSteamName(UObject* WorldContextObject, int32 UserIndex)
{
	if (IOnlineSubsystem* OSS = GetOSS(WorldContextObject))
	{
		if (IOnlineIdentityPtr Identity = OSS->GetIdentityInterface())
		{
			FString Nick = Identity->GetPlayerNickname(UserIndex);
			return Nick.IsEmpty() ? TEXT("Unknown Player") : Nick;
		}
	}
	
	return TEXT("No OSS");
}

FString UNexusSteamUtils::GetLocalSteamID(UObject* WorldContextObject, int32 UserIndex)
{
	if (IOnlineSubsystem* OSS = GetOSS(WorldContextObject))
	{
		if (IOnlineIdentityPtr Identity = OSS->GetIdentityInterface())
		{
			if (TSharedPtr<const FUniqueNetId> Id = Identity->GetUniquePlayerId(UserIndex))
			{
				return Id->ToString();
			}
		}
	}
	return TEXT("Invalid");
}

bool UNexusSteamUtils::GetCachedFriends(UObject* WorldContextObject, TArray<FSteamFriendInfo>& OutFriends, int32 UserIndex)
{
	FillFriendsFromOSS(WorldContextObject, OutFriends, UserIndex, TEXT("default"));
	return OutFriends.Num() > 0;
}

void UNexusSteamUtils::ReadSteamFriends(UObject* WorldContextObject, const FOnSteamFriendsLoaded& OnCompleted, int32 UserIndex, FName ListName)
{
	IOnlineSubsystem* OSS = GetOSS(WorldContextObject);
	if (!OSS)
	{
		OnCompleted.ExecuteIfBound(false, {});
		return;
	}
	
	IOnlineFriendsPtr Friends = OSS->GetFriendsInterface();
	if (!Friends.IsValid())
	{
		LOG_STEAM(Warning, TEXT("ReadSteamFriends: Friends interface missing."));
		OnCompleted.ExecuteIfBound(false, {});
		return;
	}

	TWeakObjectPtr WeakContext(WorldContextObject);

	Friends->ReadFriendsList(UserIndex, ListName.ToString(), FOnReadFriendsListComplete::CreateLambda(
		[WeakContext, OnCompleted, UserIndex, ListName](int32 InLocalUserNum, bool bWasSuccessful, const FString& ListStr, const FString& ErrorStr)
		{
			if (!WeakContext.IsValid())
				return;

			TArray<FSteamFriendInfo> FreshFriends;
			if (bWasSuccessful)
			{
				FillFriendsFromOSS(WeakContext.Get(), FreshFriends, UserIndex, ListName);
				LOG_STEAM(Log, TEXT("Friends loaded: %d found."), FreshFriends.Num());
			}
			else
			{
				LOG_STEAM(Warning, TEXT("ReadFriendsList failed: %s"), *ErrorStr);
			}

			OnCompleted.ExecuteIfBound(bWasSuccessful, FreshFriends);
		}
	));
}

bool UNexusSteamUtils::InviteFriendToSession(UObject* WorldContextObject, const FUniqueNetIdRepl& FriendId, FName SessionName, int32 UserIndex)
{
	if (!FriendId.IsValid())
		return false;

	IOnlineSubsystem* OSS = GetOSS(WorldContextObject);
	if (!OSS)
		return false;
	
	IOnlineSessionPtr Session = OSS->GetSessionInterface();
	if (!Session.IsValid())
		return false;

	bool bOk = Session->SendSessionInviteToFriend(UserIndex, SessionName, *FriendId.GetUniqueNetId());
	LOG_STEAM(Log, TEXT("Invite sent to %s: %s"), *FriendId.ToString(), bOk ? TEXT("Success") : TEXT("Fail"));
	return bOk;
}

bool UNexusSteamUtils::ShowInviteOverlay(UObject* WorldContextObject, FName SessionName, int32 UserIndex)
{
	if (IOnlineSubsystem* OSS = GetOSS(WorldContextObject))
	{
		if (IOnlineExternalUIPtr ExtUI = OSS->GetExternalUIInterface())
		{
			return ExtUI->ShowInviteUI(UserIndex, SessionName);
		}
	}
	
	return false;
}

bool UNexusSteamUtils::ShowProfileOverlay(UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId, int32 UserIndex)
{
	if (!PlayerId.IsValid())
		return false;

	if (IOnlineSubsystem* OSS = GetOSS(WorldContextObject))
	{
		if (IOnlineExternalUIPtr ExtUI = OSS->GetExternalUIInterface())
		{
			if (IOnlineIdentityPtr Identity = OSS->GetIdentityInterface())
			{
				if (TSharedPtr<const FUniqueNetId> LocalId = Identity->GetUniquePlayerId(UserIndex))
				{
					return ExtUI->ShowProfileUI(*LocalId, *PlayerId.GetUniqueNetId(), FOnProfileUIClosedDelegate());
				}
			}
		}
	}
	return false;
}

bool UNexusSteamUtils::IsSteamActive(UObject* WorldContextObject)
{
	IOnlineSubsystem* OSS = GetOSS(WorldContextObject);
	return OSS && OSS->GetSubsystemName() == FName("STEAM");
}

bool UNexusSteamUtils::GetLocalPresence(UObject* WorldContextObject, FSteamPresence& OutPresence, int32 UserIndex)
{
	OutPresence = FSteamPresence();

	IOnlineSubsystem* OSS = GetOSS(WorldContextObject);
	if (!OSS) return false;

	IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
	IOnlinePresencePtr Presence = OSS->GetPresenceInterface();

	if (!Identity.IsValid())
		return false;

	TSharedPtr<const FUniqueNetId> LocalId = Identity->GetUniquePlayerId(UserIndex);
	if (!LocalId.IsValid())
		return false;

	if (Presence.IsValid())
	{
		TSharedPtr<FOnlineUserPresence> Cached;
		if (Presence->GetCachedPresence(*LocalId, Cached) == EOnlineCachedResult::Success && Cached.IsValid())
		{
			OutPresence.StatusText = Cached->Status.StatusStr;
			OutPresence.bIsOnline = Cached->bIsOnline;
			OutPresence.bIsPlaying = Cached->bIsPlaying || Cached->bIsPlayingThisGame;
			OutPresence.bIsJoinable = Cached->bIsJoinable;
			return true;
		}
	}

	ELoginStatus::Type LoginStatus = Identity->GetLoginStatus(UserIndex);
	OutPresence.bIsOnline = (LoginStatus == ELoginStatus::LoggedIn);
	OutPresence.StatusText = OutPresence.bIsOnline ? TEXT("Online") : TEXT("Offline");
	
	return true;
}