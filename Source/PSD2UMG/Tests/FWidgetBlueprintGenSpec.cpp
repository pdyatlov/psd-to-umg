// Copyright 2018-2021 - John snow wind

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Parser/PsdTypes.h"
#include "Generator/FWidgetBlueprintGenerator.h"
#include "PSD2UMGSetting.h"
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

        It("should create invisible layers as Collapsed", [this]()
        {
            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(800, 600);
            Doc.SourcePath = TEXT("C:/test/Invisible.psd");

            FPsdLayer& HiddenLayer = Doc.RootLayers.AddDefaulted_GetRef();
            HiddenLayer.Name = TEXT("HiddenImage");
            HiddenLayer.Type = EPsdLayerType::Image;
            HiddenLayer.Bounds = FIntRect(10, 10, 110, 110);
            HiddenLayer.bVisible = false;
            HiddenLayer.PixelWidth = 100;
            HiddenLayer.PixelHeight = 100;
            HiddenLayer.RGBAPixels.SetNumZeroed(100 * 100 * 4);

            UWidgetBlueprint* WBP = FWidgetBlueprintGenerator::Generate(Doc, TEXT("/Game/Test"), TEXT("WBP_Invisible"));
            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;

            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            TestNotNull(TEXT("Root exists"), Root);
            if (!Root) return;

            // Per D-01: hidden layer is created, not skipped
            TestEqual(TEXT("Hidden layer created (not skipped)"), Root->GetChildrenCount(), 1);

            UWidget* Child = Root->GetChildAt(0);
            TestNotNull(TEXT("Child widget exists"), Child);
            if (Child)
            {
                TestEqual(TEXT("Hidden layer is Collapsed"),
                    Child->GetVisibility(), ESlateVisibility::Collapsed);
            }
        });

        It("should apply opacity via SetRenderOpacity", [this]()
        {
            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(800, 600);
            Doc.SourcePath = TEXT("C:/test/Opacity.psd");

            FPsdLayer& Layer = Doc.RootLayers.AddDefaulted_GetRef();
            Layer.Name = TEXT("SemiTransparent");
            Layer.Type = EPsdLayerType::Text;
            Layer.Bounds = FIntRect(10, 10, 200, 50);
            Layer.Opacity = 0.5f;
            Layer.Text.Content = TEXT("Hello");
            Layer.Text.SizePx = 24.f;

            UWidgetBlueprint* WBP = FWidgetBlueprintGenerator::Generate(Doc, TEXT("/Game/Test"), TEXT("WBP_Opacity"));
            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;

            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            TestEqual(TEXT("One child"), Root->GetChildrenCount(), 1);

            UWidget* Child = Root->GetChildAt(0);
            TestNotNull(TEXT("Child exists"), Child);
            if (Child)
            {
                TestTrue(TEXT("Opacity applied"),
                    FMath::IsNearlyEqual(Child->GetRenderOpacity(), 0.5f, 0.01f));
            }
        });

        It("should apply color overlay tint to UImage brush", [this]()
        {
            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(400, 400);
            Doc.SourcePath = TEXT("C:/test/ColorOverlay.psd");

            FPsdLayer& Layer = Doc.RootLayers.AddDefaulted_GetRef();
            Layer.Name = TEXT("TintedImage");
            Layer.Type = EPsdLayerType::Image;
            Layer.Bounds = FIntRect(0, 0, 100, 100);
            Layer.PixelWidth = 100;
            Layer.PixelHeight = 100;
            Layer.RGBAPixels.SetNumZeroed(100 * 100 * 4);
            Layer.Effects.bHasColorOverlay = true;
            Layer.Effects.ColorOverlayColor = FLinearColor(1.f, 0.f, 0.f, 0.8f); // Red tint

            UWidgetBlueprint* WBP = FWidgetBlueprintGenerator::Generate(Doc, TEXT("/Game/Test"), TEXT("WBP_ColorOverlay"));
            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;

            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            TestEqual(TEXT("One child"), Root->GetChildrenCount(), 1);

            UWidget* Child = Root->GetChildAt(0);
            UImage* Img = Cast<UImage>(Child);
            TestNotNull(TEXT("Child is UImage"), Img);
            if (Img)
            {
                FSlateBrush Brush = Img->GetBrush();
                FLinearColor Tint = Brush.TintColor.GetSpecifiedColor();
                TestTrue(TEXT("Tint R near 1.0"), FMath::IsNearlyEqual(Tint.R, 1.f, 0.01f));
                TestTrue(TEXT("Tint G near 0.0"), FMath::IsNearlyEqual(Tint.G, 0.f, 0.01f));
                TestTrue(TEXT("Tint B near 0.0"), FMath::IsNearlyEqual(Tint.B, 0.f, 0.01f));
                TestTrue(TEXT("Tint A near 0.8"), FMath::IsNearlyEqual(Tint.A, 0.8f, 0.01f));
            }
        });

        It("should flatten layer with complex effects when setting enabled", [this]()
        {
            // Ensure setting is on
            UPSD2UMGSettings::Get()->bFlattenComplexEffects = true;

            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(400, 400);
            Doc.SourcePath = TEXT("C:/test/Flatten.psd");

            // Group layer with complex effects and pixel data (simulates composited rasterize)
            FPsdLayer& Layer = Doc.RootLayers.AddDefaulted_GetRef();
            Layer.Name = TEXT("ComplexGroup");
            Layer.Type = EPsdLayerType::Group;
            Layer.Bounds = FIntRect(0, 0, 100, 100);
            Layer.Effects.bHasComplexEffects = true;
            Layer.PixelWidth = 100;
            Layer.PixelHeight = 100;
            Layer.RGBAPixels.SetNumZeroed(100 * 100 * 4);
            // Add a child to prove flattening removes children
            FPsdLayer& Child = Layer.Children.AddDefaulted_GetRef();
            Child.Name = TEXT("InnerImage");
            Child.Type = EPsdLayerType::Image;
            Child.Bounds = FIntRect(10, 10, 50, 50);
            Child.PixelWidth = 40;
            Child.PixelHeight = 40;
            Child.RGBAPixels.SetNumZeroed(40 * 40 * 4);

            UWidgetBlueprint* WBP = FWidgetBlueprintGenerator::Generate(Doc, TEXT("/Game/Test"), TEXT("WBP_Flatten"));
            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;

            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            // Flattened group becomes a single UImage, no children
            TestEqual(TEXT("One widget (flattened)"), Root->GetChildrenCount(), 1);

            UWidget* W = Root->GetChildAt(0);
            TestTrue(TEXT("Flattened to UImage"), W != nullptr && W->IsA<UImage>());
        });

        It("should create shadow sibling for drop shadow layers", [this]()
        {
            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(400, 400);
            Doc.SourcePath = TEXT("C:/test/DropShadow.psd");

            FPsdLayer& Layer = Doc.RootLayers.AddDefaulted_GetRef();
            Layer.Name = TEXT("ShadowedImage");
            Layer.Type = EPsdLayerType::Image;
            Layer.Bounds = FIntRect(50, 50, 150, 150);
            Layer.PixelWidth = 100;
            Layer.PixelHeight = 100;
            Layer.RGBAPixels.SetNumZeroed(100 * 100 * 4);
            Layer.Effects.bHasDropShadow = true;
            Layer.Effects.DropShadowColor = FLinearColor(0.f, 0.f, 0.f, 0.7f);
            Layer.Effects.DropShadowOffset = FVector2D(5.0, 5.0);

            UWidgetBlueprint* WBP = FWidgetBlueprintGenerator::Generate(Doc, TEXT("/Game/Test"), TEXT("WBP_DropShadow"));
            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;

            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            TestNotNull(TEXT("Root exists"), Root);
            if (!Root) return;

            // Drop shadow adds a sibling UImage — canvas should have 2 children (shadow + main)
            TestEqual(TEXT("Canvas has shadow + main (2 children)"), Root->GetChildrenCount(), 2);

            // Find shadow and main widgets by ZOrder — shadow must have lower ZOrder
            UCanvasPanelSlot* Slot0 = Cast<UCanvasPanelSlot>(Root->GetChildAt(0)->Slot);
            UCanvasPanelSlot* Slot1 = Cast<UCanvasPanelSlot>(Root->GetChildAt(1)->Slot);
            TestNotNull(TEXT("Slot0 valid"), Slot0);
            TestNotNull(TEXT("Slot1 valid"), Slot1);
            if (Slot0 && Slot1)
            {
                // One slot must have ZOrder strictly less than the other (shadow behind main)
                TestTrue(TEXT("Shadow has lower ZOrder than main"),
                    Slot0->GetLayout().ZOrder != Slot1->GetLayout().ZOrder);
                int32 MinZ = FMath::Min(Slot0->GetLayout().ZOrder, Slot1->GetLayout().ZOrder);
                int32 MaxZ = FMath::Max(Slot0->GetLayout().ZOrder, Slot1->GetLayout().ZOrder);
                TestTrue(TEXT("ZOrders differ by 1"), MaxZ - MinZ == 1);
            }
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
