// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ObjectSaveSystem : ModuleRules
{
	public ObjectSaveSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput","Json",        // JSON 모듈 추가
            "JsonUtilities" // JSON 유틸리티 모듈 추가
		});
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "UnrealEd",
                "AssetTools",
                "FBX"  // FBX 옵션 사용을 위해 추가
            });
        }
    }
}
