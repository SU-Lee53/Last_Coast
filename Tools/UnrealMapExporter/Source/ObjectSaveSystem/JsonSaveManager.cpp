#include "JsonSaveManager.h"

// ✅ 가장 먼저 이것들 추가 (순서 중요!)
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
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"

// Material 관련 헤더
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionLandscapeLayerBlend.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"

// StaticMesh 관련
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

bool UJsonSaveManager::ExportHeightmapAsPNG(
    const TArray<uint16>& HeightData,
    int32 Width,
    int32 Height,
    const FString& FileName)
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
    for (uint16 HeightValue : HeightData)
    {
        MinHeight = FMath::Min(MinHeight, HeightValue);
        MaxHeight = FMath::Max(MaxHeight, HeightValue);
    }

    UE_LOG(LogTemp, Log, TEXT("Height range: %d - %d"), MinHeight, MaxHeight);

    // 그레이스케일로 변환
    for (int32 i = 0; i < HeightData.Num(); i++)
    {
        uint16 HeightValue = HeightData[i];

        uint8 GrayValue = 0;
        if (MaxHeight > MinHeight)
        {
            float Normalized = (float)(HeightValue - MinHeight) / (float)(MaxHeight - MinHeight);
            GrayValue = (uint8)(Normalized * 255.0f);
        }

        FColor Color(GrayValue, GrayValue, GrayValue, 255);
        ColorData.Add(Color);
    }

    // ✅ IImageWrapper를 사용한 무손실 PNG 저장
    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

    if (!ImageWrapper.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create image wrapper"));
        return false;
    }

    // Raw RGBA 데이터 준비
    TArray<uint8> RawRGBA;
    RawRGBA.Reserve(Width * Height * 4);

    for (const FColor& Color : ColorData)
    {
        RawRGBA.Add(Color.R);
        RawRGBA.Add(Color.G);
        RawRGBA.Add(Color.B);
        RawRGBA.Add(Color.A);
    }

    if (ImageWrapper->SetRaw(RawRGBA.GetData(), RawRGBA.Num(), Width, Height, ERGBFormat::RGBA, 8))
    {
        const TArray64<uint8>& CompressedData = ImageWrapper->GetCompressed(100);

        bool bSuccess = FFileHelper::SaveArrayToFile(CompressedData, *FullPath);

        if (bSuccess)
        {
            UE_LOG(LogTemp, Log, TEXT("✅ Lossless heightmap PNG saved: %s"), *FullPath);
        }

        return bSuccess;
    }

    return false;
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
    LandscapeJson->SetNumberField(TEXT("HeightZScale"), LANDSCAPE_ZSCALE);

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
#if WITH_EDITOR
    // ✅ 먼저 텍스처 export하고 결과 받기
    TMap<FName, FExportedLayerTextures> ExportedTextures =
        ExportLayerTextures(Landscape, LayerIndexMap);

    // ✅ Export된 텍스처 정보를 포함하여 JSON 생성
    TArray<TSharedPtr<FJsonValue>> LayersArray =
        GetLayersInfoJson(Landscape, LayerIndexMap, ExportedTextures);
#else
    TMap<FName, FExportedLayerTextures> ExportedTextures;
    TArray<TSharedPtr<FJsonValue>> LayersArray =
        GetLayersInfoJson(Landscape, LayerIndexMap, ExportedTextures);
#endif

    RootJson->SetArrayField(TEXT("Layers"), LayersArray);

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


