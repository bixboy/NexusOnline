#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ListView.h"
#include "NexusChatChannelList.generated.h"

class UButton;
class UNexusChatWindow;

UCLASS()
class NEXUSCHAT_API UNexusChatChannelList : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	UListView* ChannelListView;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* CloseButton;

	UFUNCTION(BlueprintCallable, Category = "NexusChat")
	void RefreshList();

    void Init(UNexusChatWindow* InChatWindow);

protected:
	virtual void NativeConstruct() override;
    
    UFUNCTION()
    void OnChannelItemClicked(UObject* Item);
    
    UFUNCTION()
    void OnCloseClicked();

private:
    UPROPERTY()
    UNexusChatWindow* ChatWindow;
};
