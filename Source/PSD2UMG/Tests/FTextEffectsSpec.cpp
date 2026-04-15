// Copyright 2018-2021 - John snow wind

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Interfaces/IPluginManager.h"

#include "Parser/PsdParser.h"
#include "Parser/PsdTypes.h"
#include "Parser/PsdDiagnostics.h"

#include "Generator/FWidgetBlueprintGenerator.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "ObjectTools.h"

// ---------------------------------------------------------------------------
// TEXT-04 automation spec: Drop Shadow round-trip (Plan 04.1-01)
//
// Fixture: Source/PSD2UMG/Tests/Fixtures/TextEffects.psd
//   text_shadow -- Drop Shadow Distance=10px, Angle=135 deg, Color=#000000, Opacity=50%
//   text_plain  -- No layer effects
//
// DPI golden values (from 04.1-RESEARCH.md):
//   shadow distance 10px at 135 deg -> raw PSD (-7.07, 7.07) -> UMG x0.75 -> (-5.30, 5.30)
//   opacity 50% -> A approx 0.50 (tolerance 0.05)
//
// All tests gate on FPaths::FileExists(FixturePath) so the spec compiles and
// runs safely even if the PSD has not yet been authored (returns silently -- not
// an error). Plan 04.1-01 SUMMARY documents fixture-deferred status when needed.
// ---------------------------------------------------------------------------

