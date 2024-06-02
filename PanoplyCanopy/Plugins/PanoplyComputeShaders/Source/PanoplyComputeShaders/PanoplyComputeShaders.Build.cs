// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PanoplyComputeShaders : ModuleRules
{
	public PanoplyComputeShaders(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "Projects"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });
    }
}
