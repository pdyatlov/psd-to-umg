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

    // Deduplicate: two layers with identical sanitized names (e.g. "background @anchor:fill"
    // in different groups) must not share a package — CreatePackage returns the same in-memory
    // package on the second call and NewObject reinitializes the existing UTexture2D, leaving
    // the first UImage's brush stale (white box).  We must check inside the loaded package
    // with FindObject, not StaticFindObject with a package-path string (wrong UE path format).
    FString UniqueAssetName = AssetName;
    FString PackagePath = PackageBasePath / UniqueAssetName;

    UPackage* Pkg = CreatePackage(*PackagePath);
    if (!Pkg)
    {
        UE_LOG(LogPSD2UMG, Warning, TEXT("FTextureImporter: Failed to create package for layer '%s'"), *Layer.Name);
        return nullptr;
    }
    Pkg->FullyLoad();

    if (FindObject<UTexture2D>(Pkg, *UniqueAssetName) != nullptr)
    {
        for (int32 Suffix = 1; ; ++Suffix)
        {
            UniqueAssetName = FString::Printf(TEXT("%s_%d"), *AssetName, Suffix);
            PackagePath = PackageBasePath / UniqueAssetName;
            Pkg = CreatePackage(*PackagePath);
            if (Pkg)
            {
                Pkg->FullyLoad();
                if (FindObject<UTexture2D>(Pkg, *UniqueAssetName) == nullptr)
                    break;
            }
        }
        UE_LOG(LogPSD2UMG, Log, TEXT("FTextureImporter: Asset name '%s' already taken — using '%s'"), *AssetName, *UniqueAssetName);
    }

    UTexture2D* Tex = NewObject<UTexture2D>(Pkg, FName(*UniqueAssetName), RF_Public | RF_Standalone);
    if (!Tex)
    {
        UE_LOG(LogPSD2UMG, Warning, TEXT("FTextureImporter: Failed to create UTexture2D for layer '%s'"), *Layer.Name);
        return nullptr;
    }

    Tex->PreEditChange(nullptr);
    Tex->Source.Init(Layer.PixelWidth, Layer.PixelHeight, 1, 1, TSF_BGRA8, BGRAPixels.GetData());
    Tex->SRGB = true;
    // Phase 13 / GRAD-02: gradient layers use TC_BC7 (high-quality block
    // compression) to eliminate BC1/DXT1 banding at UI resolutions. All
    // other layer types (Image, SmartObject) retain TC_Default (BC1/BC3).
    Tex->CompressionSettings = (Layer.Type == EPsdLayerType::Gradient)
        ? TC_BC7
        : TC_Default;
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
