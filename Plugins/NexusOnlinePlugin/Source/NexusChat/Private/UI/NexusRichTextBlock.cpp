#include "UI/NexusRichTextBlock.h"
#include "UI/NexusLinkDecorator.h"


UNexusRichTextBlock::UNexusRichTextBlock(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	DecoratorClasses.AddUnique(UNexusLinkDecorator::StaticClass());
}

TSharedRef<SWidget> UNexusRichTextBlock::RebuildWidget()
{
	DecoratorClasses.AddUnique(UNexusLinkDecorator::StaticClass());
	return Super::RebuildWidget();
}
