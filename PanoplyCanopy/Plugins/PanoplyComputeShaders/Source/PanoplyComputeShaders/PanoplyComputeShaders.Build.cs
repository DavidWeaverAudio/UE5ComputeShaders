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
            "RenderCore",
            "Renderer",
            "RHI",
            "Projects"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });

        // Add the shader directory to the include path
        PublicIncludePaths.AddRange(new string[] {
            "PanoplyComputeShaders",
            "PanoplyComputeShaders/Shaders"
        });
    }
}
