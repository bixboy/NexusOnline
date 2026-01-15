#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Types/OnlineSessionData.h"
#include "Data/SessionSearchFilter.h"
#include "Data/SessionFilterPreset.h"
#include "AsyncTask_CreateSession.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSessionCreated);

UCLASS()
class NEXUSFRAMEWORK_API UAsyncTask_CreateSession : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintAssignable, Category="Nexus|Online|Session")
	FOnSessionCreated OnSuccess;

	UPROPERTY(BlueprintAssignable, Category="Nexus|Online|Session")
	FOnSessionCreated OnFailure;
	
	/**
	 * Crée une session multijoueur.
	 *
	 * @param SettingsData         Configuration de base (Map, MaxPlayers, Mode, etc.).
	 * @param AdditionalSettings   Filtres ou métadonnées supplémentaires.
	 * @param bAutoTravel          Si VRAI, effectue un ServerTravel vers la map dès la création. Si FAUX, appelle juste OnSuccess
	 * @param Preset               Preset de filtres optionnel.
	 */
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject", AutoCreateRefTerm="AdditionalSettings"), Category="Nexus|Online|Session")
	static UAsyncTask_CreateSession* CreateSession(
	   UObject* WorldContextObject, 
	   const FSessionSettingsData& SettingsData,
	   const TArray<FSessionSearchFilter>& AdditionalSettings,
	   bool bAutoTravel = true, 
	   USessionFilterPreset* Preset = nullptr
	);

	virtual void Activate() override;

private:
	
	/** Étape 1 : Callback appelé quand l'ancienne session est bien détruite */
	void OnOldSessionDestroyed(FName SessionName, bool bWasSuccessful);

	/** Étape 2 : Lance la vraie création */
	void CreateSessionInternal();

	/** Étape 3 : Fin du processus */
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);

	
	// ───────────────────────────────
	// Données
	// ───────────────────────────────
	UPROPERTY()
	UObject* WorldContextObject = nullptr;
	
	FSessionSettingsData Data;
	bool bShouldAutoTravel = true;

	UPROPERTY()
	TArray<FSessionSearchFilter> SessionAdditionalSettings;
	
	UPROPERTY()
	TObjectPtr<USessionFilterPreset> SessionPreset;

	FDelegateHandle CreateDelegateHandle;
	FDelegateHandle DestroyDelegateHandle;
};