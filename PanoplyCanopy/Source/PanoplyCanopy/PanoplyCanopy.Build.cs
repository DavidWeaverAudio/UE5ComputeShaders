using UnrealBuildTool;

public class PanoplyCanopy : ModuleRules
{
    public PanoplyCanopy(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "RHI",                // Add this module for RHI commands
            "RenderCore",         // Add this module for rendering utilities
            "Renderer",           // Add this module for the rendering interface
            "Projects",
            "ComputeFramework",   // Add this module for compute shader utilities
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });

        // Ensure shader directory is added to the include path
        PublicIncludePaths.AddRange(new string[] {
            "PanoplyCanopy",
            "PanoplyCanopy/Content/Shaders"
        });
    }
}
