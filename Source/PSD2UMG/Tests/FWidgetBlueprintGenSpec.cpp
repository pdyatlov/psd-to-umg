// Copyright 2018-2021 - John snow wind

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Parser/PsdTypes.h"
#include "Generator/FWidgetBlueprintGenerator.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanelSlot.h"

BEGIN_DEFINE_SPEC(FWidgetBlueprintGenSpec, "PSD2UMG.Generator", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FWidgetBlueprintGenSpec)

void FWidgetBlueprintGenSpec::Define()
{
    Describe("FWidgetBlueprintGenerator::Generate", [this]()
    {
        It("should create a valid WBP from a minimal document", [this]()
        {
            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(1920, 1080);
            Doc.SourcePath = TEXT("C:/test/TestHUD.psd");

            // Add a text layer
            FPsdLayer TextLayer;
            TextLayer.Name = TEXT("Title");
            TextLayer.Type = EPsdLayerType::Text;
            TextLayer.Bounds = FIntRect(100, 50, 500, 100);
            TextLayer.bVisible = true;
            TextLayer.Text.Content = TEXT("Hello World");
            TextLayer.Text.SizePx = 24.f;
            TextLayer.Text.Color = FLinearColor::White;
            Doc.RootLayers.Add(TextLayer);

            // Add a group layer
            FPsdLayer GroupLayer;
            GroupLayer.Name = TEXT("Panel");
            GroupLayer.Type = EPsdLayerType::Group;
            GroupLayer.Bounds = FIntRect(0, 0, 960, 540);
            GroupLayer.bVisible = true;
            Doc.RootLayers.Add(GroupLayer);

            UWidgetBlueprint* WBP = FWidgetBlueprintGenerator::Generate(
                Doc, TEXT("/Game/_Test/WBPGen"), TEXT("WBP_TestHUD"));

            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;

            TestNotNull(TEXT("WidgetTree exists"), WBP->WidgetTree.Get());
            TestNotNull(TEXT("Root widget exists"), WBP->WidgetTree->RootWidget.Get());

            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            TestNotNull(TEXT("Root is UCanvasPanel"), Root);
            if (!Root) return;

            // Should have 2 children (text + group)
            TestEqual(TEXT("Root has 2 children"), Root->GetChildrenCount(), 2);
        });

        It("should skip invisible layers", [this]()
        {
            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(800, 600);
            Doc.SourcePath = TEXT("C:/test/Skip.psd");

            FPsdLayer Hidden;
            Hidden.Name = TEXT("HiddenLayer");
            Hidden.Type = EPsdLayerType::Text;
            Hidden.Bounds = FIntRect(0, 0, 100, 100);
            Hidden.bVisible = false;
            Hidden.Text.Content = TEXT("Hidden");
            Doc.RootLayers.Add(Hidden);

            UWidgetBlueprint* WBP = FWidgetBlueprintGenerator::Generate(
                Doc, TEXT("/Game/_Test/WBPGen"), TEXT("WBP_SkipTest"));

            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;

            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            TestNotNull(TEXT("Root exists"), Root);
            if (!Root) return;

            TestEqual(TEXT("Hidden layer skipped"), Root->GetChildrenCount(), 0);
        });

        It("should assign ZOrder inversely from layer index", [this]()
        {
            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(800, 600);
            Doc.SourcePath = TEXT("C:/test/ZOrder.psd");

            // 3 text layers: index 0 should get highest ZOrder
            for (int32 i = 0; i < 3; ++i)
            {
                FPsdLayer L;
                L.Name = FString::Printf(TEXT("Layer_%d"), i);
                L.Type = EPsdLayerType::Text;
                L.Bounds = FIntRect(i * 100, 0, i * 100 + 50, 50);
                L.bVisible = true;
                L.Text.Content = FString::Printf(TEXT("Text %d"), i);
                L.Text.SizePx = 16.f;
                Doc.RootLayers.Add(L);
            }

            UWidgetBlueprint* WBP = FWidgetBlueprintGenerator::Generate(
                Doc, TEXT("/Game/_Test/WBPGen"), TEXT("WBP_ZOrderTest"));

            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;

            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            if (!Root) return;

            // First layer (index 0) should have ZOrder 2 (= 3-1-0)
            UCanvasPanelSlot* Slot0 = Cast<UCanvasPanelSlot>(Root->GetChildAt(0)->Slot);
            if (Slot0)
            {
                TestEqual(TEXT("Layer 0 ZOrder = 2"), Slot0->GetZOrder(), 2);
            }
        });

        It("should create UImage for image layer with non-zero pixels", [this]()
        {
            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(800, 600);
            Doc.SourcePath = TEXT("C:/test/ImageTest.psd");

            FPsdLayer ImgLayer;
            ImgLayer.Name = TEXT("icon_star");
            ImgLayer.Type = EPsdLayerType::Image;
            ImgLayer.Bounds = FIntRect(100, 100, 164, 164);
            ImgLayer.bVisible = true;
            ImgLayer.PixelWidth = 64;
            ImgLayer.PixelHeight = 64;
            ImgLayer.RGBAPixels.SetNumZeroed(64 * 64 * 4);
            Doc.RootLayers.Add(ImgLayer);

            UWidgetBlueprint* WBP = FWidgetBlueprintGenerator::Generate(
                Doc, TEXT("/Game/_Test/WBPGen"), TEXT("WBP_ImageTest"));

            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;

            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            TestNotNull(TEXT("Root exists"), Root);
            if (!Root) return;

            TestEqual(TEXT("Image layer produced a child"), Root->GetChildrenCount(), 1);
            // Verify the child is a UImage
            UImage* ImgWidget = Cast<UImage>(Root->GetChildAt(0));
            TestNotNull(TEXT("Child is UImage"), ImgWidget);
        });
    });
}

#endif // WITH_DEV_AUTOMATION_TESTS
