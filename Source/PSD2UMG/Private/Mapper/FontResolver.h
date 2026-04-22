// Copyright 2018-2021 - John snow wind
#pragma once

#include "CoreMinimal.h"

class UFont;
class UPSD2UMGSettings;
struct FPsdTextRun;

namespace PSD2UMG
{
    enum class EFontResolutionSource : uint8
    {
        Exact,
        CaseInsensitive,
        AutoDiscovered,
        Default,
        EngineDefault,
    };

    struct FFontResolution
    {
        UFont* Font = nullptr;                  // nullptr means "leave engine default"
        FName TypefaceName = NAME_None;         // NAME_None means "leave default typeface"
        bool bBoldRequested = false;
        bool bItalicRequested = false;
        EFontResolutionSource Source = EFontResolutionSource::EngineDefault;
    };

    class FFontResolver
    {
    public:
        /** Resolve a Photoshop PostScript name to a UE font + typeface. */
        static FFontResolution Resolve(
            const FString& PsPostscriptName,
            bool bParserBold,
            bool bParserItalic,
            const UPSD2UMGSettings* Settings);

        /** Strip weight/style suffix and return base name + flags. Exposed for tests. */
        static void ParseSuffix(
            const FString& PsPostscriptName,
            FString& OutBaseName,
            bool& bOutBold,
            bool& bOutItalic);

        /** Map (bBold, bItalic) to the canonical UE typeface FName. */
        static FName MakeTypefaceName(bool bBold, bool bItalic);

        /**
         * Clear the auto-discovery cache (D-04). Called by the import factory
         * after each import so cache state cannot outlive a single PSD import.
         */
        static void InvalidateDiscoveryCache();
    };
}
