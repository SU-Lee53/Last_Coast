#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JsonSaveManager.generated.h"
class ALandscape;
class ULandscapeComponent;
class UTexture2D;
struct FWeightmapLayerAllocationInfo;

struct FLayerTextureInfo
{
    FString LayerName;
    UTexture2D* AlbedoTexture;
    UTexture2D* NormalTexture;
    UTexture2D* RoughnessTexture;
    UTexture2D* MetallicTexture;
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

private:
    // Transform을 JSON 오브젝트로 변환
    static TSharedPtr<FJsonObject> TransformToJson(const FTransform& Transform);


    static bool ExportMeshToFBX(UStaticMesh* Mesh, const FString& FileName, bool bShowOptions = false);

    static TSharedPtr<FJsonObject> LandscapeToJson(ALandscape* Landscape);
    static void CalculateLandscapeSize(ALandscape* Landscape, int32& OutNumX, int32& OutNumY);

    static TArray<TSharedPtr<FJsonValue>> GetLayersInfoJson(ALandscape* Landscape,TMap<FName, int32>& OutLayerIndexMap);

    static TArray<TSharedPtr<FJsonValue>> GetComponentWeightMapsJson(ULandscapeComponent* Component,int32 ComponentIndex,const TMap<FName, int32>& LayerIndexMap);

    static bool ExportWeightMapTextureToPNG(
        ULandscapeComponent* Component,
        int32 WeightmapTextureIndex,
        const FString& FileName,
        const TArray<FWeightmapLayerAllocationInfo>& AllocInfos);

   //static bool ExportTextureToDDS(UTexture2D* Texture, const FString& FilePath);
   //
   //static bool ExportLayerTextures(ALandscape* Landscape, const TMap<FName, int32>& LayerIndexMap);
   //
   //
   //static TArray<FLayerTextureInfo> ExtractLayerTexturesFromMaterial(ALandscape* Landscape);

    static TSharedPtr<FJsonObject> LandscapeComponentToJson(ULandscapeComponent* Component,int32 ComponentIndex,const TMap<FName, int32>& LayerIndexMap);

    static bool ExportHeightmapAsPNG(const TArray<uint16>& HeightData, int32 Width, int32 Height, const FString& FileName);
};


