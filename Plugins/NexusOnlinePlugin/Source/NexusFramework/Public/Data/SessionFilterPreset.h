#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/SessionSearchFilter.h"
#include "SessionFilterPreset.generated.h"

class USessionFilterRule;
class USessionSortRule;

/**
 * Data asset Blueprint regroupant des filtres et règles avancées.
 */
UCLASS(BlueprintType)
class NEXUSFRAMEWORK_API USessionFilterPreset : public UDataAsset
{
        GENERATED_BODY()

public:
        /** Exemple Blueprint : créer un preset, ajouter plusieurs FSessionSearchFilter puis l'utiliser sur l'AsyncTask_FindSessions. */
        /** Filtres simples appliqués aux QuerySettings et au post-traitement. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Nexus|Online|Filter")
        TArray<FSessionSearchFilter> SimpleFilters;

        /** Règles avancées instanciées. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category="Nexus|Online|Filter")
        TArray<TObjectPtr<USessionFilterRule>> AdvancedRules;

        /** Règles de tri à appliquer dans l'ordre de priorité. */
        UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category="Nexus|Online|Filter")
        TArray<TObjectPtr<USessionSortRule>> SortRules;
};

