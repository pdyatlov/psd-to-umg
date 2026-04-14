// Copyright 2018-2021 - John snow wind
#include "Generator/FWidgetBlueprintGenerator.h"

#include "Generator/FAnchorCalculator.h"
#include "Generator/FSmartObjectImporter.h"
#include "Mapper/FLayerMappingRegistry.h"
#include "Parser/FLayerTagParser.h"
#include "Parser/PsdTypes.h"
#include "PSD2UMGLog.h"
#include "PSD2UMGSetting.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/UObjectGlobals.h"
#include "WidgetBlueprintFactory.h"
#include "Blueprint/UserWidget.h"
#include "EditorAssetLibrary.h"
#include "Engine/Blueprint.h"
#include "WidgetBlueprint.h"

// ---------------------------------------------------------------------------
// LAYOUT-03: Row/column alignment constants and detection helpers
// ---------------------------------------------------------------------------
static constexpr int32 AlignmentTolerancePx = 6;

// R-05: a layer with an explicit stretch/fill anchor is placed directly on the
// canvas; the auto-row/column heuristic must not re-parent it into an HBox/VBox.
static bool IsExplicitStretchAnchor(const FPsdLayer& L)
{
    return L.ParsedTags.Anchor == EPsdAnchorTag::StretchH
        || L.ParsedTags.Anchor == EPsdAnchorTag::StretchV
        || L.ParsedTags.Anchor == EPsdAnchorTag::Fill;
}

static bool AnyChildHasExplicitStretch(const TArray<FPsdLayer>& Layers)
{
    for (const FPsdLayer& L : Layers)
    {
        if (IsExplicitStretchAnchor(L)) return true;
    }
    return false;
}

/** Returns true if all visual layers share the same vertical center (within Tolerance), forming a horizontal row. */
static bool DetectHorizontalRow(const TArray<FPsdLayer>& Layers, int32 Tolerance)
{
    if (Layers.Num() < 2) return false;

    // Collect visual layers (skip zero-size non-groups and Unknown types)
    TArray<const FPsdLayer*> Visual;
    for (const FPsdLayer& L : Layers)
    {
        if (L.Type == EPsdLayerType::Unknown) continue;
        if (L.Bounds.IsEmpty() && L.Type != EPsdLayerType::Group) continue;
        Visual.Add(&L);
    }
    if (Visual.Num() < 2) return false;

    // Compute vertical center of first visual layer
    const float RefCenterY = (static_cast<float>(Visual[0]->Bounds.Min.Y) + static_cast<float>(Visual[0]->Bounds.Max.Y)) * 0.5f;

    for (int32 i = 1; i < Visual.Num(); ++i)
    {
        const float CenterY = (static_cast<float>(Visual[i]->Bounds.Min.Y) + static_cast<float>(Visual[i]->Bounds.Max.Y)) * 0.5f;
        if (FMath::Abs(CenterY - RefCenterY) > static_cast<float>(Tolerance))
        {
            return false;
        }
    }
    return true;
}