TArray<TSharedPtr<FJsonValue>> UJsonSaveManager::GetLayersInfoJson(
    ALandscape* Landscape,
    TMap<FName, int32>& OutLayerIndexMap,
    const TMap<FName, FExportedLayerTextures>& ExportedTextures)
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
            if (!AllocInfo.LayerInfo) continue;

            FName LayerFName = AllocInfo.LayerInfo->LayerName;

            // Visibility 레이어 제외
            if (LayerFName.ToString().Equals(TEXT("Visibility"), ESearchCase::IgnoreCase))
            {
                continue;
            }

            if (!OutLayerIndexMap.Contains(LayerFName))
            {
                int32 Index = OutLayerIndexMap.Num();
                OutLayerIndexMap.Add(LayerFName, Index);

                TSharedPtr<FJsonObject> LayerJson = MakeShareable(new FJsonObject);
                LayerJson->SetNumberField(TEXT("Index"), Index);
                LayerJson->SetStringField(TEXT("Name"), LayerFName.ToString());

                // ✅ 실제로 export된 텍스처만 포함
                const FExportedLayerTextures* ExportedLayer = ExportedTextures.Find(LayerFName);

                if (ExportedLayer)
                {
                    TSharedPtr<FJsonObject> TexturesJson = MakeShareable(new FJsonObject);

                    // 실제로 export된 텍스처만 JSON에 추가
                    for (const auto& TexturePair : ExportedLayer->TextureFiles)
                    {
                        FString TextureType = TexturePair.Key;
                        FString FileName = TexturePair.Value;

                        // Type의 첫 글자를 대문자로 (albedo -> Albedo)
                        if (TextureType.Len() > 0)
                        {
                            TextureType[0] = FChar::ToUpper(TextureType[0]);
                        }

                        TexturesJson->SetStringField(TextureType, FileName);
                    }

                    LayerJson->SetObjectField(TEXT("Textures"), TexturesJson);
                    LayerJson->SetNumberField(TEXT("Tiling"), ExportedLayer->Tiling);
                }
                else
                {
                    // Export된 텍스처가 없는 경우 빈 객체
                    TSharedPtr<FJsonObject> TexturesJson = MakeShareable(new FJsonObject);
                    LayerJson->SetObjectField(TEXT("Textures"), TexturesJson);
                    LayerJson->SetNumberField(TEXT("Tiling"), 0.01f);

                    UE_LOG(LogTemp, Warning, TEXT("No exported textures found for layer: %s"),
                        *LayerFName.ToString());
                }

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
    FString FullPath = Directory + FileName + TEXT(".png");

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*Directory))
    {
        PlatformFile.CreateDirectoryTree(*Directory);
    }

    int32 ComponentSizeQuads = Component->ComponentSizeQuads;
    int32 ComponentSizeVerts = ComponentSizeQuads + 1;

    UE_LOG(LogTemp, Warning, TEXT("===== Exporting %s ====="), *FileName);
    UE_LOG(LogTemp, Warning, TEXT("WeightmapTextureIndex: %d"), WeightmapTextureIndex);

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

    // 포맷 확인
    ETextureSourceFormat SourceFormat = WeightmapTexture->Source.GetFormat();
    UE_LOG(LogTemp, Warning, TEXT("Source format: %d"), (int32)SourceFormat);

    // 포맷에 따라 BytesPerPixel 결정
    int32 BytesPerPixel = 4;
    bool bIsBGRA = false;

    if (SourceFormat == ETextureSourceFormat::TSF_G8)
    {
        BytesPerPixel = 1;
    }
    else if (SourceFormat == ETextureSourceFormat::TSF_BGRA8)
    {
        BytesPerPixel = 4;
        bIsBGRA = true;
    }
    else if (SourceFormat == ETextureSourceFormat::TSF_BGRE8)
    {
        BytesPerPixel = 4;
        bIsBGRA = true;
    }
    else if (SourceFormat == ETextureSourceFormat::TSF_RGBA16)
    {
        BytesPerPixel = 8;
    }

    // 채널별로 데이터를 분리해서 저장
    TMap<int32, TArray<uint8>> ChannelData;

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

            // 해당 채널의 데이터만 추출
            for (int32 Y = 0; Y < TextureHeight; Y++)
            {
                for (int32 X = 0; X < TextureWidth; X++)
                {
                    int32 PixelIndex = (Y * TextureWidth + X) * BytesPerPixel;
                    uint8 Weight = 0;

                    if (BytesPerPixel == 1)
                    {
                        if (PixelIndex < RawData.Num())
                        {
                            Weight = RawData[PixelIndex];
                        }
                    }
                    else if (BytesPerPixel == 4)
                    {
                        if (PixelIndex + 3 < RawData.Num())
                        {
                            if (bIsBGRA)
                            {
                                int32 BGRAMap[4] = { 2, 1, 0, 3 };
                                Weight = RawData[PixelIndex + BGRAMap[Channel]];
                            }
                            else
                            {
                                Weight = RawData[PixelIndex + Channel];
                            }
                        }
                    }
                    else if (BytesPerPixel == 8)
                    {
                        int32 ChannelOffset = Channel * 2;
                        if (PixelIndex + ChannelOffset + 1 < RawData.Num())
                        {
                            Weight = RawData[PixelIndex + ChannelOffset + 1];
                        }
                    }

                    Weights.Add(Weight);
                }
            }

            ChannelData.Add(Channel, Weights);
        }
    }

    // ColorData 생성
    TArray<FColor> ColorData;
    ColorData.Reserve(ComponentSizeVerts * ComponentSizeVerts);

    for (int32 Y = 0; Y < ComponentSizeVerts && Y < TextureHeight; Y++)
    {
        for (int32 X = 0; X < ComponentSizeVerts && X < TextureWidth; X++)
        {
            int32 Index = Y * TextureWidth + X;
            FColor Color(0, 0, 0, 0);

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

            if (ColorData.Num() < 5)
            {
                UE_LOG(LogTemp, Warning, TEXT("  Pixel[%d]: RGBA=(%d,%d,%d,%d) Sum=%d"),
                    ColorData.Num(), Color.R, Color.G, Color.B, Color.A,
                    Color.R + Color.G + Color.B + Color.A);
            }

            ColorData.Add(Color);
        }
    }

    // ✅ IImageWrapper를 사용한 무손실 PNG 저장
    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

    if (!ImageWrapper.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create image wrapper"));
        return false;
    }

    // Raw RGBA 데이터 준비
    TArray<uint8> RawRGBA;
    RawRGBA.Reserve(ComponentSizeVerts * ComponentSizeVerts * 4);

    for (const FColor& Color : ColorData)
    {
        RawRGBA.Add(Color.R);
        RawRGBA.Add(Color.G);
        RawRGBA.Add(Color.B);
        RawRGBA.Add(Color.A);
    }

    if (ImageWrapper->SetRaw(RawRGBA.GetData(), RawRGBA.Num(),
        ComponentSizeVerts, ComponentSizeVerts, ERGBFormat::RGBA, 8))
    {
        const TArray64<uint8>& CompressedData = ImageWrapper->GetCompressed(100);

        bool bSuccess = FFileHelper::SaveArrayToFile(CompressedData, *FullPath);

        if (bSuccess)
        {
            UE_LOG(LogTemp, Log, TEXT("✅ Lossless weight map exported: %s (%dx%d)"),
                *FullPath, ComponentSizeVerts, ComponentSizeVerts);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to save weight map: %s"), *FullPath);
        }

        return bSuccess;
    }

    return false;
}

