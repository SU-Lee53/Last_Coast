#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JsonSaveManager.generated.h"
class ALandscape;
class ULandscapeComponent;
class UTexture2D;
class UMaterialExpressionLandscapeLayerBlend;
class ARecastNavMesh;
struct FWeightmapLayerAllocationInfo;

struct FExportedLayerTextures
{
    FString LayerName;
    TMap<FString, FString> TextureFiles; // Type -> FileName
    float Tiling;
};


UCLASS()
class OBJECTSAVESYSTEM_API UJsonSaveManager : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:

    // 여러 액터 저장
    UFUNCTION(BlueprintCallable, Category = "JSON Save System")
    static bool SaveActorsToJson(const TArray<AActor*>& Actors, const FString& FileName);

    // 여러 액터 저장
    UFUNCTION(BlueprintCallable, Category = "JSON Save System")
    static bool SaveActorsMeshToFBX(const TArray<AActor*>& Actors);

    // Landscape export 함수들
    UFUNCTION(BlueprintCallable, Category = "JSON Save System")
    static bool SaveLandscapeToJson(ALandscape* Landscape, const FString& FileName);

    UFUNCTION(BlueprintCallable, Category = "JSON Save System")
    static bool ExportLandscapeHeightmap(ALandscape* Landscape, const FString& FileName);

    UFUNCTION(BlueprintCallable, Category = "JSON Save System")
    static bool ExportNavMeshToJson(UWorld* World, const FString& FileName);

    

private:
    // Transform을 JSON 오브젝트로 변환
    static TSharedPtr<FJsonObject> TransformToJson(const FTransform& Transform);

    static TSharedPtr<FJsonObject> DirectionToJson(const FTransform& Transform);

    static bool ExportMeshToFBX(UStaticMesh* Mesh, const FString& FileName, bool bShowOptions = false);

    static TSharedPtr<FJsonObject> LandscapeToJson(ALandscape* Landscape);

    static void CalculateLandscapeSize(ALandscape* Landscape, int32& OutNumX, int32& OutNumY);

    static TArray<TSharedPtr<FJsonValue>> GetLayersInfoJson(
        ALandscape* Landscape,
        TMap<FName, int32>& OutLayerIndexMap,
        const TMap<FName, FExportedLayerTextures>& ExportedTextures);

    static TArray<TSharedPtr<FJsonValue>> GetComponentWeightMapsJson(ULandscapeComponent* Component,int32 ComponentIndex,const TMap<FName, int32>& LayerIndexMap);

    static bool ExportWeightMapTextureToPNG(
        ULandscapeComponent* Component,
        int32 WeightmapTextureIndex,
        const FString& FileName,
        const TArray<FWeightmapLayerAllocationInfo>& AllocInfos);


    static TSharedPtr<FJsonObject> LandscapeComponentToJson(ULandscapeComponent* Component,int32 ComponentIndex,const TMap<FName, int32>& LayerIndexMap);

    static bool ExportHeightmapAsPNG(const TArray<uint16>& HeightData, int32 Width, int32 Height, const FString& FileName);

    static TMap<FName, FExportedLayerTextures> ExportLayerTextures(
        ALandscape* Landscape,
        const TMap<FName, int32>& LayerIndexMap);

    static bool ExportTextureToPNG(
        UTexture2D* Texture,
        const FString& FilePath);


    static void CollectTexturesFromExpression(
        UMaterialExpression* Expression,
        TArray<UTexture2D*>& OutTextures,
        TSet<UMaterialExpression*>& VisitedExpressions);


    static FString DetermineTextureTypeFromConnection(
        UMaterialExpression* TextureExpression,
        UMaterialExpressionLandscapeLayerBlend* LayerBlendNode,
        int32 LayerIndex);

    static bool IsConnectedToMaterialInput(
        UMaterialExpression* Expression,
        UMaterial* Material,
        EMaterialProperty PropertyType);

    static bool IsExpressionConnectedToInput(
        UMaterialExpression* TargetExpression,
        UMaterialExpression* CurrentExpression,
        TSet<UMaterialExpression*>& VisitedNodes);


    static TSharedPtr<FJsonObject> NavMeshToJson(ARecastNavMesh* NavMesh);

    static TSharedPtr<FJsonObject> Vector3ToJson(const FVector& UnrealVector);
};


