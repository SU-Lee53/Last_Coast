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
            "InputCore",
            "EnhancedInput",
            "Json",
            "JsonUtilities",
            "Landscape",
            "Foliage",
            "ImageWrapper",
            "ImageWriteQueue",
            "RenderCore",
            "RHI"
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "UnrealEd",
                "AssetTools",
                "FBX",
                "LandscapeEditor"
            });
        }
    }
}