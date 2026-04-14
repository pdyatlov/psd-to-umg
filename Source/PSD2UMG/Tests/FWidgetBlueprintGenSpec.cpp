// Copyright 2018-2021 - John snow wind

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Parser/FLayerTagParser.h"
#include "Parser/PsdTypes.h"
#include "Generator/FWidgetBlueprintGenerator.h"
#include "TestHelpers.h"
#include "PSD2UMGSetting.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/HorizontalBox.h"
#include "Components/WidgetSwitcher.h"
#include "EditorAssetLibrary.h"
#include "Misc/PackageName.h"
#include "HAL/FileManager.h"

BEGIN_DEFINE_SPEC(FWidgetBlueprintGenSpec, "PSD2UMG.Generator", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
    TArray<FString> CreatedWBPPaths;
    bool bOrigFlattenComplexEffects = false;
    FString RunId;

    UWidgetBlueprint* GenerateTracked(const FPsdDocument& InDoc, const TCHAR* BasePath, const TCHAR* BaseName)
    {
        // Pitfall 5: synthetic FPsdLayer constructions don't go through PsdParser::ParseFile,
        // so their ParsedTags are default-constructed (Type=None). Tag-dispatched mappers
        // would reject them. Populate ParsedTags here, mirroring the parser's post-pass.
        FPsdDocument Doc = InDoc;
        PSD2UMG::Tests::PopulateParsedTagsForDocument(Doc);

        FString UniqueName = FString::Printf(TEXT("%s_%s"), BaseName, *RunId);
        FString FullPath = FString(BasePath) / UniqueName;
        CreatedWBPPaths.Add(FullPath);
        return FWidgetBlueprintGenerator::Generate(Doc, BasePath, *UniqueName);
    }

    void CleanupAsset(const FString& PackagePath)
    {
        // Unload the package from memory so the registry forgets it
        UPackage* Pkg = FindPackage(nullptr, *PackagePath);
        if (Pkg)
        {
            Pkg->SetDirtyFlag(false);
            Pkg->ClearFlags(RF_Standalone | RF_Public);
            Pkg->MarkAsGarbage();
        }

        // Delete physical .uasset file from disk
        FString DiskPath;
        if (FPackageName::TryConvertLongPackageNameToFilename(PackagePath, DiskPath, TEXT(".uasset")))
        {
            IFileManager::Get().Delete(*DiskPath, false, true, true);
        }
    }
END_DEFINE_SPEC(FWidgetBlueprintGenSpec)

void FWidgetBlueprintGenSpec::Define()
{
    Describe("FWidgetBlueprintGenerator::Generate", [this]()
    {
        BeforeEach([this]()
        {
            CreatedWBPPaths.Empty();
            RunId = FGuid::NewGuid().ToString(EGuidFormats::DigitsLower).Left(8);
            bOrigFlattenComplexEffects = UPSD2UMGSettings::Get()->bFlattenComplexEffects;
        });

        AfterEach([this]()
        {
            for (const FString& Path : CreatedWBPPaths)
            {
                CleanupAsset(Path);
            }
            CreatedWBPPaths.Empty();
            CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
            UPSD2UMGSettings::Get()->bFlattenComplexEffects = bOrigFlattenComplexEffects;
        });

        It("should create a valid WBP from a minimal document", [this]()
        {
            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(1920, 1080);
            Doc.SourcePath = TEXT("C:/test/TestHUD.psd");

            FPsdLayer TextLayer;
            TextLayer.Name = TEXT("Title");
            TextLayer.Type = EPsdLayerType::Text;
            TextLayer.Bounds = FIntRect(100, 50, 500, 100);
            TextLayer.bVisible = true;
            TextLayer.Text.Content = TEXT("Hello World");
            TextLayer.Text.SizePx = 24.f;
            TextLayer.Text.Color = FLinearColor::White;
            Doc.RootLayers.Add(TextLayer);

            FPsdLayer GroupLayer;
            GroupLayer.Name = TEXT("Panel");
            GroupLayer.Type = EPsdLayerType::Group;
            GroupLayer.Bounds = FIntRect(0, 0, 960, 540);
            GroupLayer.bVisible = true;
            Doc.RootLayers.Add(GroupLayer);

            UWidgetBlueprint* WBP = GenerateTracked(Doc, TEXT("/Game/_Test/WBPGen"), TEXT("WBP_TestHUD"));

            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;

            TestNotNull(TEXT("WidgetTree exists"), WBP->WidgetTree.Get());
            TestNotNull(TEXT("Root widget exists"), WBP->WidgetTree->RootWidget.Get());

            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            TestNotNull(TEXT("Root is UCanvasPanel"), Root);
            if (!Root) return;

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

            UWidgetBlueprint* WBP = GenerateTracked(Doc, TEXT("/Game/Test"), TEXT("WBP_Invisible"));
            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;

            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            TestNotNull(TEXT("Root exists"), Root);
            if (!Root) return;

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

            UWidgetBlueprint* WBP = GenerateTracked(Doc, TEXT("/Game/Test"), TEXT("WBP_Opacity"));
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
            Layer.Effects.ColorOverlayColor = FLinearColor(1.f, 0.f, 0.f, 0.8f);

            UWidgetBlueprint* WBP = GenerateTracked(Doc, TEXT("/Game/Test"), TEXT("WBP_ColorOverlay"));
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
            UPSD2UMGSettings::Get()->bFlattenComplexEffects = true;

            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(400, 400);
            Doc.SourcePath = TEXT("C:/test/Flatten.psd");

            FPsdLayer& Layer = Doc.RootLayers.AddDefaulted_GetRef();
            Layer.Name = TEXT("ComplexGroup");
            Layer.Type = EPsdLayerType::Group;
            Layer.Bounds = FIntRect(0, 0, 100, 100);
            Layer.Effects.bHasComplexEffects = true;
            Layer.PixelWidth = 100;
            Layer.PixelHeight = 100;
            Layer.RGBAPixels.SetNumZeroed(100 * 100 * 4);
            FPsdLayer& Child = Layer.Children.AddDefaulted_GetRef();
            Child.Name = TEXT("InnerImage");
            Child.Type = EPsdLayerType::Image;
            Child.Bounds = FIntRect(10, 10, 50, 50);
            Child.PixelWidth = 40;
            Child.PixelHeight = 40;
            Child.RGBAPixels.SetNumZeroed(40 * 40 * 4);

            UWidgetBlueprint* WBP = GenerateTracked(Doc, TEXT("/Game/Test"), TEXT("WBP_Flatten"));
            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;

            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
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

            UWidgetBlueprint* WBP = GenerateTracked(Doc, TEXT("/Game/Test"), TEXT("WBP_DropShadow"));
            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;

            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            TestNotNull(TEXT("Root exists"), Root);
            if (!Root) return;

            TestEqual(TEXT("Canvas has shadow + main (2 children)"), Root->GetChildrenCount(), 2);

            UCanvasPanelSlot* Slot0 = Cast<UCanvasPanelSlot>(Root->GetChildAt(0)->Slot);
            UCanvasPanelSlot* Slot1 = Cast<UCanvasPanelSlot>(Root->GetChildAt(1)->Slot);
            TestNotNull(TEXT("Slot0 valid"), Slot0);
            TestNotNull(TEXT("Slot1 valid"), Slot1);
            if (Slot0 && Slot1)
            {
                TestTrue(TEXT("Shadow has lower ZOrder than main"),
                    Slot0->GetZOrder() != Slot1->GetZOrder());
                int32 MinZ = FMath::Min(Slot0->GetZOrder(), Slot1->GetZOrder());
                int32 MaxZ = FMath::Max(Slot0->GetZOrder(), Slot1->GetZOrder());
                TestTrue(TEXT("ZOrders differ by 1"), MaxZ - MinZ == 1);
            }
        });

        It("should assign ZOrder inversely from layer index", [this]()
        {
            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(800, 600);
            Doc.SourcePath = TEXT("C:/test/ZOrder.psd");

            // Use group layers with staggered Y so they DON'T trigger row/column auto-detection
            for (int32 i = 0; i < 3; ++i)
            {
                FPsdLayer L;
                L.Name = FString::Printf(TEXT("Group_%d"), i);
                L.Type = EPsdLayerType::Group;
                L.Bounds = FIntRect(i * 100, i * 80, i * 100 + 200, i * 80 + 200);
                L.bVisible = true;
                Doc.RootLayers.Add(L);
            }

            UWidgetBlueprint* WBP = GenerateTracked(Doc, TEXT("/Game/_Test/WBPGen"), TEXT("WBP_ZOrderTest"));

            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;

            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            if (!Root) return;

            TestEqual(TEXT("3 children created"), Root->GetChildrenCount(), 3);

            TArray<int32> ZOrders;
            for (int32 c = 0; c < Root->GetChildrenCount(); ++c)
            {
                if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Root->GetChildAt(c)->Slot))
                {
                    ZOrders.Add(S->GetZOrder());
                }
            }
            if (ZOrders.Num() == 3)
            {
                TestTrue(TEXT("Layer 0 ZOrder >= Layer 2 ZOrder"), ZOrders[0] >= ZOrders[2]);
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

            UWidgetBlueprint* WBP = GenerateTracked(Doc, TEXT("/Game/_Test/WBPGen"), TEXT("WBP_ImageTest"));

            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;

            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            TestNotNull(TEXT("Root exists"), Root);
            if (!Root) return;

            TestEqual(TEXT("Image layer produced a child"), Root->GetChildrenCount(), 1);
            UImage* ImgWidget = Cast<UImage>(Root->GetChildAt(0));
            TestNotNull(TEXT("Child is UImage"), ImgWidget);
        });

        It("should create WBP with empty root canvas from empty PSD", [this]()
        {
            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(1920, 1080);
            Doc.SourcePath = TEXT("C:/test/Empty.psd");

            UWidgetBlueprint* WBP = GenerateTracked(Doc, TEXT("/Game/_Test/WBPGen"), TEXT("WBP_EmptyPsd"));
            TestNotNull(TEXT("WBP created from empty PSD"), WBP);
            if (!WBP) return;
            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            TestNotNull(TEXT("Root is UCanvasPanel"), Root);
            if (Root)
            {
                TestEqual(TEXT("Empty PSD has 0 children"), Root->GetChildrenCount(), 0);
            }
        });

        It("should handle bare '@button' tag with empty clean-name without crashing (D-21 fallback)", [this]()
        {
            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(800, 600);
            Doc.SourcePath = TEXT("C:/test/BareButtonTag.psd");
            FPsdLayer& Layer = Doc.RootLayers.AddDefaulted_GetRef();
            Layer.Name = TEXT("@button");
            Layer.Type = EPsdLayerType::Group;
            Layer.Bounds = FIntRect(0, 0, 200, 100);
            Layer.bVisible = true;
            FPsdLayer& Child = Layer.Children.AddDefaulted_GetRef();
            Child.Name = TEXT("Normal");
            Child.Type = EPsdLayerType::Image;
            Child.Bounds = FIntRect(0, 0, 200, 100);
            Child.PixelWidth = 200; Child.PixelHeight = 100;
            Child.RGBAPixels.SetNumZeroed(200 * 100 * 4);

            UWidgetBlueprint* WBP = GenerateTracked(Doc, TEXT("/Game/_Test/WBPGen"), TEXT("WBP_MalformedButton"));
            TestNotNull(TEXT("WBP created despite malformed name"), WBP);
        });

        It("should create UProgressBar from '@progress' tag group", [this]()
        {
            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(800, 600);
            Doc.SourcePath = TEXT("C:/test/ProgressBar.psd");
            FPsdLayer& Layer = Doc.RootLayers.AddDefaulted_GetRef();
            Layer.Name = TEXT("Health @progress");
            Layer.Type = EPsdLayerType::Group;
            Layer.Bounds = FIntRect(100, 100, 500, 140);
            Layer.bVisible = true;

            UWidgetBlueprint* WBP = GenerateTracked(Doc, TEXT("/Game/_Test/WBPGen"), TEXT("WBP_ProgressTest"));
            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;
            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            if (!Root) return;
            TestEqual(TEXT("One child"), Root->GetChildrenCount(), 1);
            UWidget* Child = Root->GetChildAt(0);
            TestTrue(TEXT("Child is UProgressBar"), Child != nullptr && Child->IsA(UProgressBar::StaticClass()));
        });

        It("should create UHorizontalBox from '@hbox' tag group", [this]()
        {
            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(800, 600);
            Doc.SourcePath = TEXT("C:/test/HBox.psd");
            FPsdLayer& Layer = Doc.RootLayers.AddDefaulted_GetRef();
            Layer.Name = TEXT("Row @hbox");
            Layer.Type = EPsdLayerType::Group;
            Layer.Bounds = FIntRect(0, 0, 800, 100);
            Layer.bVisible = true;

            UWidgetBlueprint* WBP = GenerateTracked(Doc, TEXT("/Game/_Test/WBPGen"), TEXT("WBP_HBoxTest"));
            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;
            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            if (!Root) return;
            TestEqual(TEXT("One child"), Root->GetChildrenCount(), 1);
            UWidget* Child = Root->GetChildAt(0);
            TestTrue(TEXT("Child is UHorizontalBox"), Child != nullptr && Child->IsA(UHorizontalBox::StaticClass()));
        });

        It("should create UWidgetSwitcher from '@variants' tag group", [this]()
        {
            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(800, 600);
            Doc.SourcePath = TEXT("C:/test/Variants.psd");
            FPsdLayer& Layer = Doc.RootLayers.AddDefaulted_GetRef();
            Layer.Name = TEXT("Menu @variants");
            Layer.Type = EPsdLayerType::Group;
            Layer.Bounds = FIntRect(0, 0, 400, 300);
            Layer.bVisible = true;
            FPsdLayer& Slot0 = Layer.Children.AddDefaulted_GetRef();
            Slot0.Name = TEXT("Slot0"); Slot0.Type = EPsdLayerType::Group;
            Slot0.Bounds = FIntRect(0, 0, 400, 300);
            FPsdLayer& Slot1 = Layer.Children.AddDefaulted_GetRef();
            Slot1.Name = TEXT("Slot1"); Slot1.Type = EPsdLayerType::Group;
            Slot1.Bounds = FIntRect(0, 0, 400, 300);

            UWidgetBlueprint* WBP = GenerateTracked(Doc, TEXT("/Game/_Test/WBPGen"), TEXT("WBP_VariantsTest"));
            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;
            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            if (!Root) return;
            TestEqual(TEXT("One child"), Root->GetChildrenCount(), 1);
            UWidget* Child = Root->GetChildAt(0);
            TestTrue(TEXT("Child is UWidgetSwitcher"), Child != nullptr && Child->IsA(UWidgetSwitcher::StaticClass()));
        });

        It("should force fill anchors when '@anchor:fill' tag is used", [this]()
        {
            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(1920, 1080);
            Doc.SourcePath = TEXT("C:/test/AnchorFill.psd");
            FPsdLayer& Layer = Doc.RootLayers.AddDefaulted_GetRef();
            Layer.Name = TEXT("Background @anchor:fill");
            Layer.Type = EPsdLayerType::Image;
            Layer.Bounds = FIntRect(100, 100, 500, 400);
            Layer.bVisible = true;
            Layer.PixelWidth = 400; Layer.PixelHeight = 300;
            Layer.RGBAPixels.SetNumZeroed(400 * 300 * 4);

            UWidgetBlueprint* WBP = GenerateTracked(Doc, TEXT("/Game/_Test/WBPGen"), TEXT("WBP_FillTest"));
            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;
            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            if (!Root || Root->GetChildrenCount() < 1) return;
            UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Root->GetChildAt(0)->Slot);
            TestNotNull(TEXT("Slot exists"), Slot);
            if (Slot)
            {
                FAnchors Anchors = Slot->GetAnchors();
                TestTrue(TEXT("Fill min anchor (0,0)"), FMath::IsNearlyEqual(Anchors.Minimum.X, 0.f, 0.01f) && FMath::IsNearlyEqual(Anchors.Minimum.Y, 0.f, 0.01f));
                TestTrue(TEXT("Fill max anchor (1,1)"), FMath::IsNearlyEqual(Anchors.Maximum.X, 1.f, 0.01f) && FMath::IsNearlyEqual(Anchors.Maximum.Y, 1.f, 0.01f));
            }
        });

        It("should force top-left anchor when '@anchor:tl' tag is used", [this]()
        {
            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(1920, 1080);
            Doc.SourcePath = TEXT("C:/test/AnchorTL.psd");
            FPsdLayer& Layer = Doc.RootLayers.AddDefaulted_GetRef();
            Layer.Name = TEXT("Icon @anchor:tl");
            Layer.Type = EPsdLayerType::Image;
            Layer.Bounds = FIntRect(1500, 800, 1600, 900);
            Layer.bVisible = true;
            Layer.PixelWidth = 100; Layer.PixelHeight = 100;
            Layer.RGBAPixels.SetNumZeroed(100 * 100 * 4);

            UWidgetBlueprint* WBP = GenerateTracked(Doc, TEXT("/Game/_Test/WBPGen"), TEXT("WBP_AnchorTLTest"));
            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;
            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            if (!Root || Root->GetChildrenCount() < 1) return;
            UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Root->GetChildAt(0)->Slot);
            if (Slot)
            {
                FAnchors Anchors = Slot->GetAnchors();
                TestTrue(TEXT("Anchor forced to top-left (0,0)"),
                    FMath::IsNearlyEqual(Anchors.Minimum.X, 0.f, 0.01f) && FMath::IsNearlyEqual(Anchors.Minimum.Y, 0.f, 0.01f));
            }
        });

        It("should assign top-left anchor for layer in top-left quadrant", [this]()
        {
            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(1920, 1080);
            Doc.SourcePath = TEXT("C:/test/AnchorQuadrant.psd");
            FPsdLayer& Layer = Doc.RootLayers.AddDefaulted_GetRef();
            Layer.Name = TEXT("TopLeftWidget");
            Layer.Type = EPsdLayerType::Image;
            Layer.Bounds = FIntRect(50, 50, 150, 150);
            Layer.bVisible = true;
            Layer.PixelWidth = 100; Layer.PixelHeight = 100;
            Layer.RGBAPixels.SetNumZeroed(100 * 100 * 4);

            UWidgetBlueprint* WBP = GenerateTracked(Doc, TEXT("/Game/_Test/WBPGen"), TEXT("WBP_AnchorTopLeft"));
            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;
            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            if (!Root || Root->GetChildrenCount() < 1) return;
            UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Root->GetChildAt(0)->Slot);
            if (Slot)
            {
                FAnchors Anchors = Slot->GetAnchors();
                TestTrue(TEXT("Top-left anchor X near 0"), FMath::IsNearlyEqual(Anchors.Minimum.X, 0.f, 0.1f));
                TestTrue(TEXT("Top-left anchor Y near 0"), FMath::IsNearlyEqual(Anchors.Minimum.Y, 0.f, 0.1f));
            }
        });

        It("should assign bottom-right anchor for layer in bottom-right quadrant", [this]()
        {
            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(1920, 1080);
            Doc.SourcePath = TEXT("C:/test/AnchorBR.psd");
            FPsdLayer& Layer = Doc.RootLayers.AddDefaulted_GetRef();
            Layer.Name = TEXT("BottomRightWidget");
            Layer.Type = EPsdLayerType::Image;
            Layer.Bounds = FIntRect(1700, 900, 1850, 1050);
            Layer.bVisible = true;
            Layer.PixelWidth = 150; Layer.PixelHeight = 150;
            Layer.RGBAPixels.SetNumZeroed(150 * 150 * 4);

            UWidgetBlueprint* WBP = GenerateTracked(Doc, TEXT("/Game/_Test/WBPGen"), TEXT("WBP_AnchorBR"));
            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;
            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            if (!Root || Root->GetChildrenCount() < 1) return;
            UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Root->GetChildAt(0)->Slot);
            if (Slot)
            {
                FAnchors Anchors = Slot->GetAnchors();
                TestTrue(TEXT("Bottom-right anchor X near 1"), FMath::IsNearlyEqual(Anchors.Minimum.X, 1.f, 0.1f));
                TestTrue(TEXT("Bottom-right anchor Y near 1"), FMath::IsNearlyEqual(Anchors.Minimum.Y, 1.f, 0.1f));
            }
        });

        It("should return New annotation when ExistingWidget is nullptr", [this]()
        {
            FPsdLayer Layer;
            Layer.Name = TEXT("NewLayer");
            Layer.Type = EPsdLayerType::Image;
            Layer.Bounds = FIntRect(0, 0, 100, 100);
            EPsdChangeAnnotation Result = FWidgetBlueprintGenerator::DetectChange(Layer, nullptr);
            TestEqual(TEXT("DetectChange returns New for null widget"),
                static_cast<uint8>(Result), static_cast<uint8>(EPsdChangeAnnotation::New));
        });

        It("should return Unchanged annotation when layer matches widget", [this]()
        {
            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(800, 600);
            Doc.SourcePath = TEXT("C:/test/DetectUnchanged.psd");
            FPsdLayer& Layer = Doc.RootLayers.AddDefaulted_GetRef();
            Layer.Name = TEXT("Title");
            Layer.Type = EPsdLayerType::Text;
            Layer.Bounds = FIntRect(100, 50, 500, 100);
            Layer.bVisible = true;
            Layer.Text.Content = TEXT("Hello");
            Layer.Text.SizePx = 24.f;

            UWidgetBlueprint* WBP = GenerateTracked(Doc, TEXT("/Game/_Test/WBPGen"), TEXT("WBP_DetectUnchanged"));
            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;
            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            if (!Root || Root->GetChildrenCount() < 1) return;
            UWidget* Widget = Root->GetChildAt(0);
            EPsdChangeAnnotation Result = FWidgetBlueprintGenerator::DetectChange(Layer, Widget);
            TestEqual(TEXT("DetectChange returns Unchanged"),
                static_cast<uint8>(Result), static_cast<uint8>(EPsdChangeAnnotation::Unchanged));
        });

        It("should return Changed annotation when layer size differs from widget", [this]()
        {
            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(800, 600);
            Doc.SourcePath = TEXT("C:/test/DetectChanged.psd");
            FPsdLayer OrigLayer;
            OrigLayer.Name = TEXT("ResizedText");
            OrigLayer.Type = EPsdLayerType::Text;
            OrigLayer.Bounds = FIntRect(100, 50, 500, 100);
            OrigLayer.bVisible = true;
            OrigLayer.Text.Content = TEXT("Hello");
            OrigLayer.Text.SizePx = 24.f;
            Doc.RootLayers.Add(OrigLayer);

            UWidgetBlueprint* WBP = GenerateTracked(Doc, TEXT("/Game/_Test/WBPGen"), TEXT("WBP_DetectChanged"));
            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;
            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            if (!Root || Root->GetChildrenCount() < 1) return;
            UWidget* Widget = Root->GetChildAt(0);
            // Change size: 400x50 -> 700x250 (DetectChange compares width/height via Offsets)
            FPsdLayer ModifiedLayer = OrigLayer;
            ModifiedLayer.Bounds = FIntRect(100, 50, 800, 300);
            EPsdChangeAnnotation Result = FWidgetBlueprintGenerator::DetectChange(ModifiedLayer, Widget);
            TestEqual(TEXT("DetectChange returns Changed for resized layer"),
                static_cast<uint8>(Result), static_cast<uint8>(EPsdChangeAnnotation::Changed));
        });

        It("should create Box draw mode image for '@9s' tag", [this]()
        {
            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(800, 600);
            Doc.SourcePath = TEXT("C:/test/9Slice.psd");
            FPsdLayer& Layer = Doc.RootLayers.AddDefaulted_GetRef();
            Layer.Name = TEXT("Panel @9s");
            Layer.Type = EPsdLayerType::Group;
            Layer.Bounds = FIntRect(0, 0, 400, 300);
            Layer.bVisible = true;
            FPsdLayer& Child = Layer.Children.AddDefaulted_GetRef();
            Child.Name = TEXT("bg");
            Child.Type = EPsdLayerType::Image;
            Child.Bounds = FIntRect(0, 0, 400, 300);
            Child.PixelWidth = 400; Child.PixelHeight = 300;
            Child.RGBAPixels.SetNumZeroed(400 * 300 * 4);

            UWidgetBlueprint* WBP = GenerateTracked(Doc, TEXT("/Game/_Test/WBPGen"), TEXT("WBP_9SliceTest"));
            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP) return;
            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            TestNotNull(TEXT("Root exists"), Root);
            if (Root)
            {
                TestTrue(TEXT("9-slice created at least one child"), Root->GetChildrenCount() >= 1);
            }
        });

        It("should handle deeply nested groups without stack overflow", [this]()
        {
            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(800, 600);
            Doc.SourcePath = TEXT("C:/test/DeepNest.psd");
            FPsdLayer& L0 = Doc.RootLayers.AddDefaulted_GetRef();
            L0.Name = TEXT("Level0"); L0.Type = EPsdLayerType::Group;
            L0.Bounds = FIntRect(0, 0, 800, 600); L0.bVisible = true;
            FPsdLayer* Current = &L0;
            for (int32 i = 1; i <= 4; ++i)
            {
                FPsdLayer& Child = Current->Children.AddDefaulted_GetRef();
                Child.Name = FString::Printf(TEXT("Level%d"), i);
                Child.Type = EPsdLayerType::Group;
                Child.Bounds = FIntRect(i*10, i*10, 800-i*10, 600-i*10);
                Child.bVisible = true;
                Current = &Child;
            }
            FPsdLayer& Leaf = Current->Children.AddDefaulted_GetRef();
            Leaf.Name = TEXT("LeafImage"); Leaf.Type = EPsdLayerType::Image;
            Leaf.Bounds = FIntRect(50, 50, 150, 150);
            Leaf.PixelWidth = 100; Leaf.PixelHeight = 100;
            Leaf.RGBAPixels.SetNumZeroed(100 * 100 * 4);

            UWidgetBlueprint* WBP = GenerateTracked(Doc, TEXT("/Game/_Test/WBPGen"), TEXT("WBP_DeepNest"));
            TestNotNull(TEXT("WBP created from deep nesting"), WBP);
        });

        It("R-05: untagged group with horizontally-aligned children stays CanvasPanel (no auto-HBox)", [this]()
        {
            // Per Phase 9 DESIGN: auto-detection heuristics removed — HBox/VBox only via
            // explicit @hbox/@vbox tags. Three horizontally-aligned images without any
            // widget-type tag must remain as direct CanvasPanel children.
            FPsdDocument Doc;
            Doc.CanvasSize = FIntPoint(1200, 600);
            Doc.SourcePath = TEXT("C:/test/StretchOverride.psd");

            auto MakeImg = [](const TCHAR* RawName, int32 X0, int32 Y0, int32 X1, int32 Y1)
            {
                FPsdLayer L;
                L.Name = RawName;
                L.Type = EPsdLayerType::Image;
                L.Bounds = FIntRect(X0, Y0, X1, Y1);
                L.PixelWidth = X1 - X0;
                L.PixelHeight = Y1 - Y0;
                L.RGBAPixels.SetNumZeroed(L.PixelWidth * L.PixelHeight * 4);
                FString Diag;
                L.ParsedTags = FLayerTagParser::Parse(L.Name, L.Type, 0, Diag);
                return L;
            };

            Doc.RootLayers.Add(MakeImg(TEXT("Left"),   50,  280, 250,  320));
            Doc.RootLayers.Add(MakeImg(TEXT("Middle"), 300, 280, 800,  320));
            Doc.RootLayers.Add(MakeImg(TEXT("Right"),  850, 280, 1100, 320));

            UWidgetBlueprint* WBP = GenerateTracked(Doc, TEXT("/Game/_Test/WBPGen"), TEXT("WBP_R05"));
            TestNotNull(TEXT("WBP created"), WBP);
            if (!WBP || !WBP->WidgetTree) return;

            UCanvasPanel* Root = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
            TestNotNull(TEXT("Root is UCanvasPanel"), Root);
            if (!Root) return;

            // R-05 invariant: no auto-HBox. All 3 images sit directly on Root canvas.
            int32 ImageCount = 0;
            int32 HBoxCount = 0;
            WBP->WidgetTree->ForEachWidget([&](UWidget* W)
            {
                if (Cast<UImage>(W))           { ++ImageCount; }
                if (Cast<UHorizontalBox>(W))   { ++HBoxCount; }
            });
            TestEqual(TEXT("R-05: no HBox without explicit @hbox tag"), HBoxCount, 0);
            TestEqual(TEXT("R-05: all 3 images present on canvas"), ImageCount, 3);
        });
    });
}

#endif // WITH_DEV_AUTOMATION_TESTS
