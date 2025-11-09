#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NexusSteamUtils.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnSteamFriendsLoaded, bool, bSuccess);

USTRUCT(BlueprintType)
struct FSteamPresence
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString StatusText;
	UPROPERTY(BlueprintReadOnly) bool bIsOnline = false;
	UPROPERTY(BlueprintReadOnly) bool bIsPlaying = false;
	UPROPERTY(BlueprintReadOnly) bool bIsJoinable = false;
};

USTRUCT(BlueprintType)
struct FSteamFriendInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString DisplayName;
	UPROPERTY(BlueprintReadOnly) FString RealName;
	UPROPERTY(BlueprintReadOnly) FString PresenceStatus;
	UPROPERTY(BlueprintReadOnly) bool bIsOnline = false;
	UPROPERTY(BlueprintReadOnly) bool bIsPlaying = false;
	UPROPERTY(BlueprintReadOnly) bool bIsJoinable = false;
	UPROPERTY(BlueprintReadOnly) FUniqueNetIdRepl UniqueId;
};

UCLASS()
class NEXUSFRAMEWORK_API UNexusSteamUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Nexus|Steam", meta=(WorldContext="WorldContextObject"))
	static FString GetLocalSteamName(UObject* WorldContextObject, int32 UserIndex = 0);

	UFUNCTION(BlueprintCallable, Category="Nexus|Steam", meta=(WorldContext="WorldContextObject"))
	static FString GetLocalSteamID(UObject* WorldContextObject, int32 UserIndex = 0);

	UFUNCTION(BlueprintCallable, Category="Nexus|Steam", meta=(WorldContext="WorldContextObject"))
	static void GetSteamFriends(UObject* WorldContextObject, TArray<FSteamFriendInfo>& OutFriends, int32 UserIndex = 0);

	UFUNCTION(BlueprintCallable, Category="Nexus|Steam", meta=(WorldContext="WorldContextObject"))
	static void ReadSteamFriends(UObject* WorldContextObject, const FOnSteamFriendsLoaded& OnCompleted, int32 UserIndex = 0, FName ListName = "default");

	UFUNCTION(BlueprintCallable, Category="Nexus|Steam", meta=(WorldContext="WorldContextObject"))
	static void GetSteamFriendsCached(UObject* WorldContextObject, TArray<FSteamFriendInfo>& OutFriends, float RefreshDelaySeconds = 5.f, int32 UserIndex = 0, FName ListName = "default");

	UFUNCTION(BlueprintCallable, Category="Nexus|Steam", meta=(WorldContext="WorldContextObject"))
	static bool InviteFriendToSession(UObject* WorldContextObject, const FUniqueNetIdRepl& FriendId, FName SessionName = "GameSession", int32 UserIndex = 0);

	UFUNCTION(BlueprintCallable, Category="Nexus|Steam", meta=(WorldContext="WorldContextObject"))
	static bool ShowInviteOverlay(UObject* WorldContextObject, FName SessionName = "GameSession", int32 UserIndex = 0);

	UFUNCTION(BlueprintCallable, Category="Nexus|Steam", meta=(WorldContext="WorldContextObject"))
	static bool ShowProfileOverlay(UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId, int32 UserIndex = 0);

	UFUNCTION(BlueprintPure, Category="Nexus|Steam", meta=(WorldContext="WorldContextObject"))
	static bool IsSteamActive(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category="Nexus|Steam", meta=(WorldContext="WorldContextObject"))
	static bool GetLocalPresence(UObject* WorldContextObject, FSteamPresence& OutPresence, int32 UserIndex = 0);

private:
	static IOnlineSubsystem* GetOSS(UObject* WorldContextObject);
	static IOnlineIdentityPtr GetIdentity(UObject* WorldContextObject);
	static IOnlineFriendsPtr GetFriends(UObject* WorldContextObject);
	static IOnlineExternalUIPtr GetExternalUI(UObject* WorldContextObject);
	static IOnlinePresencePtr GetPresence(UObject* WorldContextObject);
	static IOnlineSessionPtr GetSession(UObject* WorldContextObject);

	static TSharedPtr<const FUniqueNetId> GetLocalUserId(UObject* WorldContextObject, int32 UserIndex);
	static void BuildFriendInfo(const TSharedRef<class FOnlineFriend>& InFriend, FSteamFriendInfo& OutInfo);
	static void FillFriendsFromCacheOrOSS(UObject* WorldContextObject, TArray<FSteamFriendInfo>& OutFriends, int32 UserIndex, FName ListName);
};