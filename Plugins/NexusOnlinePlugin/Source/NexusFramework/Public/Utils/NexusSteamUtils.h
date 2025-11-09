#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NexusSteamUtils.generated.h"

USTRUCT(BlueprintType)
struct FSteamFriendInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString DisplayName;

	UPROPERTY(BlueprintReadOnly)
	FString RealName;

	UPROPERTY(BlueprintReadOnly)
	FString PresenceStatus;

	UPROPERTY(BlueprintReadOnly)
	FUniqueNetIdRepl UniqueId;
};

UCLASS()
class NEXUSFRAMEWORK_API UNexusSteamUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Retourne le nom du joueur Steam local */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Steam")
	static FString GetLocalSteamName();

	/** Retourne l’ID Steam (64 bits) du joueur local */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Steam")
	static FString GetLocalSteamID();

	/** Retourne la liste des amis Steam en ligne */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Steam")
	static void GetSteamFriends(TArray<FSteamFriendInfo>& OutFriends);

	/** Invite un ami à rejoindre la session actuelle */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Steam")
	static bool InviteFriendToSession(const FUniqueNetIdRepl& FriendId);

	/** Ouvre l’overlay Steam d’invitation directement */
	UFUNCTION(BlueprintCallable, Category = "Nexus|Steam")
	static bool ShowInviteOverlay();
};