TMap<FName, FExportedLayerTextures> UJsonSaveManager::ExportLayerTextures(
    ALandscape* Landscape,
    const TMap<FName, int32>& LayerIndexMap)
{
    TMap<FName, FExportedLayerTextures> ExportedLayersMap;

    if (!Landscape)
    {
        UE_LOG(LogTemp, Error, TEXT("Landscape is null"));
        return ExportedLayersMap;
    }

    UMaterialInterface* LandscapeMaterial = Landscape->GetLandscapeMaterial();
    if (!LandscapeMaterial)
    {
        UE_LOG(LogTemp, Warning, TEXT("No landscape material found"));
        return ExportedLayersMap;
    }

    UMaterial* BaseMaterial = LandscapeMaterial->GetMaterial();
    if (!BaseMaterial)
    {
        UE_LOG(LogTemp, Warning, TEXT("No base material found"));
        return ExportedLayersMap;
    }

    FString TextureDirectory = FPaths::ProjectSavedDir() + TEXT("../ExportedLayerTextures/");

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*TextureDirectory))
    {
        PlatformFile.CreateDirectoryTree(*TextureDirectory);
    }

    // Material의 모든 Expression 순회하여 LandscapeLayerBlend 노드 찾기
    TArray<UMaterialExpressionLandscapeLayerBlend*> LayerBlendNodes;

    for (UMaterialExpression* Expression : BaseMaterial->GetExpressions())
    {
        if (UMaterialExpressionLandscapeLayerBlend* LayerBlend =
            Cast<UMaterialExpressionLandscapeLayerBlend>(Expression))
        {
            LayerBlendNodes.Add(LayerBlend);
        }
    }

    if (LayerBlendNodes.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No LandscapeLayerBlend nodes found in material"));
        return ExportedLayersMap;
    }

    UE_LOG(LogTemp, Log, TEXT("Found %d LandscapeLayerBlend node(s)"), LayerBlendNodes.Num());

    // 각 LandscapeLayerBlend 노드 처리
    int32 BlendNodeIndex = 0;
    for (UMaterialExpressionLandscapeLayerBlend* LayerBlend : LayerBlendNodes)
    {
        UE_LOG(LogTemp, Log, TEXT("=== Processing LayerBlend Node %d ==="), BlendNodeIndex);

        // 각 레이어별로 연결된 텍스처 찾기
        for (int32 LayerIdx = 0; LayerIdx < LayerBlend->Layers.Num(); LayerIdx++)
        {
            const FLayerBlendInput& Layer = LayerBlend->Layers[LayerIdx];
            FName LayerFName = Layer.LayerName;
            FString LayerNameStr = LayerFName.ToString().ToLower();

            // Visibility 레이어 제외
            if (LayerNameStr.Equals(TEXT("visibility"), ESearchCase::IgnoreCase))
            {
                continue;
            }

            UE_LOG(LogTemp, Log, TEXT("  Layer[%d]: %s"), LayerIdx, *LayerNameStr);

            if (!Layer.LayerInput.Expression)
            {
                UE_LOG(LogTemp, Warning, TEXT("    No input expression"));
                continue;
            }

            // ✅ 레이어 정보 초기화
            FExportedLayerTextures& LayerTextures = ExportedLayersMap.FindOrAdd(LayerFName);
            LayerTextures.LayerName = LayerFName.ToString();
            LayerTextures.Tiling = 0.01f; // 기본값

            // Tiling 값 추출
            if (LandscapeMaterial)
            {
                TArray<FName> PossibleTilingNames = {
                    FName(*(LayerNameStr + TEXT("_Tiling"))),
                    FName(*(LayerNameStr + TEXT("_Scale"))),
                    FName(*LayerNameStr)
                };

                for (const FName& ParamName : PossibleTilingNames)
                {
                    float ExtractedValue = 0.0f;
                    if (LandscapeMaterial->GetScalarParameterValue(ParamName, ExtractedValue))
                    {
                        if (ExtractedValue > 0.0f)
                        {
                            LayerTextures.Tiling = ExtractedValue;
                            break;
                        }
                    }
                }
            }

            // 이 레이어에 직접 연결된 텍스처들 수집
            TArray<UTexture2D*> LayerTexturesList;
            TSet<UMaterialExpression*> VisitedExpressions;
            CollectTexturesFromExpression(Layer.LayerInput.Expression, LayerTexturesList, VisitedExpressions);

            UE_LOG(LogTemp, Log, TEXT("    Found %d texture(s)"), LayerTexturesList.Num());

            // ✅ 텍스처들을 연결 정보로 분류하여 export
            for (UTexture2D* Texture : LayerTexturesList)
            {
                if (!Texture) continue;

                FString TextureName = Texture->GetName();
                UE_LOG(LogTemp, Log, TEXT("      Texture: %s"), *TextureName);

                // 텍스처가 속한 TextureSample 노드 찾기
                UMaterialExpressionTextureSample* TextureSampleNode = nullptr;
                for (UMaterialExpression* Expression : BaseMaterial->GetExpressions())
                {
                    if (UMaterialExpressionTextureSample* TS = Cast<UMaterialExpressionTextureSample>(Expression))
                    {
                        if (TS->Texture == Texture)
                        {
                            TextureSampleNode = TS;
                            break;
                        }
                    }
                }

                // 연결 정보 기반으로 타입 결정
                FString TextureType = DetermineTextureTypeFromConnection(
                    TextureSampleNode, LayerBlend, LayerIdx);

                FString FileName = LayerNameStr + TEXT("_") + TextureType + TEXT(".png");
                FString OutputPath = TextureDirectory + FileName;

                // ✅ Export 성공 시에만 맵에 추가
                if (ExportTextureToPNG(Texture, OutputPath))
                {
                    LayerTextures.TextureFiles.Add(TextureType, FileName);
                    UE_LOG(LogTemp, Log, TEXT("        ✅ Exported as %s: %s"), *TextureType, *FileName);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("        ❌ Failed to export: %s"), *FileName);
                }
            }
        }

        BlendNodeIndex++;
    }

    return ExportedLayersMap;
}

