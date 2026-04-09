// Copyright 2018-2021 - John snow wind
#pragma once
#include "CoreMinimal.h"

struct FPsdLayer;
class UTexture2D;

class PSD2UMG_API FTextureImporter
{
public:
    // Import a single layer's RGBA pixels as a persistent UTexture2D.
    // PackageBasePath = e.g. "/Game/UI/Textures/MyHUD"
    // Returns nullptr on skip (empty pixels, zero bounds).
    static UTexture2D* ImportLayer(const FPsdLayer& Layer, const FString& PackageBasePath);

    // Build the base path from settings: {TextureAssetDir}/Textures/{PsdName}
    static FString BuildTexturePath(const FString& PsdName);

    // Deduplicate asset names: append _01, _02 if name already used
    static FString DeduplicateName(const FString& BaseName, const TSet<FString>& UsedNames);
};
