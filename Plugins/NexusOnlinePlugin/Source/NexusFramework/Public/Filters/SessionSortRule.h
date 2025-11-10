#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SessionSortRule.generated.h"

class FOnlineSessionSearch;
class FOnlineSessionSearchResult;

/**
 * Règle de tri personnalisable pour organiser les résultats.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class NEXUSFRAMEWORK_API USessionSortRule : public UObject
{
        GENERATED_BODY()

public:
        /** Active/Désactive la règle. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Sort")
        bool bEnabled = true;

        /** Priorité croissante, les règles les plus basses s'exécutent en premier. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Sort")
        int32 Priority = 0;

        /** Possibilité d'influencer les QuerySettings (ex: ping max). */
        virtual void ConfigureSearchSettings(FOnlineSessionSearch& SearchSettings) const;

        /** Méthode de comparaison utilisée pour le tri final. */
        virtual bool Compare(const FOnlineSessionSearchResult& A, const FOnlineSessionSearchResult& B) const;

        /** Description lisible utilisée pour les logs. */
        virtual FString GetRuleDescription() const;
};

