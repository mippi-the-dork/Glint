using UnrealBuildTool;
public class GlintEditor : ModuleRules
{
    public GlintEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        PrivateDependencyModuleNames.AddRange(new [] { "Core", "CoreUObject", "Engine", "Glint", "UnrealEd", "AssetTools", "Slate", "SlateCore" });
    }
}
