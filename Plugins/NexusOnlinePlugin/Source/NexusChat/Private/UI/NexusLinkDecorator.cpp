#include "UI/NexusLinkDecorator.h"
#include "Core/NexusChatSubsystem.h"
#include "Components/RichTextBlock.h"
#include "Framework/Text/SlateHyperlinkRun.h"
#include "Styling/SlateTypes.h"
#include "Engine/World.h"
#include "Engine/Engine.h"


class FNexusLinkTextDecorator : public ITextDecorator
{
public:
	FNexusLinkTextDecorator(UNexusLinkDecorator* InOwner, URichTextBlock* InWidgetOwner) : Owner(InOwner), WidgetOwner(InWidgetOwner)
	{
	}

	virtual bool Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const override
	{
		return RunParseResult.Name == TEXT("link");
	}

	virtual TSharedRef<ISlateRun> Create(const TSharedRef<FTextLayout>& TextLayout, const FTextRunParseResults& RunParseResult, const FString& OriginalText,
		const TSharedRef<FString>& InOutModelText, const ISlateStyle* Style) override
	{
		// ──────────────────────────────────────────────
		// 1. Extraction des données (Parse)
		// ──────────────────────────────────────────────
		const FTextRange& ContentRange = RunParseResult.ContentRange;
		FString DisplayText = OriginalText.Mid(ContentRange.BeginIndex, ContentRange.EndIndex - ContentRange.BeginIndex);

		FString LinkType;
		if (const FTextRange* TypeAttr = RunParseResult.MetaData.Find(TEXT("type")))
		{
			LinkType = OriginalText.Mid(TypeAttr->BeginIndex, TypeAttr->EndIndex - TypeAttr->BeginIndex);
		}

		FString LinkData;
		if (const FTextRange* DataAttr = RunParseResult.MetaData.Find(TEXT("data")))
		{
			LinkData = OriginalText.Mid(DataAttr->BeginIndex, DataAttr->EndIndex - DataAttr->BeginIndex);
			LinkData = LinkData.Replace(TEXT("&quot;"), TEXT("\""));
		}

		// ──────────────────────────────────────────────
		// 2. Construction du Modèle
		// ──────────────────────────────────────────────
		const int32 StartIndex = InOutModelText->Len();
		InOutModelText->Append(DisplayText);
		FTextRange ModelRange(StartIndex, InOutModelText->Len());

		// ──────────────────────────────────────────────
		// 3. Styling (Couleurs & Hover)
		// ──────────────────────────────────────────────
		
		FTextBlockStyle TextStyle = WidgetOwner.IsValid() ? WidgetOwner->GetDefaultTextStyle() : FTextBlockStyle::GetDefault();
		TextStyle.SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.6f, 1.0f)));

		FHyperlinkStyle HyperlinkStyle;
		HyperlinkStyle.SetTextStyle(TextStyle);

		// ──────────────────────────────────────────────
		// 4. Création du Run Interactif
		// ──────────────────────────────────────────────
		UNexusLinkDecorator* OwnerPtr = Owner;
		TWeakObjectPtr<URichTextBlock> WeakWidget = WidgetOwner;

		return FSlateHyperlinkRun::Create(
			FRunInfo(RunParseResult.Name),
			InOutModelText,
			HyperlinkStyle,
			FSlateHyperlinkRun::FOnClick::CreateLambda([OwnerPtr, WeakWidget, LinkType, LinkData](const FSlateHyperlinkRun::FMetadata&)
			{
				if (OwnerPtr)
				{
					OwnerPtr->HandleLinkClick(WeakWidget.Get(), LinkType, LinkData);
				}
			}),
			FSlateHyperlinkRun::FOnGenerateTooltip(),
			FSlateHyperlinkRun::FOnGetTooltipText(),
			ModelRange
		);
	}

private:
	UNexusLinkDecorator* Owner;
	TWeakObjectPtr<URichTextBlock> WidgetOwner;
};

// ──────────────────────────────────────────────────────────────────────────────
// IMPLEMENTATION DE LA CLASSE UOBJECT
// ──────────────────────────────────────────────────────────────────────────────

UNexusLinkDecorator::UNexusLinkDecorator(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

TSharedPtr<ITextDecorator> UNexusLinkDecorator::CreateDecorator(URichTextBlock* InOwner)
{
	return MakeShared<FNexusLinkTextDecorator>(this, InOwner);
}

void UNexusLinkDecorator::HandleLinkClick(URichTextBlock* ContextWidget, const FString& LinkType, const FString& LinkData)
{
	UWorld* World = nullptr;
	if (ContextWidget)
	{
		World = ContextWidget->GetWorld();
	}
	
	if (!World)
	{
		World = GEngine->GetCurrentPlayWorld();
	}

	if (World)
	{
		if (UNexusChatSubsystem* ChatSubsystem = World->GetSubsystem<UNexusChatSubsystem>())
		{
			ChatSubsystem->HandleLinkClicked(LinkType, LinkData);
		}
	}
}