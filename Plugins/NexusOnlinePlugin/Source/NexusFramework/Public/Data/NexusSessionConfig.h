#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Types/OnlineSessionData.h"
#include "Data/SessionFilterPreset.h"
#include "NexusSessionConfig.generated.h"

class FOnlineSessionSettings;


UCLASS(Blueprintable, BlueprintType)
class NEXUSFRAMEWORK_API UNexusSessionConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Nexus|Config", meta=(ShowOnlyInnerProperties))
	FSessionSettingsData SessionSettings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Nexus|Config")
	TObjectPtr<USessionFilterPreset> SearchMetadata;

	virtual void ModifySessionSettings(FOnlineSessionSettings& OutSettings) const;
	
	void ApplyToOnlineSettings(FOnlineSessionSettings& OutSettings) const;
};
