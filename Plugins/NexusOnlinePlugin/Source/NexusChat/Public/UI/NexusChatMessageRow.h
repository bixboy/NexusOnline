#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Types/NexusChatTypes.h"
#include "Components/RichTextBlock.h"
#include "NexusChatMessageRow.generated.h"


UCLASS()
class NEXUSCHAT_API UNexusChatMessageRow : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget), BlueprintReadWrite)
	URichTextBlock* MessageText;
	
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	
	UFUNCTION(BlueprintNativeEvent, Category = "NexusChat")
	void InitWidget(const FNexusChatMessage& Message);
	virtual void InitWidget_Implementation(const FNexusChatMessage& Message);
};