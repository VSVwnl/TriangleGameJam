// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TriangleGameJam : ModuleRules
{
	public TriangleGameJam(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"TriangleGameJam",
			"TriangleGameJam/Variant_Platforming",
			"TriangleGameJam/Variant_Platforming/Animation",
			"TriangleGameJam/Variant_Combat",
			"TriangleGameJam/Variant_Combat/AI",
			"TriangleGameJam/Variant_Combat/Animation",
			"TriangleGameJam/Variant_Combat/Gameplay",
			"TriangleGameJam/Variant_Combat/Interfaces",
			"TriangleGameJam/Variant_Combat/UI",
			"TriangleGameJam/Variant_SideScrolling",
			"TriangleGameJam/Variant_SideScrolling/AI",
			"TriangleGameJam/Variant_SideScrolling/Gameplay",
			"TriangleGameJam/Variant_SideScrolling/Interfaces",
			"TriangleGameJam/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
