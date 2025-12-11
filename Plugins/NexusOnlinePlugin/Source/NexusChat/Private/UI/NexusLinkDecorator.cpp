#include "UI/NexusLinkDecorator.h"
#include "Core/NexusChatSubsystem.h"
#include "Components/RichTextBlock.h"
#include "Framework/Text/SlateHyperlinkRun.h"
#include "Styling/SlateTypes.h"

/**
 * Internal Slate decorator that handles <link> tag parsing and rendering.
 * Creates clickable hyperlinks with custom styling.
 */
class FNexusLinkTextDecorator : public ITextDecorator
{
public:
	FNexusLinkTextDecorator(UNexusLinkDecorator* InOwner)
		: Owner(InOwner)
	{
	}

	virtual bool Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const override
	{
		return RunParseResult.Name == TEXT("link");
	}

	virtual TSharedRef<ISlateRun> Create(
		const TSharedRef<class FTextLayout>& TextLayout,
		const FTextRunParseResults& RunParseResult,
		const FString& OriginalText,
		const TSharedRef<FString>& InOutModelText,
		const ISlateStyle* Style) override
	{
		// Extract display text (content between <link> and </>)
		const FTextRange& ContentRange = RunParseResult.ContentRange;
		FString DisplayText = OriginalText.Mid(ContentRange.BeginIndex, ContentRange.EndIndex - ContentRange.BeginIndex);

		// Extract "type" attribute
		FString LinkType;
		if (const FTextRange* TypeAttr = RunParseResult.MetaData.Find(TEXT("type")))
		{
			LinkType = OriginalText.Mid(TypeAttr->BeginIndex, TypeAttr->EndIndex - TypeAttr->BeginIndex);
		}

		// Extract "data" attribute
		FString LinkData;
		if (const FTextRange* DataAttr = RunParseResult.MetaData.Find(TEXT("data")))
		{
			LinkData = OriginalText.Mid(DataAttr->BeginIndex, DataAttr->EndIndex - DataAttr->BeginIndex);
			LinkData = LinkData.Replace(TEXT("&quot;"), TEXT("\""));
		}

		// Build model text range
		const int32 StartIndex = InOutModelText->Len();
		InOutModelText->Append(DisplayText);
		FTextRange ModelRange(StartIndex, InOutModelText->Len());

		// Build run info with metadata
		FRunInfo RunInfo(RunParseResult.Name);
		for (const TPair<FString, FTextRange>& Pair : RunParseResult.MetaData)
		{
			RunInfo.MetaData.Add(Pair.Key, OriginalText.Mid(Pair.Value.BeginIndex, Pair.Value.EndIndex - Pair.Value.BeginIndex));
		}

		// Configure hyperlink style (light blue)
		FTextBlockStyle TextStyle = FTextBlockStyle::GetDefault();
		TextStyle.SetColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.7f, 1.0f)));
		
		FHyperlinkStyle HyperlinkStyle;
		HyperlinkStyle.SetTextStyle(TextStyle);

		// Create hyperlink with click handler
		UNexusLinkDecorator* OwnerPtr = Owner;
		return FSlateHyperlinkRun::Create(
			RunInfo,
			InOutModelText,
			HyperlinkStyle,
			FSlateHyperlinkRun::FOnClick::CreateLambda([OwnerPtr, LinkType, LinkData](const FSlateHyperlinkRun::FMetadata&)
			{
				if (OwnerPtr)
				{
					OwnerPtr->HandleLinkClick(LinkType, LinkData);
				}
			}),
			FSlateHyperlinkRun::FOnGenerateTooltip(),
			FSlateHyperlinkRun::FOnGetTooltipText(),
			ModelRange
		);
	}

private:
	UNexusLinkDecorator* Owner;
};

// ──────────────────────────────────────────────────────────────────────────────

UNexusLinkDecorator::UNexusLinkDecorator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedPtr<ITextDecorator> UNexusLinkDecorator::CreateDecorator(URichTextBlock* InOwner)
{
	return MakeShared<FNexusLinkTextDecorator>(this);
}

void UNexusLinkDecorator::HandleLinkClick(const FString& LinkType, const FString& LinkData)
{
	if (UWorld* World = GEngine->GetCurrentPlayWorld())
	{
		if (UNexusChatSubsystem* ChatSubsystem = World->GetSubsystem<UNexusChatSubsystem>())
		{
			ChatSubsystem->HandleLinkClicked(LinkType, LinkData);
		}
	}
}
