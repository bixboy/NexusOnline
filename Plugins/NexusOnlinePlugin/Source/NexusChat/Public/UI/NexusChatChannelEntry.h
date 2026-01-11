#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Types/NexusChatTypes.h"
#include "NexusChatChannelEntry.generated.h"

class UTextBlock;
class UBorder;

/**
 * Data object for the list view
 */
UCLASS(BlueprintType)
class NEXUSCHAT_API UNexusChatChannelData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "NexusChat")
	FName ChannelName;

	UPROPERTY(BlueprintReadOnly, Category = "NexusChat")
	bool bIsPrivate = false;

	UPROPERTY(BlueprintReadOnly, Category = "NexusChat")
	bool bIsHeader = false;
};


UCLASS()
class NEXUSCHAT_API UNexusChatChannelEntry : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ChannelNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	UBorder* BackgroundBorder;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	UPROPERTY()
	UNexusChatChannelData* MyData;
};
