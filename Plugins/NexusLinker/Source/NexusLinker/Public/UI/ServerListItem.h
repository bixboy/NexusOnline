#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STableRow.h"
#include "Core/NetworkToolTypes.h"


DECLARE_DELEGATE_OneParam(FOnActionClicked, TSharedPtr<FServerProfile>);

class SServerListItem : public SMultiColumnTableRow<TSharedPtr<FServerProfile>>
{
public:
	SLATE_BEGIN_ARGS(SServerListItem) {}
	SLATE_ARGUMENT(TSharedPtr<FServerProfile>, Item)
	SLATE_EVENT(FOnActionClicked, OnJoinClicked)
	SLATE_EVENT(FOnActionClicked, OnStartClicked)
	SLATE_EVENT(FOnActionClicked, OnStopClicked)
SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);
	
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;

private:
	/** La donnée associée à cette ligne */
	TSharedPtr<FServerProfile> ProfileItem;

	/** Les callbacks */
	FOnActionClicked OnJoinClicked;
	FOnActionClicked OnStartClicked;
	FOnActionClicked OnStopClicked;

	// Handlers internes pour les clics
	FReply HandleJoinClicked();
	FReply HandleStartClicked();
	FReply HandleStopClicked();
};