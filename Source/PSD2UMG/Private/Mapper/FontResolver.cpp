// Copyright 2018-2021 - John snow wind

#include "Mapper/FontResolver.h"
#include "PSD2UMGSetting.h"
#include "PSD2UMGLog.h"
#include "Engine/Font.h"

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

        // 3. DefaultFont fallback.
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

        // 4. Engine default.
        Result.Source = EFontResolutionSource::EngineDefault;
        UE_LOG(LogPSD2UMG, Warning,
            TEXT("Font '%s' not found in FontMap and no DefaultFont configured; using engine default"),
            *PsPostscriptName);
        return Result;
    }
}
