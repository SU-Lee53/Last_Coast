// JsonSaveManager.cpp
#include "JsonSaveManager.h"

// Core
#include "Engine/StaticMeshActor.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// JSON
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

// Landscape
#include "Landscape.h"
#include "LandscapeComponent.h"
#include "LandscapeLayerInfoObject.h"
#include "LandscapeDataAccess.h"
#include "LandscapeInfo.h"

// Editor
#if WITH_EDITOR
#include "Engine/StaticMesh.h"
#include "Exporters/Exporter.h"
#include "AssetExportTask.h"
#include "LandscapeEdit.h"
#endif

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
#if WITH_EDITOR
bool UJsonSaveManager::SaveActorsMeshToFBX(const TArray<AActor*>& Actors)
{
    if (Actors.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No actors to export"));
        return false;
    }

    TSet<UStaticMesh*> ExportedMeshes;
    TSet<UStaticMesh*> FailedMeshes;
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
                    // 중복되지 않은 메시만 export
                    if (!ExportedMeshes.Contains(Mesh) && !FailedMeshes.Contains(Mesh))
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
                            FailedMeshes.Add(Mesh);
                            UE_LOG(LogTemp, Warning, TEXT("Failed to export mesh: %s"), *MeshFileName);
                        }
                    }
                }
            }
        }
    }

    // 결과 요약
    int32 TotalUniqueMeshes = ExportedMeshes.Num() + FailedMeshes.Num();
    bool bSuccess = (FailedMeshes.Num() == 0) && (ExportedMeshes.Num() > 0);

    UE_LOG(LogTemp, Log, TEXT("Mesh export complete: %d succeeded, %d failed out of %d unique meshes"),
        ExportedMeshes.Num(), FailedMeshes.Num(), TotalUniqueMeshes);

    return bSuccess;
}
#endif

FString UJsonSaveManager::GetSaveFilePath(const FString& FileName)
{
    return FPaths::ProjectSavedDir() + TEXT("../") + TEXT("SceneJson/") + FileName + TEXT(".json");
}


TSharedPtr<FJsonObject> UJsonSaveManager::TransformToJson(const FTransform& Transform)
{
    TSharedPtr<FJsonObject> TransformJson = MakeShareable(new FJsonObject);

    // 언리얼 월드 변환 행렬
    FMatrix UnrealWorldMatrix = Transform.ToMatrixWithScale();


    // 좌표계 변환 행렬
    FMatrix ConversionMatrix = FMatrix(
        FPlane(0, 1, 0, 0),
        FPlane(0, 0, 1, 0),
        FPlane(1, 0, 0, 0),
        FPlane(0, 0, 0, 1)
    );

    // DirectX 행렬 변환
    FMatrix DirectXWorldMatrix = ConversionMatrix * UnrealWorldMatrix * ConversionMatrix.GetTransposed();

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


// ============================================================================
// Landscape Export 함수들
// ============================================================================

bool UJsonSaveManager::SaveLandscapeToJson(ALandscape* Landscape, const FString& FileName)
{
    if (!Landscape)
    {
        UE_LOG(LogTemp, Error, TEXT("Landscape is null"));
        return false;
    }

    TSharedPtr<FJsonObject> LandscapeJson = LandscapeToJson(Landscape);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(LandscapeJson.ToSharedRef(), Writer);

    FString FilePath = GetSaveFilePath(FileName);
    bool bSuccess = FFileHelper::SaveStringToFile(OutputString, *FilePath);

    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("Landscape saved to: %s"), *FilePath);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save landscape to: %s"), *FilePath);
    }

    return bSuccess;
}

#if WITH_EDITOR
bool UJsonSaveManager::ExportLandscapeHeightmap(ALandscape* Landscape, const FString& FileName)
{
    if (!Landscape)
    {
        UE_LOG(LogTemp, Error, TEXT("Landscape is null"));
        return false;
    }

    // Heightmap을 raw 파일로 export
    FString HeightmapDirectory = FPaths::ProjectSavedDir() + TEXT("../ExportedHeightmaps/");
    FString FullPath = HeightmapDirectory + FileName + TEXT(".raw");

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*HeightmapDirectory))
    {
        PlatformFile.CreateDirectoryTree(*HeightmapDirectory);
    }

    // Heightmap 데이터 수집
    TArray<uint16> HeightData;
    const TArray<ULandscapeComponent*>& Components = Landscape->LandscapeComponents;

    for (ULandscapeComponent* Component : Components)
    {
        if (Component)
        {
            FLandscapeComponentDataInterface DataInterface(Component);
            int32 ComponentSizeQuads = Component->ComponentSizeQuads;

            for (int32 Y = 0; Y <= ComponentSizeQuads; Y++)
            {
                for (int32 X = 0; X <= ComponentSizeQuads; X++)
                {
                    uint16 Height = DataInterface.GetHeight(X, Y);
                    HeightData.Add(Height);
                }
            }
        }
    }

    // Raw 파일로 저장
    const uint8* DataPtr = reinterpret_cast<const uint8*>(HeightData.GetData());
    int32 DataSize = HeightData.Num() * sizeof(uint16);

    bool bSuccess = FFileHelper::SaveArrayToFile(
        TArrayView<const uint8>(DataPtr, DataSize),
        *FullPath
    );

    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("Heightmap exported to: %s (%d vertices)"),
            *FullPath, HeightData.Num());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to export heightmap to: %s"), *FullPath);
    }

    return bSuccess;
}
#endif

