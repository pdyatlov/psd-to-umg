// Copyright 2018-2021 - John snow wind

#include "Generator/FTextureImporter.h"
#include "PSD2UMGLog.h"
#include "PSD2UMGSetting.h"
#include "Parser/PsdTypes.h"

#include "Engine/Texture2D.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"
#include "EditorAssetLibrary.h"

UTexture2D* FTextureImporter::ImportLayer(const FPsdLayer& Layer, const FString& PackageBasePath)
{
    if (Layer.RGBAPixels.Num() == 0 || Layer.PixelWidth <= 0 || Layer.PixelHeight <= 0)
    {
        UE_LOG(LogPSD2UMG, Warning, TEXT("FTextureImporter: Skipping layer '%s' — empty pixel data or zero bounds"), *Layer.Name);
        return nullptr;
    }

    // Copy RGBA pixels and swap R and B channels to produce BGRA
    TArray<uint8> BGRAPixels = Layer.RGBAPixels;
    for (int32 i = 0; i < BGRAPixels.Num(); i += 4)
    {
        Swap(BGRAPixels[i], BGRAPixels[i + 2]);
    }

    // Sanitize asset name (UE does not allow spaces or special chars in asset names)
    FString AssetName = Layer.Name;
    for (TCHAR& Ch : AssetName)
    {
        if (!FChar::IsAlnum(Ch) && Ch != TEXT('_'))
        {
            Ch = TEXT('_');
        }
    }

    const FString PackagePath = PackageBasePath / AssetName;

    UPackage* Pkg = CreatePackage(*PackagePath);
    if (!Pkg)
    {
        UE_LOG(LogPSD2UMG, Warning, TEXT("FTextureImporter: Failed to create package for layer '%s'"), *Layer.Name);
        return nullptr;
    }
    Pkg->FullyLoad();

    UTexture2D* Tex = NewObject<UTexture2D>(Pkg, FName(*AssetName), RF_Public | RF_Standalone);
    if (!Tex)
    {
        UE_LOG(LogPSD2UMG, Warning, TEXT("FTextureImporter: Failed to create UTexture2D for layer '%s'"), *Layer.Name);
        return nullptr;
    }

    Tex->PreEditChange(nullptr);
    Tex->Source.Init(Layer.PixelWidth, Layer.PixelHeight, 1, 1, TSF_BGRA8, BGRAPixels.GetData());
    Tex->SRGB = true;
    Tex->CompressionSettings = TC_Default;
    Tex->PostEditChange();

    Pkg->MarkPackageDirty();
    UEditorAssetLibrary::SaveLoadedAsset(Tex, false);

    return Tex;
}

FString FTextureImporter::BuildTexturePath(const FString& PsdName)
{
    FString Dir;
    if (const UPSD2UMGSettings* Settings = UPSD2UMGSettings::Get())
    {
        Dir = Settings->TextureAssetDir.Path;
    }
    if (Dir.IsEmpty())
    {
        Dir = TEXT("/Game/UI");
    }
    return Dir / TEXT("Textures") / PsdName;
}

FString FTextureImporter::DeduplicateName(const FString& BaseName, const TSet<FString>& UsedNames)
{
    if (!UsedNames.Contains(BaseName))
    {
        return BaseName;
    }
    for (int32 Index = 1; Index <= 99; ++Index)
    {
        const FString Candidate = FString::Printf(TEXT("%s_%02d"), *BaseName, Index);
        if (!UsedNames.Contains(Candidate))
        {
            return Candidate;
        }
    }
    // Fallback: append timestamp to guarantee uniqueness
    return FString::Printf(TEXT("%s_%d"), *BaseName, FPlatformTime::Cycles());
}
