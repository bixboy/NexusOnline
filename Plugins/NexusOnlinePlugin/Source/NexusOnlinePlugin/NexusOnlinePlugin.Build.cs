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
			"OnlineSubsystemUtils",
			"NexusFramework"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore"
		});


	}
}
