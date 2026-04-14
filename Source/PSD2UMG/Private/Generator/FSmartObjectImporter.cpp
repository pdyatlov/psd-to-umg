// Copyright 2018-2021 - John snow wind

#include "Generator/FSmartObjectImporter.h"
#include "Generator/FWidgetBlueprintGenerator.h"
#include "Parser/PsdParser.h"
#include "Parser/PsdTypes.h"
#include "Parser/PsdDiagnostics.h"
#include "PSD2UMGSetting.h"
#include "PSD2UMGLog.h"

#include "Misc/Paths.h"
#include "WidgetBlueprint.h"

// ---------------------------------------------------------------------------
// Thread-local state — allows depth and package path tracking without
// changing any public interfaces (IPsdLayerMapper, FWidgetBlueprintGenerator).
// ---------------------------------------------------------------------------
static thread_local int32  GSmartObjectDepth       = 0;
static thread_local FString GCurrentPackagePath    = TEXT("");

// ---------------------------------------------------------------------------
// FSmartObjectImporter
// ---------------------------------------------------------------------------

void FSmartObjectImporter::SetCurrentPackagePath(const FString& PackagePath)
{
    GCurrentPackagePath = PackagePath;
}

const FString& FSmartObjectImporter::GetCurrentPackagePath()
{
    return GCurrentPackagePath;
}

UWidgetBlueprint* FSmartObjectImporter::ImportAsChildWBP(
    const FPsdLayer& SmartObjLayer,
    const FString& ParentWbpPackagePath,
    int32 /*CurrentDepth*/,
    const TOptional<FString>& ExplicitTypeName)
{
    const UPSD2UMGSettings* Settings = UPSD2UMGSettings::Get();
    if (!Settings)
    {
        UE_LOG(LogPSD2UMG, Warning, TEXT("FSmartObjectImporter: Settings unavailable; falling back to raster for '%s'"), *SmartObjLayer.Name);
        return nullptr;
    }

    // --- Depth guard ---
    if (GSmartObjectDepth >= Settings->MaxSmartObjectDepth)
    {
        UE_LOG(LogPSD2UMG, Warning,
            TEXT("FSmartObjectImporter: MaxSmartObjectDepth (%d) reached for Smart Object '%s'; falling back to rasterized image."),
            Settings->MaxSmartObjectDepth, *SmartObjLayer.Name);
        return nullptr;
    }

    // --- Linked file resolution ---
    if (SmartObjLayer.SmartObjectFilePath.IsEmpty())
    {
        UE_LOG(LogPSD2UMG, Warning,
            TEXT("FSmartObjectImporter: Smart Object '%s' is embedded (no linked filepath); falling back to rasterized image."),
            *SmartObjLayer.Name);
        return nullptr;
    }

    // Resolve the linked path relative to the parent PSD directory
    // GCurrentPackagePath is a /Game/… path; the physical PSD source is stored
    // in the FPsdDocument but mappers don't have it. We resolve against the
    // SmartObjectFilePath directly (PhotoshopAPI typically stores absolute or
    // project-relative paths on Windows).
    const FString ResolvedPath = FPaths::ConvertRelativePathToFull(SmartObjLayer.SmartObjectFilePath);

    if (!FPaths::FileExists(ResolvedPath))
    {
        UE_LOG(LogPSD2UMG, Warning,
            TEXT("FSmartObjectImporter: Linked file not found for Smart Object '%s' (path='%s'); falling back to rasterized image."),
            *SmartObjLayer.Name, *ResolvedPath);
        return nullptr;
    }

    // --- Recursive parse + generate ---
    FPsdDocument InnerDoc;
    FPsdParseDiagnostics InnerDiag;

    const bool bParsed = PSD2UMG::Parser::ParseFile(ResolvedPath, InnerDoc, InnerDiag);
    if (!bParsed)
    {
        UE_LOG(LogPSD2UMG, Warning,
            TEXT("FSmartObjectImporter: Failed to parse Smart Object '%s' from '%s'; falling back to rasterized image."),
            *SmartObjLayer.Name, *ResolvedPath);
        return nullptr;
    }

    // Child WBP goes into the same asset directory as the parent, named after either
    // the explicit @smartobject:TypeName tag value (D-11) or, falling back, the layer name.
    FString TypeNameSource;
    if (ExplicitTypeName.IsSet() && !ExplicitTypeName.GetValue().IsEmpty())
    {
        TypeNameSource = ExplicitTypeName.GetValue();
        UE_LOG(LogPSD2UMG, Log,
            TEXT("FSmartObjectLayerMapper: Using explicit SmartObject type name '%s' from @smartobject tag (layer='%s')"),
            *TypeNameSource, *SmartObjLayer.Name);
    }
    else
    {
        TypeNameSource = SmartObjLayer.Name;
    }
    const FString ChildAssetName = FString::Printf(TEXT("WBP_SO_%s"), *TypeNameSource);
    // Sanitise name: replace spaces with underscores
    const FString SafeAssetName = ChildAssetName.Replace(TEXT(" "), TEXT("_"));

    // Increment depth and track; use a scope guard via RAII
    struct FDepthGuard
    {
        FDepthGuard()  { ++GSmartObjectDepth; }
        ~FDepthGuard() { --GSmartObjectDepth; }
    } DepthGuard;

    // Save/restore package path for recursive call
    const FString PreviousPackagePath = GCurrentPackagePath;
    GCurrentPackagePath = ParentWbpPackagePath;

    UWidgetBlueprint* ChildWBP = nullptr;
    try
    {
        ChildWBP = FWidgetBlueprintGenerator::Generate(InnerDoc, ParentWbpPackagePath, SafeAssetName);
    }
    catch (...)
    {
        UE_LOG(LogPSD2UMG, Warning,
            TEXT("FSmartObjectImporter: Exception during recursive Generate for '%s'; falling back to rasterized image."),
            *SmartObjLayer.Name);
    }

    GCurrentPackagePath = PreviousPackagePath;

    if (!ChildWBP)
    {
        UE_LOG(LogPSD2UMG, Warning,
            TEXT("FSmartObjectImporter: Generate returned null for Smart Object '%s'; falling back to rasterized image."),
            *SmartObjLayer.Name);
    }

    return ChildWBP;
}
