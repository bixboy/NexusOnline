// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NexusOnlinePlugin : ModuleRules
{
	public NexusOnlinePlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"OnlineSubsystem",
			"OnlineSubsystemUtils"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore"
		});

		PublicIncludePaths.AddRange(new string[]
		{
			"NexusOnlineFramework/Public"
		});

		PrivateIncludePaths.AddRange(new string[]
		{
			"NexusOnlineFramework/Private"
		});
	}
}
