// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NoPhotos : ModuleRules
{
	public NoPhotos(ReadOnlyTargetRules Target) : base(Target)
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
			"NoPhotos",
			"NoPhotos/Variant_Platforming",
			"NoPhotos/Variant_Platforming/Animation",
			"NoPhotos/Variant_Combat",
			"NoPhotos/Variant_Combat/AI",
			"NoPhotos/Variant_Combat/Animation",
			"NoPhotos/Variant_Combat/Gameplay",
			"NoPhotos/Variant_Combat/Interfaces",
			"NoPhotos/Variant_Combat/UI",
			"NoPhotos/Variant_SideScrolling",
			"NoPhotos/Variant_SideScrolling/AI",
			"NoPhotos/Variant_SideScrolling/Gameplay",
			"NoPhotos/Variant_SideScrolling/Interfaces",
			"NoPhotos/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
