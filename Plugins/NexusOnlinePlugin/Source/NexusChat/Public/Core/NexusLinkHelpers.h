#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NexusLinkHelpers.generated.h"


UCLASS()
class NEXUSCHAT_API UNexusLinkHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ────────────────────────────────────────────
	// GÉNÉRATION DE LIENS GÉNÉRIQUES
	// ────────────────────────────────────────────

	/**
	 * Crée une balise <link> correctement formatée et sécurisée.
	 * 
	 * @param Type Le type de lien (ex: "item", "quest", "guild")
	 * @param Data Les données cachées passées au handler (ex: ID de l'item)
	 * @param DisplayText Le texte visible par le joueur
	 */
	UFUNCTION(BlueprintPure, Category = "NexusChat|Links", meta = (DisplayName = "Make Link"))
	static FString MakeLink(const FString& Type, const FString& Data, const FString& DisplayText);

	/**
	 * Helper pour créer un lien vers un joueur.
	 */
	UFUNCTION(BlueprintPure, Category = "NexusChat|Links", meta = (DisplayName = "Make Player Link"))
	static FString MakePlayerLink(const FString& PlayerName, int32 MaxDisplayLength = 0);

	/**
	 * Helper pour créer un lien web explicite.
	 */
	UFUNCTION(BlueprintPure, Category = "NexusChat|Links", meta = (DisplayName = "Make URL Link"))
	static FString MakeUrlLink(const FString& Url, const FString& DisplayText = "");

	/**
	 * Analyse un texte brut et transforme automatiquement les URLs (http/https) en liens cliquables.
	 * Ignore intelligemment les URLs qui sont déjà dans des balises.
	 */
	UFUNCTION(BlueprintPure, Category = "NexusChat|Links", meta = (DisplayName = "Auto Format URLs"))
	static FString AutoFormatUrls(const FString& Text);

	// ────────────────────────────────────────────
	// HELPERS CUSTOM
	// ────────────────────────────────────────────

	/**
	 * Raccourci pour créer un lien d'objet (Item).
	 * 
	 * Type: "item"
	 */
	UFUNCTION(BlueprintPure, Category = "NexusChat|Links", meta = (DisplayName = "Make Item Link"))
	static FString MakeItemLink(const FString& ItemId, const FString& ItemName);

	/**
	 * Raccourci pour créer un lien de quête.
	 * 
	 * Type: "quest"
	 */
	UFUNCTION(BlueprintPure, Category = "NexusChat|Links", meta = (DisplayName = "Make Quest Link"))
	static FString MakeQuestLink(const FString& QuestId, const FString& QuestName);

	/**
	 * Raccourci pour créer un lien de position GPS/Map.
	 * 
	 * Type: "gps"
	 */
	UFUNCTION(BlueprintPure, Category = "NexusChat|Links", meta = (DisplayName = "Make GPS Link"))
	static FString MakeLocationLink(const FString& LocationId, const FString& LocationName);
};