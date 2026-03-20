#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/SessionSearchFilter.h"
#include "Filters/SessionFilterRule.h"
#include "Filters/SessionSortRule.h"
#include "SessionFilterPreset.generated.h"


UCLASS(BlueprintType)
class NEXUSFRAMEWORK_API USessionFilterPreset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Filtres simples appliqués aux QuerySettings et au post-traitement. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Nexus|Online|Filter")
	TArray<FSessionSearchFilter> SimpleFilters;

	/** Règles avancées instanciées. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Export, Category="Nexus|Online|Filter")
	TArray<USessionFilterRule*> AdvancedRules;

	/** Règles de tri à appliquer dans l'ordre de priorité. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Export, Category="Nexus|Online|Filter")
	TArray<USessionSortRule*> SortRules;
};
