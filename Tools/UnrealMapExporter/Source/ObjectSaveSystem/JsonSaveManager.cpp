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
                    ActorJson->SetStringField(TEXT("ActorClass"), Actor->GetClass()->GetName());
                    ActorJson->SetObjectField(TEXT("Transform"), TransformToJson(Actor->GetActorTransform()));
                    ActorJson->SetStringField(TEXT("MeshName"), Mesh->GetName());

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
    return FPaths::ProjectSavedDir() + TEXT("/SaveData/") + FileName + TEXT(".json");
}

TSharedPtr<FJsonObject> UJsonSaveManager::TransformToJson(const FTransform& Transform)
{
    TSharedPtr<FJsonObject> TransformJson = MakeShareable(new FJsonObject);

    // Location
    FVector Location = Transform.GetLocation();
    TSharedPtr<FJsonObject> LocationJson = MakeShareable(new FJsonObject);
    LocationJson->SetNumberField(TEXT("X"), Location.X);
    LocationJson->SetNumberField(TEXT("Y"), Location.Y);
    LocationJson->SetNumberField(TEXT("Z"), Location.Z);
    TransformJson->SetObjectField(TEXT("Location"), LocationJson);

    // Rotation
    FRotator Rotation = Transform.Rotator();
    TSharedPtr<FJsonObject> RotationJson = MakeShareable(new FJsonObject);
    RotationJson->SetNumberField(TEXT("Pitch"), Rotation.Pitch);
    RotationJson->SetNumberField(TEXT("Yaw"), Rotation.Yaw);
    RotationJson->SetNumberField(TEXT("Roll"), Rotation.Roll);
    TransformJson->SetObjectField(TEXT("Rotation"), RotationJson);

    // Scale
    FVector Scale = Transform.GetScale3D();
    TSharedPtr<FJsonObject> ScaleJson = MakeShareable(new FJsonObject);
    ScaleJson->SetNumberField(TEXT("X"), Scale.X);
    ScaleJson->SetNumberField(TEXT("Y"), Scale.Y);
    ScaleJson->SetNumberField(TEXT("Z"), Scale.Z);
    TransformJson->SetObjectField(TEXT("Scale"), ScaleJson);

    return TransformJson;
}

