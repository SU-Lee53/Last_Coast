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

TSharedPtr<FJsonObject> UJsonSaveManager::TransformToJson(const FTransform& Transform)
{
    TSharedPtr<FJsonObject> TransformJson = MakeShareable(new FJsonObject);

    // Location
    FVector Location = Transform.GetLocation();
    TArray<TSharedPtr<FJsonValue>> LocationArray;
    LocationArray.Add(MakeShareable(new FJsonValueNumber(Location.X)));
    LocationArray.Add(MakeShareable(new FJsonValueNumber(Location.Y)));
    LocationArray.Add(MakeShareable(new FJsonValueNumber(Location.Z)));
    TransformJson->SetArrayField(TEXT("Location"), LocationArray);

    // Rotation
    FRotator Rotation = Transform.Rotator();
    TArray<TSharedPtr<FJsonValue>> RotationArray;
    RotationArray.Add(MakeShareable(new FJsonValueNumber(Rotation.Pitch)));
    RotationArray.Add(MakeShareable(new FJsonValueNumber(Rotation.Yaw)));
    RotationArray.Add(MakeShareable(new FJsonValueNumber(Rotation.Roll)));
    TransformJson->SetArrayField(TEXT("Rotation"), RotationArray);

    // Scale
    FVector Scale = Transform.GetScale3D();
    TArray<TSharedPtr<FJsonValue>> ScaleArray;
    ScaleArray.Add(MakeShareable(new FJsonValueNumber(Scale.X)));
    ScaleArray.Add(MakeShareable(new FJsonValueNumber(Scale.Y)));
    ScaleArray.Add(MakeShareable(new FJsonValueNumber(Scale.Z)));
    TransformJson->SetArrayField(TEXT("Scale"), ScaleArray);

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