TSharedPtr<FJsonObject> UJsonSaveManager::LandscapeToJson(ALandscape* Landscape)
{
    TSharedPtr<FJsonObject> LandscapeJson = MakeShareable(new FJsonObject);

    // 기본 정보
    LandscapeJson->SetStringField(TEXT("LandscapeName"), Landscape->GetName());
    LandscapeJson->SetObjectField(TEXT("Transform"), TransformToJson(Landscape->GetActorTransform()));

    // Landscape 크기 정보
    LandscapeJson->SetNumberField(TEXT("ComponentSizeQuads"), Landscape->ComponentSizeQuads);
    LandscapeJson->SetNumberField(TEXT("SubsectionSizeQuads"), Landscape->SubsectionSizeQuads);
    LandscapeJson->SetNumberField(TEXT("NumSubsections"), Landscape->NumSubsections);

    // Material 정보
    UMaterialInterface* Material = Landscape->GetLandscapeMaterial();
    if (Material)
    {
        LandscapeJson->SetStringField(TEXT("Material"), Material->GetName());
    }

    // Components 배열
    TArray<TSharedPtr<FJsonValue>> ComponentsArray;
    const TArray<ULandscapeComponent*>& Components = Landscape->LandscapeComponents;

    for (int32 i = 0; i < Components.Num(); i++)
    {
        ULandscapeComponent* Component = Components[i];
        if (Component)
        {
            TSharedPtr<FJsonObject> ComponentJson = LandscapeComponentToJson(Component);
            ComponentsArray.Add(MakeShareable(new FJsonValueObject(ComponentJson)));

            if ((i + 1) % 10 == 0 || i == Components.Num() - 1)
            {
                UE_LOG(LogTemp, Log, TEXT("Processed component %d/%d"), i + 1, Components.Num());
            }
        }
    }

    LandscapeJson->SetArrayField(TEXT("Components"), ComponentsArray);
    LandscapeJson->SetNumberField(TEXT("TotalComponents"), Components.Num());

    return LandscapeJson;
}

TSharedPtr<FJsonObject> UJsonSaveManager::LandscapeComponentToJson(ULandscapeComponent* Component)
{
    TSharedPtr<FJsonObject> ComponentJson = MakeShareable(new FJsonObject);

    // Section Base (타일 위치)
    FIntPoint SectionBase = Component->GetSectionBase();
    TSharedPtr<FJsonObject> SectionBaseJson = MakeShareable(new FJsonObject);
    SectionBaseJson->SetNumberField(TEXT("X"), SectionBase.X);
    SectionBaseJson->SetNumberField(TEXT("Y"), SectionBase.Y);
    ComponentJson->SetObjectField(TEXT("SectionBase"), SectionBaseJson);

    // Component 크기 정보
    ComponentJson->SetNumberField(TEXT("ComponentSizeQuads"), Component->ComponentSizeQuads);
    ComponentJson->SetNumberField(TEXT("SubsectionSizeQuads"), Component->SubsectionSizeQuads);

    // Height 데이터
    TArray<TSharedPtr<FJsonValue>> HeightDataArray = GetHeightDataJson(Component);
    ComponentJson->SetArrayField(TEXT("HeightData"), HeightDataArray);
    ComponentJson->SetNumberField(TEXT("HeightDataCount"), HeightDataArray.Num());

    // Layer (Weight Map) 데이터
    TArray<TSharedPtr<FJsonValue>> LayersArray = GetLayerDataJson(Component);
    ComponentJson->SetArrayField(TEXT("Layers"), LayersArray);
    ComponentJson->SetNumberField(TEXT("LayerCount"), LayersArray.Num());

    return ComponentJson;
}

TArray<TSharedPtr<FJsonValue>> UJsonSaveManager::GetHeightDataJson(ULandscapeComponent* Component)
{
    TArray<TSharedPtr<FJsonValue>> HeightArray;

    FLandscapeComponentDataInterface DataInterface(Component);
    int32 ComponentSizeQuads = Component->ComponentSizeQuads;
    int32 VertexCount = (ComponentSizeQuads + 1) * (ComponentSizeQuads + 1);

    HeightArray.Reserve(VertexCount);

    for (int32 Y = 0; Y <= ComponentSizeQuads; Y++)
    {
        for (int32 X = 0; X <= ComponentSizeQuads; X++)
        {
            uint16 HeightValue = DataInterface.GetHeight(X, Y);
            float Height = ((float)HeightValue - 32768.0f) * LANDSCAPE_ZSCALE;
            HeightArray.Add(MakeShareable(new FJsonValueNumber(Height)));
        }
    }

    return HeightArray;
}

TArray<TSharedPtr<FJsonValue>> UJsonSaveManager::GetLayerDataJson(ULandscapeComponent* Component)
{
    TArray<TSharedPtr<FJsonValue>> LayersArray;

    if (!Component)
    {
        return LayersArray;
    }

    const TArray<FWeightmapLayerAllocationInfo>& AllocInfos =
        Component->GetWeightmapLayerAllocations();

    const TArray<UTexture2D*>& WeightmapTextures = Component->GetWeightmapTextures();

    for (const FWeightmapLayerAllocationInfo& AllocInfo : AllocInfos)
    {
        if (!AllocInfo.LayerInfo)
        {
            continue;
        }

        if (AllocInfo.WeightmapTextureIndex >= WeightmapTextures.Num())
        {
            UE_LOG(LogTemp, Warning, TEXT("Invalid weightmap texture index for layer %s"),
                *AllocInfo.LayerInfo->LayerName.ToString());
            continue;
        }

        UTexture2D* WeightmapTexture = WeightmapTextures[AllocInfo.WeightmapTextureIndex];
        if (!WeightmapTexture || !WeightmapTexture->GetPlatformData())
        {
            continue;
        }

        TSharedPtr<FJsonObject> LayerJson = MakeShareable(new FJsonObject);
        LayerJson->SetStringField(TEXT("LayerName"),
            AllocInfo.LayerInfo->LayerName.ToString());

        // ✅ Texture 데이터 직접 읽기
        FTexturePlatformData* PlatformData = WeightmapTexture->GetPlatformData();
        if (PlatformData->Mips.Num() > 0)
        {
            FTexture2DMipMap& Mip = PlatformData->Mips[0];
            void* TextureData = Mip.BulkData.Lock(LOCK_READ_ONLY);

            if (TextureData)
            {
                const uint8* ByteData = static_cast<const uint8*>(TextureData);

                int32 ComponentSizeQuads = Component->ComponentSizeQuads;
                int32 ComponentSizeVerts = ComponentSizeQuads + 1;
                int32 TextureWidth = Mip.SizeX;
                int32 TextureHeight = Mip.SizeY;

                // Weightmap은 RGBA 4채널
                int32 ChannelOffset = AllocInfo.WeightmapTextureChannel; // 0,1,2,3
                int32 BytesPerPixel = 4;

                TArray<TSharedPtr<FJsonValue>> WeightsArray;
                WeightsArray.Reserve(ComponentSizeVerts * ComponentSizeVerts);

                for (int32 Y = 0; Y < ComponentSizeVerts && Y < TextureHeight; Y++)
                {
                    for (int32 X = 0; X < ComponentSizeVerts && X < TextureWidth; X++)
                    {
                        int32 PixelIndex = (Y * TextureWidth + X) * BytesPerPixel;
                        uint8 WeightValue = ByteData[PixelIndex + ChannelOffset];

                        float NormalizedWeight = (float)WeightValue / 255.0f;
                        WeightsArray.Add(MakeShareable(new FJsonValueNumber(NormalizedWeight)));
                    }
                }

                Mip.BulkData.Unlock();

                LayerJson->SetArrayField(TEXT("Weights"), WeightsArray);
                LayerJson->SetNumberField(TEXT("WeightCount"), WeightsArray.Num());

                LayersArray.Add(MakeShareable(new FJsonValueObject(LayerJson)));

                UE_LOG(LogTemp, Log, TEXT("Exported layer: %s with %d weights"),
                    *AllocInfo.LayerInfo->LayerName.ToString(), WeightsArray.Num());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to lock texture data for layer %s"),
                    *AllocInfo.LayerInfo->LayerName.ToString());
            }
        }
    }

    return LayersArray;
}
