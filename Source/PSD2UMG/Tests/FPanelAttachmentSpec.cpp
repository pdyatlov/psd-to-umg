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
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/ScrollBox.h"
#include "Components/Overlay.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "ObjectTools.h"

// ---------------------------------------------------------------------------
// PANEL-01..07 automation spec: Panel child attachment round-trip (Plan 10-03)
//
// Fixture: Source/PSD2UMG/Tests/Fixtures/Panels.psd
//   VBoxGroup @vbox        -- 3 children (ItemA @text, ItemB @image, ItemC @button)
//   HBoxGroup @hbox        -- 2 children
//   ScrollBoxGroup @scrollbox -- 4 children
//   OverlayGroup @overlay  -- 2 children
//   CanvasGroup @canvas    -- 2 children (regression anchor for canvas path)
//   OuterVBox @vbox        -- contains InnerCanvas @canvas, which contains NestedItem @text
//
// All tests gate on FPaths::FileExists(FixturePath) so the spec compiles and
// runs safely even if the PSD has not yet been authored (returns silently --
// not an error). Follows FTextEffectsSpec fixture-gate pattern.
// ---------------------------------------------------------------------------

BEGIN_DEFINE_SPEC(FPanelAttachmentSpec, "PSD2UMG.PanelAttachment",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

    FString FixturePath;
    FPsdDocument Doc;
    FPsdParseDiagnostics Diag;
    bool bFixtureExists = false;
    bool bParsed = false;
    UWidgetBlueprint* WBP = nullptr;

    // Helper: find a widget by name using ForEachWidget (works across all nesting levels).
    static UWidget* FindWidgetByName(UWidgetTree* Tree, const FName& Name);

END_DEFINE_SPEC(FPanelAttachmentSpec)

UWidget* FPanelAttachmentSpec::FindWidgetByName(UWidgetTree* Tree, const FName& Name)
{
    UWidget* Out = nullptr;
    Tree->ForEachWidget([&](UWidget* W)
    {
        if (!Out && W && W->GetFName() == Name)
        {
            Out = W;
        }
    });
    return Out;
}

void FPanelAttachmentSpec::Define()
{
    Describe("PanelAttachment fixture round-trip", [this]()
    {
        BeforeEach([this]()
        {
            WBP = nullptr;
            bParsed = false;
            Doc = FPsdDocument();
            Diag = FPsdParseDiagnostics();

            TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("PSD2UMG"));
            FixturePath = FPaths::Combine(Plugin->GetBaseDir(),
                TEXT("Source/PSD2UMG/Tests/Fixtures/Panels.psd"));
            bFixtureExists = FPaths::FileExists(FixturePath);

            if (bFixtureExists)
            {
                bParsed = PSD2UMG::Parser::ParseFile(FixturePath, Doc, Diag);
                if (bParsed)
                {
                    WBP = FWidgetBlueprintGenerator::Generate(
                        Doc,
                        TEXT("/Engine/Transient"),
                        TEXT("WBP_PanelsSpec"));
                    if (!WBP)
                    {
                        AddError(TEXT("FWidgetBlueprintGenerator::Generate returned nullptr for Panels.psd"));
                    }
                }
                else
                {
                    AddError(TEXT("FPsdParser::ParseFile failed for Panels.psd"));
                }
            }
        });

        // ------------------------------------------------------------------
        // PANEL-01: @vbox group -> UVerticalBox, 3 children in z-order
        // ------------------------------------------------------------------
        It("VBoxGroup_IsVerticalBoxWith3Children (PANEL-01)", [this]()
        {
            if (!bFixtureExists) return; // fixture not yet authored -- skip silently
            if (!TestTrue(TEXT("fixture parses"), bParsed)) return;
            if (!TestNotNull(TEXT("WBP"), WBP)) return;
            if (!TestNotNull(TEXT("WidgetTree"), WBP->WidgetTree.Get())) return;

            UWidget* W = FindWidgetByName(WBP->WidgetTree, FName(TEXT("VBoxGroup")));
            if (!TestNotNull(TEXT("VBoxGroup widget exists"), W)) return;

            UVerticalBox* VBox = Cast<UVerticalBox>(W);
            if (!TestNotNull(TEXT("VBoxGroup is UVerticalBox"), VBox)) return;

            TestEqual(TEXT("VBoxGroup child count == 3"), VBox->GetChildrenCount(), 3);

            // Children: ItemA @text, ItemB @image, ItemC @button (in PSD z-order)
            UWidget* ItemA = FindWidgetByName(WBP->WidgetTree, FName(TEXT("ItemA")));
            TestNotNull(TEXT("ItemA child exists"), ItemA);
            if (ItemA)
            {
                TestNotNull(TEXT("ItemA is UTextBlock"), Cast<UTextBlock>(ItemA));
            }

            UWidget* ItemB = FindWidgetByName(WBP->WidgetTree, FName(TEXT("ItemB")));
            TestNotNull(TEXT("ItemB child exists"), ItemB);
            if (ItemB)
            {
                TestNotNull(TEXT("ItemB is UImage"), Cast<UImage>(ItemB));
            }

            UWidget* ItemC = FindWidgetByName(WBP->WidgetTree, FName(TEXT("ItemC")));
            TestNotNull(TEXT("ItemC child exists (button container)"), ItemC);
        });

        // ------------------------------------------------------------------
        // PANEL-02: @hbox group -> UHorizontalBox, 2 children
        // ------------------------------------------------------------------
        It("HBoxGroup_IsHorizontalBoxWith2Children (PANEL-02)", [this]()
        {
            if (!bFixtureExists) return;
            if (!TestTrue(TEXT("fixture parses"), bParsed)) return;
            if (!TestNotNull(TEXT("WBP"), WBP)) return;
            if (!TestNotNull(TEXT("WidgetTree"), WBP->WidgetTree.Get())) return;

            UWidget* W = FindWidgetByName(WBP->WidgetTree, FName(TEXT("HBoxGroup")));
            if (!TestNotNull(TEXT("HBoxGroup widget exists"), W)) return;

            UHorizontalBox* HBox = Cast<UHorizontalBox>(W);
            if (!TestNotNull(TEXT("HBoxGroup is UHorizontalBox"), HBox)) return;

            TestEqual(TEXT("HBoxGroup child count == 2"), HBox->GetChildrenCount(), 2);
        });

        // ------------------------------------------------------------------
        // PANEL-03: @scrollbox group -> UScrollBox, 4 children
        // ------------------------------------------------------------------
        It("ScrollBoxGroup_IsScrollBoxWith4Children (PANEL-03)", [this]()
        {
            if (!bFixtureExists) return;
            if (!TestTrue(TEXT("fixture parses"), bParsed)) return;
            if (!TestNotNull(TEXT("WBP"), WBP)) return;
            if (!TestNotNull(TEXT("WidgetTree"), WBP->WidgetTree.Get())) return;

            UWidget* W = FindWidgetByName(WBP->WidgetTree, FName(TEXT("ScrollBoxGroup")));
            if (!TestNotNull(TEXT("ScrollBoxGroup widget exists"), W)) return;

            UScrollBox* SBox = Cast<UScrollBox>(W);
            if (!TestNotNull(TEXT("ScrollBoxGroup is UScrollBox"), SBox)) return;

            TestEqual(TEXT("ScrollBoxGroup child count == 4"), SBox->GetChildrenCount(), 4);
        });

        // ------------------------------------------------------------------
        // PANEL-04: @overlay group -> UOverlay, 2 children
        // ------------------------------------------------------------------
        It("OverlayGroup_IsOverlayWith2Children (PANEL-04)", [this]()
        {
            if (!bFixtureExists) return;
            if (!TestTrue(TEXT("fixture parses"), bParsed)) return;
            if (!TestNotNull(TEXT("WBP"), WBP)) return;
            if (!TestNotNull(TEXT("WidgetTree"), WBP->WidgetTree.Get())) return;

            UWidget* W = FindWidgetByName(WBP->WidgetTree, FName(TEXT("OverlayGroup")));
            if (!TestNotNull(TEXT("OverlayGroup widget exists"), W)) return;

            UOverlay* Overlay = Cast<UOverlay>(W);
            if (!TestNotNull(TEXT("OverlayGroup is UOverlay"), Overlay)) return;

            TestEqual(TEXT("OverlayGroup child count == 2"), Overlay->GetChildrenCount(), 2);
        });

        // ------------------------------------------------------------------
        // PANEL-05: @canvas group -> UCanvasPanel, anchors/offsets preserved (regression)
        // ------------------------------------------------------------------
        It("CanvasGroup_PreservesAnchorsAndOffsets (PANEL-05)", [this]()
        {
            if (!bFixtureExists) return;
            if (!TestTrue(TEXT("fixture parses"), bParsed)) return;
            if (!TestNotNull(TEXT("WBP"), WBP)) return;
            if (!TestNotNull(TEXT("WidgetTree"), WBP->WidgetTree.Get())) return;

            UWidget* W = FindWidgetByName(WBP->WidgetTree, FName(TEXT("CanvasGroup")));
            if (!TestNotNull(TEXT("CanvasGroup widget exists"), W)) return;

            UCanvasPanel* Canvas = Cast<UCanvasPanel>(W);
            if (!TestNotNull(TEXT("CanvasGroup is UCanvasPanel"), Canvas)) return;

            TestEqual(TEXT("CanvasGroup child count == 2"), Canvas->GetChildrenCount(), 2);

            // Spot-check C1 child: it must have a UCanvasPanelSlot with Offsets.Left
            // near the PSD x position (50px per fixture definition).
            UWidget* C1 = FindWidgetByName(WBP->WidgetTree, FName(TEXT("C1")));
            if (!TestNotNull(TEXT("C1 widget exists"), C1)) return;

            UCanvasPanelSlot* C1Slot = Cast<UCanvasPanelSlot>(C1->Slot);
            if (!TestNotNull(TEXT("C1 slot is UCanvasPanelSlot"), C1Slot)) return;

            // C1 was placed at x=50 in fixture; Left offset should be within 1px.
            TestTrue(TEXT("C1 Offsets.Left near 50px"),
                FMath::IsNearlyEqual(C1Slot->GetOffsets().Left, 50.f, 1.f));
        });

        // ------------------------------------------------------------------
        // PANEL-06: nested @vbox -> @canvas -> @text dispatches correctly
        // ------------------------------------------------------------------
        It("NestedMixedGroups_DispatchCorrectly (PANEL-06)", [this]()
        {
            if (!bFixtureExists) return;
            if (!TestTrue(TEXT("fixture parses"), bParsed)) return;
            if (!TestNotNull(TEXT("WBP"), WBP)) return;
            if (!TestNotNull(TEXT("WidgetTree"), WBP->WidgetTree.Get())) return;

            // OuterVBox @vbox -> UVerticalBox, 1 child
            UWidget* OuterW = FindWidgetByName(WBP->WidgetTree, FName(TEXT("OuterVBox")));
            if (!TestNotNull(TEXT("OuterVBox widget exists"), OuterW)) return;

            UVerticalBox* OuterVBox = Cast<UVerticalBox>(OuterW);
            if (!TestNotNull(TEXT("OuterVBox is UVerticalBox"), OuterVBox)) return;
            TestEqual(TEXT("OuterVBox child count == 1"), OuterVBox->GetChildrenCount(), 1);

            // InnerCanvas @canvas -> UCanvasPanel, 1 child
            UWidget* InnerW = FindWidgetByName(WBP->WidgetTree, FName(TEXT("InnerCanvas")));
            if (!TestNotNull(TEXT("InnerCanvas widget exists"), InnerW)) return;

            UCanvasPanel* InnerCanvas = Cast<UCanvasPanel>(InnerW);
            if (!TestNotNull(TEXT("InnerCanvas is UCanvasPanel"), InnerCanvas)) return;
            TestEqual(TEXT("InnerCanvas child count == 1"), InnerCanvas->GetChildrenCount(), 1);

            // NestedItem @text -> UTextBlock inside a UCanvasPanelSlot with non-zero offsets
            UWidget* NestedW = FindWidgetByName(WBP->WidgetTree, FName(TEXT("NestedItem")));
            if (!TestNotNull(TEXT("NestedItem widget exists"), NestedW)) return;

            TestNotNull(TEXT("NestedItem is UTextBlock"), Cast<UTextBlock>(NestedW));

            UCanvasPanelSlot* NestedSlot = Cast<UCanvasPanelSlot>(NestedW->Slot);
            if (!TestNotNull(TEXT("NestedItem slot is UCanvasPanelSlot (canvas parent applies positional data)"), NestedSlot)) return;

            // NestedItem is at (10,10) per fixture -- offsets must be non-zero
            const FMargin Offsets = NestedSlot->GetOffsets();
            TestTrue(TEXT("NestedItem Offsets.Left near 10px"),
                FMath::IsNearlyEqual(Offsets.Left, 10.f, 1.f));
            TestTrue(TEXT("NestedItem Offsets.Top near 10px"),
                FMath::IsNearlyEqual(Offsets.Top, 10.f, 1.f));
        });

        // ------------------------------------------------------------------
        // PANEL-01..04 negative: non-canvas children do NOT get UCanvasPanelSlot
        // ------------------------------------------------------------------
        It("NonCanvasChildSlotsHaveNoPositionalData (PANEL-01..04 negative)", [this]()
        {
            if (!bFixtureExists) return;
            if (!TestTrue(TEXT("fixture parses"), bParsed)) return;
            if (!TestNotNull(TEXT("WBP"), WBP)) return;
            if (!TestNotNull(TEXT("WidgetTree"), WBP->WidgetTree.Get())) return;

            // ItemA is a direct child of VBoxGroup @vbox -- its slot must be UVerticalBoxSlot,
            // not UCanvasPanelSlot.
            UWidget* ItemA = FindWidgetByName(WBP->WidgetTree, FName(TEXT("ItemA")));
            if (!TestNotNull(TEXT("ItemA widget exists"), ItemA)) return;

            TestNull(TEXT("ItemA slot is NOT a UCanvasPanelSlot (is VBox slot)"),
                Cast<UCanvasPanelSlot>(ItemA->Slot));

            UVerticalBoxSlot* VBoxSlot = Cast<UVerticalBoxSlot>(ItemA->Slot);
            TestNotNull(TEXT("ItemA slot IS a UVerticalBoxSlot"), VBoxSlot);
        });

        // ------------------------------------------------------------------
        // PANEL-01..04 reimport regression sentinel (covers 10-02 UpdateCanvas non-canvas branch)
        // ------------------------------------------------------------------
        It("ReimportPreservesNonCanvasChildren (reimport regression sentinel)", [this]()
        {
            if (!bFixtureExists) return;
            if (!TestTrue(TEXT("fixture parses"), bParsed)) return;
            if (!TestNotNull(TEXT("WBP"), WBP)) return;
            if (!TestNotNull(TEXT("WidgetTree"), WBP->WidgetTree.Get())) return;

            // Trivial no-op reimport: run Update on the same WBP with the same doc.
            // This exercises the non-canvas clear-and-rebuild path without a PSD change.
            const TSet<FString> SkippedLayerNames;
            const bool bUpdated = FWidgetBlueprintGenerator::Update(WBP, Doc, SkippedLayerNames);
            if (!TestTrue(TEXT("Update returns true on no-op reimport"), bUpdated)) return;
            if (!TestNotNull(TEXT("WidgetTree still valid after Update"), WBP->WidgetTree.Get())) return;

            // Re-fetch VBoxGroup and verify it still has 3 children with correct classes.
            UWidget* VBoxW = FindWidgetByName(WBP->WidgetTree, FName(TEXT("VBoxGroup")));
            if (!TestNotNull(TEXT("VBoxGroup still exists after Update"), VBoxW)) return;

            UVerticalBox* VBox = Cast<UVerticalBox>(VBoxW);
            if (!TestNotNull(TEXT("VBoxGroup still a UVerticalBox after Update"), VBox)) return;
            TestEqual(TEXT("VBoxGroup child count still 3 after Update"), VBox->GetChildrenCount(), 3);

            // First child (ItemA) must still be a UTextBlock with a UVerticalBoxSlot.
            UWidget* ItemA = FindWidgetByName(WBP->WidgetTree, FName(TEXT("ItemA")));
            if (!TestNotNull(TEXT("ItemA still exists after Update"), ItemA)) return;
            TestNotNull(TEXT("ItemA still a UTextBlock after Update"), Cast<UTextBlock>(ItemA));
            TestNotNull(TEXT("ItemA slot still a UVerticalBoxSlot after Update"),
                Cast<UVerticalBoxSlot>(ItemA->Slot));

            // ScrollBoxGroup must still have 4 children.
            UWidget* ScrollW = FindWidgetByName(WBP->WidgetTree, FName(TEXT("ScrollBoxGroup")));
            if (!TestNotNull(TEXT("ScrollBoxGroup still exists after Update"), ScrollW)) return;

            UScrollBox* SBox = Cast<UScrollBox>(ScrollW);
            if (!TestNotNull(TEXT("ScrollBoxGroup still a UScrollBox after Update"), SBox)) return;
            TestEqual(TEXT("ScrollBoxGroup child count still 4 after Update"), SBox->GetChildrenCount(), 4);
        });

        AfterEach([this]()
        {
            const FString AssetPath = TEXT("/Engine/Transient/WBP_PanelsSpec.WBP_PanelsSpec");
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
