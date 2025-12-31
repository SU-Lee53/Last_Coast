// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "ObjectSaveSystem/ObjectSaveSystemGameMode.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeObjectSaveSystemGameMode() {}

// Begin Cross Module References
ENGINE_API UClass* Z_Construct_UClass_AGameModeBase();
OBJECTSAVESYSTEM_API UClass* Z_Construct_UClass_AObjectSaveSystemGameMode();
OBJECTSAVESYSTEM_API UClass* Z_Construct_UClass_AObjectSaveSystemGameMode_NoRegister();
UPackage* Z_Construct_UPackage__Script_ObjectSaveSystem();
// End Cross Module References

// Begin Class AObjectSaveSystemGameMode
void AObjectSaveSystemGameMode::StaticRegisterNativesAObjectSaveSystemGameMode()
{
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AObjectSaveSystemGameMode);
UClass* Z_Construct_UClass_AObjectSaveSystemGameMode_NoRegister()
{
	return AObjectSaveSystemGameMode::StaticClass();
}
struct Z_Construct_UClass_AObjectSaveSystemGameMode_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "HideCategories", "Info Rendering MovementReplication Replication Actor Input Movement Collision Rendering HLOD WorldPartition DataLayers Transformation" },
		{ "IncludePath", "ObjectSaveSystemGameMode.h" },
		{ "ModuleRelativePath", "ObjectSaveSystemGameMode.h" },
		{ "ShowCategories", "Input|MouseInput Input|TouchInput" },
	};
#endif // WITH_METADATA
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AObjectSaveSystemGameMode>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
UObject* (*const Z_Construct_UClass_AObjectSaveSystemGameMode_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_AGameModeBase,
	(UObject* (*)())Z_Construct_UPackage__Script_ObjectSaveSystem,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_AObjectSaveSystemGameMode_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_AObjectSaveSystemGameMode_Statics::ClassParams = {
	&AObjectSaveSystemGameMode::StaticClass,
	"Game",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	nullptr,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	0,
	0,
	0x008802ACu,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_AObjectSaveSystemGameMode_Statics::Class_MetaDataParams), Z_Construct_UClass_AObjectSaveSystemGameMode_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_AObjectSaveSystemGameMode()
{
	if (!Z_Registration_Info_UClass_AObjectSaveSystemGameMode.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AObjectSaveSystemGameMode.OuterSingleton, Z_Construct_UClass_AObjectSaveSystemGameMode_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_AObjectSaveSystemGameMode.OuterSingleton;
}
template<> OBJECTSAVESYSTEM_API UClass* StaticClass<AObjectSaveSystemGameMode>()
{
	return AObjectSaveSystemGameMode::StaticClass();
}
DEFINE_VTABLE_PTR_HELPER_CTOR(AObjectSaveSystemGameMode);
AObjectSaveSystemGameMode::~AObjectSaveSystemGameMode() {}
// End Class AObjectSaveSystemGameMode

// Begin Registration
struct Z_CompiledInDeferFile_FID_GitHub_Last_Coast_Tools_UnrealMapExporter_Source_ObjectSaveSystem_ObjectSaveSystemGameMode_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_AObjectSaveSystemGameMode, AObjectSaveSystemGameMode::StaticClass, TEXT("AObjectSaveSystemGameMode"), &Z_Registration_Info_UClass_AObjectSaveSystemGameMode, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AObjectSaveSystemGameMode), 285578260U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_GitHub_Last_Coast_Tools_UnrealMapExporter_Source_ObjectSaveSystem_ObjectSaveSystemGameMode_h_993469899(TEXT("/Script/ObjectSaveSystem"),
	Z_CompiledInDeferFile_FID_GitHub_Last_Coast_Tools_UnrealMapExporter_Source_ObjectSaveSystem_ObjectSaveSystemGameMode_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_GitHub_Last_Coast_Tools_UnrealMapExporter_Source_ObjectSaveSystem_ObjectSaveSystemGameMode_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
