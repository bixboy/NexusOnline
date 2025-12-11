#pragma once

#include "CoreMinimal.h"
#include "Components/RichTextBlockDecorator.h"
#include "NexusLinkDecorator.generated.h"

/**
 * Rich text decorator for handling clickable <link> tags.
 * 
 * Supports: <link type="TYPE" data="DATA">Display Text</>
 * 
 * When clicked, delegates to UNexusChatSubsystem which broadcasts
 * events (OnAnyLinkClicked, OnPlayerLinkClicked, etc.) and calls
 * any registered custom handlers.
 * 
 * Usage: Add to RichTextBlock's DecoratorClasses, or use NexusRichTextBlock.
 */
UCLASS()
class NEXUSCHAT_API UNexusLinkDecorator : public URichTextBlockDecorator
{
	GENERATED_BODY()

public:
	UNexusLinkDecorator(const FObjectInitializer& ObjectInitializer);

	virtual TSharedPtr<ITextDecorator> CreateDecorator(URichTextBlock* InOwner) override;

	/** Called internally by the Slate decorator when a link is clicked */
	void HandleLinkClick(const FString& LinkType, const FString& LinkData);
};
