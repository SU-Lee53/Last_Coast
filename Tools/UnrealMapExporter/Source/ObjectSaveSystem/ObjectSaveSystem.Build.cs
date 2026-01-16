// Copyright Epic Games, Inc. All Rights Reserved.

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
            "Landscape",  // ✅ Landscape 모듈 추가!
            "Foliage"
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "UnrealEd",
                "AssetTools",
                "FBX",              // FBX export
                "LandscapeEditor"   // ✅ Landscape Editor 모듈 추가!
            });
        }
    }
}