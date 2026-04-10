// Copyright 2018-2021 - John snow wind
#include "Generator/FWidgetBlueprintGenerator.h"

#include "Generator/FAnchorCalculator.h"
#include "Mapper/FLayerMappingRegistry.h"
#include "Parser/PsdTypes.h"
#include "PSD2UMGLog.h"
#include "PSD2UMGSetting.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/UObjectGlobals.h"
#include "WidgetBlueprintFactory.h"
#include "Blueprint/UserWidget.h"
#include "EditorAssetLibrary.h"
#include "Engine/Blueprint.h"
#include "WidgetBlueprint.h"

// ---------------------------------------------------------------------------
// Internal helper: recursively populate a canvas panel from a layer array
// ---------------------------------------------------------------------------
static void PopulateCanvas(
    FLayerMappingRegistry& Registry,
    UWidgetTree* Tree,
    UCanvasPanel* Parent,
    const TArray<FPsdLayer>& Layers,
    const FPsdDocument& Doc,
    const FIntPoint& CanvasSize)
{
    const int32 TotalLayers = Layers.Num();
    for (int32 i = 0; i < TotalLayers; ++i)
    {
        const FPsdLayer& Layer = Layers[i];

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
                AnchorResult.CleanName = LayerPtr->Name;
            }
            else
            {
                AnchorResult = FAnchorCalculator::Calculate(LayerPtr->Name, LayerPtr->Bounds, CanvasSize);
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
                PopulateCanvas(Registry, Tree, ChildCanvas, Layer.Children, Doc, CanvasSize);
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
    const FString& WbpAssetName)
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
    FLayerMappingRegistry Registry;
    PopulateCanvas(Registry, WBP->WidgetTree, RootCanvas, Doc.RootLayers, Doc, Doc.CanvasSize);

    // Step 5: Compile AFTER full tree population (critical — compiling before population leaves empty BP)
    FKismetEditorUtilities::CompileBlueprint(WBP);

    // Step 6: Mark dirty and save
    WbpPackage->MarkPackageDirty();
    UEditorAssetLibrary::SaveLoadedAsset(WBP, false);

    UE_LOG(LogPSD2UMG, Log, TEXT("FWidgetBlueprintGenerator: created and saved WBP: %s"), *FullPath);
    return WBP;
}