bool UJsonSaveManager::ExportTextureToPNG(UTexture2D* Texture, const FString& FilePath)
{
    if (!Texture)
    {
        UE_LOG(LogTemp, Warning, TEXT("Texture is null"));
        return false;
    }

    if (!Texture->Source.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("Texture source is not valid: %s"), *Texture->GetName());
        return false;
    }

    // Source 데이터 가져오기
    TArray64<uint8> RawData;
    if (!Texture->Source.GetMipData(RawData, 0))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get mip data from texture: %s"), *Texture->GetName());
        return false;
    }

    int32 Width = Texture->Source.GetSizeX();
    int32 Height = Texture->Source.GetSizeY();
    ETextureSourceFormat SourceFormat = Texture->Source.GetFormat();

    UE_LOG(LogTemp, Log, TEXT("Exporting texture: %s (%dx%d, Format=%d)"),
        *Texture->GetName(), Width, Height, (int32)SourceFormat);

    // FColor 배열로 변환
    TArray<FColor> ColorData;
    ColorData.Reserve(Width * Height);

    int32 BytesPerPixel = 4;
    bool bIsBGRA = false;

    // 포맷에 따른 처리
    switch (SourceFormat)
    {
    case TSF_G8:
        BytesPerPixel = 1;
        for (int32 i = 0; i < Width * Height; i++)
        {
            uint8 Gray = RawData[i];
            ColorData.Add(FColor(Gray, Gray, Gray, 255));
        }
        break;

    case TSF_BGRA8:
    case TSF_BGRE8:
        bIsBGRA = true;
        BytesPerPixel = 4;
        for (int32 i = 0; i < Width * Height; i++)
        {
            int32 Index = i * 4;
            uint8 B = RawData[Index + 0];
            uint8 G = RawData[Index + 1];
            uint8 R = RawData[Index + 2];
            uint8 A = RawData[Index + 3];
            ColorData.Add(FColor(R, G, B, A));
        }
        break;

    case TSF_RGBA16:
        BytesPerPixel = 8;
        for (int32 i = 0; i < Width * Height; i++)
        {
            int32 Index = i * 8;
            // 16비트를 8비트로 변환 (상위 바이트 사용)
            uint8 R = RawData[Index + 1];
            uint8 G = RawData[Index + 3];
            uint8 B = RawData[Index + 5];
            uint8 A = RawData[Index + 7];
            ColorData.Add(FColor(R, G, B, A));
        }
        break;

    default:
        // 기본적으로 RGBA로 처리
        for (int32 i = 0; i < Width * Height; i++)
        {
            int32 Index = i * 4;
            if (Index + 3 < RawData.Num())
            {
                uint8 R = RawData[Index + 0];
                uint8 G = RawData[Index + 1];
                uint8 B = RawData[Index + 2];
                uint8 A = RawData[Index + 3];
                ColorData.Add(FColor(R, G, B, A));
            }
        }
        break;
    }

    // ✅ IImageWrapper를 사용한 무손실 PNG 저장
    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

    if (!ImageWrapper.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create image wrapper"));
        return false;
    }

    // Raw RGBA 데이터로 설정 (8비트)
    TArray<uint8> RawRGBA;
    RawRGBA.Reserve(Width * Height * 4);

    for (const FColor& Color : ColorData)
    {
        RawRGBA.Add(Color.R);
        RawRGBA.Add(Color.G);
        RawRGBA.Add(Color.B);
        RawRGBA.Add(Color.A);
    }

    // PNG 설정
    if (ImageWrapper->SetRaw(RawRGBA.GetData(), RawRGBA.Num(), Width, Height, ERGBFormat::RGBA, 8))
    {
        // PNG 압축 (100 = 최고 품질, 무손실)
        const TArray64<uint8>& CompressedData = ImageWrapper->GetCompressed(100);

        // 파일 저장
        bool bSuccess = FFileHelper::SaveArrayToFile(CompressedData, *FilePath);

        if (bSuccess)
        {
            UE_LOG(LogTemp, Log, TEXT("✅ Lossless PNG exported: %s (%dx%d, %lld bytes)"),
                *FilePath, Width, Height, CompressedData.Num());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to save PNG file: %s"), *FilePath);
        }

        return bSuccess;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to set raw data for PNG encoding"));
        return false;
    }
}

