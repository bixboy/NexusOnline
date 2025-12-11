#include "UI/NexusHyperlinkDecorator.h"
#include "Components/RichTextBlock.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Framework/Text/SlateTextRun.h"
#include "Framework/Text/SlateHyperlinkRun.h"
#include "Framework/Text/RichTextMarkupProcessing.h"
#include "HAL/PlatformProcess.h"
#include "Internationalization/Regex.h"
#include "Styling/SlateTypes.h"

// ──────────────────────────────────────────────
// INTERNAL SLATE DECORATOR
// ──────────────────────────────────────────────

class FNexusRichTextDecorator : public ITextDecorator
{
public:
	FNexusRichTextDecorator(UNexusHyperlinkDecorator* InOwner)
		: Owner(InOwner)
	{
	}

	virtual bool Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const override
	{
		// Support <a> tags
		return RunParseResult.Name == TEXT("a");
	}

	// UE 5.6 compatible signature (non-const method)
	virtual TSharedRef<ISlateRun> Create(
		const TSharedRef<class FTextLayout>& TextLayout,
		const FTextRunParseResults& RunParseResult,
		const FString& OriginalText,
		const TSharedRef<FString>& InOutModelText,
		const ISlateStyle* Style) override
	{
		FString DisplayText;
		FString LinkId;
		FString LinkHref;

		// Extract the display text (content between <a> and </a>)
		const FTextRange& ContentRange = RunParseResult.ContentRange;
		DisplayText = OriginalText.Mid(ContentRange.BeginIndex, ContentRange.EndIndex - ContentRange.BeginIndex);

		// Extract attributes
		const FTextRange* IdAttr = RunParseResult.MetaData.Find(TEXT("id"));
		if (IdAttr)
		{
			LinkId = OriginalText.Mid(IdAttr->BeginIndex, IdAttr->EndIndex - IdAttr->BeginIndex);
		}

		const FTextRange* HrefAttr = RunParseResult.MetaData.Find(TEXT("href"));
		if (HrefAttr)
		{
			LinkHref = OriginalText.Mid(HrefAttr->BeginIndex, HrefAttr->EndIndex - HrefAttr->BeginIndex);
		}

		// Append display text to model
		const int32 StartIndex = InOutModelText->Len();
		InOutModelText->Append(DisplayText);
		const int32 EndIndex = InOutModelText->Len();

		FTextRange ModelRange(StartIndex, EndIndex);

		// Create run info with metadata
		FRunInfo RunInfo(RunParseResult.Name);
		for (const TPair<FString, FTextRange>& Pair : RunParseResult.MetaData)
		{
			RunInfo.MetaData.Add(Pair.Key, OriginalText.Mid(Pair.Value.BeginIndex, Pair.Value.EndIndex - Pair.Value.BeginIndex));
		}

		// Create hyperlink style
		FTextBlockStyle TextStyle = FTextBlockStyle::GetDefault();
		TextStyle.SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.6f, 1.0f))); // Blue link color

		FHyperlinkStyle HyperlinkStyle;
		HyperlinkStyle.SetTextStyle(TextStyle);

		// Capture Owner pointer for lambda
		UNexusHyperlinkDecorator* OwnerPtr = Owner;

		// Create hyperlink run with click handler
		// UE 5.6 signature: RunInfo, Text, Style, OnClick, OnGenerateTooltip, OnGetTooltipText, Range
		TSharedRef<FSlateHyperlinkRun> HyperlinkRun = FSlateHyperlinkRun::Create(
			RunInfo,
			InOutModelText,
			HyperlinkStyle,
			FSlateHyperlinkRun::FOnClick::CreateLambda([OwnerPtr, LinkId, LinkHref](const FSlateHyperlinkRun::FMetadata& Metadata)
			{
				if (OwnerPtr)
				{
					if (LinkId == TEXT("player"))
					{
						OwnerPtr->HandlePlayerClick(LinkHref);
					}
					else if (LinkId == TEXT("url"))
					{
						OwnerPtr->HandleUrlClick(LinkHref);
					}
				}
			}),
			FSlateHyperlinkRun::FOnGenerateTooltip(),
			FSlateHyperlinkRun::FOnGetTooltipText(),
			ModelRange
		);

		return HyperlinkRun;
	}

private:
	UNexusHyperlinkDecorator* Owner;
};

// ──────────────────────────────────────────────
// UNEXUS HYPERLINK DECORATOR
// ──────────────────────────────────────────────

UNexusHyperlinkDecorator::UNexusHyperlinkDecorator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedPtr<ITextDecorator> UNexusHyperlinkDecorator::CreateDecorator(URichTextBlock* InOwner)
{
	return MakeShared<FNexusRichTextDecorator>(this);
}

void UNexusHyperlinkDecorator::HandlePlayerClick(const FString& PlayerName)
{
	OnPlayerClicked.Broadcast(PlayerName);
}

void UNexusHyperlinkDecorator::HandleUrlClick(const FString& Url)
{
	// Open URL in default browser
	FPlatformProcess::LaunchURL(*Url, nullptr, nullptr);
	
	// Broadcast event for any additional handling
	OnUrlClicked.Broadcast(Url);
}

FString UNexusHyperlinkDecorator::MakePlayerLink(const FString& PlayerName, int32 MaxDisplayLength)
{
	FString DisplayName = PlayerName;
	if (MaxDisplayLength > 0 && DisplayName.Len() > MaxDisplayLength)
	{
		DisplayName = DisplayName.Left(MaxDisplayLength) + TEXT("...");
	}
	
	return FString::Printf(TEXT("<a id=\"player\" href=\"%s\">%s</>"), *PlayerName, *DisplayName);
}

FString UNexusHyperlinkDecorator::MakeUrlLink(const FString& Url, const FString& DisplayText)
{
	const FString Display = DisplayText.IsEmpty() ? Url : DisplayText;
	return FString::Printf(TEXT("<a id=\"url\" href=\"%s\">%s</>"), *Url, *Display);
}

FString UNexusHyperlinkDecorator::AutoFormatUrls(const FString& Text)
{
	// Regex pattern to match URLs (http:// or https://)
	const FRegexPattern UrlPattern(TEXT("(https?://[^\\s<>\"]+)"));
	FRegexMatcher Matcher(UrlPattern, Text);

	FString Result = Text;
	TArray<TPair<int32, int32>> Matches;

	// Find all matches first
	while (Matcher.FindNext())
	{
		Matches.Add(TPair<int32, int32>(Matcher.GetMatchBeginning(), Matcher.GetMatchEnding()));
	}

	// Replace from end to start to preserve indices
	for (int32 i = Matches.Num() - 1; i >= 0; --i)
	{
		const int32 Start = Matches[i].Key;
		const int32 End = Matches[i].Value;
		const FString Url = Text.Mid(Start, End - Start);
		
		// Don't format if already inside an <a> tag
		const FString Before = Result.Left(Start);
		if (!Before.EndsWith(TEXT("href=\"")))
		{
			const FString FormattedUrl = MakeUrlLink(Url, Url);
			Result = Result.Left(Start) + FormattedUrl + Result.Mid(End);
		}
	}

	return Result;
}
