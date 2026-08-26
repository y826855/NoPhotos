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
			"GameplayTags",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"ChaosSolverEngine",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"CableComponent",
			"Niagara",
			"UMG",
			"Slate",
			"OnlineSubsystem"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"OnlineSubsystemUtils",
			"ImageCore",
			"GeometryCollectionEngine",
			"PhysicsCore",
			"NavigationSystem"
		});

		PublicIncludePaths.Add(ModuleDirectory);

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

	}
}
