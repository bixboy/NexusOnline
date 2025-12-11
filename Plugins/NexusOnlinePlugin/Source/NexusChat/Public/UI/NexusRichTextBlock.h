#pragma once

#include "CoreMinimal.h"
#include "Components/RichTextBlock.h"
#include "NexusRichTextBlock.generated.h"

/**
 * Custom RichTextBlock with the NexusLinkDecorator built-in.
 * 
 * Use this widget instead of URichTextBlock in your chat UI to automatically
 * support clickable links without needing to manually add the decorator.
 * 
 * Supported link syntax:
 *   <link type="TYPE" data="DATA">Display Text</>
 * 
 * Built-in types: "player", "url"
 * Custom types: Register handlers via UNexusChatSubsystem::RegisterLinkHandler()
 */
UCLASS(meta = (DisplayName = "Nexus Rich Text Block"))
class NEXUSCHAT_API UNexusRichTextBlock : public URichTextBlock
{
	GENERATED_BODY()

public:
	UNexusRichTextBlock(const FObjectInitializer& ObjectInitializer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
};
