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
#include "UObject/UnrealType.h"
#include "ObjectTools.h"
#include "Components/RichTextBlock.h"
#include "Engine/DataTable.h"

BEGIN_DEFINE_SPEC(FTextPipelineSpec, "PSD2UMG.Typography.Pipeline",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

    FPsdDocument Doc;
    FPsdParseDiagnostics Diag;
    bool bParsed = false;

    static UTextBlock* FindText(UWidgetTree* Tree, const FString& NameContains);
    static URichTextBlock* FindRichText(UWidgetTree* Tree, const FString& NameContains);
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

URichTextBlock* FTextPipelineSpec::FindRichText(UWidgetTree* Tree, const FString& NameContains)
{
    URichTextBlock* Out = nullptr;
    Tree->ForEachWidget([&](UWidget* W)
    {
        if (!Out && W && W->GetName().Contains(NameContains))
        {
            Out = Cast<URichTextBlock>(W);
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

            // text_centered — TEXT-F-02. Justification is a protected UPROPERTY on
            // UTextLayoutWidget — there is no GetJustification() accessor, so read it
            // via reflection (BlueprintReadWrite makes the FProperty discoverable).
            if (UTextBlock* T = FindText(Tree, TEXT("text_centered")))
            {
                if (FByteProperty* JustProp = FindFProperty<FByteProperty>(
                        UTextLayoutWidget::StaticClass(), TEXT("Justification")))
                {
                    const uint8 J = JustProp->GetPropertyValue_InContainer(T);
                    TestEqual(TEXT("centered justification is Center"),
                        (int32)J, (int32)ETextJustify::Center);
                }
                else
                {
                    AddError(TEXT("Justification UPROPERTY not found via reflection"));
                }
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

    Describe("RichText.psd -> Spans", [this]()
    {
        BeforeEach([this]()
        {
            Doc = FPsdDocument();
            Diag = FPsdParseDiagnostics();
            bParsed = false;

            TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("PSD2UMG"));
            if (!Plugin.IsValid())
            {
                AddError(TEXT("PSD2UMG plugin not found via IPluginManager"));
                return;
            }

            const FString Fixture = FPaths::Combine(Plugin->GetBaseDir(),
                TEXT("Source/PSD2UMG/Tests/Fixtures/RichText.psd"));

            if (!FPaths::FileExists(Fixture))
            {
                AddError(FString::Printf(TEXT("RichText fixture missing: %s"), *Fixture));
                return;
            }

            bParsed = PSD2UMG::Parser::ParseFile(Fixture, Doc, Diag);
        });

        It("parses RichText.psd with no errors (sanity)", [this]()
        {
            TestTrue(TEXT("parsed"), bParsed);
            TestFalse(TEXT("no errors"), Diag.HasErrors());
            TestEqual(TEXT("RootLayers.Num"), Doc.RootLayers.Num(), 2);
        });

        It("text_two_colors has Spans.Num() >= 2 (RICH-01 parser)", [this]()
        {
            if (!bParsed) return;
            const FPsdLayer* Found = nullptr;
            for (const FPsdLayer& L : Doc.RootLayers)
            {
                if (L.Name.Equals(TEXT("text_two_colors"), ESearchCase::CaseSensitive))
                {
                    Found = &L;
                    break;
                }
            }
            if (!TestNotNull(TEXT("text_two_colors exists in fixture"), Found)) return;
            TestTrue(TEXT("Spans populated by parser multi-run loop"),
                Found->Text.Spans.Num() >= 2);
        });

        It("text_two_colors span colors include red and green (RICH-01 parser)", [this]()
        {
            if (!bParsed) return;
            const FPsdLayer* Found = nullptr;
            for (const FPsdLayer& L : Doc.RootLayers)
            {
                if (L.Name.Equals(TEXT("text_two_colors"), ESearchCase::CaseSensitive))
                {
                    Found = &L;
                    break;
                }
            }
            if (!TestNotNull(TEXT("text_two_colors exists in fixture"), Found)) return;
            if (Found->Text.Spans.Num() < 2)
            {
                AddError(TEXT("text_two_colors Spans must have at least 2 entries before color check"));
                return;
            }

            bool bRedFound = false;
            bool bGreenFound = false;
            for (const FPsdTextRunSpan& S : Found->Text.Spans)
            {
                // sRGB #FF0000 linearises R ~= 1, G ~= 0, B ~= 0 via FromSRGBColor.
                if (S.Color.R >= 0.7f && S.Color.G <= 0.1f && S.Color.B <= 0.1f)
                    bRedFound = true;
                // sRGB #00FF00 linearises R ~= 0, G ~= 1, B ~= 0.
                if (S.Color.R <= 0.1f && S.Color.G >= 0.7f && S.Color.B <= 0.1f)
                    bGreenFound = true;
            }
            TestTrue(TEXT("at least one span is approximately red"), bRedFound);
            TestTrue(TEXT("at least one span is approximately green"), bGreenFound);
        });

        It("text_bold_normal has at least one bold span and one normal span (RICH-02 parser)", [this]()
        {
            if (!bParsed) return;
            const FPsdLayer* Found = nullptr;
            for (const FPsdLayer& L : Doc.RootLayers)
            {
                if (L.Name.Equals(TEXT("text_bold_normal"), ESearchCase::CaseSensitive))
                {
                    Found = &L;
                    break;
                }
            }
            if (!TestNotNull(TEXT("text_bold_normal exists in fixture"), Found)) return;
            if (Found->Text.Spans.Num() < 2)
            {
                AddError(TEXT("text_bold_normal Spans must have at least 2 entries before bold check"));
                return;
            }

            bool bAnyBold = false;
            bool bAnyNormal = false;
            for (const FPsdTextRunSpan& S : Found->Text.Spans)
            {
                if (S.bBold) bAnyBold = true;
                else bAnyNormal = true;
            }
            TestTrue(TEXT("at least one span is bold"), bAnyBold);
            TestTrue(TEXT("at least one span is normal weight"), bAnyNormal);
        });
    });

    Describe("RichText.psd -> URichTextBlock", [this]()
    {
        BeforeEach([this]()
        {
            Doc = FPsdDocument();
            Diag = FPsdParseDiagnostics();
            bParsed = false;

            TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("PSD2UMG"));
            if (!Plugin.IsValid())
            {
                AddError(TEXT("PSD2UMG plugin not found via IPluginManager"));
                return;
            }

            const FString Fixture = FPaths::Combine(Plugin->GetBaseDir(),
                TEXT("Source/PSD2UMG/Tests/Fixtures/RichText.psd"));

            if (!FPaths::FileExists(Fixture))
            {
                AddError(FString::Printf(TEXT("RichText fixture missing: %s"), *Fixture));
                return;
            }

            bParsed = PSD2UMG::Parser::ParseFile(Fixture, Doc, Diag);
        });

        It("generates URichTextBlock for text_two_colors (RICH-01 mapper)", [this]()
        {
            if (!bParsed) return;

            UWidgetBlueprint* WBP = FWidgetBlueprintGenerator::Generate(
                Doc,
                TEXT("/Game/_Test/WBPGen"),
                TEXT("WBP_RichText_Spec"));

            if (!TestNotNull(TEXT("WBP"), WBP)) return;
            if (!TestNotNull(TEXT("WidgetTree"), WBP->WidgetTree.Get())) return;

            UWidgetTree* Tree = WBP->WidgetTree;
            URichTextBlock* RTB = FindRichText(Tree, TEXT("text_two_colors"));
            TestNotNull(TEXT("text_two_colors is a URichTextBlock (not UTextBlock)"), RTB);
        });

        It("URichTextBlock for text_two_colors has non-null TextStyleSet (RICH-01 mapper)", [this]()
        {
            if (!bParsed) return;

            UWidgetBlueprint* WBP = FWidgetBlueprintGenerator::Generate(
                Doc,
                TEXT("/Game/_Test/WBPGen"),
                TEXT("WBP_RichText_Spec2"));

            if (!TestNotNull(TEXT("WBP"), WBP)) return;
            if (!TestNotNull(TEXT("WidgetTree"), WBP->WidgetTree.Get())) return;

            URichTextBlock* RTB = FindRichText(WBP->WidgetTree, TEXT("text_two_colors"));
            if (!TestNotNull(TEXT("RTB found"), RTB)) return;

            UDataTable* Style = RTB->GetTextStyleSet();
            TestNotNull(TEXT("TextStyleSet is non-null (DataTable asset persisted)"), Style);
            if (Style)
            {
                TestTrue(TEXT("TextStyleSet is a persistent asset (not transient)"),
                    !Style->HasAnyFlags(RF_Transient));
            }
        });

        It("URichTextBlock markup contains multiple style tags (RICH-01 + RICH-02 mapper)", [this]()
        {
            if (!bParsed) return;

            UWidgetBlueprint* WBP = FWidgetBlueprintGenerator::Generate(
                Doc,
                TEXT("/Game/_Test/WBPGen"),
                TEXT("WBP_RichText_Spec3"));

            if (!TestNotNull(TEXT("WBP"), WBP)) return;
            if (!TestNotNull(TEXT("WidgetTree"), WBP->WidgetTree.Get())) return;

            URichTextBlock* RTB = FindRichText(WBP->WidgetTree, TEXT("text_two_colors"));
            if (!TestNotNull(TEXT("RTB found"), RTB)) return;

            const FString Markup = RTB->GetText().ToString();
            // Expect at least 2 opening span tags like <s0>...</> <s1>...</>
            int32 OpenTagCount = 0;
            int32 Pos = 0;
            while ((Pos = Markup.Find(TEXT("<s"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Pos)) != INDEX_NONE)
            {
                ++OpenTagCount;
                ++Pos;
            }
            TestTrue(TEXT("markup contains >= 2 <sN> tags"), OpenTagCount >= 2);
            TestTrue(TEXT("markup contains </> closer"), Markup.Contains(TEXT("</>")));
        });

        AfterEach([this]()
        {
            for (const TCHAR* Name : { TEXT("WBP_RichText_Spec"), TEXT("WBP_RichText_Spec2"), TEXT("WBP_RichText_Spec3") })
            {
                const FString AssetPath = FString::Printf(TEXT("/Game/_Test/WBPGen/%s.%s"), Name, Name);
                if (UObject* Existing = FindObject<UObject>(nullptr, *AssetPath))
                {
                    TArray<UObject*> ToDelete = { Existing };
                    ObjectTools::ForceDeleteObjects(ToDelete, /*bShowConfirmation=*/false);
                }
            }
        });
    });
}

#endif // WITH_DEV_AUTOMATION_TESTS
