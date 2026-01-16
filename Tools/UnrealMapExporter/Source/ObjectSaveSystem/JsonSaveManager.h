#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JsonSaveManager.generated.h"

class ALandscape;
class ULandscapeComponent;



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

    static FString GetSaveFilePath(const FString& FileName);
#if WITH_EDITOR
    static bool ExportMeshToFBX(UStaticMesh* Mesh, const FString& FileName, bool bShowOptions = false);
#endif

    static TSharedPtr<FJsonObject> LandscapeToJson(ALandscape* Landscape);
    static TSharedPtr<FJsonObject> LandscapeComponentToJson(ULandscapeComponent* Component);
    static TArray<TSharedPtr<FJsonValue>> GetHeightDataJson(ULandscapeComponent* Component);
    static TArray<TSharedPtr<FJsonValue>> GetLayerDataJson(ULandscapeComponent* Component);
};