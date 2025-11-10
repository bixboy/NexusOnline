#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystemTypes.h"
#include "SessionSearchFilter.generated.h"

class FOnlineSessionSearch;
class FOnlineSessionSearchResult;
class FOnlineSessionSettings;
struct FOnlineSessionSetting;

/**
 * Type de valeur pour les filtres de session simples.
 */
UENUM(BlueprintType)
enum class ENexusSessionFilterValueType : uint8
{
        String  UMETA(DisplayName="String"),
        Int32   UMETA(DisplayName="Int32"),
        Float   UMETA(DisplayName="Float"),
        Bool    UMETA(DisplayName="Bool")
};

/**
 * Opérations de comparaison Blueprint exposées.
 */
UENUM(BlueprintType)
enum class ENexusSessionComparisonOp : uint8
{
        Equals              UMETA(DisplayName="=="),
        NotEquals           UMETA(DisplayName="!="),
        GreaterThan         UMETA(DisplayName="> "),
        GreaterThanEquals   UMETA(DisplayName="> ="),
        LessThan            UMETA(DisplayName="<"),
        LessThanEquals      UMETA(DisplayName="<="),
        Exists              UMETA(DisplayName="Exists")
};

/**
 * Valeur Blueprint configurable utilisée par FSessionSearchFilter.
 */
USTRUCT(BlueprintType)
struct FSessionFilterValue
{
        GENERATED_BODY()

public:
        /** Type de la valeur comparée. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session")
        ENexusSessionFilterValueType Type = ENexusSessionFilterValueType::String;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session", meta=(EditCondition="Type==ENexusSessionFilterValueType::String"))
        FString StringValue;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session", meta=(EditCondition="Type==ENexusSessionFilterValueType::Int32"))
        int32 IntValue = 0;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session", meta=(EditCondition="Type==ENexusSessionFilterValueType::Float"))
        float FloatValue = 0.f;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session", meta=(EditCondition="Type==ENexusSessionFilterValueType::Bool"))
        bool bBoolValue = false;

        /** Convertit la valeur en FVariantData. */
        bool ToVariantData(struct FVariantData& OutData) const;

        /** Compare la valeur locale avec un FVariantData distant. */
        bool CompareVariant(const struct FVariantData& OtherData, EOnlineComparisonOp::Type Op) const;

        /** Chaîne lisible utilisée pour le debug. */
        FString ToDebugString() const;
};

/**
 * Filtre simple configuré côté Blueprint, appliqué aux QuerySettings et en post-filtrage.
 */
USTRUCT(BlueprintType)
struct FSessionSearchFilter
{
        GENERATED_BODY()

public:
        /** Clé utilisée dans les SessionSettings (ex : REGION_KEY). */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session")
        FName Key = NAME_None;

        /** Valeur attendue. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session")
        FSessionFilterValue Value;

        /** Comparaison utilisée pour les filtres simples. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session")
        ENexusSessionComparisonOp ComparisonOp = ENexusSessionComparisonOp::Equals;

        /** Détermine si le filtre est envoyé dans QuerySettings. */
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nexus|Online|Session")
        bool bApplyToQuerySettings = true;

        /** Application directe aux QuerySettings. */
        bool ApplyToSearchSettings(FOnlineSessionSearch& Search) const;

        /** Vérifie si un résultat correspond au filtre. */
        bool MatchesResult(const FOnlineSessionSearchResult& Result) const;

        /** Convertit l'opération Blueprint vers l'opération OnlineSubsystem. */
        EOnlineComparisonOp::Type ToOnlineOp() const;
};

/** Ensemble d'outils utilitaires pour l'exploitation des filtres. */
namespace NexusSessionFilterUtils
{
        /** Applique un tableau de filtres sur des SessionSettings (création ou requête). */
        void ApplyFiltersToSettings(const TArray<FSessionSearchFilter>& Filters, FOnlineSessionSearch& SearchSettings);
        void ApplyFiltersToSettings(const TArray<FSessionSearchFilter>& Filters, FOnlineSessionSettings& SessionSettings);

        /** Vérifie un résultat contre une liste de filtres. */
        bool PassesAllFilters(const TArray<FSessionSearchFilter>& Filters, const FOnlineSessionSearchResult& Result);
}

