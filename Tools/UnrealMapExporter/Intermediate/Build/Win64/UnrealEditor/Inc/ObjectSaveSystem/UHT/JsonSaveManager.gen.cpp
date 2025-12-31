// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "ObjectSaveSystem/JsonSaveManager.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeJsonSaveManager() {}

// Begin Cross Module References
ENGINE_API UClass* Z_Construct_UClass_AActor_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_UBlueprintFunctionLibrary();
OBJECTSAVESYSTEM_API UClass* Z_Construct_UClass_UJsonSaveManager();
OBJECTSAVESYSTEM_API UClass* Z_Construct_UClass_UJsonSaveManager_NoRegister();
UPackage* Z_Construct_UPackage__Script_ObjectSaveSystem();
// End Cross Module References

// Begin Class UJsonSaveManager Function GetSaveFilePath
struct Z_Construct_UFunction_UJsonSaveManager_GetSaveFilePath_Statics
{
	struct JsonSaveManager_eventGetSaveFilePath_Parms
	{
		FString FileName;
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "JSON Save System" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
#endif
		{ "ModuleRelativePath", "JsonSaveManager.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_FileName_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStrPropertyParams NewProp_FileName;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UJsonSaveManager_GetSaveFilePath_Statics::NewProp_FileName = { "FileName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(JsonSaveManager_eventGetSaveFilePath_Parms, FileName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_FileName_MetaData), NewProp_FileName_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UJsonSaveManager_GetSaveFilePath_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(JsonSaveManager_eventGetSaveFilePath_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UJsonSaveManager_GetSaveFilePath_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UJsonSaveManager_GetSaveFilePath_Statics::NewProp_FileName,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UJsonSaveManager_GetSaveFilePath_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UJsonSaveManager_GetSaveFilePath_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UJsonSaveManager_GetSaveFilePath_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UJsonSaveManager, nullptr, "GetSaveFilePath", nullptr, nullptr, Z_Construct_UFunction_UJsonSaveManager_GetSaveFilePath_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UJsonSaveManager_GetSaveFilePath_Statics::PropPointers), sizeof(Z_Construct_UFunction_UJsonSaveManager_GetSaveFilePath_Statics::JsonSaveManager_eventGetSaveFilePath_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UJsonSaveManager_GetSaveFilePath_Statics::Function_MetaDataParams), Z_Construct_UFunction_UJsonSaveManager_GetSaveFilePath_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UJsonSaveManager_GetSaveFilePath_Statics::JsonSaveManager_eventGetSaveFilePath_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UJsonSaveManager_GetSaveFilePath()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UJsonSaveManager_GetSaveFilePath_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UJsonSaveManager::execGetSaveFilePath)
{
	P_GET_PROPERTY(FStrProperty,Z_Param_FileName);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=UJsonSaveManager::GetSaveFilePath(Z_Param_FileName);
	P_NATIVE_END;
}
// End Class UJsonSaveManager Function GetSaveFilePath

// Begin Class UJsonSaveManager Function SaveActorsToJson
struct Z_Construct_UFunction_UJsonSaveManager_SaveActorsToJson_Statics
{
	struct JsonSaveManager_eventSaveActorsToJson_Parms
	{
		TArray<AActor*> Actors;
		FString FileName;
		bool ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "JSON Save System" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\n" },
#endif
		{ "ModuleRelativePath", "JsonSaveManager.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd \xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Actors_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_FileName_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Actors_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_Actors;
	static const UECodeGen_Private::FStrPropertyParams NewProp_FileName;
	static void NewProp_ReturnValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UJsonSaveManager_SaveActorsToJson_Statics::NewProp_Actors_Inner = { "Actors", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_AActor_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UFunction_UJsonSaveManager_SaveActorsToJson_Statics::NewProp_Actors = { "Actors", nullptr, (EPropertyFlags)0x0010000008000182, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(JsonSaveManager_eventSaveActorsToJson_Parms, Actors), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Actors_MetaData), NewProp_Actors_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UJsonSaveManager_SaveActorsToJson_Statics::NewProp_FileName = { "FileName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(JsonSaveManager_eventSaveActorsToJson_Parms, FileName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_FileName_MetaData), NewProp_FileName_MetaData) };
void Z_Construct_UFunction_UJsonSaveManager_SaveActorsToJson_Statics::NewProp_ReturnValue_SetBit(void* Obj)
{
	((JsonSaveManager_eventSaveActorsToJson_Parms*)Obj)->ReturnValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UJsonSaveManager_SaveActorsToJson_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(JsonSaveManager_eventSaveActorsToJson_Parms), &Z_Construct_UFunction_UJsonSaveManager_SaveActorsToJson_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UJsonSaveManager_SaveActorsToJson_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UJsonSaveManager_SaveActorsToJson_Statics::NewProp_Actors_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UJsonSaveManager_SaveActorsToJson_Statics::NewProp_Actors,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UJsonSaveManager_SaveActorsToJson_Statics::NewProp_FileName,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UJsonSaveManager_SaveActorsToJson_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UJsonSaveManager_SaveActorsToJson_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UJsonSaveManager_SaveActorsToJson_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UJsonSaveManager, nullptr, "SaveActorsToJson", nullptr, nullptr, Z_Construct_UFunction_UJsonSaveManager_SaveActorsToJson_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UJsonSaveManager_SaveActorsToJson_Statics::PropPointers), sizeof(Z_Construct_UFunction_UJsonSaveManager_SaveActorsToJson_Statics::JsonSaveManager_eventSaveActorsToJson_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04422401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UJsonSaveManager_SaveActorsToJson_Statics::Function_MetaDataParams), Z_Construct_UFunction_UJsonSaveManager_SaveActorsToJson_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UJsonSaveManager_SaveActorsToJson_Statics::JsonSaveManager_eventSaveActorsToJson_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UJsonSaveManager_SaveActorsToJson()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UJsonSaveManager_SaveActorsToJson_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UJsonSaveManager::execSaveActorsToJson)
{
	P_GET_TARRAY_REF(AActor*,Z_Param_Out_Actors);
	P_GET_PROPERTY(FStrProperty,Z_Param_FileName);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)Z_Param__Result=UJsonSaveManager::SaveActorsToJson(Z_Param_Out_Actors,Z_Param_FileName);
	P_NATIVE_END;
}
// End Class UJsonSaveManager Function SaveActorsToJson

// Begin Class UJsonSaveManager
void UJsonSaveManager::StaticRegisterNativesUJsonSaveManager()
{
	UClass* Class = UJsonSaveManager::StaticClass();
	static const FNameNativePtrPair Funcs[] = {
		{ "GetSaveFilePath", &UJsonSaveManager::execGetSaveFilePath },
		{ "SaveActorsToJson", &UJsonSaveManager::execSaveActorsToJson },
	};
	FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UJsonSaveManager);
UClass* Z_Construct_UClass_UJsonSaveManager_NoRegister()
{
	return UJsonSaveManager::StaticClass();
}
struct Z_Construct_UClass_UJsonSaveManager_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "JsonSaveManager.h" },
		{ "ModuleRelativePath", "JsonSaveManager.h" },
	};
#endif // WITH_METADATA
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UJsonSaveManager_GetSaveFilePath, "GetSaveFilePath" }, // 1888683305
		{ &Z_Construct_UFunction_UJsonSaveManager_SaveActorsToJson, "SaveActorsToJson" }, // 486126201
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UJsonSaveManager>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
UObject* (*const Z_Construct_UClass_UJsonSaveManager_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UBlueprintFunctionLibrary,
	(UObject* (*)())Z_Construct_UPackage__Script_ObjectSaveSystem,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UJsonSaveManager_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UJsonSaveManager_Statics::ClassParams = {
	&UJsonSaveManager::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	nullptr,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	0,
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UJsonSaveManager_Statics::Class_MetaDataParams), Z_Construct_UClass_UJsonSaveManager_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UJsonSaveManager()
{
	if (!Z_Registration_Info_UClass_UJsonSaveManager.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UJsonSaveManager.OuterSingleton, Z_Construct_UClass_UJsonSaveManager_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UJsonSaveManager.OuterSingleton;
}
template<> OBJECTSAVESYSTEM_API UClass* StaticClass<UJsonSaveManager>()
{
	return UJsonSaveManager::StaticClass();
}
UJsonSaveManager::UJsonSaveManager(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR(UJsonSaveManager);
UJsonSaveManager::~UJsonSaveManager() {}
// End Class UJsonSaveManager

// Begin Registration
struct Z_CompiledInDeferFile_FID_GitHub_Last_Coast_Tools_UnrealMapExporter_Source_ObjectSaveSystem_JsonSaveManager_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UJsonSaveManager, UJsonSaveManager::StaticClass, TEXT("UJsonSaveManager"), &Z_Registration_Info_UClass_UJsonSaveManager, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UJsonSaveManager), 1043184654U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_GitHub_Last_Coast_Tools_UnrealMapExporter_Source_ObjectSaveSystem_JsonSaveManager_h_1567748005(TEXT("/Script/ObjectSaveSystem"),
	Z_CompiledInDeferFile_FID_GitHub_Last_Coast_Tools_UnrealMapExporter_Source_ObjectSaveSystem_JsonSaveManager_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_GitHub_Last_Coast_Tools_UnrealMapExporter_Source_ObjectSaveSystem_JsonSaveManager_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
