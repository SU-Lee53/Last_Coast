#include "JsonSaveManager.h"
#include "HAL/PlatformFilemanager.h"
#include "Engine\StaticMeshActor.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#if WITH_EDITOR
#include "Exporters/Exporter.h"
#include "Exporters/FbxExportOption.h"
#include "AssetExportTask.h"
#endif

bool UJsonSaveManager::SaveActorsToJson(const TArray<AActor*>& Actors, const FString& FileName)
{
    FString OutputString = "[\n";  // 배열 시작

    TSet<UStaticMesh*> ExportedMeshes;
    bool bFirstExport = true;

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

#if WITH_EDITOR
                    // 중복되지 않은 메시만 export
                    if (!ExportedMeshes.Contains(Mesh))
                    {
                        FString MeshFileName = Mesh->GetName();
                        bool bShowOptions = bFirstExport;
                        bFirstExport = false;
                        if (ExportMeshToFBX(Mesh, MeshFileName, bShowOptions))
                        {
                            ExportedMeshes.Add(Mesh);
                            UE_LOG(LogTemp, Log, TEXT("Exported mesh: %s"), *MeshFileName);
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("Failed to export mesh: %s"), *MeshFileName);
                        }
                    }
                    else
                    {
                        // 이미 export된 메시는 파일명만 참조
                        ActorJson->SetStringField(TEXT("ExportedFBX"), Mesh->GetName() + TEXT(".fbx"));
                    }
#endif

                    
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
    return FPaths::ProjectSavedDir() + TEXT("../") + TEXT("SceneJson/") + FileName + TEXT(".json");
}


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

#if WITH_EDITOR
bool UJsonSaveManager::ExportMeshToFBX(UStaticMesh* Mesh, const FString& FileName, bool bShowOptions)
{
    if (!Mesh) return false;

    FString MeshDirectory = FPaths::ProjectSavedDir() + TEXT("../ExportedMeshes/");
    FString FullPath = MeshDirectory + FileName + TEXT(".fbx");

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*MeshDirectory))
    {
        PlatformFile.CreateDirectoryTree(*MeshDirectory);
    }

    UAssetExportTask* ExportTask = NewObject<UAssetExportTask>();
    ExportTask->Object = Mesh;
    ExportTask->Exporter = nullptr;
    ExportTask->Filename = FullPath;
    ExportTask->bSelected = false;
    ExportTask->bReplaceIdentical = true;
    ExportTask->bPrompt = bShowOptions;  // ✅ 매개변수로 제어
    ExportTask->bUseFileArchive = false;
    ExportTask->bWriteEmptyFiles = false;
    ExportTask->bAutomated = !bShowOptions;

    bool bSuccess = UExporter::RunAssetExportTask(ExportTask);

    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully exported mesh to: %s"), *FullPath);
    }

    return bSuccess;
}
#endif