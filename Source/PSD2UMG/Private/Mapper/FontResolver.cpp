// Copyright 2018-2021 - John snow wind

#include "Mapper/FontResolver.h"
#include "PSD2UMGSetting.h"
#include "PSD2UMGLog.h"
#include "Engine/Font.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetData.h"
#include "UObject/WeakObjectPtr.h"

namespace PSD2UMG
{
    // Suffix table ordered longest-first so greedy match works.
    struct FSuffixEntry
    {
        const TCHAR* Suffix;
        bool bBold;
        bool bItalic;
    };

    static const FSuffixEntry SuffixTable[] =
    {
        { TEXT("-BoldItalicMT"), true,  true  },
        { TEXT("-BoldItalic"),   true,  true  },
        { TEXT("-BoldMT"),      true,  false },
        { TEXT("-Bold"),        true,  false },
        { TEXT("-ItalicMT"),    false, true  },
        { TEXT("-Italic"),      false, true  },
        { TEXT("-Oblique"),     false, true  },
        { TEXT("-It"),          false, true  },
    };

    // Auto-discovery cache (D-04). Lazy-populated on first Resolve call that
    // reaches the AutoDiscovered step; cleared via InvalidateDiscoveryCache()
    // at end of every import (called from PsdImportFactory::FactoryCreateBinary).
    //
    // Key: lowercase asset name (matches FAssetData::AssetName after ToLower).
    // Value: TWeakObjectPtr<UFont> so GC doesn't pin fonts between imports.
    static TMap<FString, TWeakObjectPtr<UFont>> GDiscoveryCache;
    static bool GDiscoveryCachePopulated = false;

    static void PopulateDiscoveryCache()
    {
        GDiscoveryCache.Reset();

        FAssetRegistryModule& AssetRegistryModule =
            FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
        IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

        FARFilter Filter;
        Filter.ClassPaths.Add(UFont::StaticClass()->GetClassPathName());
        Filter.PackagePaths.Add(FName(TEXT("/Game")));
        Filter.PackagePaths.Add(FName(TEXT("/Engine/EngineFonts")));
        Filter.bRecursivePaths = true;

        TArray<FAssetData> Assets;
        AssetRegistry.GetAssets(Filter, Assets);

        GDiscoveryCache.Reserve(Assets.Num());
        for (const FAssetData& Data : Assets)
        {
            const FString Key = Data.AssetName.ToString().ToLower();
            // Store a soft reference as TWeakObjectPtr — the asset may not be
            // in memory yet; Resolve will GetAsset() -> loads synchronously.
            if (UObject* Loaded = Data.GetAsset())
            {
                if (UFont* AsFont = Cast<UFont>(Loaded))
                {
                    GDiscoveryCache.Add(Key, AsFont);
                }
            }
        }

        GDiscoveryCachePopulated = true;

        UE_LOG(LogPSD2UMG, Verbose,
            TEXT("FFontResolver: discovery cache populated with %d UFont assets from /Game and /Engine/EngineFonts"),
            GDiscoveryCache.Num());
    }

    void FFontResolver::InvalidateDiscoveryCache()
    {
        GDiscoveryCache.Reset();
        GDiscoveryCachePopulated = false;
    }

    void FFontResolver::ParseSuffix(
        const FString& PsPostscriptName,
        FString& OutBaseName,
        bool& bOutBold,
        bool& bOutItalic)
    {
        bOutBold = false;
        bOutItalic = false;

        for (const FSuffixEntry& Entry : SuffixTable)
        {
            if (PsPostscriptName.EndsWith(Entry.Suffix, ESearchCase::IgnoreCase))
            {
                const int32 SuffixLen = FCString::Strlen(Entry.Suffix);
                OutBaseName = PsPostscriptName.Left(PsPostscriptName.Len() - SuffixLen);
                bOutBold = Entry.bBold;
                bOutItalic = Entry.bItalic;
                return;
            }
        }

        OutBaseName = PsPostscriptName;
    }

