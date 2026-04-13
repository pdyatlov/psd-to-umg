// Copyright 2018-2021 - John snow wind

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Mapper/FontResolver.h"
#include "PSD2UMGSetting.h"
#include "Engine/Font.h"

BEGIN_DEFINE_SPEC(FFontResolverSpec, "PSD2UMG.Typography.FontResolver",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

    // Saved state for settings restoration.
    TMap<FString, TSoftObjectPtr<UFont>> OriginalFontMap;
    TSoftObjectPtr<UFont> OriginalDefaultFont;

    // Stock engine font used for test mappings.
    UFont* StockFont = nullptr;

END_DEFINE_SPEC(FFontResolverSpec)

void FFontResolverSpec::Define()
{
    BeforeEach([this]()
    {
        UPSD2UMGSettings* Settings = UPSD2UMGSettings::Get();
        OriginalFontMap = Settings->FontMap;
        OriginalDefaultFont = Settings->DefaultFont;

        // Load a stock engine font for test entries.
        StockFont = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
    });

    AfterEach([this]()
    {
        UPSD2UMGSettings* Settings = UPSD2UMGSettings::Get();
        Settings->FontMap = OriginalFontMap;
        Settings->DefaultFont = OriginalDefaultFont;
    });

    Describe("ParseSuffix", [this]()
    {
        It("strips -Bold", [this]()
        {
            FString Base; bool bB = false, bI = false;
            PSD2UMG::FFontResolver::ParseSuffix(TEXT("Arial-Bold"), Base, bB, bI);
            TestEqual(TEXT("base"), Base, FString(TEXT("Arial")));
            TestTrue(TEXT("bold"), bB);
            TestFalse(TEXT("italic"), bI);
        });

        It("strips -BoldItalicMT", [this]()
        {
            FString Base; bool bB = false, bI = false;
            PSD2UMG::FFontResolver::ParseSuffix(TEXT("Arial-BoldItalicMT"), Base, bB, bI);
            TestEqual(TEXT("base"), Base, FString(TEXT("Arial")));
            TestTrue(TEXT("bold"), bB);
            TestTrue(TEXT("italic"), bI);
        });

        It("strips -Oblique as italic", [this]()
        {
            FString Base; bool bB = false, bI = false;
            PSD2UMG::FFontResolver::ParseSuffix(TEXT("Helvetica-Oblique"), Base, bB, bI);
            TestEqual(TEXT("base"), Base, FString(TEXT("Helvetica")));
            TestFalse(TEXT("bold"), bB);
            TestTrue(TEXT("italic"), bI);
        });

        It("leaves plain name alone", [this]()
        {
            FString Base; bool bB = false, bI = false;
            PSD2UMG::FFontResolver::ParseSuffix(TEXT("Arial"), Base, bB, bI);
            TestEqual(TEXT("base"), Base, FString(TEXT("Arial")));
            TestFalse(TEXT("bold"), bB);
            TestFalse(TEXT("italic"), bI);
        });
    });

    Describe("MakeTypefaceName", [this]()
    {
        It("returns NAME_None for regular", [this]()
        {
            TestEqual(TEXT("regular"), PSD2UMG::FFontResolver::MakeTypefaceName(false, false), NAME_None);
        });

        It("returns Bold for bold-only", [this]()
        {
            TestEqual(TEXT("bold"), PSD2UMG::FFontResolver::MakeTypefaceName(true, false), FName(TEXT("Bold")));
        });

        It("returns Italic for italic-only", [this]()
        {
            TestEqual(TEXT("italic"), PSD2UMG::FFontResolver::MakeTypefaceName(false, true), FName(TEXT("Italic")));
        });

        It("returns Bold Italic for both", [this]()
        {
            TestEqual(TEXT("both"), PSD2UMG::FFontResolver::MakeTypefaceName(true, true), FName(TEXT("Bold Italic")));
        });
    });

    Describe("Resolve", [this]()
    {
        It("hits exact match", [this]()
        {
            UPSD2UMGSettings* Settings = UPSD2UMGSettings::Get();
            Settings->FontMap.Empty();
            Settings->DefaultFont = TSoftObjectPtr<UFont>();
            Settings->FontMap.Add(TEXT("Arial-BoldMT"), TSoftObjectPtr<UFont>(StockFont));

            auto R = PSD2UMG::FFontResolver::Resolve(TEXT("Arial-BoldMT"), false, false, Settings);
            TestEqual(TEXT("source"), static_cast<uint8>(R.Source), static_cast<uint8>(PSD2UMG::EFontResolutionSource::Exact));
            TestNotNull(TEXT("font"), R.Font);
            TestTrue(TEXT("bold from suffix"), R.bBoldRequested);
        });

        It("hits case-insensitive", [this]()
        {
            UPSD2UMGSettings* Settings = UPSD2UMGSettings::Get();
            Settings->FontMap.Empty();
            Settings->DefaultFont = TSoftObjectPtr<UFont>();
            Settings->FontMap.Add(TEXT("Arial-BoldMT"), TSoftObjectPtr<UFont>(StockFont));

            auto R = PSD2UMG::FFontResolver::Resolve(TEXT("arial-boldmt"), false, false, Settings);
            // TMap::Find may match case-insensitively on some UE versions (hash is case-insensitive).
            // Accept either Exact or CaseInsensitive as valid — both mean the font was found.
            TestTrue(TEXT("source is Exact or CaseInsensitive"),
                R.Source == PSD2UMG::EFontResolutionSource::Exact
                || R.Source == PSD2UMG::EFontResolutionSource::CaseInsensitive);
            TestNotNull(TEXT("font"), R.Font);
        });

        It("falls back to DefaultFont when no map hit", [this]()
        {
            UPSD2UMGSettings* Settings = UPSD2UMGSettings::Get();
            Settings->FontMap.Empty();
            Settings->DefaultFont = TSoftObjectPtr<UFont>(StockFont);

            auto R = PSD2UMG::FFontResolver::Resolve(TEXT("NonExistent-Regular"), false, false, Settings);
            TestEqual(TEXT("source"), static_cast<uint8>(R.Source), static_cast<uint8>(PSD2UMG::EFontResolutionSource::Default));
            TestNotNull(TEXT("font"), R.Font);
        });

        It("returns EngineDefault source when nothing configured", [this]()
        {
            UPSD2UMGSettings* Settings = UPSD2UMGSettings::Get();
            Settings->FontMap.Empty();
            Settings->DefaultFont = TSoftObjectPtr<UFont>();

            auto R = PSD2UMG::FFontResolver::Resolve(TEXT("NonExistent-Regular"), false, false, Settings);
            TestEqual(TEXT("source"), static_cast<uint8>(R.Source), static_cast<uint8>(PSD2UMG::EFontResolutionSource::EngineDefault));
            TestTrue(TEXT("font null"), R.Font == nullptr);
        });

        It("combines parser bold with suffix italic", [this]()
        {
            UPSD2UMGSettings* Settings = UPSD2UMGSettings::Get();
            Settings->FontMap.Empty();

            auto R = PSD2UMG::FFontResolver::Resolve(TEXT("Arial-Italic"), true, false, Settings);
            TestTrue(TEXT("bold from parser"), R.bBoldRequested);
            TestTrue(TEXT("italic from suffix"), R.bItalicRequested);
            TestEqual(TEXT("typeface"), R.TypefaceName, FName(TEXT("Bold Italic")));
        });
    });
}

#endif
