#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "Interfaces/OnlinePresenceInterface.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "NexusSteamUtils.generated.h" // Toujours en dernier

class IOnlineSubsystem;


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


DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnSteamFriendsLoaded, bool, bSuccess, const TArray<FSteamFriendInfo>&, FriendsList);

UCLASS()
class NEXUSFRAMEWORK_API UNexusSteamUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ────────────────────────────────────────────────
	// Identité
	// ────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category="Nexus|Steam", meta=(WorldContext="WorldContextObject"))
	static FString GetLocalSteamName(UObject* WorldContextObject, int32 UserIndex = 0);

	UFUNCTION(BlueprintCallable, Category="Nexus|Steam", meta=(WorldContext="WorldContextObject"))
	static FString GetLocalSteamID(UObject* WorldContextObject, int32 UserIndex = 0);

	// ────────────────────────────────────────────────
	// Amis
	// ────────────────────────────────────────────────
	
	UFUNCTION(BlueprintCallable, Category="Nexus|Steam", meta=(WorldContext="WorldContextObject"))
	static bool GetCachedFriends(UObject* WorldContextObject, TArray<FSteamFriendInfo>& OutFriends, int32 UserIndex = 0);
	
	UFUNCTION(BlueprintCallable, Category="Nexus|Steam", meta=(WorldContext="WorldContextObject", AutoCreateRefTerm="OnCompleted"))
	static void ReadSteamFriends(UObject* WorldContextObject, const FOnSteamFriendsLoaded& OnCompleted, int32 UserIndex = 0, FName ListName = "default");

	// ────────────────────────────────────────────────
	// Sessions & UI
	// ────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category="Nexus|Steam", meta=(WorldContext="WorldContextObject"))
	static bool InviteFriendToSession(UObject* WorldContextObject, const FUniqueNetIdRepl& FriendId, FName SessionName = "GameSession", int32 UserIndex = 0);

	UFUNCTION(BlueprintCallable, Category="Nexus|Steam", meta=(WorldContext="WorldContextObject"))
	static bool ShowInviteOverlay(UObject* WorldContextObject, FName SessionName = "GameSession", int32 UserIndex = 0);

	UFUNCTION(BlueprintCallable, Category="Nexus|Steam", meta=(WorldContext="WorldContextObject"))
	static bool ShowProfileOverlay(UObject* WorldContextObject, const FUniqueNetIdRepl& PlayerId, int32 UserIndex = 0);

	// ────────────────────────────────────────────────
	// État & Presence
	// ────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category="Nexus|Steam", meta=(WorldContext="WorldContextObject"))
	static bool IsSteamActive(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category="Nexus|Steam", meta=(WorldContext="WorldContextObject"))
	static bool GetLocalPresence(UObject* WorldContextObject, FSteamPresence& OutPresence, int32 UserIndex = 0);

private:
	static void BuildFriendInfo(const TSharedRef<class FOnlineFriend>& InFriend, FSteamFriendInfo& OutInfo);
	static void FillFriendsFromOSS(UObject* WorldContextObject, TArray<FSteamFriendInfo>& OutFriends, int32 UserIndex, FName ListName);
};