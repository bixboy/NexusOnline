using UnrealBuildTool;

public class NexusSteam : ModuleRules
{
    public NexusSteam(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "OnlineSubsystem",
            "OnlineSubsystemUtils",
            "OnlineSubsystemSteam",
            "CoreOnline"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
        });
    }
}
