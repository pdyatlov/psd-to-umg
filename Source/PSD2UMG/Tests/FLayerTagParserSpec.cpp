// Copyright 2018-2021 - John snow wind

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Mapper/AllMappers.h"
#include "Parser/FLayerTagParser.h"
#include "Parser/PsdTypes.h"
#include "PSD2UMGSetting.h"

/**
 * Pure unit tests for FLayerTagParser (Phase 9 Wave 0).
 * Covers locked grammar decisions D-01..D-21 plus @variants, @state, @anim, @ia.
 *
 * Headless: no UE asset / fixture / disk dependency. Fast.
 */
BEGIN_DEFINE_SPEC(FLayerTagParserSpec, "PSD2UMG.Parser.LayerTagParser",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

    static FParsedLayerTags ParseGroup(const TCHAR* Name, FString& Diag, int32 Index = 0)
    {
        return FLayerTagParser::Parse(FStringView(Name), EPsdLayerType::Group, Index, Diag);
    }

    static FParsedLayerTags ParseImage(const TCHAR* Name, FString& Diag, int32 Index = 0)
    {
        return FLayerTagParser::Parse(FStringView(Name), EPsdLayerType::Image, Index, Diag);
    }

    static FParsedLayerTags ParseText(const TCHAR* Name, FString& Diag, int32 Index = 0)
    {
        return FLayerTagParser::Parse(FStringView(Name), EPsdLayerType::Text, Index, Diag);
    }

END_DEFINE_SPEC(FLayerTagParserSpec)

void FLayerTagParserSpec::Define()
{
    Describe("Parse -- D-01 canonical syntax", [this]()
    {
        It("extracts CleanName, Type, and Anchor from 'PlayButton @button @anchor:tl'", [this]()
        {
            FString Diag;
            const FParsedLayerTags T = ParseGroup(TEXT("PlayButton @button @anchor:tl"), Diag);
            TestEqual(TEXT("CleanName"), T.CleanName, FString(TEXT("PlayButton")));
            TestEqual(TEXT("Type"), (int32)T.Type, (int32)EPsdTagType::Button);
            TestEqual(TEXT("Anchor"), (int32)T.Anchor, (int32)EPsdAnchorTag::TL);
        });
    });

    Describe("Parse -- D-02 default-type inference", [this]()
    {
        It("group with no type tag defaults to Canvas", [this]()
        {
            FString Diag;
            const FParsedLayerTags T = ParseGroup(TEXT("Root"), Diag);
            TestEqual(TEXT("Type"), (int32)T.Type, (int32)EPsdTagType::Canvas);
        });

        It("pixel layer with no type tag defaults to Image", [this]()
        {
            FString Diag;
            const FParsedLayerTags T = ParseImage(TEXT("Bg"), Diag);
            TestEqual(TEXT("Type"), (int32)T.Type, (int32)EPsdTagType::Image);
        });

        It("text layer with no type tag defaults to Text", [this]()
        {
            FString Diag;
            const FParsedLayerTags T = ParseText(TEXT("Title"), Diag);
            TestEqual(TEXT("Type"), (int32)T.Type, (int32)EPsdTagType::Text);
        });
    });

    Describe("Parse -- D-03/D-07 9-slice", [this]()
    {
        It("explicit margins '@9s:16,16,16,16' set bExplicit=true", [this]()
        {
            FString Diag;
            const FParsedLayerTags T = ParseImage(TEXT("Panel @9s:16,16,16,16"), Diag);
            TestTrue(TEXT("NineSlice set"), T.NineSlice.IsSet());
            if (T.NineSlice.IsSet())
            {
                TestEqual(TEXT("L"), T.NineSlice->L, 16.f);
                TestEqual(TEXT("T"), T.NineSlice->T, 16.f);
                TestEqual(TEXT("R"), T.NineSlice->R, 16.f);
                TestEqual(TEXT("B"), T.NineSlice->B, 16.f);
                TestTrue(TEXT("bExplicit"), T.NineSlice->bExplicit);
            }
        });

        It("shorthand '@9s' defaults to 16px and bExplicit=false", [this]()
        {
            FString Diag;
            const FParsedLayerTags T = ParseImage(TEXT("Panel @9s"), Diag);
            TestTrue(TEXT("NineSlice set"), T.NineSlice.IsSet());
            if (T.NineSlice.IsSet())
            {
                TestEqual(TEXT("L"), T.NineSlice->L, 16.f);
                TestEqual(TEXT("T"), T.NineSlice->T, 16.f);
                TestEqual(TEXT("R"), T.NineSlice->R, 16.f);
                TestEqual(TEXT("B"), T.NineSlice->B, 16.f);
                TestFalse(TEXT("bExplicit"), T.NineSlice->bExplicit);
            }
        });
    });

    Describe("Parse -- D-04 case sensitivity", [this]()
    {
        It("tag names are case-insensitive ('@BUTTON @Anchor:TL')", [this]()
        {
            FString Diag;
            const FParsedLayerTags T = ParseGroup(TEXT("Btn @BUTTON @Anchor:TL"), Diag);
            TestEqual(TEXT("Type"), (int32)T.Type, (int32)EPsdTagType::Button);
            TestEqual(TEXT("Anchor"), (int32)T.Anchor, (int32)EPsdAnchorTag::TL);
        });

        It("@ia value preserves case ('IA_Confirm' verbatim)", [this]()
        {
            FString Diag;
            const FParsedLayerTags T = ParseGroup(TEXT("Btn @button @ia:IA_Confirm"), Diag);
            TestTrue(TEXT("InputAction set"), T.InputAction.IsSet());
            if (T.InputAction.IsSet())
            {
                TestEqual(TEXT("InputAction"), T.InputAction.GetValue(), FString(TEXT("IA_Confirm")));
            }
        });
    });

    Describe("Parse -- D-16/D-20 name extraction", [this]()
    {
        It("clean name is text before first @ (D-20: tag-first input has empty name)", [this]()
        {
            // "@anchor:tl Name @button" -- per grammar, first '@' ends clean name.
            // CleanName is empty -> D-21 fallback "Button_NN".
            FString Diag;
            const FParsedLayerTags T = ParseGroup(TEXT("@anchor:tl Name @button"), Diag, 3);
            TestEqual(TEXT("Type"), (int32)T.Type, (int32)EPsdTagType::Button);
            TestEqual(TEXT("Anchor"), (int32)T.Anchor, (int32)EPsdAnchorTag::TL);
            TestEqual(TEXT("CleanName fallback"), T.CleanName, FString(TEXT("Button_03")));
        });

        It("name-first form: 'My Widget @button' -> CleanName='My_Widget'", [this]()
        {
            FString Diag;
            const FParsedLayerTags T = ParseGroup(TEXT("  My Widget @button"), Diag);
            TestEqual(TEXT("CleanName"), T.CleanName, FString(TEXT("My_Widget")));
            TestEqual(TEXT("Type"), (int32)T.Type, (int32)EPsdTagType::Button);
        });
    });

    Describe("Parse -- D-17 last-wins conflict", [this]()
    {
        It("'X @anchor:tl @anchor:c' yields Anchor=C with conflict diagnostic", [this]()
        {
            FString Diag;
            const FParsedLayerTags T = ParseGroup(TEXT("X @anchor:tl @anchor:c"), Diag);
            TestEqual(TEXT("Anchor"), (int32)T.Anchor, (int32)EPsdAnchorTag::C);
            TestTrue(TEXT("diagnostic mentions conflict"), Diag.Contains(TEXT("conflict")));
        });
    });

    Describe("Parse -- D-18 unknown tag", [this]()
    {
        It("'X @button @frobnicate' captures unknown tag without failure", [this]()
        {
            FString Diag;
            const FParsedLayerTags T = ParseGroup(TEXT("X @button @frobnicate"), Diag);
            TestEqual(TEXT("Type"), (int32)T.Type, (int32)EPsdTagType::Button);
            TestEqual(TEXT("UnknownTags.Num"), T.UnknownTags.Num(), 1);
            if (T.UnknownTags.Num() > 0)
            {
                TestEqual(TEXT("UnknownTags[0]"), T.UnknownTags[0], FString(TEXT("frobnicate")));
            }
            TestTrue(TEXT("diagnostic mentions Unknown"), Diag.Contains(TEXT("Unknown")));
        });
    });

    Describe("Parse -- D-19 multiple type tags", [this]()
    {
        It("'X @button @image' last-wins to Image with error diagnostic", [this]()
        {
            FString Diag;
            const FParsedLayerTags T = ParseGroup(TEXT("X @button @image"), Diag);
            TestEqual(TEXT("Type (last wins)"), (int32)T.Type, (int32)EPsdTagType::Image);
            TestTrue(TEXT("diagnostic mentions error or multiple type"),
                Diag.Contains(TEXT("error")) || Diag.Contains(TEXT("multiple type")));
        });
    });

    Describe("Parse -- D-20 internal-space normalisation", [this]()
    {
        It("'  My Widget @button' -> 'My_Widget'", [this]()
        {
            FString Diag;
            const FParsedLayerTags T = ParseGroup(TEXT("  My Widget @button"), Diag);
            TestEqual(TEXT("CleanName"), T.CleanName, FString(TEXT("My_Widget")));
        });
    });

    Describe("Parse -- D-21 empty-name fallback", [this]()
    {
        It("'@button' at LayerIndex=7 -> 'Button_07'", [this]()
        {
            FString Diag;
            const FParsedLayerTags T = ParseGroup(TEXT("@button"), Diag, 7);
            TestEqual(TEXT("CleanName"), T.CleanName, FString(TEXT("Button_07")));
            TestTrue(TEXT("diagnostic mentions empty name"), Diag.Contains(TEXT("empty name")));
        });
    });

    Describe("Parse -- @variants modifier", [this]()
    {
        It("'Menu @variants' on a group -> bIsVariants=true, Type stays Canvas (default)", [this]()
        {
            FString Diag;
            const FParsedLayerTags T = ParseGroup(TEXT("Menu @variants"), Diag);
            TestTrue(TEXT("bIsVariants"), T.bIsVariants);
            TestEqual(TEXT("Type still Canvas (group default)"), (int32)T.Type, (int32)EPsdTagType::Canvas);
        });
    });

    Describe("Parse -- @state tags (D-12)", [this]()
    {
        It("'Btn_Hover @state:hover' -> State=Hover", [this]()
        {
            FString Diag;
            const FParsedLayerTags T = ParseImage(TEXT("Btn_Hover @state:hover"), Diag);
            TestEqual(TEXT("State"), (int32)T.State, (int32)EPsdStateTag::Hover);
        });

        It("'Btn_Bg @state:bg' -> State=Bg", [this]()
        {
            FString Diag;
            const FParsedLayerTags T = ParseImage(TEXT("Btn_Bg @state:bg"), Diag);
            TestEqual(TEXT("State"), (int32)T.State, (int32)EPsdStateTag::Bg);
        });
    });

    Describe("Parse -- @anim tags (D-10)", [this]()
    {
        It("'FadeIn @anim:show' -> Anim=Show", [this]()
        {
            FString Diag;
            const FParsedLayerTags T = ParseGroup(TEXT("FadeIn @anim:show"), Diag);
            TestEqual(TEXT("Anim"), (int32)T.Anim, (int32)EPsdAnimTag::Show);
        });
    });

    Describe("FindChildByState -- D-12/D-13", [this]()
    {
        It("returns the explicit @state:hover child for DesiredState=Hover", [this]()
        {
            TArray<FPsdLayer> Children;
            FPsdLayer A; A.Type = EPsdLayerType::Image; A.Name = TEXT("Normal");
            { FString D; A.ParsedTags = FLayerTagParser::Parse(A.Name, A.Type, 0, D); }
            FPsdLayer B; B.Type = EPsdLayerType::Image; B.Name = TEXT("H @state:hover");
            { FString D; B.ParsedTags = FLayerTagParser::Parse(B.Name, B.Type, 1, D); }
            Children.Add(A); Children.Add(B);

            const FPsdLayer* Found = FLayerTagParser::FindChildByState(Children, EPsdStateTag::Hover);
            if (TestNotNull(TEXT("Found"), Found))
            {
                TestEqual(TEXT("Found.Name"), Found->Name, FString(TEXT("H @state:hover")));
            }
        });

        It("falls back to first untagged Image child for DesiredState=Normal", [this]()
        {
            TArray<FPsdLayer> Children;
            FPsdLayer A; A.Type = EPsdLayerType::Image; A.Name = TEXT("FirstImage");
            { FString D; A.ParsedTags = FLayerTagParser::Parse(A.Name, A.Type, 0, D); }
            FPsdLayer B; B.Type = EPsdLayerType::Image; B.Name = TEXT("Other");
            { FString D; B.ParsedTags = FLayerTagParser::Parse(B.Name, B.Type, 1, D); }
            Children.Add(A); Children.Add(B);

            const FPsdLayer* Found = FLayerTagParser::FindChildByState(Children, EPsdStateTag::Normal);
            if (TestNotNull(TEXT("Found"), Found))
            {
                TestEqual(TEXT("Found.Name"), Found->Name, FString(TEXT("FirstImage")));
            }
        });

        It("returns nullptr for DesiredState=Pressed when no child matches", [this]()
        {
            TArray<FPsdLayer> Children;
            FPsdLayer A; A.Type = EPsdLayerType::Image; A.Name = TEXT("Just");
            { FString D; A.ParsedTags = FLayerTagParser::Parse(A.Name, A.Type, 0, D); }
            Children.Add(A);

            const FPsdLayer* Found = FLayerTagParser::FindChildByState(Children, EPsdStateTag::Pressed);
            TestNull(TEXT("Found"), Found);
        });
    });

    Describe("R-06 -- CommonUI priority gate", [this]()
    {
        It("bUseCommonUI=true: CommonUI mapper accepts @button, vanilla Button mapper still accepts (registry priority decides)", [this]()
        {
            UPSD2UMGSettings* Settings = UPSD2UMGSettings::GetMutableDefault<UPSD2UMGSettings>();
            const bool bPrev = Settings->bUseCommonUI;
            Settings->bUseCommonUI = true;

            FPsdLayer L; L.Type = EPsdLayerType::Group; L.Name = TEXT("Confirm @button");
            { FString D; L.ParsedTags = FLayerTagParser::Parse(L.Name, L.Type, 0, D); }

            FCommonUIButtonLayerMapper CommonUI;
            FButtonLayerMapper Vanilla;
            TestTrue(TEXT("CommonUI accepts when bUseCommonUI=true"), CommonUI.CanMap(L));
            TestTrue(TEXT("Vanilla also accepts; registry priority (210 > 200) selects CommonUI"), Vanilla.CanMap(L));
            TestTrue(TEXT("CommonUI priority wins"), CommonUI.GetPriority() > Vanilla.GetPriority());

            Settings->bUseCommonUI = bPrev;
        });

        It("bUseCommonUI=false: CommonUI mapper rejects @button; vanilla Button mapper handles it", [this]()
        {
            UPSD2UMGSettings* Settings = UPSD2UMGSettings::GetMutableDefault<UPSD2UMGSettings>();
            const bool bPrev = Settings->bUseCommonUI;
            Settings->bUseCommonUI = false;

            FPsdLayer L; L.Type = EPsdLayerType::Group; L.Name = TEXT("Confirm @button");
            { FString D; L.ParsedTags = FLayerTagParser::Parse(L.Name, L.Type, 0, D); }

            FCommonUIButtonLayerMapper CommonUI;
            FButtonLayerMapper Vanilla;
            TestFalse(TEXT("CommonUI rejects when bUseCommonUI=false"), CommonUI.CanMap(L));
            TestTrue(TEXT("Vanilla still accepts"), Vanilla.CanMap(L));

            Settings->bUseCommonUI = bPrev;
        });
    });
}

#endif // WITH_DEV_AUTOMATION_TESTS
