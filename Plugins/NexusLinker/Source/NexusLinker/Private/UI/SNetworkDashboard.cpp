#include "UI/SNetworkDashboard.h"
#include "Core/NetworkToolSubsystem.h"
#include "Core/NetworkToolSettings.h"
#include "Editor.h"
#include "UI/ServerListItem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/SMultiLineEditableText.h"

#define LOCTEXT_NAMESPACE "NetworkDashboard"


void SNetworkDashboard::Construct(const FArguments& InArgs)
{
    if (UNetworkToolSubsystem* Subsystem = GetNetworkSubsystem())
    {
        Subsystem->OnServerStatusLog.AddSP(this, &SNetworkDashboard::UpdateStatusLog);
    }

    ChildSlot
    [
        SNew(SSplitter)
        .Orientation(Orient_Horizontal)

        // ------------------------------------------------
        // PANNEAU GAUCHE : LISTE DES SERVEURS
        // ------------------------------------------------
        + SSplitter::Slot()
        .Value(0.6f)
        [
            SNew(SVerticalBox)
            // Header de la liste
            + SVerticalBox::Slot().AutoHeight().Padding(5)
            [
                SNew(STextBlock).Text(LOCTEXT("ServerListTitle", "Remote Servers Profiles"))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
            ]
            // La Liste elle-même
            + SVerticalBox::Slot().FillHeight(1.0f).Padding(5)
            [
                SAssignNew(ServerListView, SListView<TSharedPtr<FServerProfile>>)
                .ItemHeight(30)
                .ListItemsSource(&ServerListItems)
                .OnGenerateRow(this, &SNetworkDashboard::OnGenerateRowForList)
                .HeaderRow
                (
                    SNew(SHeaderRow)
                    + SHeaderRow::Column("Name").DefaultLabel(LOCTEXT("ColName", "Server Name")).FillWidth(0.3f)
                    + SHeaderRow::Column("IP").DefaultLabel(LOCTEXT("ColIP", "IP Address")).FillWidth(0.3f)
                    + SHeaderRow::Column("Actions").DefaultLabel(LOCTEXT("ColActions", "Controls")).FillWidth(0.4f)
                )
            ]
            // Bouton Refresh
            + SVerticalBox::Slot().AutoHeight().Padding(5)
            [
                SNew(SButton)
                .HAlign(HAlign_Center)
                .OnClicked_Lambda([this]() { RefreshServerList(); return FReply::Handled(); })
                .Content()
                [
                    SNew(STextBlock).Text(LOCTEXT("RefreshBtn", "Reload Profiles from Settings"))
                ]
            ]
        ]

        // ------------------------------------------------
        // PANNEAU DROIT : OUTILS LOCAUX & LOGS
        // ------------------------------------------------
        + SSplitter::Slot()
        .Value(0.4f)
        [
            SNew(SVerticalBox)
            
            // --- ZONE 1: LOCAL CLUSTER ---
            + SVerticalBox::Slot().AutoHeight().Padding(10)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock).Text(LOCTEXT("LocalTitle", "Local Cluster Testing"))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 5)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().Padding(0, 2, 10, 0)
                    [
                        SNew(STextBlock).Text(LOCTEXT("ClientCount", "Clients:"))
                    ]
                    + SHorizontalBox::Slot().FillWidth(1.0f)
                    [
                        SNew(SSpinBox<int32>)
                        .MinValue(1).MaxValue(8)
                        .Value(LocalClientCount)
                        .OnValueChanged_Lambda([this](int32 NewVal) { LocalClientCount = NewVal; })
                    ]
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 5)
                [
                    SNew(SCheckBox)
                    .IsChecked(ECheckBoxState::Unchecked)
                    .OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { bLocalDedicatedServer = (NewState == ECheckBoxState::Checked); })
                    .Content()
                    [
                        SNew(STextBlock).Text(LOCTEXT("DedServer", "Run Dedicated Server"))
                    ]
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 10)
                [
                    SNew(SButton)
                    .HAlign(HAlign_Center)
                    .OnClicked(this, &SNetworkDashboard::OnLaunchLocalClusterClicked)
                    .Content() [ SNew(STextBlock).Text(LOCTEXT("LaunchBtn", "LAUNCH LOCAL CLUSTER")) ]
                ]
            ]

            + SVerticalBox::Slot().AutoHeight() [ SNew(SSeparator) ]

            // --- ZONE 2: NETWORK EMULATION ---
            + SVerticalBox::Slot().AutoHeight().Padding(10)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock).Text(LOCTEXT("LagTitle", "Network Emulation"))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 5)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 10, 0) [ SNew(STextBlock).Text(LOCTEXT("LagLbl", "Lag (ms):")) ]
                    + SHorizontalBox::Slot().FillWidth(1.0f)
                    [
                        SNew(SSpinBox<int32>).MinValue(0).MaxValue(2000).Value(0)
                        .OnValueChanged_Lambda([this](int32 Val) { SimPktLag = Val; })
                    ]
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 5)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 10, 0) [ SNew(STextBlock).Text(LOCTEXT("LossLbl", "Loss (%):")) ]
                    + SHorizontalBox::Slot().FillWidth(1.0f)
                    [
                        SNew(SSpinBox<int32>).MinValue(0).MaxValue(100).Value(0)
                        .OnValueChanged_Lambda([this](int32 Val) { SimPktLoss = Val; })
                    ]
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 5)
                [
                    SNew(SButton).HAlign(HAlign_Center).OnClicked(this, &SNetworkDashboard::OnApplyLagClicked)
                    .Content() [ SNew(STextBlock).Text(LOCTEXT("ApplyLag", "Apply Lag Settings")) ]
                ]
            ]

            + SVerticalBox::Slot().AutoHeight() [ SNew(SSeparator) ]

            // --- ZONE 3: LOGS ---
            + SVerticalBox::Slot().FillHeight(1.0f).Padding(10)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight() [ SNew(STextBlock).Text(LOCTEXT("LogTitle", "Tool Status Log")) ]
                + SVerticalBox::Slot().FillHeight(1.0f)
                [
                    SAssignNew(StatusLogBox, SMultiLineEditableText)
                    .IsReadOnly(true)
                    .AutoWrapText(true)
                    .TextStyle(FCoreStyle::Get(), "Log.Normal")
                ]
            ]
        ]
    ];

    RefreshServerList();
}

