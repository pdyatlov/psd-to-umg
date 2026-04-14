// Copyright 2018-2021 - John snow wind
#pragma once
#include "CoreMinimal.h"
#include "Misc/Optional.h"

struct FPsdLayer;
class UWidgetBlueprint;

/**
 * Helper responsible for recursively importing a Smart Object layer as a child
 * Widget Blueprint. When import fails (linked file missing, depth limit reached,
 * or any other error) the caller falls back to treating the layer as a rasterized
 * UImage using the composited preview pixels already stored in FPsdLayer.RGBAPixels.
 *
 * Depth tracking uses thread_local state so FWidgetBlueprintGenerator::Generate()
 * and IPsdLayerMapper::Map() signatures do not need to change.
 */
class FSmartObjectImporter
{
public:
    /** Attempt recursive import of a Smart Object as a child WBP.
     *  Returns the created WBP, or nullptr on failure (caller should fallback to image).
     *  CurrentDepth is ignored in favor of internal thread_local depth tracking. */
    static UWidgetBlueprint* ImportAsChildWBP(
        const FPsdLayer& SmartObjLayer,
        const FString& ParentWbpPackagePath,
        int32 CurrentDepth,
        const TOptional<FString>& ExplicitTypeName = {});

    /** Called by FWidgetBlueprintGenerator::Generate() before mapping layers so
     *  mappers can obtain the current WBP package path without interface changes. */
    static void SetCurrentPackagePath(const FString& PackagePath);

    /** Returns the package path set via SetCurrentPackagePath(). */
    static const FString& GetCurrentPackagePath();
};
