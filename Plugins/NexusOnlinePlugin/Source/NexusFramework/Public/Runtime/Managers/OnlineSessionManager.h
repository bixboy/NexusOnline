#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionManager.generated.h"

class IOnlineSession;
class FUniqueNetId;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerCountChanged, int32, NewPlayerCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerJoinedSignature, const FString&, PlayerId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerLeftSignature, const FString&, PlayerId);

/**
 * Replicated actor responsible for keeping track of the current session player count and mirroring the value on every machine.
 * The manager queries the OnlineSubsystem session interface and exposes Blueprint events that react to player count changes.
 */
UCLASS(Blueprintable, BlueprintType)
class NEXUSFRAMEWORK_API AOnlineSessionManager : public AActor
{
    GENERATED_BODY()

public:
    AOnlineSessionManager();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Name of the session tracked by this manager. Defaults to GameSession if unset. */
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Nexus|Online")
    FName TrackedSessionName;

    /** Current amount of registered players (mirrors OnlineSubsystem registered players). */
    UPROPERTY(ReplicatedUsing=OnRep_PlayerCount, BlueprintReadOnly, Category="Nexus|Online")
    int32 PlayerCount;

    /** Broadcast whenever PlayerCount is updated (server + clients). */
    UPROPERTY(BlueprintAssignable, Category="Nexus|Online")
    FOnPlayerCountChanged OnPlayerCountChanged;

    /** Optional: broadcast the unique id of a player that has just joined. */
    UPROPERTY(BlueprintAssignable, Category="Nexus|Online")
    FOnPlayerJoinedSignature OnPlayerJoined;

    /** Optional: broadcast the unique id of a player that has just left. */
    UPROPERTY(BlueprintAssignable, Category="Nexus|Online")
    FOnPlayerLeftSignature OnPlayerLeft;

    /** Rep-notify executed on clients when PlayerCount changes. */
    UFUNCTION()
    void OnRep_PlayerCount();

    /** Blueprint helper to request the server to refresh the player count. */
    UFUNCTION(BlueprintCallable, Category="Nexus|Online")
    void ForceUpdatePlayerCount();

    /** Server RPC used internally (and callable from clients) to refresh the player count. */
    UFUNCTION(Server, Reliable)
    void Server_UpdatePlayerCount();

    /** Returns the list of registered player identifiers (UniqueNetId strings). */
    UFUNCTION(BlueprintCallable, Category="Nexus|Online")
    TArray<FString> GetPlayerList() const;

    /** Utility used to find the manager within the world. */
    UFUNCTION(BlueprintCallable, Category="Nexus|Online", meta=(WorldContext="WorldContextObject"))
    static AOnlineSessionManager* Get(UObject* WorldContextObject);

protected:
    /** Cached value used to detect increments/decrements when replicating to clients. */
    UPROPERTY(Transient)
    int32 LastNotifiedPlayerCount;

    /** Holds the delegate binding for participant changes (if supported). */
    FDelegateHandle ParticipantsChangedDelegateHandle;

    void RefreshPlayerCount();

    void BroadcastPlayerCountChange(int32 PreviousCount, int32 NewCount, bool bAllowFallbackEvents);

    void BindSessionDelegates();
    void UnbindSessionDelegates();

    void HandleParticipantsChanged(FName SessionName, const FUniqueNetId& PlayerId, bool bJoined);
};