void UJsonSaveManager::CollectTexturesFromExpression(
    UMaterialExpression* Expression,
    TArray<UTexture2D*>& OutTextures,
    TSet<UMaterialExpression*>& VisitedExpressions)
{
    if (!Expression || VisitedExpressions.Contains(Expression))
        return;

    VisitedExpressions.Add(Expression);

    // TextureSample 노드인 경우
    if (UMaterialExpressionTextureSample* TextureSample =
        Cast<UMaterialExpressionTextureSample>(Expression))
    {
        if (UTexture2D* Texture = Cast<UTexture2D>(TextureSample->Texture))
        {
            OutTextures.AddUnique(Texture);
        }
        return;
    }

    // 입력 순회 (안전한 방법)
    int32 InputIndex = 0;
    while (true)
    {
        FExpressionInput* Input = Expression->GetInput(InputIndex);
        if (!Input) break;

        if (Input->Expression)
        {
            CollectTexturesFromExpression(Input->Expression, OutTextures, VisitedExpressions);
        }

        InputIndex++;
        if (InputIndex > 100) break; // 안전장치
    }
}

FString UJsonSaveManager::DetermineTextureTypeFromConnection(
    UMaterialExpression* TextureExpression,
    UMaterialExpressionLandscapeLayerBlend* LayerBlendNode,
    int32 LayerIndex)
{
    if (!TextureExpression || !LayerBlendNode)
        return TEXT("unknown");

    // TextureSample 노드인지 확인
    UMaterialExpressionTextureSample* TextureSample =
        Cast<UMaterialExpressionTextureSample>(TextureExpression);

    if (!TextureSample || !TextureSample->Texture)
        return TEXT("unknown");

    UTexture2D* Texture = Cast<UTexture2D>(TextureSample->Texture);
    if (!Texture)
        return TEXT("unknown");

    // ✅ 1단계: LayerBlend의 출력을 추적하여 Material의 어느 입력에 연결되었는지 확인
    UMaterial* Material = LayerBlendNode->Material;
    if (Material)
    {
        // BaseColor에 연결되었는지 확인
        if (IsConnectedToMaterialInput(LayerBlendNode, Material, MP_BaseColor))
        {
            return TEXT("albedo");
        }
        // Normal에 연결되었는지 확인
        if (IsConnectedToMaterialInput(LayerBlendNode, Material, MP_Normal))
        {
            return TEXT("normal");
        }
        // Roughness에 연결되었는지 확인
        if (IsConnectedToMaterialInput(LayerBlendNode, Material, MP_Roughness))
        {
            return TEXT("roughness");
        }
        // Metallic에 연결되었는지 확인
        if (IsConnectedToMaterialInput(LayerBlendNode, Material, MP_Metallic))
        {
            return TEXT("metallic");
        }
        // Specular에 연결되었는지 확인
        if (IsConnectedToMaterialInput(LayerBlendNode, Material, MP_Specular))
        {
            return TEXT("specular");
        }
    }

    // ✅ 2단계: 텍스처 속성으로 추론
    TextureCompressionSettings CompressionSettings = Texture->CompressionSettings;

    if (CompressionSettings == TC_Normalmap)
    {
        return TEXT("normal");
    }

    // sRGB이면 일반적으로 Color Map
    if (Texture->SRGB)
    {
        return TEXT("albedo");
    }

    // Linear이고 Grayscale이면 Data Map
    if (CompressionSettings == TC_Grayscale || CompressionSettings == TC_Alpha)
    {
        return TEXT("roughness");
    }

    // ✅ 3단계: 텍스처 이름으로 추론 (마지막 수단)
    FString TextureName = Texture->GetName().ToLower();

    // Normal 확인
    if (TextureName.Contains(TEXT("normal")) || TextureName.Contains(TEXT("_n_")))
    {
        return TEXT("normal");
    }

    // Roughness 확인 (rock과 혼동 방지)
    if (TextureName.Contains(TEXT("roughness")) || TextureName.Contains(TEXT("_r_")) ||
        (TextureName.Contains(TEXT("rough")) && !TextureName.Contains(TEXT("rock"))))
    {
        return TEXT("roughness");
    }

    // Metallic 확인
    if (TextureName.Contains(TEXT("metallic")) || TextureName.Contains(TEXT("_m_")))
    {
        return TEXT("metallic");
    }

    // Albedo/BaseColor 확인
    if (TextureName.Contains(TEXT("albedo")) || TextureName.Contains(TEXT("basecolor")) ||
        TextureName.Contains(TEXT("diffuse")) || TextureName.Contains(TEXT("_d_")) ||
        TextureName.Contains(TEXT("color")))
    {
        return TEXT("albedo");
    }

    // 기본값: sRGB이면 albedo, 아니면 data
    return Texture->SRGB ? TEXT("albedo") : TEXT("data");
}


