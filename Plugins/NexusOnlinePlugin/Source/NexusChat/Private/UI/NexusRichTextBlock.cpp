#include "UI/NexusRichTextBlock.h"
#include "UI/NexusLinkDecorator.h"

UNexusRichTextBlock::UNexusRichTextBlock(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Automatically add the NexusLinkDecorator to this widget
	DecoratorClasses.AddUnique(UNexusLinkDecorator::StaticClass());
}

TSharedRef<SWidget> UNexusRichTextBlock::RebuildWidget()
{
	// Ensure the decorator is always present
	DecoratorClasses.AddUnique(UNexusLinkDecorator::StaticClass());
	
	return Super::RebuildWidget();
}
