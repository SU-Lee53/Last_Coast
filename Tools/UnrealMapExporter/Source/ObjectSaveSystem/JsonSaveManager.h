#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JsonSaveManager.generated.h"

UCLASS()
class OBJECTSAVESYSTEM_API UJsonSaveManager : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:

    // 여러 액터 저장
    UFUNCTION(BlueprintCallable, Category = "JSON Save System")
    static bool SaveActorsToJson(const TArray<AActor*>& Actors, const FString& FileName);

    // 파일 경로 가져오기
    UFUNCTION(BlueprintCallable, Category = "JSON Save System")
    static FString GetSaveFilePath(const FString& FileName);

private:
    // Transform을 JSON 오브젝트로 변환
    static TSharedPtr<FJsonObject> TransformToJson(const FTransform& Transform);
};