/** Returns true if all visual layers share the same horizontal center (within Tolerance), forming a vertical column. */
static bool DetectVerticalColumn(const TArray<FPsdLayer>& Layers, int32 Tolerance)
{
    if (Layers.Num() < 2) return false;

    TArray<const FPsdLayer*> Visual;
    for (const FPsdLayer& L : Layers)
    {
        if (L.Type == EPsdLayerType::Unknown) continue;
        if (L.Bounds.IsEmpty() && L.Type != EPsdLayerType::Group) continue;
        Visual.Add(&L);
    }
    if (Visual.Num() < 2) return false;

    const float RefCenterX = (static_cast<float>(Visual[0]->Bounds.Min.X) + static_cast<float>(Visual[0]->Bounds.Max.X)) * 0.5f;

    for (int32 i = 1; i < Visual.Num(); ++i)
    {
        const float CenterX = (static_cast<float>(Visual[i]->Bounds.Min.X) + static_cast<float>(Visual[i]->Bounds.Max.X)) * 0.5f;
        if (FMath::Abs(CenterX - RefCenterX) > static_cast<float>(Tolerance))
        {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Internal helper: recursively populate a canvas panel from a layer array
// ---------------------------------------------------------------------------
static void PopulateCanvas(
    FLayerMappingRegistry& Registry,
    UWidgetTree* Tree,
    UCanvasPanel* Parent,
    const TArray<FPsdLayer>& Layers,
    const FPsdDocument& Doc,
    const FIntPoint& CanvasSize,
    const TSet<FString>& SkippedLayerNames = TSet<FString>())
{
    // LAYOUT-03: Auto-detect row/column alignment (per D-09, D-11)
    // Only fires when ALL children align — conservative guard against false positives.
    // R-05: if any child carries an explicit @anchor:stretch-h / stretch-v / fill,
    // the auto-grouping heuristic is suppressed entirely so the explicit anchor wins.
    if (Layers.Num() >= 2 && !AnyChildHasExplicitStretch(Layers))
    {
        if (DetectHorizontalRow(Layers, AlignmentTolerancePx))
        {
            UE_LOG(LogPSD2UMG, Log, TEXT("PopulateCanvas: detected horizontal row (%d layers) — wrapping in UHorizontalBox"), Layers.Num());

            UHorizontalBox* HBox = Tree->ConstructWidget<UHorizontalBox>();
            UCanvasPanelSlot* HBoxSlot = Parent->AddChildToCanvas(HBox);
            if (HBoxSlot)
            {
                // Fill entire parent canvas so children share the same coordinate origin
                FAnchorData HBoxData;
                HBoxData.Anchors = FAnchors(0.f, 0.f, 1.f, 1.f);
                HBoxData.Offsets = FMargin(0.f, 0.f, 0.f, 0.f);
                HBoxData.Alignment = FVector2D(0.f, 0.f);
                HBoxSlot->SetLayout(HBoxData);
            }

            // Sort layers left-to-right by X min
            TArray<FPsdLayer> Sorted = Layers;
            Sorted.Sort([](const FPsdLayer& A, const FPsdLayer& B)
            {
                return A.Bounds.Min.X < B.Bounds.Min.X;
            });

            for (const FPsdLayer& Layer : Sorted)
            {
                UWidget* Child = Registry.MapLayer(Layer, Doc, Tree);
                if (Child)
                {
                    UHorizontalBoxSlot* ChildSlot = HBox->AddChildToHorizontalBox(Child);
                    if (ChildSlot)
                    {
                        ChildSlot->SetPadding(FMargin(0.f));
                        ChildSlot->SetHorizontalAlignment(HAlign_Fill);
                        ChildSlot->SetVerticalAlignment(VAlign_Fill);
                    }
                }
            }
            return; // Skip normal CanvasPanel loop
        }
        else if (DetectVerticalColumn(Layers, AlignmentTolerancePx))
        {
            UE_LOG(LogPSD2UMG, Log, TEXT("PopulateCanvas: detected vertical column (%d layers) — wrapping in UVerticalBox"), Layers.Num());

            UVerticalBox* VBox = Tree->ConstructWidget<UVerticalBox>();
            UCanvasPanelSlot* VBoxSlot = Parent->AddChildToCanvas(VBox);
            if (VBoxSlot)
            {
                FAnchorData VBoxData;
                VBoxData.Anchors = FAnchors(0.f, 0.f, 1.f, 1.f);
                VBoxData.Offsets = FMargin(0.f, 0.f, 0.f, 0.f);
                VBoxData.Alignment = FVector2D(0.f, 0.f);
                VBoxSlot->SetLayout(VBoxData);
            }

            // Sort layers top-to-bottom by Y min
            TArray<FPsdLayer> Sorted = Layers;
            Sorted.Sort([](const FPsdLayer& A, const FPsdLayer& B)
            {
                return A.Bounds.Min.Y < B.Bounds.Min.Y;
            });

            for (const FPsdLayer& Layer : Sorted)
            {
                UWidget* Child = Registry.MapLayer(Layer, Doc, Tree);
                if (Child)
                {
                    UVerticalBoxSlot* ChildSlot = VBox->AddChildToVerticalBox(Child);
                    if (ChildSlot)
                    {
                        ChildSlot->SetPadding(FMargin(0.f));
                        ChildSlot->SetHorizontalAlignment(HAlign_Fill);
                        ChildSlot->SetVerticalAlignment(VAlign_Fill);
                    }
                }
            }
            return; // Skip normal CanvasPanel loop
        }
    }

    const int32 TotalLayers = Layers.Num();
    for (int32 i = 0; i < TotalLayers; ++i)
    {
        const FPsdLayer& Layer = Layers[i];

        // Skip layers the user unchecked in the preview dialog
        if (SkippedLayerNames.Contains(Layer.Name))
        {
            continue;
        }

        // Skip zero-size non-groups (D-14)
        if (Layer.Bounds.IsEmpty() && Layer.Type != EPsdLayerType::Group)
        {
            UE_LOG(LogPSD2UMG, Warning, TEXT("Skipping zero-size layer: %s"), *Layer.Name);
            continue;
        }

        // FX-05: Flatten fallback — if layer has complex effects and setting enabled,
        // force it to be treated as a plain image (its RGBAPixels already contain
        // pixel data from the parser's ImageLayer extraction path).
        bool bFlattened = false;
        FPsdLayer FlattenedLayer; // copy only if needed
        const FPsdLayer* LayerPtr = &Layer;
        if (Layer.Effects.bHasComplexEffects
            && Layer.RGBAPixels.Num() > 0
            && UPSD2UMGSettings::Get()->bFlattenComplexEffects)
        {
            FlattenedLayer = Layer;
            FlattenedLayer.Type = EPsdLayerType::Image;
            FlattenedLayer.Children.Empty(); // flatten = no children
            LayerPtr = &FlattenedLayer;
            bFlattened = true;
            UE_LOG(LogPSD2UMG, Log, TEXT("Layer '%s' flattened due to complex effects"), *Layer.Name);
        }
        else if (Layer.Effects.bHasComplexEffects && Layer.RGBAPixels.Num() == 0)
        {
            UE_LOG(LogPSD2UMG, Warning, TEXT("Layer '%s' has complex effects but no pixel data for flatten — effects ignored (per D-08 pattern)"), *Layer.Name);
        }

        UWidget* Widget = Registry.MapLayer(*LayerPtr, Doc, Tree);
        if (!Widget)
        {
            UE_LOG(LogPSD2UMG, Warning, TEXT("No mapper for layer: %s (type=%d)"), *Layer.Name, static_cast<int32>(Layer.Type));
            continue; // D-08: skip + warn
        }

        // Add to canvas and configure slot
        UCanvasPanelSlot* Slot = Parent->AddChildToCanvas(Widget);
        if (Slot)
        {
            FAnchorData Data;
            Data.Alignment = FVector2D(0.f, 0.f);

            FAnchorResult AnchorResult;
            if (LayerPtr->Type == EPsdLayerType::Group)
            {
                // Groups are transparent containers: fill the parent canvas so children
                // stay in the same coordinate system as the root. PhotoshopAPI does not
                // populate bounds on group layers, so we must not use them here.
                AnchorResult.Anchors = FAnchors(0.f, 0.f, 1.f, 1.f);
                AnchorResult.bStretchH = true;
                AnchorResult.bStretchV = true;
                AnchorResult.CleanName = LayerPtr->ParsedTags.CleanName;
            }
            else
            {
                AnchorResult = FAnchorCalculator::Calculate(*LayerPtr, LayerPtr->Bounds, CanvasSize);
            }

            Data.Anchors = AnchorResult.Anchors;

            const float AnchorPixelX = AnchorResult.Anchors.Minimum.X * static_cast<float>(CanvasSize.X);
            const float AnchorPixelY = AnchorResult.Anchors.Minimum.Y * static_cast<float>(CanvasSize.Y);

            if (LayerPtr->Type == EPsdLayerType::Group)
            {
                // Group fills parent: zero margins on all sides
                Data.Offsets = FMargin(0.f, 0.f, 0.f, 0.f);
            }
            else if (AnchorResult.bStretchH && AnchorResult.bStretchV)
            {
                // Full stretch: offsets are margins from all 4 edges
                Data.Offsets = FMargin(
                    static_cast<float>(LayerPtr->Bounds.Min.X),
                    static_cast<float>(LayerPtr->Bounds.Min.Y),
                    static_cast<float>(CanvasSize.X - LayerPtr->Bounds.Max.X),
                    static_cast<float>(CanvasSize.Y - LayerPtr->Bounds.Max.Y));
            }
            else if (AnchorResult.bStretchH)
            {
                // Horizontal stretch: Left/Right margins, Top offset from anchor Y, Bottom = height
                Data.Offsets = FMargin(
                    static_cast<float>(LayerPtr->Bounds.Min.X),
                    static_cast<float>(LayerPtr->Bounds.Min.Y) - AnchorPixelY,
                    static_cast<float>(CanvasSize.X - LayerPtr->Bounds.Max.X),
                    static_cast<float>(LayerPtr->Bounds.Height()));
            }
            else if (AnchorResult.bStretchV)
            {
                // Vertical stretch: Top/Bottom margins, Left offset from anchor X, Right = width
                Data.Offsets = FMargin(
                    static_cast<float>(LayerPtr->Bounds.Min.X) - AnchorPixelX,
                    static_cast<float>(LayerPtr->Bounds.Min.Y),
                    static_cast<float>(LayerPtr->Bounds.Width()),
                    static_cast<float>(CanvasSize.Y - LayerPtr->Bounds.Max.Y));
            }
            else
            {
                // Point anchor: offset from anchor point, size = width/height
                Data.Offsets = FMargin(
                    static_cast<float>(LayerPtr->Bounds.Min.X) - AnchorPixelX,
                    static_cast<float>(LayerPtr->Bounds.Min.Y) - AnchorPixelY,
                    static_cast<float>(LayerPtr->Bounds.Width()),
                    static_cast<float>(LayerPtr->Bounds.Height()));
            }

            // TEXT-06 — paragraph text layers: override slot width with BoxWidthPx
            // so AutoWrapText has a well-defined wrap boundary.
            if (LayerPtr->Type == EPsdLayerType::Text
                && LayerPtr->Text.bHasExplicitWidth
                && LayerPtr->Text.BoxWidthPx > 0.f
                && !AnchorResult.bStretchH)
            {
                Data.Offsets.Right = LayerPtr->Text.BoxWidthPx;
            }

            Slot->SetLayout(Data);
            // ZOrder: PSD index 0 = topmost. UMG higher = on top. Invert.
            Slot->SetZOrder(TotalLayers - 1 - i);

            UE_LOG(LogPSD2UMG, Log,
                TEXT("Layer '%s': bounds=(%d,%d)-(%d,%d) canvas=%dx%d anchors=(%.2f,%.2f,%.2f,%.2f) stretchH=%d stretchV=%d offsets=(L%.1f T%.1f R%.1f B%.1f)"),
                *LayerPtr->Name,
                LayerPtr->Bounds.Min.X, LayerPtr->Bounds.Min.Y, LayerPtr->Bounds.Max.X, LayerPtr->Bounds.Max.Y,
                CanvasSize.X, CanvasSize.Y,
                AnchorResult.Anchors.Minimum.X, AnchorResult.Anchors.Minimum.Y,
                AnchorResult.Anchors.Maximum.X, AnchorResult.Anchors.Maximum.Y,
                AnchorResult.bStretchH ? 1 : 0, AnchorResult.bStretchV ? 1 : 0,
                Data.Offsets.Left, Data.Offsets.Top, Data.Offsets.Right, Data.Offsets.Bottom);

            // ---- Phase 5: Layer Effects ----

            // FX-01: Opacity (per D-03) — only set when < 1.0
            if (LayerPtr->Opacity < 1.0f)
            {
                Widget->SetRenderOpacity(LayerPtr->Opacity);
            }

            // FX-02: Visibility (per D-01, D-02) — create as Collapsed if hidden
            if (!LayerPtr->bVisible)
            {
                Widget->SetVisibility(ESlateVisibility::Collapsed);
            }

            // FX-03: Color Overlay (per D-04, D-05) — image layers only
            if (LayerPtr->Effects.bHasColorOverlay)
            {
                if (UImage* Img = Cast<UImage>(Widget))
                {
                    FSlateBrush Brush = Img->GetBrush();
                    Brush.TintColor = FSlateColor(LayerPtr->Effects.ColorOverlayColor);
                    Img->SetBrush(Brush);
                }
                else
                {
                    UE_LOG(LogPSD2UMG, Warning,
                        TEXT("Color overlay on non-image layer '%s' ignored (per D-05)."), *LayerPtr->Name);
                }
            }

            // FX-04: Drop Shadow (per D-06, D-07, D-08)
            if (LayerPtr->Effects.bHasDropShadow && LayerPtr->Type == EPsdLayerType::Image)
            {
                if (UImage* MainImg = Cast<UImage>(Widget))
                {
                    // Create shadow UImage as sibling — shares same texture, tinted to shadow color
                    UImage* ShadowImg = Tree->ConstructWidget<UImage>(
                        UImage::StaticClass(),
                        FName(*FString::Printf(TEXT("%s_Shadow"), *LayerPtr->Name)));

                    // Copy the brush from the main image and apply shadow tint
                    FSlateBrush ShadowBrush = MainImg->GetBrush();
                    ShadowBrush.TintColor = FSlateColor(LayerPtr->Effects.DropShadowColor);
                    ShadowImg->SetBrush(ShadowBrush);

                    // Shadow opacity from shadow color alpha
                    ShadowImg->SetRenderOpacity(LayerPtr->Effects.DropShadowColor.A);

                    // Add to canvas and position with offset
                    UCanvasPanelSlot* ShadowSlot = Parent->AddChildToCanvas(ShadowImg);
                    if (ShadowSlot)
                    {
                        FAnchorData ShadowData;
                        ShadowData.Anchors = Slot->GetLayout().Anchors;
                        FMargin ShadowOffsets = Slot->GetLayout().Offsets;
                        ShadowOffsets.Left += static_cast<float>(LayerPtr->Effects.DropShadowOffset.X);
                        ShadowOffsets.Top += static_cast<float>(LayerPtr->Effects.DropShadowOffset.Y);
                        ShadowData.Offsets = ShadowOffsets;
                        ShadowSlot->SetLayout(ShadowData);
                        // Shadow behind main widget (lower ZOrder)
                        ShadowSlot->SetZOrder(Slot->GetZOrder() - 1);
                    }
                }
            }
            else if (LayerPtr->Effects.bHasDropShadow)
            {
                UE_LOG(LogPSD2UMG, Warning,
                    TEXT("Drop shadow on non-image layer '%s' — no-op (per D-08)."), *LayerPtr->Name);
            }
        }

        // Recurse into group children (skip if flattened)
        if (!bFlattened && Layer.Type == EPsdLayerType::Group && !Layer.Children.IsEmpty())
        {
            UCanvasPanel* ChildCanvas = Cast<UCanvasPanel>(Widget);
            if (ChildCanvas)
            {
                PopulateCanvas(Registry, Tree, ChildCanvas, Layer.Children, Doc, CanvasSize, SkippedLayerNames);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// FWidgetBlueprintGenerator::Generate
// ---------------------------------------------------------------------------
UWidgetBlueprint* FWidgetBlueprintGenerator::Generate(
    const FPsdDocument& Doc,
    const FString& WbpPackagePath,
    const FString& WbpAssetName,
    const TSet<FString>& SkippedLayerNames)
{
    // Step 1: Create WBP package
    const FString FullPath = FString::Printf(TEXT("%s/%s"), *WbpPackagePath, *WbpAssetName);
    UPackage* WbpPackage = CreatePackage(*FullPath);
    if (!WbpPackage)
    {
        UE_LOG(LogPSD2UMG, Error, TEXT("FWidgetBlueprintGenerator: failed to create package for %s"), *FullPath);
        return nullptr;
    }
    WbpPackage->FullyLoad();

    // Step 1b: If a WBP with this name already exists in the package, delete it
    // first. UE's CreateBlueprint asserts that no blueprint with the target name
    // exists (Kismet2.cpp FindObject check). This happens on re-import of the
    // same PSD or when a previous test/import left a stale asset.
    if (UBlueprint* Existing = FindObject<UBlueprint>(WbpPackage, *WbpAssetName))
    {
        UE_LOG(LogPSD2UMG, Warning,
            TEXT("FWidgetBlueprintGenerator: removing existing blueprint '%s' before regeneration"),
            *WbpAssetName);
        Existing->ClearFlags(RF_Standalone | RF_Public);
        Existing->MarkAsGarbage();
        Existing->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
    }

    // Step 2: Create WBP via factory (canonical editor path — same as "New Widget Blueprint" in Content Browser)
    UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>(GetTransientPackage());
    Factory->AddToRoot();
    Factory->ParentClass = UUserWidget::StaticClass();

    UWidgetBlueprint* WBP = CastChecked<UWidgetBlueprint>(
        Factory->FactoryCreateNew(
            UWidgetBlueprint::StaticClass(),
            WbpPackage,
            FName(*WbpAssetName),
            RF_Public | RF_Standalone | RF_Transactional,
            nullptr,
            GWarn));

    Factory->RemoveFromRoot();

    if (!WBP)
    {
        UE_LOG(LogPSD2UMG, Error, TEXT("FWidgetBlueprintGenerator: FactoryCreateNew returned null for %s"), *WbpAssetName);
        return nullptr;
    }

    // Step 3: Set up root canvas
    UCanvasPanel* RootCanvas = WBP->WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("Root_Canvas"));
    WBP->WidgetTree->RootWidget = RootCanvas;

    // Step 4: Populate widget tree recursively
    // Set the thread-local package path so FSmartObjectLayerMapper can obtain
    // the parent WBP location without interface changes (per plan 06-02 depth strategy).
    FSmartObjectImporter::SetCurrentPackagePath(WbpPackagePath);
    FLayerMappingRegistry Registry;
    PopulateCanvas(Registry, WBP->WidgetTree, RootCanvas, Doc.RootLayers, Doc, Doc.CanvasSize, SkippedLayerNames);

    // Step 5: Compile AFTER full tree population (critical — compiling before population leaves empty BP)
    FKismetEditorUtilities::CompileBlueprint(WBP);

    // Step 6: Mark dirty and save
    WbpPackage->MarkPackageDirty();
    UEditorAssetLibrary::SaveLoadedAsset(WBP, false);

    UE_LOG(LogPSD2UMG, Log, TEXT("FWidgetBlueprintGenerator: created and saved WBP: %s"), *FullPath);
    return WBP;
}

// ---------------------------------------------------------------------------
// Internal helper: collect all widgets from a WidgetTree into a flat map by name
// ---------------------------------------------------------------------------
static void CollectWidgetsByName(UWidgetTree* Tree, TMap<FString, UWidget*>& OutMap)
{
    if (!Tree)
    {
        return;
    }
    Tree->ForEachWidget([&OutMap](UWidget* W)
    {
        if (W)
        {
            OutMap.Add(W->GetName(), W);
        }
    });
}

// ---------------------------------------------------------------------------
// Internal helper: recursively update canvas from layer array
// ---------------------------------------------------------------------------
static void UpdateCanvas(
    FLayerMappingRegistry& Registry,
    UWidgetTree* Tree,
    UCanvasPanel* Parent,
    const TArray<FPsdLayer>& Layers,
    const FPsdDocument& Doc,
    const FIntPoint& CanvasSize,
    TMap<FString, UWidget*>& ExistingWidgets,
    const TSet<FString>& SkippedLayerNames)
{
    for (const FPsdLayer& Layer : Layers)
    {
        if (SkippedLayerNames.Contains(Layer.Name))
        {
            continue;
        }

        UWidget** ExistingPtr = ExistingWidgets.Find(Layer.Name);
        if (ExistingPtr && *ExistingPtr)
        {
            UWidget* Existing = *ExistingPtr;
            ExistingWidgets.Remove(Layer.Name); // mark as handled

            // Update PSD-sourced properties on existing widget
            // Update canvas slot position/size
            if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Existing->Slot))
            {
                FAnchorResult AnchorResult = FAnchorCalculator::Calculate(Layer, Layer.Bounds, CanvasSize);
                FAnchorData Data;
                Data.Anchors = AnchorResult.Anchors;
                Data.Alignment = FVector2D(0.f, 0.f);

                const float AnchorPixelX = AnchorResult.Anchors.Minimum.X * static_cast<float>(CanvasSize.X);
                const float AnchorPixelY = AnchorResult.Anchors.Minimum.Y * static_cast<float>(CanvasSize.Y);

                if (!AnchorResult.bStretchH && !AnchorResult.bStretchV)
                {
                    Data.Offsets = FMargin(
                        static_cast<float>(Layer.Bounds.Min.X) - AnchorPixelX,
                        static_cast<float>(Layer.Bounds.Min.Y) - AnchorPixelY,
                        static_cast<float>(Layer.Bounds.Width()),
                        static_cast<float>(Layer.Bounds.Height()));
                }
                else
                {
                    Data.Offsets = FMargin(
                        static_cast<float>(Layer.Bounds.Min.X),
                        static_cast<float>(Layer.Bounds.Min.Y),
                        static_cast<float>(CanvasSize.X - Layer.Bounds.Max.X),
                        static_cast<float>(CanvasSize.Y - Layer.Bounds.Max.Y));
                }
                Slot->SetLayout(Data);
            }

            // Update text properties for text blocks
            if (Layer.Type == EPsdLayerType::Text)
            {
                if (UTextBlock* TextBlock = Cast<UTextBlock>(Existing))
                {
                    TextBlock->SetText(FText::FromString(Layer.Text.Content));
                    FSlateFontInfo FontInfo = TextBlock->GetFont();
                    if (Layer.Text.SizePx > 0.f)
                    {
                        FontInfo.Size = static_cast<int32>(Layer.Text.SizePx);
                    }
                    TextBlock->SetFont(FontInfo);
                    TextBlock->SetColorAndOpacity(FSlateColor(Layer.Text.Color));
                }
            }

            // Update opacity and visibility
            Existing->SetRenderOpacity(Layer.Opacity);
            Existing->SetVisibility(Layer.bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

            UE_LOG(LogPSD2UMG, Log, TEXT("Update: updated existing widget '%s'"), *Layer.Name);
        }
        else
        {
            // New layer: create widget and add to canvas
            if (Layer.Bounds.IsEmpty() && Layer.Type != EPsdLayerType::Group)
            {
                UE_LOG(LogPSD2UMG, Warning, TEXT("Update: skipping zero-size new layer: %s"), *Layer.Name);
                continue;
            }

            UWidget* NewWidget = Registry.MapLayer(Layer, Doc, Tree);
            if (!NewWidget)
            {
                UE_LOG(LogPSD2UMG, Warning, TEXT("Update: no mapper for new layer '%s'"), *Layer.Name);
                continue;
            }

            UCanvasPanelSlot* Slot = Parent->AddChildToCanvas(NewWidget);
            if (Slot)
            {
                FAnchorResult AnchorResult = FAnchorCalculator::Calculate(Layer, Layer.Bounds, CanvasSize);
                FAnchorData Data;
                Data.Alignment = FVector2D(0.f, 0.f);
                Data.Anchors = AnchorResult.Anchors;

                const float AnchorPixelX = AnchorResult.Anchors.Minimum.X * static_cast<float>(CanvasSize.X);
                const float AnchorPixelY = AnchorResult.Anchors.Minimum.Y * static_cast<float>(CanvasSize.Y);

                if (!AnchorResult.bStretchH && !AnchorResult.bStretchV)
                {
                    Data.Offsets = FMargin(
                        static_cast<float>(Layer.Bounds.Min.X) - AnchorPixelX,
                        static_cast<float>(Layer.Bounds.Min.Y) - AnchorPixelY,
                        static_cast<float>(Layer.Bounds.Width()),
                        static_cast<float>(Layer.Bounds.Height()));
                }
                else
                {
                    Data.Offsets = FMargin(
                        static_cast<float>(Layer.Bounds.Min.X),
                        static_cast<float>(Layer.Bounds.Min.Y),
                        static_cast<float>(CanvasSize.X - Layer.Bounds.Max.X),
                        static_cast<float>(CanvasSize.Y - Layer.Bounds.Max.Y));
                }
                Slot->SetLayout(Data);
            }
            UE_LOG(LogPSD2UMG, Log, TEXT("Update: added new widget '%s'"), *Layer.Name);
        }

        // Recurse into group children
        if (Layer.Type == EPsdLayerType::Group && !Layer.Children.IsEmpty())
        {
            UWidget** GroupWidgetPtr = ExistingWidgets.Find(Layer.Name);
            UCanvasPanel* ChildCanvas = nullptr;
            if (GroupWidgetPtr)
            {
                ChildCanvas = Cast<UCanvasPanel>(*GroupWidgetPtr);
            }
            if (!ChildCanvas)
            {
                // Try to find from the newly added widget
                UWidget* Added = Tree->FindWidget(FName(*Layer.Name));
                ChildCanvas = Cast<UCanvasPanel>(Added);
            }
            if (ChildCanvas)
            {
                UpdateCanvas(Registry, Tree, ChildCanvas, Layer.Children, Doc, CanvasSize, ExistingWidgets, SkippedLayerNames);
            }
        }
    }
    // Widgets remaining in ExistingWidgets after traversal are orphans (deleted PSD layers).
    // Per D-07, they are kept as-is — do NOT remove them.
}

// ---------------------------------------------------------------------------
// FWidgetBlueprintGenerator::Update
// ---------------------------------------------------------------------------
bool FWidgetBlueprintGenerator::Update(
    UWidgetBlueprint* ExistingWBP,
    const FPsdDocument& NewDoc,
    const TSet<FString>& SkippedLayerNames)
{
    if (!ExistingWBP || !ExistingWBP->WidgetTree)
    {
        UE_LOG(LogPSD2UMG, Error, TEXT("FWidgetBlueprintGenerator::Update: null WBP or WidgetTree"));
        return false;
    }

    // Build map of existing widgets by name
    TMap<FString, UWidget*> ExistingWidgets;
    CollectWidgetsByName(ExistingWBP->WidgetTree, ExistingWidgets);

    // Find root canvas
    UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(ExistingWBP->WidgetTree->RootWidget);
    if (!RootCanvas)
    {
        UE_LOG(LogPSD2UMG, Error, TEXT("FWidgetBlueprintGenerator::Update: root widget is not a UCanvasPanel"));
        return false;
    }

    // Populate updates using new document
    FSmartObjectImporter::SetCurrentPackagePath(ExistingWBP->GetOutermost()->GetName());
    FLayerMappingRegistry Registry;
    UpdateCanvas(Registry, ExistingWBP->WidgetTree, RootCanvas, NewDoc.RootLayers, NewDoc, NewDoc.CanvasSize, ExistingWidgets, SkippedLayerNames);

    // Compile and save
    FKismetEditorUtilities::CompileBlueprint(ExistingWBP);
    ExistingWBP->GetOutermost()->MarkPackageDirty();
    UEditorAssetLibrary::SaveLoadedAsset(ExistingWBP, false);

    UE_LOG(LogPSD2UMG, Log, TEXT("FWidgetBlueprintGenerator::Update: reimport complete for %s"), *ExistingWBP->GetName());
    return true;
}

// ---------------------------------------------------------------------------
// FWidgetBlueprintGenerator::DetectChange
// ---------------------------------------------------------------------------
EPsdChangeAnnotation FWidgetBlueprintGenerator::DetectChange(
    const FPsdLayer& NewLayer,
    UWidget* ExistingWidget)
{
    if (!ExistingWidget)
    {
        return EPsdChangeAnnotation::New;
    }

    // Check position/size via canvas slot
    if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(ExistingWidget->Slot))
    {
        const FAnchorData Layout = Slot->GetLayout();
        // Compare offset magnitudes loosely (1px tolerance)
        const float W = static_cast<float>(NewLayer.Bounds.Width());
        const float H = static_cast<float>(NewLayer.Bounds.Height());
        if (FMath::Abs(Layout.Offsets.Right - W) > 1.f || FMath::Abs(Layout.Offsets.Bottom - H) > 1.f)
        {
            return EPsdChangeAnnotation::Changed;
        }
    }

    // Check text content for text layers
    if (NewLayer.Type == EPsdLayerType::Text)
    {
        if (UTextBlock* TextBlock = Cast<UTextBlock>(ExistingWidget))
        {
            if (!TextBlock->GetText().EqualTo(FText::FromString(NewLayer.Text.Content)))
            {
                return EPsdChangeAnnotation::Changed;
            }
        }
    }

    // Check opacity
    if (FMath::Abs(ExistingWidget->GetRenderOpacity() - NewLayer.Opacity) > 0.01f)
    {
        return EPsdChangeAnnotation::Changed;
    }

    return EPsdChangeAnnotation::Unchanged;
}
