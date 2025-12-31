// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeObjectSaveSystem_init() {}
	static FPackageRegistrationInfo Z_Registration_Info_UPackage__Script_ObjectSaveSystem;
	FORCENOINLINE UPackage* Z_Construct_UPackage__Script_ObjectSaveSystem()
	{
		if (!Z_Registration_Info_UPackage__Script_ObjectSaveSystem.OuterSingleton)
		{
			static const UECodeGen_Private::FPackageParams PackageParams = {
				"/Script/ObjectSaveSystem",
				nullptr,
				0,
				PKG_CompiledIn | 0x00000000,
				0x3916C74D,
				0x3310BF3C,
				METADATA_PARAMS(0, nullptr)
			};
			UECodeGen_Private::ConstructUPackage(Z_Registration_Info_UPackage__Script_ObjectSaveSystem.OuterSingleton, PackageParams);
		}
		return Z_Registration_Info_UPackage__Script_ObjectSaveSystem.OuterSingleton;
	}
	static FRegisterCompiledInInfo Z_CompiledInDeferPackage_UPackage__Script_ObjectSaveSystem(Z_Construct_UPackage__Script_ObjectSaveSystem, TEXT("/Script/ObjectSaveSystem"), Z_Registration_Info_UPackage__Script_ObjectSaveSystem, CONSTRUCT_RELOAD_VERSION_INFO(FPackageReloadVersionInfo, 0x3916C74D, 0x3310BF3C));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
