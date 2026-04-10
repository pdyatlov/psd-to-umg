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
            if (UTextBlock* T = FindText(Tree, TEXT("text_regular")))
            {
                TestEqual(TEXT("regular size 18"), (int32)T->GetFont().Size, 18);
                TestFalse(TEXT("regular no wrap"), T->AutoWrapText);
            }

            // text_stroked — TEXT-03.
            if (UTextBlock* T = FindText(Tree, TEXT("text_stroked")))
            {
                TestTrue(TEXT("outline > 0"),
                    T->GetFont().OutlineSettings.OutlineSize > 0);
            }

            // text_paragraph — TEXT-06.
            if (UTextBlock* T = FindText(Tree, TEXT("text_paragraph")))
            {
                TestTrue(TEXT("paragraph wraps"), T->AutoWrapText);
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