// ✅ LayerBlend 노드가 Material의 특정 입력에 연결되었는지 확인하는 헬퍼 함수
bool UJsonSaveManager::IsConnectedToMaterialInput(
    UMaterialExpression* Expression,
    UMaterial* Material,
    EMaterialProperty PropertyType)
{
    if (!Expression || !Material)
        return false;

    // Material의 해당 속성 입력 가져오기
    FExpressionInput* MaterialInput = Material->GetExpressionInputForProperty(PropertyType);
    if (!MaterialInput || !MaterialInput->Expression)
        return false;

    // 직접 연결 확인
    if (MaterialInput->Expression == Expression)
        return true;

    // 간접 연결 확인 (재귀적으로 추적)
    TSet<UMaterialExpression*> VisitedNodes;
    return IsExpressionConnectedToInput(Expression, MaterialInput->Expression, VisitedNodes);
}

// ✅ 재귀적으로 연결 추적
bool UJsonSaveManager::IsExpressionConnectedToInput(
    UMaterialExpression* TargetExpression,
    UMaterialExpression* CurrentExpression,
    TSet<UMaterialExpression*>& VisitedNodes)
{
    if (!CurrentExpression || VisitedNodes.Contains(CurrentExpression))
        return false;

    VisitedNodes.Add(CurrentExpression);

    if (CurrentExpression == TargetExpression)
        return true;

    // 현재 노드의 모든 입력을 확인
    int32 InputIndex = 0;
    while (true)
    {
        FExpressionInput* Input = CurrentExpression->GetInput(InputIndex);
        if (!Input) break;

        if (Input->Expression)
        {
            if (IsExpressionConnectedToInput(TargetExpression, Input->Expression, VisitedNodes))
                return true;
        }

        InputIndex++;
        if (InputIndex > 100) break;
    }

    return false;
}
#endif