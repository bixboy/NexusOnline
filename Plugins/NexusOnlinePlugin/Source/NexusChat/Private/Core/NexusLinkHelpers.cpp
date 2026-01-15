#include "Core/NexusLinkHelpers.h"
#include "Internationalization/Regex.h"

FString UNexusLinkHelpers::MakeLink(const FString& Type, const FString& Data, const FString& DisplayText)
{
	FString SafeData = Data.Replace(TEXT("\""), TEXT("&quot;"));
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
	// Don't process if text already contains link tags
	if (Text.Contains(TEXT("<link")))
	{
		return Text;
	}

	// Regex pattern to match URLs (http:// or https://)
	const FRegexPattern UrlPattern(TEXT("(https?://[^\\s<>\"]+)"));
	FRegexMatcher Matcher(UrlPattern, Text);

	FString Result = Text;
	int32 Offset = 0; // Track position offset as we insert longer strings

	while (Matcher.FindNext())
	{
		const int32 MatchStart = Matcher.GetMatchBeginning();
		const int32 MatchEnd = Matcher.GetMatchEnding();
		const FString Url = Text.Mid(MatchStart, MatchEnd - MatchStart);
		
		// Skip if URL appears to be inside a data attribute
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

FString UNexusLinkHelpers::MakeItemLink(const FString& ItemId, const FString& ItemName)
{
	return MakeLink(TEXT("item"), ItemId, ItemName);
}

FString UNexusLinkHelpers::MakeQuestLink(const FString& QuestId, const FString& QuestName)
{
	return MakeLink(TEXT("quest"), QuestId, QuestName);
}

FString UNexusLinkHelpers::MakeLocationLink(const FString& LocationId, const FString& LocationName)
{
	return MakeLink(TEXT("location"), LocationId, LocationName);
}