    FName FFontResolver::MakeTypefaceName(bool bBold, bool bItalic)
    {
        if (bBold && bItalic) return FName(TEXT("Bold Italic"));
        if (bBold)            return FName(TEXT("Bold"));
        if (bItalic)          return FName(TEXT("Italic"));
        return NAME_None;
    }

    FFontResolution FFontResolver::Resolve(
        const FString& PsPostscriptName,
        bool bParserBold,
        bool bParserItalic,
        const UPSD2UMGSettings* Settings)
    {
        // Parse suffix for style flags regardless of lookup outcome.
        FString BaseName;
        bool bSuffixBold = false;
        bool bSuffixItalic = false;
        ParseSuffix(PsPostscriptName, BaseName, bSuffixBold, bSuffixItalic);

        FFontResolution Result;
        Result.bBoldRequested = bParserBold || bSuffixBold;
        Result.bItalicRequested = bParserItalic || bSuffixItalic;
        Result.TypefaceName = MakeTypefaceName(Result.bBoldRequested, Result.bItalicRequested);

        if (Settings == nullptr || PsPostscriptName.IsEmpty())
        {
            Result.Source = EFontResolutionSource::EngineDefault;
            if (!PsPostscriptName.IsEmpty())
            {
                UE_LOG(LogPSD2UMG, Warning,
                    TEXT("Font '%s' cannot be resolved — no settings available; using engine default"),
                    *PsPostscriptName);
            }
            return Result;
        }

        // 1. Exact match.
        if (const TSoftObjectPtr<UFont>* Found = Settings->FontMap.Find(PsPostscriptName))
        {
            if (UFont* Loaded = Found->LoadSynchronous())
            {
                Result.Font = Loaded;
                Result.Source = EFontResolutionSource::Exact;
                return Result;
            }
        }

        // 2. Case-insensitive match.
        for (const auto& Pair : Settings->FontMap)
        {
            if (Pair.Key.Equals(PsPostscriptName, ESearchCase::IgnoreCase))
            {
                if (UFont* Loaded = Pair.Value.LoadSynchronous())
                {
                    Result.Font = Loaded;
                    Result.Source = EFontResolutionSource::CaseInsensitive;
                    return Result;
                }
            }
        }

        // 3. Auto-discovery via AssetRegistry scan of /Game + /Engine/EngineFonts (D-01, D-04, D-05).
        //    Key lookup is lowercase base name (after ParseSuffix strip) per D-02.
        {
            if (!GDiscoveryCachePopulated)
            {
                PopulateDiscoveryCache();
            }

            const FString Key = BaseName.ToLower();
            if (!Key.IsEmpty())
            {
                if (const TWeakObjectPtr<UFont>* HitWeak = GDiscoveryCache.Find(Key))
                {
                    if (UFont* HitFont = HitWeak->Get())
                    {
                        Result.Font = HitFont;
                        Result.Source = EFontResolutionSource::AutoDiscovered;
                        return Result;
                    }
                }
            }
        }

        // 4. DefaultFont fallback.
        if (Settings->DefaultFont.ToSoftObjectPath().IsValid())
        {
            if (UFont* Loaded = Settings->DefaultFont.LoadSynchronous())
            {
                Result.Font = Loaded;
                Result.Source = EFontResolutionSource::Default;
                UE_LOG(LogPSD2UMG, Warning,
                    TEXT("Font '%s' not found in FontMap; using DefaultFont"),
                    *PsPostscriptName);
                return Result;
            }
        }

        // 5. Engine default.
        Result.Source = EFontResolutionSource::EngineDefault;
        UE_LOG(LogPSD2UMG, Warning,
            TEXT("Font '%s' not found in FontMap and no DefaultFont configured; using engine default"),
            *PsPostscriptName);
        return Result;
    }
}
