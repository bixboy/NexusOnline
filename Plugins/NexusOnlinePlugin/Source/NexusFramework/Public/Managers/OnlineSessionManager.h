#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OnlineSessionManager.generated.h"

class IOnlineSession;
class FUniqueNetId;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerCountChanged, int32, NewPlayerCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerJoinedSignature, const FString&, PlayerName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerLeftSignature, const FString&, PlayerName);


UCLASS(Blueprintable, BlueprintType)
class NEXUSFRAMEWORK_API AOnlineSessionManager : public AActor
{
    GENERATED_BODY()

public:
    AOnlineSessionManager();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


    UPROPERTY(Replicated, BlueprintReadOnly, Category="Nexus|Online")
    FName TrackedSessionName;

    UPROPERTY(ReplicatedUsing=OnRep_PlayerCount, BlueprintReadOnly, Category="Nexus|Online")
    int32 PlayerCount;

    UPROPERTY(ReplicatedUsing=OnRep_NextHostUniqueId, BlueprintReadOnly, Category="Nexus|Online|Migration")
    FString NextHostUniqueId;

    UFUNCTION(BlueprintCallable, Category="Nexus|Online|Migration")
    FString GetNextHostUniqueId() const { return NextHostUniqueId; }


    UPROPERTY(BlueprintAssignable, Category="Nexus|Online")
    FOnPlayerCountChanged OnPlayerCountChanged;

    UPROPERTY(BlueprintAssignable, Category="Nexus|Online")
    FOnPlayerJoinedSignature OnPlayerJoined;

    UPROPERTY(BlueprintAssignable, Category="Nexus|Online")
    FOnPlayerLeftSignature OnPlayerLeft;

    
    UFUNCTION(BlueprintCallable, Category="Nexus|Online")
    void ForceUpdatePlayerCount();
    
    UFUNCTION(BlueprintCallable, Category="Nexus|Online")
    TArray<FString> GetPlayerList() const;

    UFUNCTION(BlueprintCallable, Category="Nexus|Online", meta=(WorldContext="WorldContextObject"))
    static AOnlineSessionManager* Get(UObject* WorldContextObject);

    static AOnlineSessionManager* Spawn(UObject* WorldContextObject, FName InSessionName = NAME_GameSession);

protected:
    UFUNCTION()
    void OnRep_PlayerCount();

    UFUNCTION()
    void OnRep_NextHostUniqueId();

    // --- Internal Logic ---

    UPROPERTY(Transient)
    int32 LastNotifiedPlayerCount;

    FDelegateHandle RegisterPlayersHandle;
    FDelegateHandle UnregisterPlayersHandle;
    FTimerHandle TimerHandle_Refresh;

    void RefreshPlayerCount();
    void BindSessionDelegates();
    void UnbindSessionDelegates();

    void UpdateHeir();

    void OnPlayersRegistered(FName SessionName, const TArray<FUniqueNetIdRef>& Players, bool bWasSuccessful);
    void OnPlayersUnregistered(FName SessionName, const TArray<FUniqueNetIdRef>& Players, bool bWasSuccessful);

    FString GetPlayerNameFromId(const FUniqueNetId& PlayerId) const;
};