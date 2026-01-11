#pragma once
#include "CoreMinimal.h"
#include "OnlineSessionSettings.h"
#include "OnlineSessionData.generated.h"

/** 
 * Type interne de session pour le multi-session system
 */
UENUM(BlueprintType)
enum class ENexusSessionType : uint8
{
	GameSession      UMETA(DisplayName="Game Session"),
	PartySession     UMETA(DisplayName="Party Session"),
	SpectatorSession UMETA(DisplayName="Spectator Session"),
	CustomSession    UMETA(DisplayName="Custom Session")
};


/**
 * Structure regroupant tous les paramètres configurables d'une session.
 * Utilisée pour créer des sessions.
 */
USTRUCT(BlueprintType)
struct FSessionSettingsData
{
	GENERATED_BODY()

public:
	
	/** Nom de la session (affiché lors du FindSessions) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session")
	FString SessionName = "DefaultSession";

	/** Map utilisée dans cette session */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session")
	FString MapName = "DefaultMap";

	/** Mode de jeu (ex : Survival, Deathmatch...) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session")
	FString GameMode = "DefaultMode";

	/** Nombre maximum de joueurs */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session")
	int32 MaxPlayers = 4;

	/** Est-ce une session LAN ? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session")
	bool bIsLAN = false;

	/** La session est-elle privée (non visible dans la recherche) ? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session")
	bool bIsPrivate = false;

	/** Session visible uniquement pour les amis */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session")
	bool bFriendsOnly = false;

	/** Type de session (Game, Party, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session")
	ENexusSessionType SessionType = ENexusSessionType::GameSession;
	
	/** Enable Host Migration? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session")
	bool bAllowHostMigration = false;
	
	/** Unique ID for migration tracking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session")
	FString MigrationSessionID;

	/** Optional custom session ID (generated automatically if empty). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Session")
	FString SessionId;

	/** Length of random session ID to auto-generate if none provided. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Session")
	int32 SessionIdLength = 6;
};


/**
 * Structure regroupant les resultat de la recherche d'une session.
 */
USTRUCT(BlueprintType)
struct FOnlineSessionSearchResultData
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="Nexus|Online|Session")
	FString SessionDisplayName;

	UPROPERTY(BlueprintReadOnly, Category="Nexus|Online|Session")
	FString MapName;

	UPROPERTY(BlueprintReadOnly, Category="Nexus|Online|Session")
	FString GameMode;

	UPROPERTY(BlueprintReadOnly, Category="Nexus|Online|Session")
	FString SessionType;

	UPROPERTY(BlueprintReadOnly, Category="Nexus|Online|Session")
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category="Nexus|Online|Session")
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category="Nexus|Online|Session")
	int32 Ping = 0;

	FOnlineSessionSearchResult RawResult;
};

