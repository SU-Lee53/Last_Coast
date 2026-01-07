#include "JsonSaveManager.h"
#include "HAL/PlatformFilemanager.h"
#include "Engine\StaticMeshActor.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"

bool UJsonSaveManager::SaveActorsToJson(const TArray<AActor*>& Actors, const FString& FileName)
{
    FString OutputString = "[\n";  // 배열 시작

    for (int32 i = 0; i < Actors.Num(); i++)
    {
        AActor* Actor = Actors[i];
        if (Actor)
        {
            AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(Actor);
            if (StaticMeshActor && StaticMeshActor->GetStaticMeshComponent())
            {
                UStaticMesh* Mesh = StaticMeshActor->GetStaticMeshComponent()->GetStaticMesh();
                if (Mesh)
                {
                    TSharedPtr<FJsonObject> ActorJson = MakeShareable(new FJsonObject);
                    ActorJson->SetStringField(TEXT("ActorName"), Actor->GetName());
                    ActorJson->SetStringField(TEXT("MeshName"), Mesh->GetName());
                    ActorJson->SetObjectField(TEXT("Transform"), TransformToJson(Actor->GetActorTransform()));
                    
                    FString ActorString;
                    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ActorString);
                    FJsonSerializer::Serialize(ActorJson.ToSharedRef(), Writer);

                    OutputString += ActorString;

                    // 마지막 항목이 아니면 쉼표 추가
                    if (i < Actors.Num() - 1)
                    {
                        OutputString += TEXT(",\n");
                    }
                }  
            }
        }
    }

    OutputString += "\n]";  // 배열 종료

    FString FilePath = GetSaveFilePath(FileName);
    bool bSuccess = FFileHelper::SaveStringToFile(OutputString, *FilePath);

    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("%d actors saved to: %s"), Actors.Num(), *FilePath);
    }

    return bSuccess;
}

FString UJsonSaveManager::GetSaveFilePath(const FString& FileName)
{
    return FPaths::ProjectSavedDir() + TEXT("../") + FileName + TEXT(".json");
}

//TSharedPtr<FJsonObject> UJsonSaveManager::TransformToJson(const FTransform& Transform)
//{
//    TSharedPtr<FJsonObject> TransformJson = MakeShareable(new FJsonObject);
//
//    // Location - 언리얼(X,Y,Z) -> DirectX(Y,Z,X)
//    FVector Location = Transform.GetLocation();
//    TArray<TSharedPtr<FJsonValue>> LocationArray;
//    LocationArray.Add(MakeShareable(new FJsonValueNumber(Location.Y)));
//    LocationArray.Add(MakeShareable(new FJsonValueNumber(Location.Z)));
//    LocationArray.Add(MakeShareable(new FJsonValueNumber(Location.X)));
//    TransformJson->SetArrayField(TEXT("Location"), LocationArray);
//
//    // Rotation - 변환 행렬 사용
//    FMatrix UnrealRotMatrix = Transform.GetRotation().ToMatrix();
//
//    // 좌표계 변환 행렬: 언리얼(X,Y,Z) -> DirectX(Y,Z,-X)
//    // SimpleMath Forward가 -Z이므로 언리얼 +X를 DirectX -Z로 매핑
//    FMatrix ConversionMatrix = FMatrix(
//        FPlane(0, 1, 0, 0),  // DirectX X = 언리얼 Y
//        FPlane(0, 0, 1, 0),  // DirectX Y = 언리얼 Z
//        FPlane(1, 0, 0, 0),  // DirectX Z = 언리얼 X (부호 변경!)
//        FPlane(0, 0, 0, 1)
//    );
//
//    // Basis 변환: C * R
//    FMatrix DirectXRotMatrix = ConversionMatrix * UnrealRotMatrix * ConversionMatrix.GetTransposed();;
//
//    // 행렬을 쿼터니언으로 변환
//    FQuat DirectXQuat(DirectXRotMatrix);
//    DirectXQuat.Normalize();
//
//    TArray<TSharedPtr<FJsonValue>> RotationArray;
//    RotationArray.Add(MakeShareable(new FJsonValueNumber(DirectXQuat.X)));
//    RotationArray.Add(MakeShareable(new FJsonValueNumber(DirectXQuat.Y)));
//    RotationArray.Add(MakeShareable(new FJsonValueNumber(DirectXQuat.Z)));
//    RotationArray.Add(MakeShareable(new FJsonValueNumber(DirectXQuat.W)));
//    TransformJson->SetArrayField(TEXT("Rotation"), RotationArray);
//
//    // Scale - 축 순서 변환
//    FVector Scale = Transform.GetScale3D();
//    TArray<TSharedPtr<FJsonValue>> ScaleArray;
//    ScaleArray.Add(MakeShareable(new FJsonValueNumber(Scale.Y)));
//    ScaleArray.Add(MakeShareable(new FJsonValueNumber(Scale.Z)));
//    ScaleArray.Add(MakeShareable(new FJsonValueNumber(Scale.X)));
//    TransformJson->SetArrayField(TEXT("Scale"), ScaleArray);
//
//    return TransformJson;
//}

