#include "Utils/NexusSteamUtils.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "Interfaces/OnlinePresenceInterface.h" // ✅ important

FString UNexusSteamUtils::GetLocalSteamName()
{
	IOnlineSubsystem* OSS = Online::GetSubsystem(GWorld);
	if (!OSS)
		return "Unknown";

	IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
	if (!Identity.IsValid())
		return "Unknown";

	TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(0);
	if (!UserId.IsValid())
		return "Unknown";

	return Identity->GetPlayerNickname(*UserId);
}

FString UNexusSteamUtils::GetLocalSteamID()
{
	IOnlineSubsystem* OSS = Online::GetSubsystem(GWorld);
	if (!OSS)
		return "0";

	IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
	if (!Identity.IsValid())
		return "0";

	TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(0);
	return UserId.IsValid() ? UserId->ToString() : "0";
}

void UNexusSteamUtils::GetSteamFriends(TArray<FSteamFriendInfo>& OutFriends)
{
	OutFriends.Empty();

	IOnlineSubsystem* OSS = Online::GetSubsystem(GWorld);
	if (!OSS)
		return;

	IOnlineFriendsPtr Friends = OSS->GetFriendsInterface();
	if (!Friends.IsValid())
		return;

	TArray<TSharedRef<FOnlineFriend>> FriendList;
	if (!Friends->GetFriendsList(0, TEXT("default"), FriendList))
	{
		UE_LOG(LogTemp, Warning, TEXT("[NexusSteam] No friends list available (might require ReadFriendsList)."));
		return;
	}

	for (const TSharedRef<FOnlineFriend>& Friend : FriendList)
	{
		FSteamFriendInfo Info;
		Info.DisplayName = Friend->GetDisplayName();
		Info.RealName = Friend->GetRealName();
		Info.UniqueId = Friend->GetUserId();
		Info.PresenceStatus = Friend->GetPresence().Status.StatusStr;

		OutFriends.Add(Info);
	}
}

bool UNexusSteamUtils::InviteFriendToSession(const FUniqueNetIdRepl& FriendId)
{
	IOnlineSubsystem* OSS = Online::GetSubsystem(GWorld);
	if (!OSS)
		return false;

	IOnlineSessionPtr Session = OSS->GetSessionInterface();
	IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
	if (!Session.IsValid() || !Identity.IsValid())
		return false;

	TSharedPtr<const FUniqueNetId> LocalUserId = Identity->GetUniquePlayerId(0);
	if (!LocalUserId.IsValid())
		return false;

	FName SessionName = NAME_GameSession;
	return Session->SendSessionInviteToFriend(*LocalUserId, SessionName, *FriendId);
}

bool UNexusSteamUtils::ShowInviteOverlay()
{
	IOnlineSubsystem* OSS = Online::GetSubsystem(GWorld);
	if (!OSS)
		return false;

	IOnlineExternalUIPtr ExternalUI = OSS->GetExternalUIInterface();
	if (!ExternalUI.IsValid())
		return false;

	return ExternalUI->ShowInviteUI(0, NAME_GameSession);
}