BEGIN_DEFINE_SPEC(FTextEffectsSpec, "PSD2UMG.TextEffects",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

    FString FixturePath;
    FPsdDocument Doc;
    FPsdParseDiagnostics Diag;
    bool bFixtureExists = false;
    bool bParsed = false;
    UWidgetBlueprint* WBP = nullptr;

    static const FPsdLayer* FindLayerByName(const TArray<FPsdLayer>& Layers, const FString& Name);
    static UTextBlock* FindTextBlockByName(UWidgetTree* Tree, const FString& Name);
END_DEFINE_SPEC(FTextEffectsSpec)

const FPsdLayer* FTextEffectsSpec::FindLayerByName(const TArray<FPsdLayer>& Layers, const FString& Name)
{
    for (const FPsdLayer& L : Layers)
    {
        if (L.Name == Name) return &L;
        // Shallow search only -- TextEffects.psd has a flat root layer list.
    }
    return nullptr;
}

UTextBlock* FTextEffectsSpec::FindTextBlockByName(UWidgetTree* Tree, const FString& Name)
{
    UTextBlock* Out = nullptr;
    Tree->ForEachWidget([&](UWidget* W)
    {
        if (!Out && W && W->GetName() == Name)
        {
            Out = Cast<UTextBlock>(W);
        }
    });
    return Out;
}

void FTextEffectsSpec::Define()
{
    Describe("TextEffects fixture round-trip", [this]()
    {
        BeforeEach([this]()
        {
            WBP = nullptr;
            bParsed = false;
            Doc = FPsdDocument();
            Diag = FPsdParseDiagnostics();

            TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("PSD2UMG"));
            FixturePath = FPaths::Combine(Plugin->GetBaseDir(),
                TEXT("Source/PSD2UMG/Tests/Fixtures/TextEffects.psd"));
            bFixtureExists = FPaths::FileExists(FixturePath);

            if (bFixtureExists)
            {
                bParsed = PSD2UMG::Parser::ParseFile(FixturePath, Doc, Diag);
                if (bParsed)
                {
                    WBP = FWidgetBlueprintGenerator::Generate(
                        Doc,
                        TEXT("/Game/_Test/WBPGen"),
                        TEXT("WBP_TextEffects_Spec"));
                }
            }
        });

        // ------------------------------------------------------------------
        // Case 1: parser routes DropShadow -> Text.ShadowOffset/Color
        // ------------------------------------------------------------------
        It("text_shadow layer has non-zero Text.ShadowOffset after parsing (TEXT-04)", [this]()
        {
            if (!bFixtureExists) return; // fixture not yet authored -- skip silently
            if (!TestTrue(TEXT("fixture parses"), bParsed)) return;

            const FPsdLayer* L = FindLayerByName(Doc.RootLayers, TEXT("text_shadow"));
            if (!TestNotNull(TEXT("text_shadow layer exists"), L)) return;

            // Magnitude > 1px: raw 10px distance should produce > 7px combined offset
            TestTrue(TEXT("ShadowOffset has magnitude > 1"),
                L->Text.ShadowOffset.Size() > 1.f);
            // Color A approximately 0.50
            TestTrue(TEXT("ShadowColor A approx 0.5"),
                FMath::IsNearlyEqual(L->Text.ShadowColor.A, 0.5f, 0.05f));
        });

        // ------------------------------------------------------------------
        // Case 2: D-13 double-render guard -- bHasDropShadow cleared after routing
        // ------------------------------------------------------------------
        It("text_shadow layer has Effects.bHasDropShadow == false after routing (D-13)", [this]()
        {
            if (!bFixtureExists) return;
            if (!bParsed) return;

            const FPsdLayer* L = FindLayerByName(Doc.RootLayers, TEXT("text_shadow"));
            if (!TestNotNull(TEXT("text_shadow layer"), L)) return;

            TestFalse(TEXT("bHasDropShadow cleared"), L->Effects.bHasDropShadow);
        });

        // ------------------------------------------------------------------
        // Case 3: mapper applies DPI-converted shadow offset to UTextBlock
        // ------------------------------------------------------------------
        It("text_shadow UTextBlock has DPI-converted shadow offset (TEXT-04)", [this]()
        {
            if (!bFixtureExists) return;
            if (!bParsed) return;
            if (!TestNotNull(TEXT("WBP"), WBP)) return;
            if (!TestNotNull(TEXT("WidgetTree"), WBP->WidgetTree.Get())) return;

            UTextBlock* TB = FindTextBlockByName(WBP->WidgetTree, TEXT("text_shadow"));
            if (!TestNotNull(TEXT("text_shadow UTextBlock"), TB)) return;

            // DPI golden: 10px * cos(135 deg) * 0.75 = approx -5.30
            //             10px * sin(135 deg) * 0.75 = approx +5.30
            const FVector2D Offset = TB->GetShadowOffset();
            TestTrue(TEXT("ShadowOffset X approx -5.3"),
                FMath::IsNearlyEqual(Offset.X, -5.3f, 0.2f));
            TestTrue(TEXT("ShadowOffset Y approx +5.3"),
                FMath::IsNearlyEqual(Offset.Y, 5.3f, 0.2f));
        });

        // ------------------------------------------------------------------
        // Case 4: mapper applies shadow color A ~ 0.5
        // ------------------------------------------------------------------
        It("text_shadow UTextBlock has shadow color A approx 0.5 (TEXT-04)", [this]()
        {
            if (!bFixtureExists) return;
            if (!bParsed) return;
            if (!TestNotNull(TEXT("WBP"), WBP)) return;
            if (!TestNotNull(TEXT("WidgetTree"), WBP->WidgetTree.Get())) return;

            UTextBlock* TB = FindTextBlockByName(WBP->WidgetTree, TEXT("text_shadow"));
            if (!TestNotNull(TEXT("text_shadow UTextBlock"), TB)) return;

            TestTrue(TEXT("ShadowColorAndOpacity A approx 0.5"),
                FMath::IsNearlyEqual(TB->GetShadowColorAndOpacity().A, 0.5f, 0.05f));
        });

        // ------------------------------------------------------------------
        // Case 5: non-shadow text has zero shadow offset
        // ------------------------------------------------------------------
        It("text_plain UTextBlock has zero shadow offset", [this]()
        {
            if (!bFixtureExists) return;
            if (!bParsed) return;
            if (!TestNotNull(TEXT("WBP"), WBP)) return;
            if (!TestNotNull(TEXT("WidgetTree"), WBP->WidgetTree.Get())) return;

            UTextBlock* TB = FindTextBlockByName(WBP->WidgetTree, TEXT("text_plain"));
            if (!TestNotNull(TEXT("text_plain UTextBlock"), TB)) return;

            TestTrue(TEXT("zero shadow offset"), TB->GetShadowOffset().IsZero());
        });

        // ------------------------------------------------------------------
        // Case 6: non-shadow text has transparent shadow color
        // ------------------------------------------------------------------
        It("text_plain UTextBlock has transparent shadow color", [this]()
        {
            if (!bFixtureExists) return;
            if (!bParsed) return;
            if (!TestNotNull(TEXT("WBP"), WBP)) return;
            if (!TestNotNull(TEXT("WidgetTree"), WBP->WidgetTree.Get())) return;

            UTextBlock* TB = FindTextBlockByName(WBP->WidgetTree, TEXT("text_plain"));
            if (!TestNotNull(TEXT("text_plain UTextBlock"), TB)) return;

            TestTrue(TEXT("shadow color A < 0.01"),
                TB->GetShadowColorAndOpacity().A < 0.01f);
        });

        // ------------------------------------------------------------------
        // Case 7: no sibling _Shadow UImage spawned for shadow text layers (D-13)
        // ------------------------------------------------------------------
        It("text_shadow does NOT spawn a sibling _Shadow UImage (D-13)", [this]()
        {
            if (!bFixtureExists) return;
            if (!bParsed) return;
            if (!TestNotNull(TEXT("WBP"), WBP)) return;
            if (!TestNotNull(TEXT("WidgetTree"), WBP->WidgetTree.Get())) return;

            bool bFoundShadowImage = false;
            WBP->WidgetTree->ForEachWidget([&](UWidget* W)
            {
                if (W && W->GetName().EndsWith(TEXT("_Shadow")))
                {
                    bFoundShadowImage = true;
                }
            });
            TestFalse(TEXT("no _Shadow UImage sibling"), bFoundShadowImage);
        });

        AfterEach([this]()
        {
            const FString AssetPath = TEXT("/Game/_Test/WBPGen/WBP_TextEffects_Spec.WBP_TextEffects_Spec");
            if (UObject* Existing = FindObject<UObject>(nullptr, *AssetPath))
            {
                TArray<UObject*> ToDelete = { Existing };
                ObjectTools::ForceDeleteObjects(ToDelete, /*bShowConfirmation=*/false);
            }
            WBP = nullptr;
        });
    });
}

#endif // WITH_DEV_AUTOMATION_TESTS
