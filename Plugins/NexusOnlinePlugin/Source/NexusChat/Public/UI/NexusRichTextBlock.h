#pragma once
#include "CoreMinimal.h"
#include "Components/RichTextBlock.h"
#include "NexusRichTextBlock.generated.h"


UCLASS(meta = (DisplayName = "Nexus Rich Text Block"))
class NEXUSCHAT_API UNexusRichTextBlock : public URichTextBlock
{
	GENERATED_BODY()

public:
	UNexusRichTextBlock(const FObjectInitializer& ObjectInitializer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
};
