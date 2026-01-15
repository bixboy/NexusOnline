#include "NexusLinker.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "UI/SNetworkDashboard.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"


#define LOCTEXT_NAMESPACE "FNexusLinkerModule"

static const FName NexusLinkerTabName("NexusLinkerTab");

void FNexusLinkerModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(NexusLinkerTabName, FOnSpawnTab::CreateRaw(this, &FNexusLinkerModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("NexusLinkerTabTitle", "Nexus Linker Control"))
		.SetMenuType(ETabSpawnerMenuType::Enabled)
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FNexusLinkerModule::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(NexusLinkerTabName);
}

TSharedRef<SDockTab> FNexusLinkerModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(NomadTab)
		[
			SNew(SNetworkDashboard)
		];
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FNexusLinkerModule, NexusLinker)