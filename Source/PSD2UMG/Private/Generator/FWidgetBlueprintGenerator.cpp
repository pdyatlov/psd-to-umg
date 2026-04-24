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
#include "Components/PanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/GarbageCollection.h"
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
    const TSet<FString>& SkippedLayerNames = TSet<FString>(),
    const FIntPoint& ParentOffset = FIntPoint::ZeroValue)
{
    const int32 TotalLayers = Layers.Num();
    for (int32 i = 0; i < TotalLayers; ++i)
    {
        const FPsdLayer& Layer = Layers[i];

        // Skip layers the user unchecked in the preview dialog
        if (SkippedLayerNames.Contains(Layer.Name))
        {
            UE_LOG(LogPSD2UMG, Log, TEXT("PopulateChildren: '%s' skipped (SkippedLayerNames; bVisible=%d in PSD)"), *Layer.Name, Layer.bVisible ? 1 : 0);
            continue;
        }

        // @background layers are consumed as FSlateBrush by FButtonLayerMapper; skip as child widgets.
        if (Layer.ParsedTags.bIsBackground)
        {
            UE_LOG(LogPSD2UMG, Log, TEXT("PopulateChildren: '%s' skipped (@background — consumed as brush)"), *Layer.Name);
            continue;
        }

        // Skip zero-size non-groups (D-14)
        if (Layer.Bounds.IsEmpty() && Layer.Type != EPsdLayerType::Group)
        {
            UE_LOG(LogPSD2UMG, Warning, TEXT("Skipping zero-size layer: '%s' type=%d bounds=(%d,%d)-(%d,%d)"),
                *Layer.Name, (int32)Layer.Type,
                Layer.Bounds.Min.X, Layer.Bounds.Min.Y, Layer.Bounds.Max.X, Layer.Bounds.Max.Y);
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

        // If the layer's CleanName collides with a widget already in the tree
        // (duplicate Photoshop layer names — common for copy-pasted panels/cards),
        // mangle the clean name to a unique variant (e.g. 'background_1',
        // 'background_2', ...). UWidgetTree shares a single flat outer for every
        // widget, so duplicates would otherwise trip 'Cannot replace existing
        // object of a different class' during construction or compile.
        FPsdLayer UniqueLayer; // mutable copy only when we need to rename
        if (Tree && !LayerPtr->ParsedTags.CleanName.IsEmpty())
        {
            const FName BaseFName(*LayerPtr->ParsedTags.CleanName);
            if (StaticFindObjectFast(nullptr, Tree, BaseFName))
            {
                const FName UniqueFName = MakeUniqueObjectName(Tree, UWidget::StaticClass(), BaseFName);
                UE_LOG(LogPSD2UMG, Warning,
                    TEXT("Layer '%s' clean name '%s' collides with existing widget; renaming to '%s'"),
                    *LayerPtr->Name, *LayerPtr->ParsedTags.CleanName, *UniqueFName.ToString());
                UniqueLayer = *LayerPtr;
                UniqueLayer.ParsedTags.CleanName = UniqueFName.ToString();
                LayerPtr = &UniqueLayer;
            }
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

            // VBox/HBox: set Fill proportional to layer bounds so items occupy
            // the right relative space. Fill(H) / Fill(W) divides the container
            // proportionally among siblings; equal bounds = equal share.
            if (UVerticalBoxSlot* VBoxSlot = Cast<UVerticalBoxSlot>(Slot))
            {
                const float BoundsH = static_cast<float>(LayerPtr->Bounds.Height());
                if (BoundsH > 0.f)
                {
                    FSlateChildSize Size;
                    Size.SizeRule = ESlateSizeRule::Fill;
                    Size.Value = BoundsH;

                    VBoxSlot->SetSize(Size);
                }
            }
            else if (UHorizontalBoxSlot* HBoxSlot = Cast<UHorizontalBoxSlot>(Slot))
            {
                const float BoundsW = static_cast<float>(LayerPtr->Bounds.Width());
                if (BoundsW > 0.f)
                {
                    FSlateChildSize Size;
                    Size.SizeRule = ESlateSizeRule::Fill;
                    Size.Value = BoundsW;

                    HBoxSlot->SetSize(Size);
                }
            }
        }
        if (!Slot) { continue; }

        // ---- Canvas-only: anchor/offset/z-order configuration ----
        if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
        {
            FAnchorData Data;
            Data.Alignment = FVector2D(0.f, 0.f);

            // Local bounds: layer coordinates relative to this canvas panel's top-left.
            // At root level ParentOffset=(0,0) so local == absolute.
            const FIntRect EffectiveBounds = FIntRect(
                LayerPtr->Bounds.Min - ParentOffset,
                LayerPtr->Bounds.Max - ParentOffset);

            FAnchorResult AnchorResult;
            if (LayerPtr->Type == EPsdLayerType::Group)
            {
                // Both canvas and layout groups: use @anchor tag + computed bounds.
                // Canvas groups are now positional (not fill-stretch); their children
                // receive a local coordinate context when PopulateChildren recurses.
                AnchorResult = FAnchorCalculator::Calculate(*LayerPtr, EffectiveBounds, CanvasSize);
                AnchorResult.CleanName = LayerPtr->ParsedTags.CleanName;
            }
            else
            {
                AnchorResult = FAnchorCalculator::Calculate(*LayerPtr, EffectiveBounds, CanvasSize);
            }

            Data.Anchors = AnchorResult.Anchors;

            const float AnchorPixelX = AnchorResult.Anchors.Minimum.X * static_cast<float>(CanvasSize.X);
            const float AnchorPixelY = AnchorResult.Anchors.Minimum.Y * static_cast<float>(CanvasSize.Y);

            if (AnchorResult.bStretchH && AnchorResult.bStretchV)
            {
                // Full stretch: offsets are margins from all 4 edges (local space)
                Data.Offsets = FMargin(
                    static_cast<float>(EffectiveBounds.Min.X),
                    static_cast<float>(EffectiveBounds.Min.Y),
                    static_cast<float>(CanvasSize.X - EffectiveBounds.Max.X),
                    static_cast<float>(CanvasSize.Y - EffectiveBounds.Max.Y));
            }
            else if (AnchorResult.bStretchH)
            {
                // Horizontal stretch: Left/Right margins, Top offset from anchor Y, Bottom = height
                Data.Offsets = FMargin(
                    static_cast<float>(EffectiveBounds.Min.X),
                    static_cast<float>(EffectiveBounds.Min.Y) - AnchorPixelY,
                    static_cast<float>(CanvasSize.X - EffectiveBounds.Max.X),
                    static_cast<float>(EffectiveBounds.Height()));
            }
            else if (AnchorResult.bStretchV)
            {
                // Vertical stretch: Top/Bottom margins, Left offset from anchor X, Right = width
                Data.Offsets = FMargin(
                    static_cast<float>(EffectiveBounds.Min.X) - AnchorPixelX,
                    static_cast<float>(EffectiveBounds.Min.Y),
                    static_cast<float>(EffectiveBounds.Width()),
                    static_cast<float>(CanvasSize.Y - EffectiveBounds.Max.Y));
            }
            else
            {
                // Point anchor: offset from anchor point, size = width/height (local space)
                Data.Offsets = FMargin(
                    static_cast<float>(EffectiveBounds.Min.X) - AnchorPixelX,
                    static_cast<float>(EffectiveBounds.Min.Y) - AnchorPixelY,
                    static_cast<float>(EffectiveBounds.Width()),
                    static_cast<float>(EffectiveBounds.Height()));
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

            // Non-canvas layout groups (VBox, HBox, etc.): use explicit bounds size.
            // SetAutoSize would force the slot to use the widget's desired size, which
            // is 0 for VBox/HBox when children use Fill slots. Keep explicit W/H.

            UE_LOG(LogPSD2UMG, Log,
                TEXT("Layer '%s': localBounds=(%d,%d)-(%d,%d) canvas=%dx%d anchors=(%.2f,%.2f,%.2f,%.2f) stretchH=%d stretchV=%d offsets=(L%.1f T%.1f R%.1f B%.1f)"),
                *LayerPtr->Name,
                EffectiveBounds.Min.X, EffectiveBounds.Min.Y, EffectiveBounds.Max.X, EffectiveBounds.Max.Y,
                CanvasSize.X, CanvasSize.Y,
                AnchorResult.Anchors.Minimum.X, AnchorResult.Anchors.Minimum.Y,
                AnchorResult.Anchors.Maximum.X, AnchorResult.Anchors.Maximum.Y,
                AnchorResult.bStretchH ? 1 : 0, AnchorResult.bStretchV ? 1 : 0,
                Data.Offsets.Left, Data.Offsets.Top, Data.Offsets.Right, Data.Offsets.Bottom);

            // FX-04: Drop Shadow — canvas-only sibling pattern
            // Phase 15 GRPFX-01 (D-01, D-02): extended to EPsdLayerType::Group with null-brush variant
            const bool bShadowSupportedType =
                (LayerPtr->Type == EPsdLayerType::Image || LayerPtr->Type == EPsdLayerType::Group);
            if (LayerPtr->Effects.bHasDropShadow && bShadowSupportedType)
            {
                // Construct shadow sibling. Use ParsedTags.CleanName when available;
                // fall back to Name for parity with legacy image-shadow behaviour (Open Question 2 resolution).
                const FString ShadowBaseName = !LayerPtr->ParsedTags.CleanName.IsEmpty()
                    ? LayerPtr->ParsedTags.CleanName
                    : LayerPtr->Name;
                const FName ShadowFName = MakeUniqueObjectName(
                    Tree, UImage::StaticClass(),
                    FName(*FString::Printf(TEXT("%s_Shadow"), *ShadowBaseName)));
                UImage* ShadowImg = Tree->ConstructWidget<UImage>(UImage::StaticClass(), ShadowFName);

                FSlateBrush ShadowBrush;
                bool bShadowBrushReady = true;
                if (LayerPtr->Type == EPsdLayerType::Group)
                {
                    // D-02: null brush — no texture, solid color tint, sized to group bounds
                    ShadowBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
                    ShadowBrush.ImageSize = FVector2D(
                        static_cast<float>(LayerPtr->Bounds.Width()),
                        static_cast<float>(LayerPtr->Bounds.Height()));
                }
                else if (UImage* MainImg = Cast<UImage>(Widget))
                {
                    // Existing image path: copy brush (preserves texture + 9-slice margins)
                    ShadowBrush = MainImg->GetBrush();
                }
                else
                {
                    // Defensive: Image type but widget is not UImage (e.g. flatten path) — skip shadow
                    UE_LOG(LogPSD2UMG, Warning,
                        TEXT("Drop shadow on image layer '%s' — widget is not UImage, shadow skipped."),
                        *LayerPtr->Name);
                    bShadowBrushReady = false;
                }

                if (bShadowBrushReady)
                {
                    ShadowBrush.TintColor = FSlateColor(LayerPtr->Effects.DropShadowColor);
                    ShadowImg->SetBrush(ShadowBrush);
                    ShadowImg->SetRenderOpacity(LayerPtr->Effects.DropShadowColor.A);

                    // Add to canvas and position with offset
                    UCanvasPanelSlot* ShadowSlot = CanvasParent->AddChildToCanvas(ShadowImg);
                    if (ShadowSlot)
                    {
                        FAnchorData ShadowData;
                        ShadowData.Anchors = CanvasSlot->GetLayout().Anchors;
                        FMargin ShadowOffsets = CanvasSlot->GetLayout().Offsets;
                        ShadowOffsets.Left += static_cast<float>(LayerPtr->Effects.DropShadowOffset.X);
                        ShadowOffsets.Top  += static_cast<float>(LayerPtr->Effects.DropShadowOffset.Y);
                        ShadowData.Offsets = ShadowOffsets;
                        ShadowSlot->SetLayout(ShadowData);
                        // D-01: shadow ZOrder = main - 1 (behind main widget)
                        ShadowSlot->SetZOrder(CanvasSlot->GetZOrder() - 1);
                    }
                }
            }
            else if (LayerPtr->Effects.bHasDropShadow)
            {
                // Non-supported type (Text, Shape, Gradient, SolidFill, etc.) — retain existing warning
                UE_LOG(LogPSD2UMG, Warning,
                    TEXT("Drop shadow on layer '%s' (type=%d) — no-op (per D-08)."),
                    *LayerPtr->Name, static_cast<int32>(LayerPtr->Type));
            }
        } // end canvas-only block

        // ---- Drop-shadow skip warning for non-canvas parents (D-03 / Pitfall 4) ----
        // D-03: drop shadow on Group OR Image inside non-canvas parent is a no-op + warning
        if (!CanvasParent && LayerPtr->Effects.bHasDropShadow
            && (LayerPtr->Type == EPsdLayerType::Image || LayerPtr->Type == EPsdLayerType::Group))
        {
            UE_LOG(LogPSD2UMG, Warning,
                TEXT("Drop shadow on %s layer '%s' inside non-canvas parent '%s' — no-op (canvas-only sibling pattern)."),
                LayerPtr->Type == EPsdLayerType::Group ? TEXT("group") : TEXT("image"),
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

        // FX-03: Color Overlay
        // Phase 15 GRPFX-02 (D-04, D-05, D-06, D-07): image branch immediate; group branch
        // deferred to after recursion so the overlay lands as the LAST child (Pitfall 1).
        UPanelWidget* DeferredOverlayPanel = nullptr;
        if (LayerPtr->Effects.bHasColorOverlay)
        {
            if (UImage* Img = Cast<UImage>(Widget))
            {
                // Existing image path — tint brush in place
                FSlateBrush Brush = Img->GetBrush();
                Brush.TintColor = FSlateColor(LayerPtr->Effects.ColorOverlayColor);
                Img->SetBrush(Brush);
            }
            else if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
            {
                // D-04/D-07: defer overlay insertion until after PSD children recurse,
                // so overlay is the LAST child (Pitfall 1).
                DeferredOverlayPanel = Panel;
            }
            else
            {
                UE_LOG(LogPSD2UMG, Warning,
                    TEXT("Color overlay on layer '%s' (type=%d) — no UImage or UPanelWidget; ignored."),
                    *LayerPtr->Name, static_cast<int32>(LayerPtr->Type));
            }
        }

        // Recurse into group children (skip if flattened)
        UE_LOG(LogPSD2UMG, Log, TEXT("Layer '%s': type=%d children=%d bFlattened=%d"),
            *Layer.Name, static_cast<int32>(Layer.Type), Layer.Children.Num(), bFlattened ? 1 : 0);
        if (!bFlattened && Layer.Type == EPsdLayerType::Group && !Layer.Children.IsEmpty())
        {
            if (UPanelWidget* ChildPanel = Cast<UPanelWidget>(Widget))
            {
                // Canvas panel children use local coordinate space:
                // CanvasSize = panel size, ParentOffset = panel top-left in parent space.
                const bool bChildIsCanvas = Cast<UCanvasPanel>(ChildPanel) != nullptr;
                const FIntPoint ChildCanvasSize = (bChildIsCanvas && !LayerPtr->Bounds.IsEmpty())
                    ? FIntPoint(LayerPtr->Bounds.Width(), LayerPtr->Bounds.Height())
                    : CanvasSize;
                const FIntPoint ChildParentOffset = (bChildIsCanvas && !LayerPtr->Bounds.IsEmpty())
                    ? LayerPtr->Bounds.Min
                    : ParentOffset;
                PopulateChildren(Registry, Tree, ChildPanel, Layer.Children, Doc, ChildCanvasSize, SkippedLayerNames, ChildParentOffset);
            }
            else
            {
                UE_LOG(LogPSD2UMG, Error,
                    TEXT("Group layer '%s' mapped to non-panel widget '%s' (class %s); %d children dropped"),
                    *Layer.Name, *Widget->GetName(), *Widget->GetClass()->GetName(), Layer.Children.Num());
            }
        }

        // FX-03 deferred: group-panel color overlay — insert overlay UImage as LAST child
        // (after PSD children recursion completes). D-05/D-06/D-07 per CONTEXT.md.
        if (DeferredOverlayPanel)
        {
            const FString OverlayBaseName = !LayerPtr->ParsedTags.CleanName.IsEmpty()
                ? LayerPtr->ParsedTags.CleanName
                : LayerPtr->Name;
            const FName OverlayFName = MakeUniqueObjectName(
                Tree, UImage::StaticClass(),
                FName(*FString::Printf(TEXT("%s_ColorOverlay"), *OverlayBaseName)));
            UImage* OverlayImg = Tree->ConstructWidget<UImage>(UImage::StaticClass(), OverlayFName);

            FSlateBrush OverlayBrush;
            OverlayBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
            OverlayBrush.TintColor = FSlateColor(LayerPtr->Effects.ColorOverlayColor);
            OverlayImg->SetBrush(OverlayBrush);
            OverlayImg->SetRenderOpacity(LayerPtr->Effects.ColorOverlayColor.A);

            if (UCanvasPanel* CanvasGroup = Cast<UCanvasPanel>(DeferredOverlayPanel))
            {
                // D-05: fill anchors so overlay covers the entire canvas group
                UCanvasPanelSlot* OverlaySlot = CanvasGroup->AddChildToCanvas(OverlayImg);
                if (OverlaySlot)
                {
                    FAnchorData FillData;
                    FillData.Anchors   = FAnchors(0.f, 0.f, 1.f, 1.f);
                    FillData.Offsets   = FMargin(0.f, 0.f, 0.f, 0.f);
                    FillData.Alignment = FVector2D(0.f, 0.f);
                    OverlaySlot->SetLayout(FillData);
                    // Open Question 1 resolution: ZOrder = current child count - 1
                    // (overlay's own index after AddChildToCanvas, which is already
                    // past all PSD children since recursion completed).
                    OverlaySlot->SetZOrder(CanvasGroup->GetChildrenCount() - 1);
                }
            }
            else
            {
                // D-06: non-canvas panel — best-effort AddChild, no slot config available
                DeferredOverlayPanel->AddChild(OverlayImg);
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

    // Step 1b: If a WBP already exists in this package (e.g. user re-imports the
    // same PSD via File > Import instead of the Reimport action), route directly
    // to Update() rather than evicting the old WBP via MarkAsGarbage.
    // Eviction + GC during a subsequent CompileBlueprint leaves REINST_ classes
    // and garbage objects still reachable from the package when SaveLoadedAsset
    // starts its traversal, triggering crashes in IsEditorOnlyObjectWithoutWritingCache.
    if (UWidgetBlueprint* ExistingWBP = FindObject<UWidgetBlueprint>(WbpPackage, *WbpAssetName))
    {
        UE_LOG(LogPSD2UMG, Log,
            TEXT("FWidgetBlueprintGenerator: WBP '%s' already exists — routing to Update()"), *WbpAssetName);
        return Update(ExistingWBP, Doc, SkippedLayerNames) ? ExistingWBP : nullptr;
    }

    // Step 2: Create WBP via factory (canonical editor path).
    // Set RootWidgetClass = UCanvasPanel so the factory creates a canvas root.
    // FactoryCreateNew may overwrite this from project DefaultRootWidget settings,
    // but UCanvasPanel is the standard default, so it's preserved in almost all cases.
    UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>(GetTransientPackage());
    Factory->AddToRoot();
    Factory->ParentClass = UUserWidget::StaticClass();

    UE_LOG(LogPSD2UMG, Log, TEXT("FWidgetBlueprintGenerator: Step 2 FactoryCreateNew start for %s"), *WbpAssetName);
    UWidgetBlueprint* WBP = CastChecked<UWidgetBlueprint>(
        Factory->FactoryCreateNew(
            UWidgetBlueprint::StaticClass(),
            WbpPackage,
            FName(*WbpAssetName),
            RF_Public | RF_Standalone | RF_Transactional,
            nullptr,
            GWarn));
    UE_LOG(LogPSD2UMG, Log, TEXT("FWidgetBlueprintGenerator: Step 2 FactoryCreateNew done, WBP=%p"), WBP);

    Factory->RemoveFromRoot();

    if (!WBP)
    {
        UE_LOG(LogPSD2UMG, Error, TEXT("FWidgetBlueprintGenerator: FactoryCreateNew returned null for %s"), *WbpAssetName);
        return nullptr;
    }

    // Step 3: Get (or create) the root canvas.
    //
    // CRITICAL: Do NOT rename/evict the factory's root widget to the transient package.
    // AllWidgets is UPROPERTY(Instanced) — during CompileBlueprint's internal GC pass,
    // it resolves every TObjectPtr in AllWidgets. If the factory widget was moved to
    // transient, its handle can resolve to INDEX_NONE → GetObjectPtr(-1) → fatal checkf.
    //
    // Instead, USE the factory's CanvasPanel as our root. PSD layers are added as its
    // children. The factory widget stays properly registered in GUObjectArray throughout.
    UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WBP->WidgetTree->RootWidget);
    if (!RootCanvas)
    {
        // Project settings produced no canvas (DefaultRootWidget = null or non-canvas).
        // No factory widget exists to orphan, so creating ours here is safe.
        RootCanvas = WBP->WidgetTree->ConstructWidget<UCanvasPanel>(
            UCanvasPanel::StaticClass(), TEXT("Root_Canvas"));
        WBP->WidgetTree->RootWidget = RootCanvas;
    }

    // Step 4: Populate widget tree recursively
    // Set the thread-local package path so FSmartObjectLayerMapper can obtain
    // the parent WBP location without interface changes (per plan 06-02 depth strategy).
    FSmartObjectImporter::SetCurrentPackagePath(WbpPackagePath);
    FLayerMappingRegistry Registry;
    UE_LOG(LogPSD2UMG, Log, TEXT("FWidgetBlueprintGenerator: Step 4 PopulateChildren start, %d root layers"), Doc.RootLayers.Num());
    PopulateChildren(Registry, WBP->WidgetTree, RootCanvas, Doc.RootLayers, Doc, Doc.CanvasSize, SkippedLayerNames);
    UE_LOG(LogPSD2UMG, Log, TEXT("FWidgetBlueprintGenerator: Step 4 PopulateChildren done"));

    // Step 5: Compile AFTER full tree population (critical — compiling before population leaves empty BP).
    // SkipGarbageCollection defers GC to the explicit call below (matches ReloadUtilities.cpp pattern).
    UE_LOG(LogPSD2UMG, Log, TEXT("FWidgetBlueprintGenerator: Step 5 CompileBlueprint start for %s"), *WbpAssetName);
    FKismetEditorUtilities::CompileBlueprint(WBP, EBlueprintCompileOptions::SkipGarbageCollection);
    UE_LOG(LogPSD2UMG, Log, TEXT("FWidgetBlueprintGenerator: Step 5 CompileBlueprint done"));

    // Set design canvas size to match PSD so the WBP designer view shows the correct layout.
    if (WBP->GeneratedClass)
    {
        if (UUserWidget* CDO = Cast<UUserWidget>(WBP->GeneratedClass->GetDefaultObject()))
        {
            CDO->DesignTimeSize = FVector2D(static_cast<float>(Doc.CanvasSize.X), static_cast<float>(Doc.CanvasSize.Y));
            CDO->DesignSizeMode = EDesignPreviewSizeMode::Custom;
        }
    }

    // Step 5b: Flush garbage with standard editor keep-flags (RF_Standalone).
    // RF_NoFlags would strip the RF_Standalone protection from WBP, textures, and widgets,
    // letting them be collected before SaveLoadedAsset traverses the package.
    // GARBAGE_COLLECTION_KEEPFLAGS == RF_Standalone in editor mode (matches ReloadUtilities.cpp:1202).
    UE_LOG(LogPSD2UMG, Log, TEXT("FWidgetBlueprintGenerator: Step 5b CollectGarbage start"));
    CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, /*bPurgeObjectsOnFullPurge=*/true);
    UE_LOG(LogPSD2UMG, Log, TEXT("FWidgetBlueprintGenerator: Step 5b CollectGarbage done"));

    // Step 6: Mark dirty and save
    UE_LOG(LogPSD2UMG, Log, TEXT("FWidgetBlueprintGenerator: Step 6 SaveLoadedAsset start"));
    WbpPackage->MarkPackageDirty();
    UEditorAssetLibrary::SaveLoadedAsset(WBP, false);
    UE_LOG(LogPSD2UMG, Log, TEXT("FWidgetBlueprintGenerator: Step 6 SaveLoadedAsset done"));

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

            // Resolve CleanName collisions (duplicate Photoshop layer names) by
            // mangling to a unique variant. Same pattern as PopulateChildren.
            const FPsdLayer* LayerForMap = &Layer;
            FPsdLayer UniqueLayer;
            if (Tree && !Layer.ParsedTags.CleanName.IsEmpty())
            {
                const FName BaseFName(*Layer.ParsedTags.CleanName);
                if (StaticFindObjectFast(nullptr, Tree, BaseFName))
                {
                    const FName UniqueFName = MakeUniqueObjectName(Tree, UWidget::StaticClass(), BaseFName);
                    UE_LOG(LogPSD2UMG, Warning,
                        TEXT("Update: layer '%s' clean name '%s' collides; renaming to '%s'"),
                        *Layer.Name, *Layer.ParsedTags.CleanName, *UniqueFName.ToString());
                    UniqueLayer = Layer;
                    UniqueLayer.ParsedTags.CleanName = UniqueFName.ToString();
                    LayerForMap = &UniqueLayer;
                }
            }

            UWidget* NewWidget = Registry.MapLayer(*LayerForMap, Doc, Tree);
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

    // Compile, flush GC, then save — same pattern as Generate().
    FKismetEditorUtilities::CompileBlueprint(ExistingWBP, EBlueprintCompileOptions::SkipGarbageCollection);
    CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, /*bPurgeObjectsOnFullPurge=*/true);
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
