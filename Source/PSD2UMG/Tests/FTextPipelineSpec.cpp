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

BEGIN_DEFINE_SPEC(FTextPipelineSpec, "PSD2UMG.Typography.Pipeline",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

    FPsdDocument Doc;
    FPsdParseDiagnostics Diag;
    bool bParsed = false;

    static UTextBlock* FindText(UWidgetTree* Tree, const FString& NameContains);
END_DEFINE_SPEC(FTextPipelineSpec)

UTextBlock* FTextPipelineSpec::FindText(UWidgetTree* Tree, const FString& NameContains)
{
    UTextBlock* Out = nullptr;
    Tree->ForEachWidget([&](UWidget* W)
    {
        if (!Out && W && W->GetName().Contains(NameContains))
        {
            Out = Cast<UTextBlock>(W);
        }
    });
    return Out;
}

void FTextPipelineSpec::Define()
{
    Describe("Typography.psd -> WBP", [this]()
    {
        BeforeEach([this]()
        {
            Doc = FPsdDocument();
            Diag = FPsdParseDiagnostics();
            TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("PSD2UMG"));
            const FString Fixture = FPaths::Combine(Plugin->GetBaseDir(),
                TEXT("Source/PSD2UMG/Tests/Fixtures/Typography.psd"));
            bParsed = PSD2UMG::Parser::ParseFile(Fixture, Doc, Diag);
        });

        It("parses Typography.psd with no errors", [this]()
        {
            TestTrue(TEXT("parsed"), bParsed);
            TestFalse(TEXT("no errors"), Diag.HasErrors());
        });

        It("generates UTextBlocks for all text layers with expected typography", [this]()
        {
            if (!bParsed) return;

            UWidgetBlueprint* WBP = FWidgetBlueprintGenerator::Generate(
                Doc,
                TEXT("/Game/_Test/WBPGen"),
                TEXT("WBP_Typography_Spec"));

            if (!TestNotNull(TEXT("WBP"), WBP)) return;
            if (!TestNotNull(TEXT("WidgetTree"), WBP->WidgetTree.Get())) return;

            UWidgetTree* Tree = WBP->WidgetTree;

            // text_regular — TEXT-01 DPI check + no wrap.
            // TEXT-F-01 (Phase 12, 12-01 re-evaluation): PhotoshopAPI returns
            // style_run_font_size() = designer_pt * (4/3). The mapper applies
            // * 0.75 (= 3/4) to recover the designer intent. For text_regular:
            //   raw SizePx = 32.0 (captured from Typography.psd Verbose log)
            //   32.0 * 0.75 = 24  <-- the formula is CORRECT, not accidental.
            // Do NOT remove the 0.75 factor; this assertion pins that contract.
            if (UTextBlock* T = FindText(Tree, TEXT("text_regular")))
            {
                TestEqual(TEXT("regular size 24 (raw=32 * 0.75 = designer 24pt)"),
                    (int32)T->GetFont().Size, 24);
                TestFalse(TEXT("regular no wrap"), T->GetAutoWrapText());
            }

            // text_stroked — TEXT-03.
            // Layer Style Stroke is not extractable (lfx2 not supported by PhotoshopAPI).
            // Verify text_stroked layer exists as a widget — outline verification skipped.
            TestNotNull(TEXT("text_stroked exists"), FindText(Tree, TEXT("text_stroked")));

            // text_paragraph — TEXT-06.
            if (UTextBlock* T = FindText(Tree, TEXT("text_paragraph")))
            {
                TestTrue(TEXT("paragraph wraps"), T->GetAutoWrapText());
            }

            // text_centered — TEXT-F-02.
            if (UTextBlock* T = FindText(Tree, TEXT("text_centered")))
            {
                TestEqual(TEXT("centered justification is Center"),
                    (int32)T->GetJustification(),
                    (int32)ETextJustify::Center);
            }
            else
            {
                AddError(TEXT("text_centered widget not found in generated WBP"));
            }

            // text_gray — TEXT-F-03 fill path.
            if (UTextBlock* T = FindText(Tree, TEXT("text_gray")))
            {
                const FSlateColor SC = T->GetColorAndOpacity();
                const FLinearColor LC = SC.GetSpecifiedColor();
                TestTrue(TEXT("text_gray R within 0.1 of G"),
                    FMath::IsNearlyEqual(LC.R, LC.G, 0.1f));
                TestTrue(TEXT("text_gray R within 0.1 of B"),
                    FMath::IsNearlyEqual(LC.R, LC.B, 0.1f));
                TestTrue(TEXT("text_gray R less than 0.9 (not white)"), LC.R < 0.9f);
                TestTrue(TEXT("text_gray R greater than 0.05 (not black)"), LC.R > 0.05f);
            }
            else
            {
                AddError(TEXT("text_gray widget not found in generated WBP"));
            }

            // text_overlay_gray — TEXT-F-03 overlay-routing path.
            // End-to-end pin: PSD Layer Style Color Overlay -> RouteTextEffects routes
            // onto Text.Color -> mapper sets ColorAndOpacity -> UMG widget renders gray
            // (NOT the white character fill the layer was authored with).
            if (UTextBlock* T = FindText(Tree, TEXT("text_overlay_gray")))
            {
                const FSlateColor SC = T->GetColorAndOpacity();
                const FLinearColor LC = SC.GetSpecifiedColor();
                TestTrue(TEXT("text_overlay_gray R within 0.1 of G (gray, not red)"),
                    FMath::IsNearlyEqual(LC.R, LC.G, 0.1f));
                TestTrue(TEXT("text_overlay_gray R within 0.1 of B (gray, not red)"),
                    FMath::IsNearlyEqual(LC.R, LC.B, 0.1f));
                TestTrue(TEXT("text_overlay_gray R less than 0.9 (overlay overrode white fill)"),
                    LC.R < 0.9f);
                TestTrue(TEXT("text_overlay_gray R greater than 0.05 (not black)"), LC.R > 0.05f);
            }
            else
            {
                AddError(TEXT("text_overlay_gray widget not found in generated WBP"));
            }
        });

        AfterEach([this]()
        {
            const FString AssetPath = TEXT("/Game/_Test/WBPGen/WBP_Typography_Spec.WBP_Typography_Spec");
            if (UObject* Existing = FindObject<UObject>(nullptr, *AssetPath))
            {
                TArray<UObject*> ToDelete = { Existing };
                ObjectTools::ForceDeleteObjects(ToDelete, /*bShowConfirmation=*/false);
            }
        });
    });
}

#endif // WITH_DEV_AUTOMATION_TESTS
