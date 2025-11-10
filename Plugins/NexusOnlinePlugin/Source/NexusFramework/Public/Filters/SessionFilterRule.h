#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SessionFilterRule.generated.h"

class FOnlineSessionSearch;
class FOnlineSessionSearchResult;


DECLARE_LOG_CATEGORY_EXTERN(LogNexusOnlineFilter, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogNexusOnlineSort, Log, All);


/**
 * Règle de filtrage complexe pouvant être héritée côté C++.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class NEXUSFRAMEWORK_API USessionFilterRule : public UObject
{
    GENERATED_BODY()

public:
    /** Active/Désactive dynamiquement la règle. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Filter")
    bool bEnabled = true;

    /** Permet d'appliquer la règle directement aux QuerySettings (pré-filtre). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Filter")
    bool bApplyToSearchQuery = true;

    /** Priorité arbitraire pour organiser l'ordre d'exécution. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Filter")
    int32 Priority = 0;

    /** Point d'extension pour modifier les QuerySettings avant l'appel réseau. */
    virtual void ConfigureSearchSettings(FOnlineSessionSearch& SearchSettings) const;

    /** Vérifie si le résultat répond à la règle (post-filtre). */
    virtual bool PassesFilter(const FOnlineSessionSearchResult& Result) const;

    /** Texte de debug affiché dans les logs. */
    virtual FString GetRuleDescription() const;
};

