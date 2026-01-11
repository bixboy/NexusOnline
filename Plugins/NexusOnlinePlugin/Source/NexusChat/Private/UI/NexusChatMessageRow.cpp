#include "UI/NexusChatMessageRow.h"
#include "UI/NexusChatMessageObj.h"
#include "Core/NexusChatComponent.h"
#include "Core/NexusChatConfig.h"
#include "Core/NexusLinkHelpers.h"

void UNexusChatMessageRow::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	// On récupère l'objet wrapper passé par la ListView
	if (UNexusChatMessageObj* Item = Cast<UNexusChatMessageObj>(ListItemObject))
	{
		InitWidget(Item->Message);
	}
}

void UNexusChatMessageRow::InitWidget_Implementation(const FNexusChatMessage& Message)
{
	if (!MessageText)
	{
		return;
	}

	// ────────────────────────────────────────────────────
	// 1. Récupération de la Config (Couleurs & Préfixes)
	// ────────────────────────────────────────────────────
	FText ChannelPrefix = FText::GetEmpty();
	FLinearColor ChannelColor = FLinearColor::White;
	
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (UNexusChatComponent* ChatComponent = PC->FindComponentByClass<UNexusChatComponent>())
		{
			if (ChatComponent->ChatConfig)
			{
				const UNexusChatConfig* Config = ChatComponent->ChatConfig;
				
				// --- Préfixe ---
				if (const FText* FoundPrefix = Config->ChannelPrefixes.Find(Message.Channel))
				{
					ChannelPrefix = *FoundPrefix;
				}

				// --- Couleur ---
				if (Message.Channel == ENexusChatChannel::Custom)
				{
					if (const FLinearColor* FoundColor = Config->CustomChannelColors.Find(Message.ChannelName))
					{
						ChannelColor = *FoundColor;
					}
				}
				else
				{
					if (const FLinearColor* FoundColor = Config->ChannelColors.Find(Message.Channel))
					{
						ChannelColor = *FoundColor;
					}
				}
			}
		}
	}

	// ────────────────────────────────────────────────────
	// 2. Formatage du Contenu
	// ────────────────────────────────────────────────────
	FString Content = Message.MessageContent;
	FString FormattedContent = UNexusLinkHelpers::AutoFormatUrls(Content);
	
	const bool bIsSystemMessage = (Message.Channel == ENexusChatChannel::System || Message.Channel == ENexusChatChannel::GameLog);

	// ────────────────────────────────────────────────────
	// 3. Construction de la chaîne finale
	// ────────────────────────────────────────────────────
	FString PrefixStr = ChannelPrefix.IsEmpty() ? "" : ChannelPrefix.ToString() + " ";
	FString FullText;
	
	if (bIsSystemMessage)
	{
		FullText = FString::Printf(TEXT("%s%s"), *PrefixStr, *FormattedContent);
	}
	else
	{
		FString PlayerLink = UNexusLinkHelpers::MakePlayerLink(Message.SenderName, 15);
		FullText = FString::Printf(TEXT("%s%s: %s"), *PrefixStr, *PlayerLink, *FormattedContent);
	}

	// ────────────────────────────────────────────────────
	// 4. Mise à jour de l'UI
	// ────────────────────────────────────────────────────
	
	MessageText->SetText(FText::FromString(FullText));
	MessageText->SetDefaultColorAndOpacity(FSlateColor(ChannelColor));
}