#pragma once
#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "NetworkToolTypes.h"
#include "Interfaces/IHttpRequest.h"
#include "NetworkToolSubsystem.generated.h"


DECLARE_MULTICAST_DELEGATE_OneParam(FOnServerStatusChanged, FString);

UCLASS()
class UNetworkToolSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
    
	/** Lance X clients + 1 Serveur en local */
	void LaunchLocalCluster(int32 ClientCount, bool bRunDedicatedServer);

	/** Configure le PIE et lance la connexion vers un profil */
	void JoinServerProfile(const FServerProfile& Profile);

	/** Envoie une requête HTTP pour Start/Stop un serveur distant */
	void SendRemoteCommand(const FServerProfile& Profile, FString CommandType);

	/** Applique le lag */
	void SetNetworkEmulation(const FNetworkEmulationProfile& Profile);

	// Delegate pour mettre à jour l'UI quand OVH répond
	FOnServerStatusChanged OnServerStatusLog;

private:
	// Callback interne pour les réponses HTTP
	void OnRemoteCommandComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};