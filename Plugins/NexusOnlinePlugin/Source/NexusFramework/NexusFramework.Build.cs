using UnrealBuildTool;

public class NexusFramework : ModuleRules
{
    public NexusFramework(ReadOnlyTargetRules Target) : base(Target)
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
		    "UMG",
		    "Slate",
		    "SlateCore"
	    });

	    PrivateDependencyModuleNames.AddRange(new string[]
	    {
		    "Slate",
		    "SlateCore"
	    });

	    PublicIncludePaths.AddRange(new string[]
	    {
	    });

	    PrivateIncludePaths.AddRange(new string[]
	    {
	    });
    }
}