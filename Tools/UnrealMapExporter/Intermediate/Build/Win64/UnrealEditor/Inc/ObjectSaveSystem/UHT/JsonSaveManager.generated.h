// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "JsonSaveManager.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class AActor;
#ifdef OBJECTSAVESYSTEM_JsonSaveManager_generated_h
#error "JsonSaveManager.generated.h already included, missing '#pragma once' in JsonSaveManager.h"
#endif
#define OBJECTSAVESYSTEM_JsonSaveManager_generated_h

#define FID_GitHub_Last_Coast_Tools_UnrealMapExporter_Source_ObjectSaveSystem_JsonSaveManager_h_9_RPC_WRAPPERS_NO_PURE_DECLS \
	DECLARE_FUNCTION(execGetSaveFilePath); \
	DECLARE_FUNCTION(execSaveActorsToJson);


#define FID_GitHub_Last_Coast_Tools_UnrealMapExporter_Source_ObjectSaveSystem_JsonSaveManager_h_9_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUJsonSaveManager(); \
	friend struct Z_Construct_UClass_UJsonSaveManager_Statics; \
public: \
	DECLARE_CLASS(UJsonSaveManager, UBlueprintFunctionLibrary, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/ObjectSaveSystem"), NO_API) \
	DECLARE_SERIALIZER(UJsonSaveManager)


#define FID_GitHub_Last_Coast_Tools_UnrealMapExporter_Source_ObjectSaveSystem_JsonSaveManager_h_9_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UJsonSaveManager(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UJsonSaveManager(UJsonSaveManager&&); \
	UJsonSaveManager(const UJsonSaveManager&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UJsonSaveManager); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UJsonSaveManager); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UJsonSaveManager) \
	NO_API virtual ~UJsonSaveManager();


#define FID_GitHub_Last_Coast_Tools_UnrealMapExporter_Source_ObjectSaveSystem_JsonSaveManager_h_6_PROLOG
#define FID_GitHub_Last_Coast_Tools_UnrealMapExporter_Source_ObjectSaveSystem_JsonSaveManager_h_9_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_GitHub_Last_Coast_Tools_UnrealMapExporter_Source_ObjectSaveSystem_JsonSaveManager_h_9_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_GitHub_Last_Coast_Tools_UnrealMapExporter_Source_ObjectSaveSystem_JsonSaveManager_h_9_INCLASS_NO_PURE_DECLS \
	FID_GitHub_Last_Coast_Tools_UnrealMapExporter_Source_ObjectSaveSystem_JsonSaveManager_h_9_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> OBJECTSAVESYSTEM_API UClass* StaticClass<class UJsonSaveManager>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_GitHub_Last_Coast_Tools_UnrealMapExporter_Source_ObjectSaveSystem_JsonSaveManager_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
