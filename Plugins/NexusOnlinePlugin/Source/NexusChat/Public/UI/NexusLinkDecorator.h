#pragma once
#include "CoreMinimal.h"
#include "Components/RichTextBlockDecorator.h"
#include "NexusLinkDecorator.generated.h"

class URichTextBlock;


UCLASS()
class NEXUSCHAT_API UNexusLinkDecorator : public URichTextBlockDecorator
{
	GENERATED_BODY()

public:
	UNexusLinkDecorator(const FObjectInitializer& ObjectInitializer);

	virtual TSharedPtr<ITextDecorator> CreateDecorator(URichTextBlock* InOwner) override;
	
	void HandleLinkClick(URichTextBlock* ContextWidget, const FString& LinkType, const FString& LinkData);
};