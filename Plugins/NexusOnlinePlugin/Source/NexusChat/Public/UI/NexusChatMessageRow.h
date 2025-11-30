#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/NexusChatTypes.h"
#include "Components/RichTextBlock.h"
#include "NexusChatMessageRow.generated.h"

UCLASS()
class NEXUSCHAT_API UNexusChatMessageRow : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	URichTextBlock* MessageText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NexusChat")
	TMap<ENexusChatChannel, FName> ChannelStyles;

	virtual void NativeConstruct() override;
	
	UFUNCTION(BlueprintNativeEvent, Category = "NexusChat")
	void InitWidget(const FNexusChatMessage& Message);
	virtual void InitWidget_Implementation(const FNexusChatMessage& Message);
};
