#pragma once
#include "CoreMinimal.h"
#include "Core/NetworkToolTypes.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class UNetworkToolSubsystem;


class SNetworkDashboard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNetworkDashboard) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void RefreshServerList();

private:
	// --- Data pour la ListView ---
	TArray<TSharedPtr<FServerProfile>> ServerListItems;
	TSharedPtr<SListView<TSharedPtr<FServerProfile>>> ServerListView;

	// --- Variables d'état UI ---
	int32 LocalClientCount = 2;
	bool bLocalDedicatedServer = false;
	int32 SimPktLag = 0;
	int32 SimPktLoss = 0;

	// --- Composant Log ---
	TSharedPtr<class SMultiLineEditableText> StatusLogBox;

	// --- Helpers ---
	UNetworkToolSubsystem* GetNetworkSubsystem() const;
    
	// --- Callbacks d'affichage ---
	TSharedRef<ITableRow> OnGenerateRowForList(TSharedPtr<FServerProfile> Item, const TSharedRef<STableViewBase>& OwnerTable);
    
	// --- Actions ---
	void OnJoinServer(TSharedPtr<FServerProfile> Profile);
	void OnStartServer(TSharedPtr<FServerProfile> Profile);
	void OnStopServer(TSharedPtr<FServerProfile> Profile);
    
	FReply OnLaunchLocalClusterClicked();
	FReply OnApplyLagClicked();

	void UpdateStatusLog(FString NewLog);
};