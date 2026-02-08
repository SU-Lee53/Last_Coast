using System.IO;
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
            "ImageWrapper",
            "RenderCore",
            "RHI",
            "NavigationSystem",
            "Navmesh"  // ✅ 중요
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
        string NavmeshPublicPath = Path.Combine(
           EngineDirectory,
           "Source",
           "Runtime",
           "Navmesh",
           "Public"
       );

        PublicIncludePaths.Add(NavmeshPublicPath);
    }
}