TSharedPtr<FJsonObject> UJsonSaveManager::TransformToJson(const FTransform& Transform)
{
    TSharedPtr<FJsonObject> TransformJson = MakeShareable(new FJsonObject);

    // 언리얼 월드 변환 행렬
    FMatrix UnrealWorldMatrix = Transform.ToMatrixWithScale();

    // 언리얼 행렬 로그
    UE_LOG(LogTemp, Warning, TEXT("=== Unreal Matrix ==="));
    UE_LOG(LogTemp, Warning, TEXT("Right:   %s"), *UnrealWorldMatrix.GetUnitAxis(EAxis::Y).ToString());
    UE_LOG(LogTemp, Warning, TEXT("Up:      %s"), *UnrealWorldMatrix.GetUnitAxis(EAxis::Z).ToString());
    UE_LOG(LogTemp, Warning, TEXT("Forward: %s"), *UnrealWorldMatrix.GetUnitAxis(EAxis::X).ToString());
    UE_LOG(LogTemp, Warning, TEXT("Pos:     %s"), *UnrealWorldMatrix.GetOrigin().ToString());

    // 좌표계 변환 행렬
    FMatrix ConversionMatrix = FMatrix(
        FPlane(0, 1, 0, 0),
        FPlane(0, 0, 1, 0),
        FPlane(1, 0, 0, 0),
        FPlane(0, 0, 0, 1)
    );

    // DirectX 행렬 변환
    FMatrix DirectXWorldMatrix = ConversionMatrix * UnrealWorldMatrix * ConversionMatrix.GetTransposed();

    // DirectX 행렬 로그
    UE_LOG(LogTemp, Warning, TEXT("=== DirectX Matrix ==="));
    FVector DXRight = FVector(DirectXWorldMatrix.M[0][0], DirectXWorldMatrix.M[0][1], DirectXWorldMatrix.M[0][2]);
    FVector DXUp = FVector(DirectXWorldMatrix.M[1][0], DirectXWorldMatrix.M[1][2], DirectXWorldMatrix.M[1][2]);
    FVector DXForward = FVector(DirectXWorldMatrix.M[2][0], DirectXWorldMatrix.M[2][1], DirectXWorldMatrix.M[2][2]);
    FVector DXPos = FVector(DirectXWorldMatrix.M[3][0], DirectXWorldMatrix.M[3][1], DirectXWorldMatrix.M[3][2]);

    UE_LOG(LogTemp, Warning, TEXT("Right(X):   %s"), *DXRight.ToString());
    UE_LOG(LogTemp, Warning, TEXT("Up(Y):      %s"), *DXUp.ToString());
    UE_LOG(LogTemp, Warning, TEXT("Forward(Z): %s"), *DXForward.ToString());
    UE_LOG(LogTemp, Warning, TEXT("Pos:        %s"), *DXPos.ToString());

    // 행렬 저장
    TArray<TSharedPtr<FJsonValue>> MatrixArray;
    for (int32 Row = 0; Row < 4; ++Row)
    {
        for (int32 Col = 0; Col < 4; ++Col)
        {
            MatrixArray.Add(MakeShareable(new FJsonValueNumber(DirectXWorldMatrix.M[Row][Col])));
        }
    }
    TransformJson->SetArrayField(TEXT("WorldMatrix"), MatrixArray);

    return TransformJson;
}

//#if WITH_EDITOR
//#include "Exporters/Exporter.h"
//#include "AssetExportTask.h"
//
//bool UJsonSaveManager::ExportMeshToFBX(UStaticMesh* Mesh, const FString& FilePath)
//{
//    if (!Mesh) return false;
//
//    UAssetExportTask* ExportTask = NewObject<UAssetExportTask>();
//    ExportTask->Object = Mesh;
//    ExportTask->Exporter = nullptr; // Auto-detect
//    ExportTask->Filename = FilePath;
//    ExportTask->bSelected = false;
//    ExportTask->bReplaceIdentical = true;
//    ExportTask->bPrompt = false;
//    ExportTask->bUseFileArchive = false;
//    ExportTask->bWriteEmptyFiles = false;
//
//    UExporter::RunAssetExportTask(ExportTask);
//
//    return ExportTask->bSuccess;
//}
//#endif
