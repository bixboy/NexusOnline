#include "UI/ServerListItem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ServerListItem"


void SServerListItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
    ProfileItem = InArgs._Item;
    OnJoinClicked = InArgs._OnJoinClicked;
    OnStartClicked = InArgs._OnStartClicked;
    OnStopClicked = InArgs._OnStopClicked;

    SMultiColumnTableRow::Construct(FSuperRowType::FArguments().Padding(FMargin(0, 4)), InOwnerTableView);
}

TSharedRef<SWidget> SServerListItem::GenerateWidgetForColumn(const FName& InColumnName)
{
    // Colonne 1 : Nom du serveur (+ icone d'état si on veut plus tard)
    if (InColumnName == "Name")
    {
        return SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(10, 2)
            [
                SNew(STextBlock)
                .Text(FText::FromString(ProfileItem->ProfileName))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
            ];
    }
    
    // Colonne 2 : IP Address
    if (InColumnName == "IP")
    {
        return SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(10, 2)
            [
                SNew(STextBlock)
                .Text(FText::FromString(ProfileItem->IPAddress))
                .ColorAndOpacity(FSlateColor(FLinearColor::Gray))
            ];
    }
    
    // Colonne 3 : Les Boutons d'action
    if (InColumnName == "Actions")
    {
        return SNew(SHorizontalBox)
            // Bouton JOIN
            + SHorizontalBox::Slot().AutoWidth().Padding(2)
            [
                SNew(SButton)
                .OnClicked(this, &SServerListItem::HandleJoinClicked)
                .ContentPadding(FMargin(5, 2))
                [
                    SNew(STextBlock).Text(LOCTEXT("JoinBtn", "JOIN"))
                ]
            ]
            // Bouton START (Visible seulement si URL configurée)
            + SHorizontalBox::Slot().AutoWidth().Padding(2)
            [
                SNew(SButton)
                .Visibility(ProfileItem->ApiUrl_Start.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
                .OnClicked(this, &SServerListItem::HandleStartClicked)
                .ButtonColorAndOpacity(FLinearColor(0.1f, 0.6f, 0.1f))
                [
                    SNew(STextBlock).Text(LOCTEXT("StartBtn", "START"))
                ]
            ]
            // Bouton STOP
            + SHorizontalBox::Slot().AutoWidth().Padding(2)
            [
                SNew(SButton)
                .Visibility(ProfileItem->ApiUrl_Stop.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
                .OnClicked(this, &SServerListItem::HandleStopClicked)
                .ButtonColorAndOpacity(FLinearColor(0.6f, 0.1f, 0.1f))
                [
                    SNew(STextBlock).Text(LOCTEXT("StopBtn", "STOP"))
                ]
            ];
    }

    return SNullWidget::NullWidget;
}

FReply SServerListItem::HandleJoinClicked()
{
    OnJoinClicked.ExecuteIfBound(ProfileItem);
    return FReply::Handled();
}

FReply SServerListItem::HandleStartClicked()
{
    OnStartClicked.ExecuteIfBound(ProfileItem);
    return FReply::Handled();
}

FReply SServerListItem::HandleStopClicked()
{
    OnStopClicked.ExecuteIfBound(ProfileItem);
    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE