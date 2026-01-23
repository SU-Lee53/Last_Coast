#include "JsonSaveManager.h"

// ✅ 가장 먼저 이것들 추가
#include "Misc/Optional.h"
#include "Templates/SharedPointer.h"
#include "Containers/Array.h"

// 나머지 헤더들
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Landscape.h"
#include "LandscapeComponent.h"
#include "LandscapeLayerInfoObject.h"
#include "Engine/Texture2D.h"

#if WITH_EDITOR
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "LandscapeDataAccess.h"
#include "LandscapeInfo.h"
#include "ImageUtils.h"
#include "Exporters/Exporter.h"
#include "AssetExportTask.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
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
    FString Directory = FPaths::ProjectSavedDir() + TEXT("../SceneJson/");
    FString FullPath = Directory + FileName + TEXT(".json");
    bool bSuccess = FFileHelper::SaveStringToFile(OutputString, *FullPath);

    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("%d actors saved to: %s"), Actors.Num(), *FullPath);
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

#if WITH_EDITOR
bool UJsonSaveManager::ExportLandscapeHeightmap(ALandscape* Landscape, const FString& FileName)
{
    if (!Landscape)
    {
        UE_LOG(LogTemp, Error, TEXT("Landscape is null"));
        return false;
    }

    FString HeightmapDirectory = FPaths::ProjectSavedDir() + TEXT("../ExportedHeightmaps/");
    FString FullPath = HeightmapDirectory + FileName + TEXT(".raw");

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*HeightmapDirectory))
    {
        PlatformFile.CreateDirectoryTree(*HeightmapDirectory);
    }

    const TArray<ULandscapeComponent*>& Components = Landscape->LandscapeComponents;

    if (Components.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("No landscape components found"));
        return false;
    }

    int32 ComponentSizeQuads = Landscape->ComponentSizeQuads;

    // ✅ 1. 기존 함수로 Landscape 크기 계산
    int32 NumComponentsX = 0;
    int32 NumComponentsY = 0;
    CalculateLandscapeSize(Landscape, NumComponentsX, NumComponentsY);

    int32 TotalVertsX = NumComponentsX * ComponentSizeQuads + 1;
    int32 TotalVertsY = NumComponentsY * ComponentSizeQuads + 1;

    UE_LOG(LogTemp, Log, TEXT("Landscape size: %dx%d components (%dx%d vertices)"),
        NumComponentsX, NumComponentsY, TotalVertsX, TotalVertsY);

    // ✅ 2. MinX, MinY 다시 계산 (Component Map 생성용)
    int32 MinX = INT_MAX, MinY = INT_MAX;
    for (ULandscapeComponent* Component : Components)
    {
        if (Component)
        {
            FIntPoint SectionBase = Component->GetSectionBase();
            MinX = FMath::Min(MinX, SectionBase.X);
            MinY = FMath::Min(MinY, SectionBase.Y);
        }
    }

    // ✅ 3. Component Map 생성 (공간적 위치로 매핑)
    TMap<FIntPoint, ULandscapeComponent*> ComponentMap;
    for (ULandscapeComponent* Component : Components)
    {
        if (Component)
        {
            FIntPoint SectionBase = Component->GetSectionBase();
            FIntPoint ComponentKey(
                (SectionBase.X - MinX) / ComponentSizeQuads,
                (SectionBase.Y - MinY) / ComponentSizeQuads
            );
            ComponentMap.Add(ComponentKey, Component);
        }
    }

    // ✅ 4. 정렬된 순서로 HeightMap 데이터 수집
    TArray<uint16> HeightData;
    HeightData.Reserve(TotalVertsX * TotalVertsY);

    // Y 방향으로 순회
    for (int32 CompY = 0; CompY < NumComponentsY; CompY++)
    {
        // 각 Component 행의 Y 좌표들
        for (int32 LocalY = 0; LocalY <= ComponentSizeQuads; LocalY++)
        {
            // 경계 중복 제거 (Y)
            if (CompY < NumComponentsY - 1 && LocalY == ComponentSizeQuads)
            {
                break;
            }

            // X 방향으로 순회
            for (int32 CompX = 0; CompX < NumComponentsX; CompX++)
            {
                FIntPoint ComponentKey(CompX, CompY);
                ULandscapeComponent* Component = ComponentMap.FindRef(ComponentKey);

                if (Component)
                {
                    FLandscapeComponentDataInterface DataInterface(Component);

                    // 각 Component의 X 좌표들
                    for (int32 LocalX = 0; LocalX <= ComponentSizeQuads; LocalX++)
                    {
                        // 경계 중복 제거 (X)
                        if (CompX < NumComponentsX - 1 && LocalX == ComponentSizeQuads)
                        {
                            continue;
                        }

                        uint16 Height = DataInterface.GetHeight(LocalX, LocalY);
                        HeightData.Add(Height);
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Missing component at (%d, %d)"), CompX, CompY);

                    // 빈 Component는 기본값으로 채움
                    int32 VertsToAdd = (CompX == NumComponentsX - 1) ?
                        (ComponentSizeQuads + 1) : ComponentSizeQuads;

                    for (int32 i = 0; i < VertsToAdd; i++)
                    {
                        HeightData.Add(32768); // 중간 높이값
                    }
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Collected %d height values (expected %d)"),
        HeightData.Num(), TotalVertsX * TotalVertsY);

    // ✅ 6. Raw 파일로 저장
    const uint8* DataPtr = reinterpret_cast<const uint8*>(HeightData.GetData());
    int32 DataSize = HeightData.Num() * sizeof(uint16);

    bool bSuccess = FFileHelper::SaveArrayToFile(
        TArrayView<const uint8>(DataPtr, DataSize),
        *FullPath
    );

    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("Heightmap exported to: %s (%d vertices, %d bytes)"),
            *FullPath, HeightData.Num(), DataSize);

        // ✅ 7. PNG로도 저장 (시각적 확인용)
        ExportHeightmapAsPNG(HeightData, TotalVertsX, TotalVertsY, FileName);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to export heightmap to: %s"), *FullPath);
    }

    return bSuccess;
}

bool UJsonSaveManager::ExportHeightmapAsPNG(const TArray<uint16>& HeightData, int32 Width, int32 Height, const FString& FileName)
{
    FString Directory = FPaths::ProjectSavedDir() + TEXT("../ExportedHeightmaps/");
    FString FullPath = Directory + FileName + TEXT(".png");

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*Directory))
    {
        PlatformFile.CreateDirectoryTree(*Directory);
    }

    // uint16 (0-65535)를 grayscale (0-255)로 변환
    TArray<FColor> ColorData;
    ColorData.Reserve(Width * Height);

    uint16 MinHeight = 65535;
    uint16 MaxHeight = 0;

    // Min/Max 찾기
    for (uint16 Height : HeightData)
    {
        MinHeight = FMath::Min(MinHeight, Height);
        MaxHeight = FMath::Max(MaxHeight, Height);
    }

    UE_LOG(LogTemp, Log, TEXT("Height range: %d - %d"), MinHeight, MaxHeight);

    // 그레이스케일로 변환
    for (int32 i = 0; i < HeightData.Num(); i++)
    {
        uint16 HeightValue = HeightData[i];

        // 정규화: 0-65535 → 0-255
        uint8 GrayValue = 0;
        if (MaxHeight > MinHeight)
        {
            float Normalized = (float)(HeightValue - MinHeight) / (float)(MaxHeight - MinHeight);
            GrayValue = (uint8)(Normalized * 255.0f);
        }

        FColor Color(GrayValue, GrayValue, GrayValue, 255);
        ColorData.Add(Color);
    }

    // PNG 저장
    TArray<uint8> CompressedData;
    FImageUtils::CompressImageArray(Width, Height, ColorData, CompressedData);

    bool bSuccess = FFileHelper::SaveArrayToFile(CompressedData, *FullPath);

    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("Heightmap PNG saved: %s"), *FullPath);
    }

    return bSuccess;
}
#endif



bool UJsonSaveManager::SaveLandscapeToJson(ALandscape* Landscape, const FString& FileName)
{
    if (!Landscape)
    {
        UE_LOG(LogTemp, Error, TEXT("Landscape is null"));
        return false;
    }

    // 기존 LandscapeToJson 함수 사용
    TSharedPtr<FJsonObject> RootJson = LandscapeToJson(Landscape);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootJson.ToSharedRef(), Writer);

    FString Directory = FPaths::ProjectSavedDir() + TEXT("../LandScapeJson/");
    FString FullPath = Directory + FileName + TEXT(".json");
    bool bSuccess = FFileHelper::SaveStringToFile(OutputString, *FullPath);

    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("Landscape saved to: %s"), *FullPath);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save landscape to: %s"), *FullPath);
    }

    return bSuccess;
}

void UJsonSaveManager::CalculateLandscapeSize(ALandscape* Landscape, int32& OutNumX, int32& OutNumY)
{
    const TArray<ULandscapeComponent*>& Components = Landscape->LandscapeComponents;

    int32 MinX = INT_MAX, MaxX = INT_MIN;
    int32 MinY = INT_MAX, MaxY = INT_MIN;

    for (ULandscapeComponent* Component : Components)
    {
        if (Component)
        {
            FIntPoint SectionBase = Component->GetSectionBase();
            MinX = FMath::Min(MinX, SectionBase.X);
            MaxX = FMath::Max(MaxX, SectionBase.X);
            MinY = FMath::Min(MinY, SectionBase.Y);
            MaxY = FMath::Max(MaxY, SectionBase.Y);
        }
    }

    int32 ComponentSizeQuads = Landscape->ComponentSizeQuads;
    OutNumX = (MaxX - MinX) / ComponentSizeQuads + 1;
    OutNumY = (MaxY - MinY) / ComponentSizeQuads + 1;
}


TSharedPtr<FJsonObject> UJsonSaveManager::LandscapeToJson(ALandscape* Landscape)
{
    TSharedPtr<FJsonObject> RootJson = MakeShareable(new FJsonObject);

    // ========================================
    // 1. Landscape 기본 정보
    // ========================================
    TSharedPtr<FJsonObject> LandscapeJson = MakeShareable(new FJsonObject);

    // Scale
    FVector Scale = Landscape->GetActorScale3D();
    TSharedPtr<FJsonObject> ScaleJson = MakeShareable(new FJsonObject);
    ScaleJson->SetNumberField(TEXT("X"), Scale.X);
    ScaleJson->SetNumberField(TEXT("Y"), Scale.Y);
    ScaleJson->SetNumberField(TEXT("Z"), Scale.Z);
    LandscapeJson->SetObjectField(TEXT("Scale"), ScaleJson);

    // HeightMap Resolution 계산
    int32 ComponentSizeQuads = Landscape->ComponentSizeQuads;
    int32 NumComponentsX = 0;
    int32 NumComponentsY = 0;
    CalculateLandscapeSize(Landscape, NumComponentsX, NumComponentsY);

    int32 HeightMapResX = NumComponentsX * ComponentSizeQuads + 1;
    int32 HeightMapResY = NumComponentsY * ComponentSizeQuads + 1;

    TSharedPtr<FJsonObject> HeightMapResJson = MakeShareable(new FJsonObject);
    HeightMapResJson->SetNumberField(TEXT("X"), HeightMapResX);
    HeightMapResJson->SetNumberField(TEXT("Y"), HeightMapResY);
    LandscapeJson->SetObjectField(TEXT("HeightMapResolution"), HeightMapResJson);

    // HeightZScale
    //LandscapeJson->SetNumberField(TEXT("HeightZScale"), LANDSCAPE_ZSCALE);

    // HeightMap 파일 정보
    TSharedPtr<FJsonObject> HeightMapJson = MakeShareable(new FJsonObject);
    HeightMapJson->SetStringField(TEXT("Path"), TEXT("height.raw"));
    HeightMapJson->SetStringField(TEXT("Type"), TEXT("uint16"));
    HeightMapJson->SetStringField(TEXT("Endian"), TEXT("Little"));
    LandscapeJson->SetObjectField(TEXT("HeightMap"), HeightMapJson);

    RootJson->SetObjectField(TEXT("Landscape"), LandscapeJson);

    // ========================================
    // 2. Layers 정보 (전역 레이어 목록)
    // ========================================
    TMap<FName, int32> LayerIndexMap;
    TArray<TSharedPtr<FJsonValue>> LayersArray = GetLayersInfoJson(Landscape, LayerIndexMap);
    RootJson->SetArrayField(TEXT("Layers"), LayersArray);

//#if WITH_EDITOR
//    ExportLayerTextures(Landscape, LayerIndexMap);
//#endif

    // ========================================
    // 3. Components 정보
    // ========================================
    TArray<TSharedPtr<FJsonValue>> ComponentsArray;
    const TArray<ULandscapeComponent*>& Components = Landscape->LandscapeComponents;

    for (int32 i = 0; i < Components.Num(); i++)
    {
        ULandscapeComponent* Component = Components[i];
        if (Component)
        {
            TSharedPtr<FJsonObject> ComponentJson = LandscapeComponentToJson(Component, i, LayerIndexMap);
            ComponentsArray.Add(MakeShareable(new FJsonValueObject(ComponentJson)));

            if ((i + 1) % 10 == 0 || i == Components.Num() - 1)
            {
                UE_LOG(LogTemp, Log, TEXT("Processed component %d/%d"), i + 1, Components.Num());
            }
        }
    }

    RootJson->SetArrayField(TEXT("Components"), ComponentsArray);

    return RootJson;
}

TSharedPtr<FJsonObject> UJsonSaveManager::LandscapeComponentToJson(
    ULandscapeComponent* Component,
    int32 ComponentIndex,
    const TMap<FName, int32>& LayerIndexMap)
{
    TSharedPtr<FJsonObject> ComponentJson = MakeShareable(new FJsonObject);

    // Component Index
    ComponentJson->SetNumberField(TEXT("ComponentIndex"), ComponentIndex);

    // Origin (월드 좌표로 변환)
    FIntPoint SectionBase = Component->GetSectionBase();
    ALandscape* Landscape = Component->GetLandscapeActor();
    FVector Scale = Landscape ? Landscape->GetActorScale3D() : FVector(100.0f);

    TSharedPtr<FJsonObject> OriginJson = MakeShareable(new FJsonObject);
    OriginJson->SetNumberField(TEXT("X"), SectionBase.X * Scale.X);
    OriginJson->SetNumberField(TEXT("Y"), SectionBase.Y * Scale.Y);
    ComponentJson->SetObjectField(TEXT("Origin"), OriginJson);

    // NumQuads
    TSharedPtr<FJsonObject> NumQuadsJson = MakeShareable(new FJsonObject);
    NumQuadsJson->SetNumberField(TEXT("X"), Component->ComponentSizeQuads);
    NumQuadsJson->SetNumberField(TEXT("Y"), Component->ComponentSizeQuads);
    ComponentJson->SetObjectField(TEXT("NumQuads"), NumQuadsJson);

    // WeightMaps (채널 패킹 방식)
    TArray<TSharedPtr<FJsonValue>> WeightMapsArray =
        GetComponentWeightMapsJson(Component, ComponentIndex, LayerIndexMap);
    ComponentJson->SetArrayField(TEXT("WeightMaps"), WeightMapsArray);

    return ComponentJson;
}


TArray<TSharedPtr<FJsonValue>> UJsonSaveManager::GetLayersInfoJson(ALandscape* Landscape, TMap<FName, int32>& OutLayerIndexMap)
{
    TArray<TSharedPtr<FJsonValue>> LayersArray;
    const TArray<ULandscapeComponent*>& Components = Landscape->LandscapeComponents;

    // 모든 컴포넌트에서 레이어 수집
    for (ULandscapeComponent* Component : Components)
    {
        if (!Component) continue;

        const TArray<FWeightmapLayerAllocationInfo>& AllocInfos =
            Component->GetWeightmapLayerAllocations();

        for (const FWeightmapLayerAllocationInfo& AllocInfo : AllocInfos)
        {
            if (AllocInfo.LayerInfo && !OutLayerIndexMap.Contains(AllocInfo.LayerInfo->LayerName))
            {
                int32 Index = OutLayerIndexMap.Num();
                OutLayerIndexMap.Add(AllocInfo.LayerInfo->LayerName, Index);

                TSharedPtr<FJsonObject> LayerJson = MakeShareable(new FJsonObject);
                LayerJson->SetNumberField(TEXT("Index"), Index);
                LayerJson->SetStringField(TEXT("Name"), AllocInfo.LayerInfo->LayerName.ToString());

                // 텍스처 경로 (예시)
                FString LayerName = AllocInfo.LayerInfo->LayerName.ToString().ToLower();
                LayerJson->SetStringField(TEXT("Albedo"),
                    FString::Printf(TEXT("%s_albedo.dds"), *LayerName));
                LayerJson->SetStringField(TEXT("Normal"),
                    FString::Printf(TEXT("%s_normal.dds"), *LayerName));

                // Tiling 값 (Material에서 추출 가능하면 실제 값 사용)
                LayerJson->SetNumberField(TEXT("Tiling"), 0.01);

                LayersArray.Add(MakeShareable(new FJsonValueObject(LayerJson)));
            }
        }
    }

    return LayersArray;
}

TArray<TSharedPtr<FJsonValue>> UJsonSaveManager::GetComponentWeightMapsJson(
    ULandscapeComponent* Component,
    int32 ComponentIndex,
    const TMap<FName, int32>& LayerIndexMap)
{
    TArray<TSharedPtr<FJsonValue>> WeightMapsArray;

    const TArray<UTexture2D*>& WeightmapTextures = Component->GetWeightmapTextures();
    const TArray<FWeightmapLayerAllocationInfo>& AllocInfos =
        Component->GetWeightmapLayerAllocations();

    UE_LOG(LogTemp, Warning, TEXT("========== Component %d =========="), ComponentIndex);

    // Weightmap Texture별로 처리 (RGBA 4채널에 레이어 패킹)
    for (int32 TexIdx = 0; TexIdx < WeightmapTextures.Num(); TexIdx++)
    {
        UTexture2D* WeightmapTexture = WeightmapTextures[TexIdx];
        if (!WeightmapTexture) continue;

        TSharedPtr<FJsonObject> WeightMapJson = MakeShareable(new FJsonObject);

        // 파일 경로
        FString WeightMapPath = FString::Printf(TEXT("weight_c%d_%d"), ComponentIndex, TexIdx);
        WeightMapJson->SetStringField(TEXT("Path"), WeightMapPath);
        WeightMapJson->SetStringField(TEXT("Format"), TEXT("RGBA8"));

        // LayerIndex 배열 생성 (RGBA 4채널)
        TArray<TSharedPtr<FJsonValue>> LayerIndexArray;
        int32 ChannelMapping[4] = { -1, -1, -1, -1 }; // R, G, B, A

        UE_LOG(LogTemp, Warning, TEXT("  Texture %d:"), TexIdx);

        // 이 텍스처를 사용하는 레이어들 찾기
        for (const FWeightmapLayerAllocationInfo& AllocInfo : AllocInfos)
        {
            if (AllocInfo.WeightmapTextureIndex == TexIdx && AllocInfo.LayerInfo)
            {
                int32 Channel = AllocInfo.WeightmapTextureChannel; // 0,1,2,3
                FString LayerName = AllocInfo.LayerInfo->LayerName.ToString();
                const int32* LayerIdx = LayerIndexMap.Find(AllocInfo.LayerInfo->LayerName);

                UE_LOG(LogTemp, Warning, TEXT("    Channel %d: %s (LayerIndex=%d)"),
                    Channel, *LayerName, LayerIdx ? *LayerIdx : -1);

                if (LayerIdx && Channel >= 0 && Channel < 4)
                {
                    ChannelMapping[Channel] = *LayerIdx;
                }
            }
        }

        // JSON 배열로 변환
        for (int32 i = 0; i < 4; i++)
        {
            LayerIndexArray.Add(MakeShareable(new FJsonValueNumber(ChannelMapping[i])));
        }

        WeightMapJson->SetArrayField(TEXT("LayerIndex"), LayerIndexArray);

        UE_LOG(LogTemp, Warning, TEXT("    Final LayerIndex: [%d, %d, %d, %d]"),
            ChannelMapping[0], ChannelMapping[1], ChannelMapping[2], ChannelMapping[3]);

        // PNG 파일로 저장 (Component와 AllocInfos도 전달)
        ExportWeightMapTextureToPNG(Component, TexIdx, WeightMapPath, AllocInfos);

        WeightMapsArray.Add(MakeShareable(new FJsonValueObject(WeightMapJson)));
    }

    return WeightMapsArray;
}

#if WITH_EDITOR
bool UJsonSaveManager::ExportWeightMapTextureToPNG(
    ULandscapeComponent* Component,
    int32 WeightmapTextureIndex,
    const FString& FileName,
    const TArray<FWeightmapLayerAllocationInfo>& AllocInfos)
{
    if (!Component)
    {
        UE_LOG(LogTemp, Error, TEXT("Component is null"));
        return false;
    }

    FString Directory = FPaths::ProjectSavedDir() + TEXT("../ExportedWeightmaps/");
    FString FullPath = Directory + FileName + TEXT(".png"); // ✅ 추가


    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*Directory))
    {
        PlatformFile.CreateDirectoryTree(*Directory);
    }

    int32 ComponentSizeQuads = Component->ComponentSizeQuads;
    int32 ComponentSizeVerts = ComponentSizeQuads + 1;

    UE_LOG(LogTemp, Warning, TEXT("===== Exporting %s ====="), *FileName);
    UE_LOG(LogTemp, Warning, TEXT("WeightmapTextureIndex: %d"), WeightmapTextureIndex);

    // 이 텍스처 인덱스를 사용하는 레이어들 출력
    int32 LayerCount = 0;
    for (const FWeightmapLayerAllocationInfo& AllocInfo : AllocInfos)
    {
        if (AllocInfo.WeightmapTextureIndex == WeightmapTextureIndex && AllocInfo.LayerInfo)
        {
            FString LayerName = AllocInfo.LayerInfo->LayerName.ToString();

            // Visibility 제외
            if (LayerName.Equals(TEXT("Visibility"), ESearchCase::IgnoreCase) ||
                AllocInfo.LayerInfo->bNoWeightBlend)
            {
                continue;
            }

            LayerCount++;
            UE_LOG(LogTemp, Warning, TEXT("  Layer %d: %s (Channel %d)"),
                LayerCount,
                *LayerName,
                AllocInfo.WeightmapTextureChannel);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Total layers in this texture: %d"), LayerCount);

    // Weightmap 텍스처 가져오기
    const TArray<UTexture2D*>& WeightmapTextures = Component->GetWeightmapTextures();
    if (WeightmapTextureIndex >= WeightmapTextures.Num())
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid WeightmapTextureIndex: %d"), WeightmapTextureIndex);
        return false;
    }

    UTexture2D* WeightmapTexture = WeightmapTextures[WeightmapTextureIndex];
    if (!WeightmapTexture)
    {
        UE_LOG(LogTemp, Error, TEXT("WeightmapTexture is null"));
        return false;
    }

    if (!WeightmapTexture->Source.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Texture source is not valid"));
        return false;
    }

    int32 TextureWidth = WeightmapTexture->Source.GetSizeX();
    int32 TextureHeight = WeightmapTexture->Source.GetSizeY();

    UE_LOG(LogTemp, Warning, TEXT("Texture size: %dx%d"), TextureWidth, TextureHeight);

    // Source 데이터 추출
    TArray64<uint8> RawData;
    if (!WeightmapTexture->Source.GetMipData(RawData, 0))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get mip data"));
        return false;
    }

    if (RawData.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Empty mip data"));
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("Raw data size: %lld bytes"), RawData.Num());

    // ✅ 포맷 확인
    ETextureSourceFormat SourceFormat = WeightmapTexture->Source.GetFormat();
    UE_LOG(LogTemp, Warning, TEXT("Source format: %d"), (int32)SourceFormat);

    // ✅ 포맷에 따라 BytesPerPixel 결정
    int32 BytesPerPixel = 4; // 기본값
    bool bIsBGRA = false;

    // ETextureSourceFormat enum 값들:
    // TSF_G8 = 0
    // TSF_BGRA8 = 1
    // TSF_BGRE8 = 2
    // TSF_RGBA16 = 3
    // TSF_RGBA16F = 4
    // ...

    if (SourceFormat == ETextureSourceFormat::TSF_G8)
    {
        BytesPerPixel = 1;
        UE_LOG(LogTemp, Warning, TEXT("Format is G8 (grayscale)"));
    }
    else if (SourceFormat == ETextureSourceFormat::TSF_BGRA8)
    {
        BytesPerPixel = 4;
        bIsBGRA = true;
        UE_LOG(LogTemp, Warning, TEXT("Format is BGRA8"));
    }
    else if (SourceFormat == ETextureSourceFormat::TSF_BGRE8)
    {
        BytesPerPixel = 4;
        bIsBGRA = true;
        UE_LOG(LogTemp, Warning, TEXT("Format is BGRE8"));
    }
    else if (SourceFormat == ETextureSourceFormat::TSF_RGBA16)
    {
        BytesPerPixel = 8;
        UE_LOG(LogTemp, Warning, TEXT("Format is RGBA16"));
    }
    else
    {
        // 기타 포맷은 4바이트로 가정
        UE_LOG(LogTemp, Warning, TEXT("Unknown format, assuming 4 bytes per pixel"));
    }

    // ✅ 채널별로 데이터를 분리해서 저장
    TMap<int32, TArray<uint8>> ChannelData; // Channel → Weight 배열

    for (const FWeightmapLayerAllocationInfo& AllocInfo : AllocInfos)
    {
        if (AllocInfo.WeightmapTextureIndex == WeightmapTextureIndex && AllocInfo.LayerInfo)
        {
            FString LayerName = AllocInfo.LayerInfo->LayerName.ToString();

            // Visibility 제외
            if (LayerName.Equals(TEXT("Visibility"), ESearchCase::IgnoreCase) ||
                AllocInfo.LayerInfo->bNoWeightBlend)
            {
                continue;
            }

            int32 Channel = AllocInfo.WeightmapTextureChannel;
            TArray<uint8> Weights;
            Weights.Reserve(TextureWidth * TextureHeight);

            // ✅ 해당 채널의 데이터만 추출
            for (int32 Y = 0; Y < TextureHeight; Y++)
            {
                for (int32 X = 0; X < TextureWidth; X++)
                {
                    int32 PixelIndex = (Y * TextureWidth + X) * BytesPerPixel;
                    uint8 Weight = 0;

                    if (BytesPerPixel == 1)
                    {
                        // Grayscale: 모든 채널이 같은 값
                        if (PixelIndex < RawData.Num())
                        {
                            Weight = RawData[PixelIndex];
                        }
                    }
                    else if (BytesPerPixel == 4)
                    {
                        // BGRA 또는 RGBA
                        if (PixelIndex + 3 < RawData.Num())
                        {
                            if (bIsBGRA)
                            {
                                // BGRA 순서: B=0, G=1, R=2, A=3
                                // Channel 매핑: R(0)→2, G(1)→1, B(2)→0, A(3)→3
                                int32 BGRAMap[4] = { 2, 1, 0, 3 };
                                Weight = RawData[PixelIndex + BGRAMap[Channel]];
                            }
                            else
                            {
                                // RGBA 순서
                                Weight = RawData[PixelIndex + Channel];
                            }
                        }
                    }
                    else if (BytesPerPixel == 8)
                    {
                        // RGBA16: 각 채널이 2바이트, 상위 바이트 사용
                        int32 ChannelOffset = Channel * 2;
                        if (PixelIndex + ChannelOffset + 1 < RawData.Num())
                        {
                            // 16비트를 8비트로 변환 (상위 바이트 사용)
                            Weight = RawData[PixelIndex + ChannelOffset + 1];
                        }
                    }

                    Weights.Add(Weight);
                }
            }

            ChannelData.Add(Channel, Weights);
            UE_LOG(LogTemp, Warning, TEXT("  Extracted %d weights for channel %d"),
                Weights.Num(), Channel);
        }
    }

    // ✅ ColorData 생성
    TArray<FColor> ColorData;
    ColorData.Reserve(ComponentSizeVerts * ComponentSizeVerts);

    for (int32 Y = 0; Y < ComponentSizeVerts && Y < TextureHeight; Y++)
    {
        for (int32 X = 0; X < ComponentSizeVerts && X < TextureWidth; X++)
        {
            int32 Index = Y * TextureWidth + X;
            FColor Color(0, 0, 0, 0);

            // 각 채널 데이터 할당
            if (ChannelData.Contains(0) && Index < ChannelData[0].Num())
            {
                Color.R = ChannelData[0][Index];
            }
            if (ChannelData.Contains(1) && Index < ChannelData[1].Num())
            {
                Color.G = ChannelData[1][Index];
            }
            if (ChannelData.Contains(2) && Index < ChannelData[2].Num())
            {
                Color.B = ChannelData[2][Index];
            }
            if (ChannelData.Contains(3) && Index < ChannelData[3].Num())
            {
                Color.A = ChannelData[3][Index];
            }

            if (X < 5 && Y == 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("  Pixel[%d,0]: RGBA=(%d,%d,%d,%d) Sum=%d"),
                    X, Color.R, Color.G, Color.B, Color.A,
                    Color.R + Color.G + Color.B + Color.A);
            }

            ColorData.Add(Color);
        }
    }

    // PNG 저장
    TArray<uint8> CompressedData;
    FImageUtils::CompressImageArray(ComponentSizeVerts, ComponentSizeVerts, ColorData, CompressedData);

    bool bSuccess = FFileHelper::SaveArrayToFile(CompressedData, *FullPath);

    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("✅ Weight map exported: %s (%dx%d)"),
            *FullPath, ComponentSizeVerts, ComponentSizeVerts);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save weight map: %s"), *FullPath);
    }

    return bSuccess;
}
#endif



//#if WITH_EDITOR
//
//bool UJsonSaveManager::ExportTextureToDDS(UTexture2D* Texture, const FString& FilePath)
//{
//    if (!Texture)
//    {
//        UE_LOG(LogTemp, Error, TEXT("Texture is null"));
//        return false;
//    }
//
//    // 디렉토리 생성
//    FString Directory = FPaths::GetPath(FilePath);
//    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
//    if (!PlatformFile.DirectoryExists(*Directory))
//    {
//        PlatformFile.CreateDirectoryTree(*Directory);
//    }
//
//    UE_LOG(LogTemp, Log, TEXT("Exporting texture '%s' to: %s"), *Texture->GetName(), *FilePath);
//
//    // UExporter::ExportToFile 사용
//    bool bSuccess = UExporter::ExportToFile(
//        Texture,
//        nullptr,      // Exporter (nullptr = 자동 선택)
//        *FilePath,
//        false,        // bSelected
//        false,        // bReplaceIdentical
//        false         // bPrompt (true로 하면 대화상자 뜸)
//    );
//
//    if (bSuccess)
//    {
//        UE_LOG(LogTemp, Log, TEXT("✅ Texture exported to DDS: %s"), *FilePath);
//    }
//    else
//    {
//        UE_LOG(LogTemp, Warning, TEXT("ExportToFile failed, trying AssetExportTask..."));
//
//        // 대체 방법: AssetExportTask 사용
//        UAssetExportTask* ExportTask = NewObject<UAssetExportTask>();
//        ExportTask->Object = Texture;
//        ExportTask->Filename = FilePath;
//        ExportTask->bSelected = false;
//        ExportTask->bReplaceIdentical = true;
//        ExportTask->bPrompt = false;
//        ExportTask->bAutomated = true;
//
//        bSuccess = UExporter::RunAssetExportTask(ExportTask);
//
//        if (bSuccess)
//        {
//            UE_LOG(LogTemp, Log, TEXT("✅ Texture exported via AssetExportTask: %s"), *FilePath);
//        }
//        else
//        {
//            UE_LOG(LogTemp, Error, TEXT("❌ Both methods failed to export: %s"), *FilePath);
//        }
//    }
//
//    return bSuccess;
//}
//
//bool UJsonSaveManager::ExportLayerTextures(ALandscape* Landscape, const TMap<FName, int32>& LayerIndexMap)
//{
//    FString TextureDirectory = FPaths::ProjectSavedDir() + TEXT("../ExportedTextures/");
//
//    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
//    if (!PlatformFile.DirectoryExists(*TextureDirectory))
//    {
//        PlatformFile.CreateDirectoryTree(*TextureDirectory);
//    }
//
//    // Material에서 텍스처 추출
//    TArray<FLayerTextureInfo> LayerTextures = ExtractLayerTexturesFromMaterial(Landscape);
//
//    if (LayerTextures.Num() == 0)
//    {
//        UE_LOG(LogTemp, Warning, TEXT("No layer textures found in material"));
//        return false;
//    }
//
//    UE_LOG(LogTemp, Log, TEXT("===== Exporting Layer Textures ====="));
//    UE_LOG(LogTemp, Log, TEXT("Exporting textures for %d layers to: %s"),
//        LayerTextures.Num(), *TextureDirectory);
//
//    int32 ExportCount = 0;
//    int32 TotalAttempts = 0;
//
//    for (const FLayerTextureInfo& LayerInfo : LayerTextures)
//    {
//        FString LayerNameLower = LayerInfo.LayerName.ToLower();
//
//        // Albedo 텍스처
//        if (LayerInfo.AlbedoTexture)
//        {
//            TotalAttempts++;
//            FString FilePath = TextureDirectory + LayerNameLower + TEXT("_albedo.dds");
//            if (ExportTextureToDDS(LayerInfo.AlbedoTexture, FilePath))
//            {
//                ExportCount++;
//            }
//        }
//
//        // Normal 텍스처
//        if (LayerInfo.NormalTexture)
//        {
//            TotalAttempts++;
//            FString FilePath = TextureDirectory + LayerNameLower + TEXT("_normal.dds");
//            if (ExportTextureToDDS(LayerInfo.NormalTexture, FilePath))
//            {
//                ExportCount++;
//            }
//        }
//
//        // Roughness 텍스처
//        if (LayerInfo.RoughnessTexture)
//        {
//            TotalAttempts++;
//            FString FilePath = TextureDirectory + LayerNameLower + TEXT("_roughness.dds");
//            if (ExportTextureToDDS(LayerInfo.RoughnessTexture, FilePath))
//            {
//                ExportCount++;
//            }
//        }
//
//        // Metallic 텍스처
//        if (LayerInfo.MetallicTexture)
//        {
//            TotalAttempts++;
//            FString FilePath = TextureDirectory + LayerNameLower + TEXT("_metallic.dds");
//            if (ExportTextureToDDS(LayerInfo.MetallicTexture, FilePath))
//            {
//                ExportCount++;
//            }
//        }
//    }
//
//    UE_LOG(LogTemp, Log, TEXT("===== Export Complete ====="));
//    UE_LOG(LogTemp, Log, TEXT("Successfully exported %d/%d textures"), ExportCount, TotalAttempts);
//
//    return ExportCount > 0;
//}
//
//TArray<UJsonSaveManager::FLayerTextureInfo> UJsonSaveManager::ExtractLayerTexturesFromMaterial(ALandscape* Landscape)
//{
//    TArray<FLayerTextureInfo> LayerTextures;
//
//    UMaterialInterface* LandscapeMaterial = Landscape->GetLandscapeMaterial();
//    if (!LandscapeMaterial)
//    {
//        UE_LOG(LogTemp, Error, TEXT("No landscape material found"));
//        return LayerTextures;
//    }
//
//    // Material Instance 가져오기
//    UMaterialInstanceConstant* MaterialInstance = Cast<UMaterialInstanceConstant>(LandscapeMaterial);
//    if (!MaterialInstance)
//    {
//        UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(LandscapeMaterial);
//        if (DynamicMaterial)
//        {
//            MaterialInstance = Cast<UMaterialInstanceConstant>(DynamicMaterial->Parent);
//        }
//    }
//
//    if (!MaterialInstance)
//    {
//        UE_LOG(LogTemp, Error, TEXT("Material is not a Material Instance"));
//        return LayerTextures;
//    }
//
//    UE_LOG(LogTemp, Log, TEXT("===== Extracting Layer Textures ====="));
//    UE_LOG(LogTemp, Log, TEXT("Material: %s"), *LandscapeMaterial->GetName());
//
//    // 모든 Texture Parameter 가져오기
//    TArray<FMaterialParameterInfo> TextureParameterInfos;
//    TArray<FGuid> TextureParameterGuids;
//    MaterialInstance->GetAllTextureParameterInfo(TextureParameterInfos, TextureParameterGuids);
//
//    UE_LOG(LogTemp, Log, TEXT("Found %d texture parameters in material"), TextureParameterInfos.Num());
//
//    // 레이어별로 텍스처 그룹화
//    TMap<FString, FLayerTextureInfo> LayerTextureMap;
//
//    for (const FMaterialParameterInfo& ParamInfo : TextureParameterInfos)
//    {
//        FString ParamName = ParamInfo.Name.ToString();
//        UTexture* Texture = nullptr;
//
//        if (!MaterialInstance->GetTextureParameterValue(ParamInfo, Texture))
//        {
//            continue;
//        }
//
//        UTexture2D* Texture2D = Cast<UTexture2D>(Texture);
//        if (!Texture2D)
//        {
//            continue;
//        }
//
//        UE_LOG(LogTemp, Log, TEXT("  Parameter: %s -> %s"), *ParamName, *Texture2D->GetName());
//
//        // 파라미터 이름 파싱
//        FString LayerName;
//        FString TextureType;
//
//        if (!ParamName.Contains(TEXT("_")))
//        {
//            // 언더스코어가 없는 경우 - 다른 패턴 시도
//            if (ParamName.EndsWith(TEXT("Albedo")))
//            {
//                LayerName = ParamName.Left(ParamName.Len() - 6);
//                TextureType = TEXT("Albedo");
//            }
//            else if (ParamName.EndsWith(TEXT("Normal")))
//            {
//                LayerName = ParamName.Left(ParamName.Len() - 6);
//                TextureType = TEXT("Normal");
//            }
//            else if (ParamName.EndsWith(TEXT("Roughness")))
//            {
//                LayerName = ParamName.Left(ParamName.Len() - 9);
//                TextureType = TEXT("Roughness");
//            }
//            else if (ParamName.EndsWith(TEXT("Metallic")))
//            {
//                LayerName = ParamName.Left(ParamName.Len() - 8);
//                TextureType = TEXT("Metallic");
//            }
//            else
//            {
//                UE_LOG(LogTemp, Warning, TEXT("    -> Skipped (unrecognized pattern)"));
//                continue;
//            }
//        }
//        else
//        {
//            // 언더스코어로 분리
//            ParamName.Split(TEXT("_"), &LayerName, &TextureType);
//        }
//
//        if (LayerName.IsEmpty() || TextureType.IsEmpty())
//        {
//            UE_LOG(LogTemp, Warning, TEXT("    -> Skipped (empty layer or type)"));
//            continue;
//        }
//
//        // 레이어 정보 가져오기 또는 생성
//        if (!LayerTextureMap.Contains(LayerName))
//        {
//            FLayerTextureInfo Info;
//            Info.LayerName = LayerName;
//            Info.AlbedoTexture = nullptr;
//            Info.NormalTexture = nullptr;
//            Info.RoughnessTexture = nullptr;
//            Info.MetallicTexture = nullptr;
//            LayerTextureMap.Add(LayerName, Info);
//        }
//
//        FLayerTextureInfo& Info = LayerTextureMap[LayerName];
//
//        // 텍스처 타입별로 할당
//        if (TextureType.Contains(TEXT("Albedo")) || TextureType.Contains(TEXT("BaseColor")) ||
//            TextureType.Contains(TEXT("Diffuse")) || TextureType.Contains(TEXT("Color")))
//        {
//            Info.AlbedoTexture = Texture2D;
//            UE_LOG(LogTemp, Log, TEXT("    -> %s: Albedo"), *LayerName);
//        }
//        else if (TextureType.Contains(TEXT("Normal")))
//        {
//            Info.NormalTexture = Texture2D;
//            UE_LOG(LogTemp, Log, TEXT("    -> %s: Normal"), *LayerName);
//        }
//        else if (TextureType.Contains(TEXT("Roughness")) || TextureType.Contains(TEXT("Rough")))
//        {
//            Info.RoughnessTexture = Texture2D;
//            UE_LOG(LogTemp, Log, TEXT("    -> %s: Roughness"), *LayerName);
//        }
//        else if (TextureType.Contains(TEXT("Metallic")) || TextureType.Contains(TEXT("Metal")))
//        {
//            Info.MetallicTexture = Texture2D;
//            UE_LOG(LogTemp, Log, TEXT("    -> %s: Metallic"), *LayerName);
//        }
//        else
//        {
//            UE_LOG(LogTemp, Warning, TEXT("    -> Unknown texture type: %s"), *TextureType);
//        }
//    }
//
//    // Map을 Array로 변환
//    UE_LOG(LogTemp, Log, TEXT("===== Layer Summary ====="));
//    for (auto& Pair : LayerTextureMap)
//    {
//        LayerTextures.Add(Pair.Value);
//        UE_LOG(LogTemp, Log, TEXT("Layer '%s': Albedo=%s, Normal=%s, Roughness=%s, Metallic=%s"),
//            *Pair.Value.LayerName,
//            Pair.Value.AlbedoTexture ? *Pair.Value.AlbedoTexture->GetName() : TEXT("None"),
//            Pair.Value.NormalTexture ? *Pair.Value.NormalTexture->GetName() : TEXT("None"),
//            Pair.Value.RoughnessTexture ? *Pair.Value.RoughnessTexture->GetName() : TEXT("None"),
//            Pair.Value.MetallicTexture ? *Pair.Value.MetallicTexture->GetName() : TEXT("None"));
//    }
//
//    return LayerTextures;
//}
//
//#endif