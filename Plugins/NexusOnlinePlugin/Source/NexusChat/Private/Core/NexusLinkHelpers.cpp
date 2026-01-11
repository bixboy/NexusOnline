#include "Core/NexusLinkHelpers.h"
#include "Internationalization/Regex.h"


FString UNexusLinkHelpers::MakeLink(const FString& Type, const FString& Data, const FString& DisplayText)
{
	FString SafeData = Data.Replace(TEXT("&"), TEXT("&amp;"));
	SafeData = SafeData.Replace(TEXT("\""), TEXT("&quot;"));

	return FString::Printf(TEXT("<link type=\"%s\" data=\"%s\">%s</>"), *Type, *SafeData, *DisplayText);
}

FString UNexusLinkHelpers::MakePlayerLink(const FString& PlayerName, int32 MaxDisplayLength)
{
	FString DisplayName = PlayerName;
	
	if (MaxDisplayLength > 0 && DisplayName.Len() > MaxDisplayLength)
	{
		DisplayName = DisplayName.Left(MaxDisplayLength) + TEXT("...");
	}
	
	return MakeLink(TEXT("player"), PlayerName, DisplayName);
}

FString UNexusLinkHelpers::MakeUrlLink(const FString& Url, const FString& DisplayText)
{
	const FString Display = DisplayText.IsEmpty() ? Url : DisplayText;
	return MakeLink(TEXT("url"), Url, Display);
}

FString UNexusLinkHelpers::AutoFormatUrls(const FString& Text)
{
	const FRegexPattern UrlPattern(TEXT("(https?://[^\\s<>\\\"]+)"));
	FRegexMatcher Matcher(UrlPattern, Text);

	FString Result = Text;
	int32 Offset = 0;

	while (Matcher.FindNext())
	{
		const int32 MatchStart = Matcher.GetMatchBeginning();
		const int32 MatchEnd = Matcher.GetMatchEnding();
		const FString Url = Text.Mid(MatchStart, MatchEnd - MatchStart);
		
		const FString Before = Text.Left(MatchStart);
		if (Before.EndsWith(TEXT("data=\"")))
		{
			continue;
		}
		
		const FString FormattedUrl = MakeUrlLink(Url, Url);
		const int32 AdjustedStart = MatchStart + Offset;
		const int32 AdjustedEnd = MatchEnd + Offset;
		
		Result = Result.Left(AdjustedStart) + FormattedUrl + Result.Mid(AdjustedEnd);
		Offset += FormattedUrl.Len() - Url.Len();
	}

	return Result;
}

// ────────────────────────────────────────────
// HELPERS CUSTOM RAPIDES
// ────────────────────────────────────────────

FString UNexusLinkHelpers::MakeItemLink(const FString& ItemId, const FString& ItemName)
{
	return MakeLink(TEXT("item"), ItemId, FString::Printf(TEXT("[%s]"), *ItemName));
}

FString UNexusLinkHelpers::MakeQuestLink(const FString& QuestId, const FString& QuestName)
{
	return MakeLink(TEXT("quest"), QuestId, FString::Printf(TEXT("{Quête: %s}"), *QuestName));
}

FString UNexusLinkHelpers::MakeLocationLink(const FString& LocationId, const FString& LocationName)
{
	return MakeLink(TEXT("gps"), LocationId, FString::Printf(TEXT("%s"), *LocationName));
}