void SNetworkDashboard::RefreshServerList()
{
    ServerListItems.Empty();

    const UNetworkToolSettings* Settings = GetDefault<UNetworkToolSettings>();
    if (Settings)
    {
        for (const FServerProfile& Profile : Settings->ServerProfiles)
        {
            ServerListItems.Add(MakeShared<FServerProfile>(Profile));
        }
    }

    if (ServerListView.IsValid())
    {
        ServerListView->RequestListRefresh();
    }
}

TSharedRef<ITableRow> SNetworkDashboard::OnGenerateRowForList(TSharedPtr<FServerProfile> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(SServerListItem, OwnerTable)
        .Item(Item)
        .OnJoinClicked(this, &SNetworkDashboard::OnJoinServer)
        .OnStartClicked(this, &SNetworkDashboard::OnStartServer)
        .OnStopClicked(this, &SNetworkDashboard::OnStopServer);
}

void SNetworkDashboard::OnJoinServer(TSharedPtr<FServerProfile> Profile)
{
    if (auto* Sub = GetNetworkSubsystem())
        Sub->JoinServerProfile(*Profile);
}

void SNetworkDashboard::OnStartServer(TSharedPtr<FServerProfile> Profile)
{
    if (auto* Sub = GetNetworkSubsystem())
        Sub->SendRemoteCommand(*Profile, "START");
}

void SNetworkDashboard::OnStopServer(TSharedPtr<FServerProfile> Profile)
{
    if (auto* Sub = GetNetworkSubsystem())
        Sub->SendRemoteCommand(*Profile, "STOP");
}

FReply SNetworkDashboard::OnLaunchLocalClusterClicked()
{
    if (auto* Sub = GetNetworkSubsystem())
        Sub->LaunchLocalCluster(LocalClientCount, bLocalDedicatedServer);
    
    return FReply::Handled();
}

FReply SNetworkDashboard::OnApplyLagClicked()
{
    FNetworkEmulationProfile Emul;
    Emul.PktLag = SimPktLag;
    Emul.PktLoss = SimPktLoss;

    if (auto* Sub = GetNetworkSubsystem())
        Sub->SetNetworkEmulation(Emul);
        
    return FReply::Handled();
}

void SNetworkDashboard::UpdateStatusLog(FString NewLog)
{
    if (StatusLogBox.IsValid())
    {
        FString OldText = StatusLogBox->GetText().ToString();
        FString NewText = FString::Printf(TEXT("[%s] %s\n%s"), *FDateTime::Now().ToString(TEXT("%H:%M:%S")), *NewLog, *OldText);
        StatusLogBox->SetText(FText::FromString(NewText));
    }
}

UNetworkToolSubsystem* SNetworkDashboard::GetNetworkSubsystem() const
{
    if (GEditor)
    {
        return GEditor->GetEditorSubsystem<UNetworkToolSubsystem>();
    }
    
    return nullptr;
}

#undef LOCTEXT_NAMESPACE