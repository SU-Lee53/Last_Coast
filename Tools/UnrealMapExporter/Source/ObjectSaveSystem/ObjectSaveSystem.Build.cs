using UnrealBuildTool;

public class ObjectSaveSystem : ModuleRules
{
    public ObjectSaveSystem(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",           // ✅ 추가
            "EnhancedInput",       // ✅ 추가 (Enhanced Input용)
            "Json",
            "JsonUtilities",
            "Landscape",
            "ImageWrapper",
            "RenderCore",
            "RHI"
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "UnrealEd",
                "LandscapeEditor",
                "AssetTools",
                "MaterialEditor",
                "Slate",
                "SlateCore"
            });
        }
    }
}