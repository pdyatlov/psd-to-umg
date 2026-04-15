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
#include "Components/Image.h"
#include "Components/PanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/UObjectGlobals.h"
#include "WidgetBlueprintFactory.h"
#include "Blueprint/UserWidget.h"
#include "EditorAssetLibrary.h"
#include "Engine/Blueprint.h"
#include "WidgetBlueprint.h"

// ---------------------------------------------------------------------------
// Internal helper: recursively populate a panel widget from a layer array.
// Dispatches child attachment on the runtime type of Parent:
//   UCanvasPanel  -> AddChildToCanvas + full anchor/offset/z-order logic
//   UPanelWidget* -> AddChild (engine-default slot; no positional data)
// ---------------------------------------------------------------------------
static void PopulateChildren(
    FLayerMappingRegistry& Registry,
    UWidgetTree* Tree,
    UPanelWidget* Parent,
    const TArray<FPsdLayer>& Layers,
    const FPsdDocument& Doc,
    const FIntPoint& CanvasSize,
    const TSet<FString>& SkippedLayerNames = TSet<FString>())
{
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
            // Mappers dispatch on ParsedTags.Type; keep it in lock-step with Type
            // so the flattened copy routes to FImageLayerMapper instead of FGroupLayerMapper.
            FlattenedLayer.ParsedTags.Type = EPsdTagType::Image;
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

        // ---- Dispatch child attachment on parent panel type ----
        UPanelSlot* Slot = nullptr;
        UCanvasPanel* CanvasParent = Cast<UCanvasPanel>(Parent);
        if (CanvasParent)
        {
            Slot = CanvasParent->AddChildToCanvas(Widget);
        }
        else
        {
            Slot = Parent->AddChild(Widget);
            UE_LOG(LogPSD2UMG, Log,
                TEXT("Layer '%s' attached to non-canvas parent '%s' (class %s) — slot defaults applied"),
                *LayerPtr->Name, *Parent->GetName(), *Parent->GetClass()->GetName());
        }
        if (!Slot) { continue; }

        // ---- Canvas-only: anchor/offset/z-order configuration ----
        if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
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

            CanvasSlot->SetLayout(Data);
            // ZOrder: PSD index 0 = topmost. UMG higher = on top. Invert.
            CanvasSlot->SetZOrder(TotalLayers - 1 - i);

            UE_LOG(LogPSD2UMG, Log,
                TEXT("Layer '%s': bounds=(%d,%d)-(%d,%d) canvas=%dx%d anchors=(%.2f,%.2f,%.2f,%.2f) stretchH=%d stretchV=%d offsets=(L%.1f T%.1f R%.1f B%.1f)"),
                *LayerPtr->Name,
                LayerPtr->Bounds.Min.X, LayerPtr->Bounds.Min.Y, LayerPtr->Bounds.Max.X, LayerPtr->Bounds.Max.Y,
                CanvasSize.X, CanvasSize.Y,
                AnchorResult.Anchors.Minimum.X, AnchorResult.Anchors.Minimum.Y,
                AnchorResult.Anchors.Maximum.X, AnchorResult.Anchors.Maximum.Y,
                AnchorResult.bStretchH ? 1 : 0, AnchorResult.bStretchV ? 1 : 0,
                Data.Offsets.Left, Data.Offsets.Top, Data.Offsets.Right, Data.Offsets.Bottom);

            // FX-04: Drop Shadow (per D-06, D-07, D-08) — canvas-only sibling pattern
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
                    UCanvasPanelSlot* ShadowSlot = CanvasParent->AddChildToCanvas(ShadowImg);
                    if (ShadowSlot)
                    {
                        FAnchorData ShadowData;
                        ShadowData.Anchors = CanvasSlot->GetLayout().Anchors;
                        FMargin ShadowOffsets = CanvasSlot->GetLayout().Offsets;
                        ShadowOffsets.Left += static_cast<float>(LayerPtr->Effects.DropShadowOffset.X);
                        ShadowOffsets.Top += static_cast<float>(LayerPtr->Effects.DropShadowOffset.Y);
                        ShadowData.Offsets = ShadowOffsets;
                        ShadowSlot->SetLayout(ShadowData);
                        // Shadow behind main widget (lower ZOrder)
                        ShadowSlot->SetZOrder(CanvasSlot->GetZOrder() - 1);
                    }
                }
            }
            else if (LayerPtr->Effects.bHasDropShadow && LayerPtr->Type != EPsdLayerType::Image)
            {
                UE_LOG(LogPSD2UMG, Warning,
                    TEXT("Drop shadow on non-image layer '%s' — no-op (per D-08)."), *LayerPtr->Name);
            }
        } // end canvas-only block

        // ---- Drop-shadow skip warning for non-canvas parents (D-06) ----
        if (!CanvasParent && LayerPtr->Effects.bHasDropShadow && LayerPtr->Type == EPsdLayerType::Image)
        {
            UE_LOG(LogPSD2UMG, Warning,
                TEXT("Drop shadow on layer '%s' inside non-canvas parent '%s' — no-op (canvas-only sibling pattern)."),
                *LayerPtr->Name, *Parent->GetName());
        }

        // ---- Phase 5: Layer Effects — operate on Widget directly, independent of parent type ----

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

        // Recurse into group children (skip if flattened)
        if (!bFlattened && Layer.Type == EPsdLayerType::Group && !Layer.Children.IsEmpty())
        {
            if (UPanelWidget* ChildPanel = Cast<UPanelWidget>(Widget))
            {
                PopulateChildren(Registry, Tree, ChildPanel, Layer.Children, Doc, CanvasSize, SkippedLayerNames);
            }
            else
            {
                UE_LOG(LogPSD2UMG, Error,
                    TEXT("Group layer '%s' mapped to non-panel widget '%s' (class %s); %d children dropped"),
                    *Layer.Name, *Widget->GetName(), *Widget->GetClass()->GetName(), Layer.Children.Num());
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

        // Rename every widget in the old WidgetTree out of the canonical path first.
        // Without this, a surviving widget archetype (e.g., inside the BlueprintGeneratedClass
        // CDO) can collide by-name with the new tree during compilation, producing
        // "Cannot replace existing object of a different class" fatal asserts when
        // the PSD's widget class for that name changes between imports.
        if (UWidgetBlueprint* ExistingWBP = Cast<UWidgetBlueprint>(Existing))
        {
            if (UWidgetTree* OldTree = ExistingWBP->WidgetTree)
            {
                TArray<UWidget*> OldWidgets;
                OldTree->ForEachWidget([&OldWidgets](UWidget* W) { if (W) OldWidgets.Add(W); });
                for (UWidget* W : OldWidgets)
                {
                    W->ClearFlags(RF_Standalone | RF_Public);
                    W->MarkAsGarbage();
                    W->Rename(nullptr, GetTransientPackage(),
                        REN_DontCreateRedirectors | REN_NonTransactional);
                }
                OldTree->RootWidget = nullptr;
            }
        }

        Existing->ClearFlags(RF_Standalone | RF_Public);
        Existing->MarkAsGarbage();
        Existing->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);

        // Also clear the BlueprintGeneratedClass path if it survived. Its CDO holds
        // widget-archetype subobjects that otherwise reappear at the canonical names.
        if (UClass* OldClass = FindObject<UClass>(WbpPackage, *FString::Printf(TEXT("%s_C"), *WbpAssetName)))
        {
            OldClass->ClearFlags(RF_Standalone | RF_Public);
            OldClass->MarkAsGarbage();
            OldClass->Rename(nullptr, GetTransientPackage(),
                REN_DontCreateRedirectors | REN_NonTransactional);
        }

        // Force a GC pass so stale subobjects are actually reaped before the new
        // blueprint claims the path. Without this the class/archetype references
        // can linger long enough to trip the by-name check in StaticAllocateObject.
        CollectGarbage(RF_NoFlags, /*bPurgeObjectsOnFullPurge=*/true);
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
    PopulateChildren(Registry, WBP->WidgetTree, RootCanvas, Doc.RootLayers, Doc, Doc.CanvasSize, SkippedLayerNames);

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
// Internal helper: recursively update canvas from layer array.
// Parent is generalized to UPanelWidget*:
//   UCanvasPanel  -> AddChildToCanvas + anchor/offset/z-order for new children;
//                    diff-update strategy for existing canvas children.
//   UPanelWidget* -> AddChild (engine-default slot) for new children;
//                    clear-and-rebuild via PopulateChildren for non-canvas recursion (D-11).
// ---------------------------------------------------------------------------
static void UpdateCanvas(
    FLayerMappingRegistry& Registry,
    UWidgetTree* Tree,
    UPanelWidget* Parent,
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
            // Canvas-only: update slot position/size (non-canvas slots have no PSD-sourced state — D-03)
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
            // New layer: create widget and attach to parent
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

            // Dispatch attachment on parent panel type (D-10)
            UPanelSlot* Slot = nullptr;
            UCanvasPanel* CanvasParent = Cast<UCanvasPanel>(Parent);
            if (CanvasParent)
            {
                Slot = CanvasParent->AddChildToCanvas(NewWidget);
            }
            else
            {
                Slot = Parent->AddChild(NewWidget);
            }

            // Canvas-only: apply anchor/offset layout to new widget slot
            if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
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
                CanvasSlot->SetLayout(Data);
            }
            UE_LOG(LogPSD2UMG, Log, TEXT("Update: added new widget '%s'"), *Layer.Name);
        }

        // Recurse into group children
        if (Layer.Type == EPsdLayerType::Group && !Layer.Children.IsEmpty())
        {
            UPanelWidget* ChildPanel = nullptr;
            UWidget** GroupWidgetPtr = ExistingWidgets.Find(Layer.Name);
            if (GroupWidgetPtr)
            {
                ChildPanel = Cast<UPanelWidget>(*GroupWidgetPtr);
            }
            if (!ChildPanel)
            {
                // Try to find from the newly added widget
                UWidget* Added = Tree->FindWidget(FName(*Layer.Name));
                ChildPanel = Cast<UPanelWidget>(Added);
            }
            if (ChildPanel)
            {
                if (!Cast<UCanvasPanel>(ChildPanel))
                {
                    // D-11: clear-and-rebuild for non-canvas panels.
                    // Non-canvas slots carry no PSD-sourced state worth preserving,
                    // so the simplest correct approach is to clear and re-add children
                    // via the same PopulateChildren helper used by the Generate path.
                    ChildPanel->ClearChildren();
                    PopulateChildren(Registry, Tree, ChildPanel, Layer.Children, Doc, CanvasSize, SkippedLayerNames);
                }
                else
                {
                    // Canvas: preserve diff-update behavior (PANEL-05 parity)
                    UpdateCanvas(Registry, Tree, ChildPanel, Layer.Children, Doc, CanvasSize, ExistingWidgets, SkippedLayerNames);
                }
            }
            else
            {
                UE_LOG(LogPSD2UMG, Error,
                    TEXT("Update: group layer '%s' has no matching UPanelWidget in existing WBP; %d children dropped"),
                    *Layer.Name, Layer.Children.Num());
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
