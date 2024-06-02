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
            "Projects",
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });
    }
}
