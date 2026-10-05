using UnrealBuildTool;
public class Wildline : ModuleRules {
    public Wildline(ReadOnlyTargetRules Target) : base(Target) {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] {"Core","CoreUObject","Engine","InputCore","UMG","Slate","SlateCore","ProceduralMeshComponent","AIModule","NetCore","AnimationCore","ImageCore","ImageWrapper"});
        if (Target.Platform == UnrealTargetPlatform.Win64) PublicSystemLibraries.Add("comdlg32.lib");
    }
}
