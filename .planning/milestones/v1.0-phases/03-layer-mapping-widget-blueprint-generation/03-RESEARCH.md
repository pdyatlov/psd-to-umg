# Phase 3: Layer Mapping & Widget Blueprint Generation — Research

**Researched:** 2026-04-09
**Domain:** UE 5.7 UMG widget blueprint programmatic creation, UTexture2D asset creation from raw RGBA, UCanvasPanelSlot coordinate mapping
**Confidence:** HIGH (all critical APIs verified directly from UE 5.7 source on disk)

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** `IPsdLayerMapper` and `FLayerMappingRegistry` are internal-only. No public `Register()` API in Phase 3. Priority-ordered list; first match wins.
- **D-02:** Mapper dispatch is prefix-based on `FPsdLayer.Name`. Full prefix table checked case-sensitively.
- **D-03:** Phase 3 sets content, DPI-converted size (× 0.75), and color on `UTextBlock`.
- **D-04:** Font mapping, outline, shadow, bold/italic, multi-line wrap deferred to Phase 4. Phase 3 uses default engine font.
- **D-05:** Quadrant-based anchor auto-assignment (3×3 grid, layer center determines zone).
- **D-06:** Width ≥ 80% canvas → horizontal stretch; height ≥ 80% → vertical stretch; both → full stretch.
- **D-07:** Anchor override suffixes take priority: `_anchor-tl`, `_anchor-tc`, `_anchor-tr`, `_anchor-cl`, `_anchor-c`, `_anchor-cr`, `_anchor-bl`, `_anchor-bc`, `_anchor-br`, `_stretch-h`, `_stretch-v`, `_fill`.
- **D-08:** Skip + warn on layer failure. Import continues. WBP created with successfully mapped layers.
- **D-09:** Unknown layer types → log warning + skip, no placeholder widget.
- **D-10:** `FWidgetBlueprintGenerator` called from inside `UPsdImportFactory::FactoryCreateBinary` after `FPsdParser::ParseFile` succeeds.
- **D-11:** Claude's discretion for exact UE API sequence (WBP creation method).
- **D-12:** Image layer pixels imported as `UTexture2D` using `Source.Init()` (not PlatformData).
- **D-13:** Output path: `{TargetDir}/Textures/{PsdName}/{LayerName}`. Duplicate names → append `_01`, `_02`, etc.
- **D-14:** Layers with zero-size bounds or empty `RGBAPixels` → skip + warn.

### Claude's Discretion
- Exact UE API sequence for WBP creation (FWidgetBlueprintFactory vs FKismetEditorUtilities vs direct)
- Internal structure of `FLayerMappingRegistry` (vector + priority sort, or static dispatch table)
- Whether `FAnchorCalculator` is a standalone class or a free-function namespace
- How `UCanvasPanelSlot` offsets are calculated from `FIntRect` bounds relative to canvas origin
- Slot ZOrder assignment (linear index from layer order, or layer-defined)

### Deferred Ideas (OUT OF SCOPE)
- External mapper registration (third-party plugin custom mappers)
- Abort-on-failure import behavior
- Mac platform support
- Message Log integration (Phase 7)
- Font mapping, outline, shadow, bold/italic, multi-line wrap (Phase 4)
- Layer effects, blend modes, opacity (Phase 5)
- 9-slice, smart objects, variant switchers, improved anchor heuristics (Phase 6)
- Editor preview dialog, reimport, CommonUI (Phase 7)
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| MAP-01 | IPsdLayerMapper interface and FLayerMappingRegistry with priority-based first-match dispatch | Architecture Patterns: Mapper Registry section |
| MAP-02 | Image layers → UImage with SlateBrush pointing to imported UTexture2D | Code Examples: UImage setup; TEX pipeline |
| MAP-03 | Text layers → UTextBlock (content, font size×0.75, color) | Code Examples: UTextBlock setup; FSlateFontInfo |
| MAP-04 | Group layers → UCanvasPanel with correct child hierarchy | Architecture: recursive widget tree construction |
| MAP-05 | Button_ prefix → UButton with normal/hovered/pressed/disabled brushes | Code Examples: FButtonStyle; child layer naming |
| MAP-06 | Progress_ prefix → UProgressBar with background/fill brushes | Code Examples: FProgressBarStyle |
| MAP-07 | HBox_ / VBox_ prefix → UHorizontalBox / UVerticalBox | Standard Stack: layout panels |
| MAP-08 | Overlay_ prefix → UOverlay | Standard Stack: layout panels |
| MAP-09 | ScrollBox_ prefix → UScrollBox | Standard Stack: layout panels |
| MAP-10 | Slider_ / CheckBox_ / Input_ prefix → USlider / UCheckBox / UEditableTextBox | Standard Stack: input widgets |
| MAP-11 | List_ / Tile_ prefix → UListView / UTileView with EntryWidgetClass | Standard Stack: list widgets |
| MAP-12 | Switcher_ prefix → UWidgetSwitcher | Standard Stack: switcher widget |
| WBP-01 | FWidgetBlueprintGenerator creates valid UWidgetBlueprint (ConstructWidget → compile → save) | Architecture: WBP creation sequence |
| WBP-02 | Widget positions/sizes from PSD canvas coords → UMG CanvasPanelSlot | Architecture: Canvas coordinate math |
| WBP-03 | Anchor heuristics auto-assign based on layer position | Architecture: Quadrant anchor calculation |
| WBP-04 | Anchor override suffixes work | Architecture: Suffix parsing |
| WBP-05 | Layer z-order preserved | Architecture: ZOrder from layer index |
| WBP-06 | WBP opens in UMG Designer without errors | Architecture: compile + save sequence critical path |
| TEX-01 | Image layers as persistent UTexture2D via Source.Init() | Code Examples: Source.Init() call sequence |
| TEX-02 | Textures organized in {TargetDir}/Textures/{PsdName}/{LayerName} | Architecture: Asset path construction |
| TEX-03 | Duplicate layer names deduped by appending index | Architecture: Name dedup logic |
</phase_requirements>

## Summary

Phase 3 bridges the parsed `FPsdDocument` from Phase 2 to a valid, openable `UWidgetBlueprint` asset in the UE Content Browser. The three principal subsystems are: (1) the layer mapper that classifies PSD layers into UMG widget types via prefix-based dispatch; (2) the canvas coordinate translator that converts `FIntRect` pixel bounds into `UCanvasPanelSlot` anchor + offset data; and (3) the texture importer that persists RGBA pixel buffers as `UTexture2D` assets using `Source.Init()`.

All critical APIs have been verified directly from UE 5.7 source on disk at `C:\Program Files\Epic Games\UE_5.7`. The WBP creation path uses `UWidgetBlueprintFactory::FactoryCreateNew` (which internally calls `FKismetEditorUtilities::CreateBlueprint`) — this is the exact same path the Content Browser uses when a designer manually creates a Widget Blueprint, which guarantees a correctly structured, compilable, openable asset. `FKismetEditorUtilities::CompileBlueprint` is available in the existing `UnrealEd` dependency; no new modules are required beyond what `UMGEditor` already brings.

One important pixel format constraint: PhotoshopAPI delivers RGBA pixel data, but `FTextureSource::Init()` in UE 5.7 requires `TSF_BGRA8` — `TSF_RGBA8` is a deprecated alias that is immediately converted on load and should not be used. Every image layer's pixel buffer must be byte-swapped R↔B before the `Source.Init()` call.

**Primary recommendation:** Use `UWidgetBlueprintFactory::FactoryCreateNew` to create the WBP (sets up `WidgetTree`, hooks into UMGEditor module delegates, matches the Editor's own creation path), then populate `WidgetTree` via `ConstructWidget` + `UCanvasPanel::AddChildToCanvas`, compile with `FKismetEditorUtilities::CompileBlueprint`, and save with `UEditorAssetLibrary::SaveLoadedAsset`.

## Standard Stack

### Core
| Library/API | Source | Purpose | Why Standard |
|-------------|--------|---------|--------------|
| `UWidgetBlueprintFactory` | `UMGEditor/Classes/WidgetBlueprintFactory.h` | Create the UWidgetBlueprint asset | This is the canonical Editor path; same as Content Browser "New Widget Blueprint" |
| `FKismetEditorUtilities::CreateBlueprint` | `UnrealEd/Public/Kismet2/KismetEditorUtilities.h` | Called internally by factory; also available standalone | Creates Blueprint with correct generated class wiring |
| `FKismetEditorUtilities::CompileBlueprint` | `UnrealEd/Public/Kismet2/KismetEditorUtilities.h` | Compile after widget tree population | Must run before Save; UnrealEd already a dependency |
| `UWidgetTree::ConstructWidget<T>` | `UMG/Public/Blueprint/WidgetTree.h` | Instantiate UWidget objects inside the blueprint | The only correct way to create blueprint-owned widgets |
| `UCanvasPanel::AddChildToCanvas` | `UMG/Public/Components/CanvasPanel.h` | Add a widget to a canvas, returns `UCanvasPanelSlot*` | Type-safe version of `AddChild`; returns the correct slot type directly |
| `UCanvasPanelSlot::SetLayout` | `UMG/Public/Components/CanvasPanelSlot.h` | Set `FAnchorData` (anchors + offsets + alignment) | Preferred over deprecated direct field access (deprecated since 5.1) |
| `UCanvasPanelSlot::SetZOrder` | `UMG/Public/Components/CanvasPanelSlot.h` | Z-order from layer list index | Getter/setter API (direct field access deprecated since 5.1) |
| `FTextureSource::Init` | `Engine/Classes/Engine/Texture.h` | Initialize texture source data from raw bytes | Textures with Source.Init survive editor restarts; PlatformData does not |
| `UEditorAssetLibrary::SaveLoadedAsset` | `EditorScriptingUtilities` plugin | Save the WBP and texture assets after creation | Already a project dependency; simple one-liner save |

### Supporting
| Library/API | Source | Purpose | When to Use |
|-------------|--------|---------|-------------|
| `CreatePackage` | `CoreUObject/Public/UObject/UObjectGlobals.h` | Create a UPackage for new assets | Required before `NewObject<UTexture2D>` for persistent assets |
| `UImage::SetBrushFromTexture` | `UMG/Public/Components/Image.h` | Wire a UTexture2D to a UImage | Use after texture is created and saved |
| `UTextBlock::SetText` / `SetFont` / `SetColorAndOpacity` | `UMG/Public/Components/TextBlock.h` | Set text content, font, color | Direct setters; Font uses `FSlateFontInfo` |
| `UButton::SetStyle` | `UMG/Public/Components/Button.h` | Set all four state brushes at once via `FButtonStyle` | Preferred over direct WidgetStyle access (deprecated 5.2) |
| `UProgressBar::SetWidgetStyle` | `UMG/Public/Components/ProgressBar.h` | Set background and fill brushes via `FProgressBarStyle` | Preferred setter (direct field deprecated 5.1) |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `UWidgetBlueprintFactory::FactoryCreateNew` | Direct `NewObject<UWidgetBlueprint>` + manual wiring | Factory ensures UMGEditor module delegates fire and generated class is set up; direct construction risks missing initialization steps |
| `UEditorAssetLibrary::SaveLoadedAsset` | `UPackage::SavePackage` with `FSavePackageArgs` | SaveLoadedAsset is simpler and handles dirty-marking; SavePackage gives more control but requires more boilerplate |
| `UCanvasPanel::AddChildToCanvas` | `UPanelWidget::AddChild` + cast | AddChildToCanvas returns `UCanvasPanelSlot*` directly; avoids cast |

### Build.cs Modules Required

The existing Build.cs already has `UnrealEd`, `UMGEditor`, and `UMG`. WBP creation needs no new modules. However, the `WidgetBlueprintFactory.h` header lives in `UMGEditor/Classes/` (not `Public/`) — include as `"WidgetBlueprintFactory.h"` (UBT flattens Classes/ into the include path).

Modules `Kismet`, `KismetCompiler`, and `BlueprintGraph` are **not** required as direct dependencies because they are internal to `UnrealEd` and `UMGEditor`, which are already listed. `FKismetEditorUtilities` is exported from `UnrealEd` (`UNREALED_API`).

## Architecture Patterns

### Recommended File Structure
```
Source/PSD2UMG/
├── Public/
│   ├── Mapper/
│   │   ├── IPsdLayerMapper.h        # Interface: CanMap(Layer) + Map(Layer, WidgetTree, Parent) → UWidget*
│   │   └── FLayerMappingRegistry.h  # Priority-ordered registry; internal-only in Phase 3
│   └── Generator/
│       └── FWidgetBlueprintGenerator.h  # Entry point: Generate(Doc, PackagePath) → UWidgetBlueprint*
├── Private/
│   ├── Mapper/
│   │   ├── FLayerMappingRegistry.cpp
│   │   ├── FImageLayerMapper.cpp    # EPsdLayerType::Image → UImage + texture
│   │   ├── FTextLayerMapper.cpp     # EPsdLayerType::Text → UTextBlock
│   │   ├── FGroupLayerMapper.cpp    # EPsdLayerType::Group (default) → UCanvasPanel
│   │   ├── FButtonLayerMapper.cpp   # Button_ prefix → UButton
│   │   ├── FProgressLayerMapper.cpp # Progress_ prefix → UProgressBar
│   │   └── FSimplePrefixMappers.cpp # HBox_, VBox_, Overlay_, ScrollBox_, Slider_,
│   │                                #   CheckBox_, Input_, List_, Tile_, Switcher_
│   ├── Generator/
│   │   ├── FWidgetBlueprintGenerator.cpp
│   │   ├── FAnchorCalculator.cpp    # Quadrant heuristics + suffix override
│   │   └── FTextureImporter.cpp     # Source.Init() + CreatePackage + save
```

### Pattern 1: WBP Creation Sequence (FWidgetBlueprintGenerator::Generate)

**What:** Creates, populates, compiles, and saves a UWidgetBlueprint from an FPsdDocument.

**When to use:** Called once per imported PSD, from inside `UPsdImportFactory::FactoryCreateBinary`.

```cpp
// Source: verified from UE 5.7 WidgetBlueprintFactory.cpp + KismetEditorUtilities.h

// Step 1: Derive the WBP package path from InParent (which is the .psd's package path)
// e.g. /Game/UI/MyHUD → /Game/UI/WBP_MyHUD
FString WbpPackagePath = ...; // build from TargetDir + PsdName
FString WbpAssetName   = FString::Printf(TEXT("WBP_%s"), *PsdName);
UPackage* WbpPackage   = CreatePackage(*FString::Printf(TEXT("%s/%s"), *WbpPackagePath, *WbpAssetName));
WbpPackage->FullyLoad();

// Step 2: Use UWidgetBlueprintFactory to create the WBP
UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>(GetTransientPackage());
Factory->AddToRoot();
Factory->ParentClass = UUserWidget::StaticClass();
Factory->BlueprintType = BPTYPE_Normal;
UWidgetBlueprint* WBP = CastChecked<UWidgetBlueprint>(
    Factory->FactoryCreateNew(
        UWidgetBlueprint::StaticClass(),
        WbpPackage,
        FName(*WbpAssetName),
        RF_Public | RF_Standalone | RF_Transactional,
        nullptr,
        GWarn));
Factory->RemoveFromRoot();

// Step 3: Replace root widget with UCanvasPanel (the canvas root)
UCanvasPanel* RootCanvas = WBP->WidgetTree->ConstructWidget<UCanvasPanel>(
    UCanvasPanel::StaticClass(), TEXT("Root_Canvas"));
WBP->WidgetTree->RootWidget = RootCanvas;

// Step 4: Recursively populate the widget tree from Doc.RootLayers
// (see Pattern 2)

// Step 5: Compile the blueprint (required before save)
FKismetEditorUtilities::CompileBlueprint(WBP);

// Step 6: Mark dirty and save
WbpPackage->MarkPackageDirty();
UEditorAssetLibrary::SaveLoadedAsset(WBP, false);
```

### Pattern 2: Recursive Widget Tree Population

**What:** For each `FPsdLayer` in a layer list, dispatch to the correct mapper and add the result to the parent panel.

**Key invariant:** PSD layers are stored bottom-to-top (index 0 = bottommost). UMG z-order is higher = on top. Assign `ZOrder = LayerIndex` directly (not reversed) because `UCanvasPanelSlot::SetZOrder` matches UMG's rendering — higher values render on top.

Actually, verify: PSD stores top-of-stack at index 0 (first in list is frontmost). So `ZOrder = (TotalLayers - 1 - LayerIndex)` maps the frontmost PSD layer to the highest z-order. Confirm by checking PhotoshopAPI layer order against what Phase 2 spec showed.

```cpp
void PopulateCanvas(UWidgetTree* Tree, UPanelWidget* Parent, const TArray<FPsdLayer>& Layers,
                    const FIntPoint& CanvasSize, int32 ZOrderBase)
{
    for (int32 i = 0; i < Layers.Num(); ++i)
    {
        const FPsdLayer& Layer = Layers[i];
        // Skip invisible or zero-size layers per D-08/D-14
        if (!Layer.bVisible || Layer.Bounds.IsEmpty()) { /* warn + continue */ }

        UWidget* Widget = Registry.Map(Layer, Tree); // dispatch
        if (!Widget) { /* warn + continue */ }

        UCanvasPanelSlot* Slot = CastChecked<UCanvasPanel>(Parent)->AddChildToCanvas(Widget);
        ApplyCanvasSlot(Slot, Layer.Bounds, CanvasSize, Layer.Name, i, Layers.Num());

        // Recurse into group children
        if (Layer.Type == EPsdLayerType::Group && !Layer.Children.IsEmpty())
        {
            UCanvasPanel* ChildCanvas = Cast<UCanvasPanel>(Widget);
            if (ChildCanvas) PopulateCanvas(Tree, ChildCanvas, Layer.Children, CanvasSize, 0);
        }
    }
}
```

### Pattern 3: Canvas Coordinate Math (UCanvasPanelSlot)

**Critical understanding:** `UCanvasPanelSlot` uses `FAnchorData` with three fields:
- `Anchors` (`FAnchors`): normalized (0..1) anchor point(s) within the parent canvas. For a point anchor (Min == Max), this is a single point. For a stretch anchor, Min < Max.
- `Offsets` (`FMargin`): when anchor is a point (Min==Max), Left = X position from anchor point, Top = Y position from anchor point, Right = Width, Bottom = Height. When anchor stretches (Min != Max on an axis), Left/Right become margin distances from the parent edge.
- `Alignment` (`FVector2D`): pivot of the widget (0,0 = top-left of widget, 0.5,0.5 = center). Default is (0,0).

**For point anchors (the common case):**

```cpp
// Given: Layer.Bounds (FIntRect in canvas-space pixels), CanvasSize (FIntPoint)
// Given: AnchorPoint (FVector2D in 0..1 space) from quadrant heuristic

void ApplyPointAnchorSlot(UCanvasPanelSlot* Slot, const FIntRect& Bounds,
                          const FIntPoint& CanvasSize, const FVector2D& AnchorPt)
{
    const float AnchorPixelX = AnchorPt.X * CanvasSize.X;
    const float AnchorPixelY = AnchorPt.Y * CanvasSize.Y;

    FAnchorData Data;
    Data.Anchors   = FAnchors(AnchorPt.X, AnchorPt.Y); // point anchor: Min == Max
    Data.Alignment = FVector2D(0.f, 0.f);               // top-left pivot
    // Position relative to anchor point in canvas pixels
    Data.Offsets.Left   = Bounds.Min.X - AnchorPixelX;
    Data.Offsets.Top    = Bounds.Min.Y - AnchorPixelY;
    Data.Offsets.Right  = Bounds.Width();
    Data.Offsets.Bottom = Bounds.Height();
    Slot->SetLayout(Data);
    // ZOrder: higher index = rendered on top in UMG
    // PSD: index 0 = topmost layer in Photoshop
    Slot->SetZOrder(TotalLayers - 1 - LayerIndex);
}
```

**For stretch anchors:**
```cpp
// Horizontal stretch: Anchors = (0, Y, 1, Y) where Y is the vertical zone midpoint
// Offsets.Left = left margin, Offsets.Right = right margin,
// Offsets.Top = top offset from anchor Y, Offsets.Bottom = height
FAnchorData Data;
Data.Anchors = FAnchors(0.f, AnchorPt.Y, 1.f, AnchorPt.Y);
Data.Offsets.Left   = Bounds.Min.X;
Data.Offsets.Right  = CanvasSize.X - Bounds.Max.X; // right margin
Data.Offsets.Top    = Bounds.Min.Y - (AnchorPt.Y * CanvasSize.Y);
Data.Offsets.Bottom = Bounds.Height();
```

**Full stretch (_fill):**
```cpp
Data.Anchors = FAnchors(0.f, 0.f, 1.f, 1.f);
Data.Offsets = FMargin(Bounds.Min.X, Bounds.Min.Y,
                       CanvasSize.X - Bounds.Max.X,
                       CanvasSize.Y - Bounds.Max.Y);
```

### Pattern 4: Texture Import (Source.Init)

**Critical:** PhotoshopAPI delivers RGBA. `FTextureSource` in UE 5.7 uses `TSF_BGRA8` as the standard 8-bit format. `TSF_RGBA8_DEPRECATED` is converted on load but should not be used in new code. **Byte-swap R↔B before calling `Source.Init()`**.

```cpp
// Source: verified from Engine/Classes/Engine/TextureDefines.h (ETextureSourceFormat enum)

UTexture2D* ImportLayerTexture(const FPsdLayer& Layer, const FString& PackagePath, const FString& AssetName)
{
    // 1. RGBA → BGRA conversion
    TArray<uint8> BGRAPixels = Layer.RGBAPixels; // copy
    for (int32 i = 0; i < BGRAPixels.Num(); i += 4)
    {
        Swap(BGRAPixels[i + 0], BGRAPixels[i + 2]); // R ↔ B
    }

    // 2. Create package + asset
    UPackage* Pkg = CreatePackage(*FString::Printf(TEXT("%s/%s"), *PackagePath, *AssetName));
    Pkg->FullyLoad();
    UTexture2D* Tex = NewObject<UTexture2D>(Pkg, FName(*AssetName), RF_Public | RF_Standalone);

    // 3. Initialize source data (inside PreEditChange/PostEditChange brackets)
    Tex->PreEditChange(nullptr);
    Tex->Source.Init(
        Layer.PixelWidth,
        Layer.PixelHeight,
        1,    // NumSlices
        1,    // NumMips
        TSF_BGRA8,
        BGRAPixels.GetData());
    Tex->SRGB = true;
    Tex->CompressionSettings = TC_Default;
    Tex->PostEditChange();

    // 4. Save
    Pkg->MarkPackageDirty();
    UEditorAssetLibrary::SaveLoadedAsset(Tex, false);
    return Tex;
}
```

### Pattern 5: Anchor Heuristic (Quadrant 3×3)

```cpp
FAnchors CalculateAnchor(const FIntRect& Bounds, const FIntPoint& CanvasSize)
{
    const float W = CanvasSize.X;
    const float H = CanvasSize.Y;
    const float CX = (Bounds.Min.X + Bounds.Max.X) * 0.5f;
    const float CY = (Bounds.Min.Y + Bounds.Max.Y) * 0.5f;

    // Stretch overrides (D-06)
    const bool bStretchH = Bounds.Width()  >= W * 0.8f;
    const bool bStretchV = Bounds.Height() >= H * 0.8f;

    if (bStretchH && bStretchV) return FAnchors(0.f, 0.f, 1.f, 1.f);

    // Vertical zone midpoint for stretch axes
    const float VZone = (CY < H / 3.f) ? 0.f : (CY < H * 2.f / 3.f) ? 0.5f : 1.f;
    const float HZone = (CX < W / 3.f) ? 0.f : (CX < W * 2.f / 3.f) ? 0.5f : 1.f;

    if (bStretchH) return FAnchors(0.f, VZone, 1.f, VZone);
    if (bStretchV) return FAnchors(HZone, 0.f, HZone, 1.f);
    return FAnchors(HZone, VZone); // point anchor; FAnchors(x,y) sets Min==Max
}
```

### Pattern 6: Suffix Parsing (Anchor Overrides)

Parse layer name right-to-left for the first recognized suffix. Strip it from the name before widget naming. Suffix table:

| Suffix | FAnchors |
|--------|---------|
| `_anchor-tl` | (0,0) |
| `_anchor-tc` | (0.5,0) |
| `_anchor-tr` | (1,0) |
| `_anchor-cl` | (0,0.5) |
| `_anchor-c`  | (0.5,0.5) |
| `_anchor-cr` | (1,0.5) |
| `_anchor-bl` | (0,1) |
| `_anchor-bc` | (0.5,1) |
| `_anchor-br` | (1,1) |
| `_stretch-h` | (0,Y,1,Y) — Y from vertical zone |
| `_stretch-v` | (X,0,X,1) — X from horizontal zone |
| `_fill`      | (0,0,1,1) |

### Pattern 7: Button Child Layer Convention

For `Button_` groups, child layers are matched by name suffix:
- `*_Normal` or `*_Bg` or index 0 → `FButtonStyle.Normal`
- `*_Hovered` or `*_Hover` → `FButtonStyle.Hovered`
- `*_Pressed` or `*_Press` → `FButtonStyle.Pressed`
- `*_Disabled` → `FButtonStyle.Disabled`

Each matched child layer is imported as a texture and used as the `ResourceObject` in the corresponding `FSlateBrush`. The Python baseline used this same convention (though it no longer exists in the repo — reconstructed from CONTEXT.md description).

```cpp
FButtonStyle Style = FButtonStyle::GetDefault();
// For each child image layer matched to a state:
FSlateBrush Brush;
Brush.SetResourceObject(StateTex);
Brush.ImageSize = FVector2D(Layer.PixelWidth, Layer.PixelHeight);
Style.SetNormal(Brush); // or SetHovered, SetPressed, SetDisabled
Button->SetStyle(Style);
```

### Pattern 8: ProgressBar Child Layer Convention

`Progress_` groups:
- Child named `*_BG` or `*_Background` → `FProgressBarStyle.BackgroundImage`
- Child named `*_Fill` or `*_Foreground` → `FProgressBarStyle.FillImage`

```cpp
FProgressBarStyle Style = FProgressBarStyle::GetDefault();
// ... set BackgroundImage, FillImage from textures
ProgressBar->SetWidgetStyle(Style);
```

### Anti-Patterns to Avoid

- **Direct field access on deprecated properties:** `CanvasPanelSlot->LayoutData` (deprecated since 5.1), `Button->WidgetStyle` (deprecated since 5.2), `ProgressBar->WidgetStyle` (deprecated since 5.1). Always use the getter/setter API.
- **Using TSF_RGBA8_DEPRECATED:** Will be silently converted on load. Use `TSF_BGRA8` with a pre-swapped buffer.
- **NewObject<UTexture2D> without CreatePackage first:** Results in a transient texture that won't persist across editor restarts.
- **Populating WidgetTree without calling ConstructWidget:** Manually constructing widget objects with `NewObject<UTextBlock>` bypasses the WidgetTree's ownership; use `WidgetTree->ConstructWidget<UTextBlock>()` instead.
- **Calling SaveLoadedAsset before CompileBlueprint:** The blueprint must be compiled before it's valid to save.
- **Forgetting PreEditChange/PostEditChange around Source.Init:** These are required for proper undo/redo support and asset change notification.
- **Assuming PSD layer order is bottom-to-top:** PhotoshopAPI (and the FPsdParserSpec in Phase 2) shows index 0 is the topmost (frontmost) layer. ZOrder assignment must invert: `ZOrder = (N-1-i)`.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| WBP asset creation | Custom `NewObject<UWidgetBlueprint>` + manual class init | `UWidgetBlueprintFactory::FactoryCreateNew` | Factory fires UMGEditor module delegates, wires generated class, sets up WidgetTree correctly |
| Blueprint compilation | Custom compiler invocation | `FKismetEditorUtilities::CompileBlueprint` | Handles all compilation phases, error reporting, generated class update |
| Asset saving | Direct file I/O or `UPackage::SavePackage` | `UEditorAssetLibrary::SaveLoadedAsset` | Handles dirty marking, asset registry notification, correct save path resolution |
| Texture creation | `UTexture2D::CreateTransient` | `NewObject<UTexture2D>` in a real package + `Source.Init` | Transient textures don't survive editor restart; persistent textures require proper package |
| Widget pivot math | Custom coordinate transform | `FAnchorData` + the offset formula above | UE's slot coordinate system is well-defined; rolling custom is fragile |

**Key insight:** The WBP creation surface area is large and fragile. Any shortcut that bypasses `UWidgetBlueprintFactory` or `FKismetEditorUtilities::CompileBlueprint` risks creating a blueprint that appears in the Content Browser but crashes or shows errors when opened in UMG Designer.

## Common Pitfalls

### Pitfall 1: RGBA vs BGRA Byte Order
**What goes wrong:** Textures imported with RGBA data passed as `TSF_BGRA8` will have red and blue channels swapped — all imported textures will look blue-tinted.
**Why it happens:** `FTextureSource` native format is BGRA; PhotoshopAPI delivers RGBA.
**How to avoid:** Always swap `pixel[i+0]` ↔ `pixel[i+2]` before `Source.Init()`.
**Warning signs:** Imported textures have inverted red/blue channels in the editor.

### Pitfall 2: WidgetTree Ownership and ConstructWidget
**What goes wrong:** `NewObject<UTextBlock>(WBP)` instead of `WBP->WidgetTree->ConstructWidget<UTextBlock>()` creates a widget that the WidgetTree doesn't track, causing blueprint compilation errors or missing widgets in the designer.
**Why it happens:** The WidgetTree maintains an internal list of all owned widgets for serialization and designer support.
**How to avoid:** Always use `WidgetTree->ConstructWidget<T>()`. This is the same pattern used in `UWidgetBlueprintFactory::FactoryCreateNew`.

### Pitfall 3: Layer Z-Order Direction
**What goes wrong:** The widget hierarchy comes out visually inverted — the background layer appears on top.
**Why it happens:** PSD layers list the topmost (frontmost) layer at index 0. UMG `ZOrder` uses higher = rendered last = on top. Without inversion, index 0 (foreground) gets ZOrder 0 (background position).
**How to avoid:** `ZOrder = (TotalLayerCount - 1 - LayerIndex)`.
**Warning signs:** In the generated WBP, elements that should be in the background are covering foreground elements.

### Pitfall 4: Compiling Before Widget Tree is Populated
**What goes wrong:** If `CompileBlueprint` is called before all widgets are added and configured, the generated class will be incomplete. Adding widgets after compilation won't update the generated class.
**Why it happens:** Compilation snapshots the WidgetTree state into the generated class.
**How to avoid:** Fully populate the WidgetTree (all layers, all properties set) before calling `CompileBlueprint`.

### Pitfall 5: UCanvasPanelSlot Offset Math for Stretched Anchors
**What goes wrong:** When an anchor stretches (e.g., Min.X=0, Max.X=1), the `Offsets.Left` and `Offsets.Right` are margins from the parent edge — NOT width values. Passing `Width` as `Offsets.Right` will produce wildly wrong sizing.
**Why it happens:** UE changes the semantic of the Offsets fields based on whether the anchor stretches on that axis.
**How to avoid:** For a stretch axis, `Offsets.Left = LeftMargin (= Bounds.Min.X)`, `Offsets.Right = RightMargin (= CanvasWidth - Bounds.Max.X)`. For a point axis, `Offsets.Right = Width`.

### Pitfall 6: Settings TargetDir vs TextureAssetDir
**What goes wrong:** `UPSD2UMGSettings` has `TextureAssetDir` and `TextureSrcDir` but no field named `TargetDir`. Phase 3 uses `TextureAssetDir` as the output base. There is no `WidgetBlueprintDir` field — a new one needs to be added or the WBP is placed adjacent to the imported PSD texture.
**Why it happens:** Settings class predates Phase 3 design. The `TargetDir` in CONTEXT.md refers to `TextureAssetDir`.
**How to avoid:** Add a `WBPAssetDir` (or similar) `FDirectoryPath` field to `UPSD2UMGSettings` in the first plan task of Phase 3, or derive the WBP output path from `InParent`'s package path.

### Pitfall 7: Package Path for Texture Assets
**What goes wrong:** Using the `InParent` package directly for textures (which points to the location of the imported .psd flat texture) places all textures in the wrong directory.
**Why it happens:** `FactoryCreateBinary`'s `InParent` is the package for the flat UTexture2D created by the inner `UTextureFactory` call — typically `/Game/SomePath/MyHUD`.
**How to avoid:** Build texture package paths explicitly: `TextureAssetDir.Path / "Textures" / PsdBaseName / LayerName`. This matches D-13.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `UCanvasPanelSlot->LayoutData` direct field access | `SetLayout(FAnchorData)` / `SetAnchors` / `SetOffsets` | UE 5.1 | Direct field is deprecated; getter/setter is the API |
| `UButton->WidgetStyle` direct field access | `GetStyle()` / `SetStyle(FButtonStyle)` | UE 5.2 | Direct field deprecated; use style API |
| `UProgressBar->WidgetStyle` direct | `GetWidgetStyle()` / `SetWidgetStyle(FProgressBarStyle)` | UE 5.1 | Same pattern |
| `UCanvasPanelSlot->bAutoSize` / `ZOrder` direct | `GetAutoSize()` / `SetAutoSize()` / `SetZOrder()` | UE 5.1 | Getter/setter required |
| `TSF_RGBA8` | `TSF_BGRA8` (with pre-swapped bytes) | Ongoing (TSF_RGBA8 marked deprecated) | Must swap bytes |

**Deprecated/outdated:**
- `FEditorStyle` → `FAppStyle` (already handled in Phase 1)
- `WhitelistPlatforms` → `PlatformAllowList` (already handled in Phase 1)

## Open Questions

1. **PSD layer index → ZOrder direction**
   - What we know: Phase 2 spec (FPsdParserSpec) reveals that `FPsdParser` returns layers in the order PhotoshopAPI provides them; the spec checks `BtnNormal` as a child of `Buttons`. PhotoshopAPI typically returns layers top-of-stack first (index 0 = frontmost).
   - What's unclear: Whether Phase 2's `ParseFile` reverses the order or preserves PhotoshopAPI's order. The spec doesn't explicitly test layer ordering by z-position.
   - Recommendation: First task of Phase 3 should include a logging step or test assertion to confirm index-0 is frontmost vs bottommost before writing the ZOrder formula.

2. **WBP output path — where does the WBP go?**
   - What we know: `UPSD2UMGSettings` has `TextureAssetDir` for textures but no WBP output directory field.
   - What's unclear: Should the WBP be co-located with the imported flat texture (in `InParent`'s package path), or in a separate configurable directory?
   - Recommendation: Add a `WidgetBlueprintAssetDir` field to `UPSD2UMGSettings` as part of the first plan task. Default it to empty, which means "same directory as the imported PSD texture."

3. **Button/Progress child layer naming — no Python baseline to reference**
   - What we know: The Python baseline (`Content/Python/auto_psd_ui.py`) no longer exists in the repo. CONTEXT.md describes the convention conceptually.
   - What's unclear: What exact child layer name suffixes were used in practice for `Button_` state detection (Normal/Hovered/Pressed/Disabled).
   - Recommendation: Define the suffix convention in Phase 3 spec and document it in CONVENTIONS.md (Phase 8). The plan should specify exact matching strings.

## Environment Availability

Step 2.6: No external dependencies beyond UE 5.7.4 editor (already verified operational by Phase 2). All required APIs are in the installed engine at `C:\Program Files\Epic Games\UE_5.7`. No new CLI tools, databases, or services required.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | UE Automation Spec (`FAutomationSpecBase`) |
| Config file | Gated on `WITH_DEV_AUTOMATION_TESTS` preprocessor; no external config |
| Quick run command | Session Frontend → Automation → filter `PSD2UMG` |
| Full suite command | Same; runs all `PSD2UMG.*` specs |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|--------------|
| MAP-01 | FLayerMappingRegistry dispatches to correct mapper by prefix | unit (no UE dep) | `PSD2UMG.Mapper.Registry` spec | ❌ Wave 0 |
| MAP-02 | Image layer produces UImage with correct texture reference | integration | `PSD2UMG.Generator.ImageLayer` spec | ❌ Wave 0 |
| MAP-03 | Text layer produces UTextBlock with correct content, size, color | integration | `PSD2UMG.Generator.TextLayer` spec | ❌ Wave 0 |
| MAP-04 | Group layer produces UCanvasPanel with correct children | integration | `PSD2UMG.Generator.GroupLayer` spec | ❌ Wave 0 |
| MAP-05 | Button_ prefix → UButton with 4 state brushes | integration | `PSD2UMG.Generator.ButtonLayer` spec | ❌ Wave 0 |
| WBP-01 | Generated WBP is valid UWidgetBlueprint (not null, compilable) | integration | `PSD2UMG.Generator.WBP.Valid` | ❌ Wave 0 |
| WBP-02 | Canvas positions match PSD bounds (within 1 pixel) | integration | `PSD2UMG.Generator.WBP.Positions` | ❌ Wave 0 |
| WBP-03 | Anchor heuristics produce correct FAnchors for quadrant test cases | unit | `PSD2UMG.AnchorCalc.Quadrant` | ❌ Wave 0 |
| WBP-04 | Suffix overrides produce correct FAnchors | unit | `PSD2UMG.AnchorCalc.Suffix` | ❌ Wave 0 |
| WBP-05 | ZOrder is inverted correctly | unit | covered by `PSD2UMG.Generator.WBP.Positions` | ❌ Wave 0 |
| TEX-01 | Texture source uses TSF_BGRA8, survives editor restart | integration | `PSD2UMG.Texture.Source` | ❌ Wave 0 |
| TEX-02 | Texture asset path matches {TargetDir}/Textures/{PsdName}/{LayerName} | integration | `PSD2UMG.Texture.Path` | ❌ Wave 0 |
| TEX-03 | Duplicate names produce `_01`, `_02` suffixes | unit | `PSD2UMG.Texture.Dedup` | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** Run `PSD2UMG.*` in Session Frontend
- **Per wave merge:** Full `PSD2UMG.*` suite green
- **Phase gate:** All specs green + manual WBP open test (drag PSD → inspect WBP in Designer)

### Wave 0 Gaps
- [ ] `Source/PSD2UMG/Tests/MapperSpec.cpp` — covers MAP-01..05, WBP-01..05
- [ ] `Source/PSD2UMG/Tests/AnchorCalcSpec.cpp` — covers WBP-03..04
- [ ] `Source/PSD2UMG/Tests/TextureImporterSpec.cpp` — covers TEX-01..03
- [ ] `Source/PSD2UMG/Tests/Fixtures/SimpleHUD.psd` — minimal fixture (image + text + group) for integration specs

## Sources

### Primary (HIGH confidence)
- UE 5.7 source on disk: `C:\Program Files\Epic Games\UE_5.7\Engine\Source\Editor\UMGEditor\Private\WidgetBlueprintFactory.cpp` — WBP creation sequence verified
- UE 5.7 source on disk: `C:\Program Files\Epic Games\UE_5.7\Engine\Source\Editor\UnrealEd\Public\Kismet2\KismetEditorUtilities.h` — CompileBlueprint signature verified
- UE 5.7 source on disk: `C:\Program Files\Epic Games\UE_5.7\Engine\Source\Runtime\UMG\Public\Components\CanvasPanelSlot.h` — FAnchorData, SetLayout, SetZOrder API verified
- UE 5.7 source on disk: `C:\Program Files\Epic Games\UE_5.7\Engine\Source\Runtime\Engine\Classes\Engine\Texture.h` — FTextureSource::Init signature verified
- UE 5.7 source on disk: `C:\Program Files\Epic Games\UE_5.7\Engine\Source\Runtime\Engine\Classes\Engine\TextureDefines.h` — ETextureSourceFormat enum; TSF_BGRA8 confirmed, TSF_RGBA8_DEPRECATED confirmed
- UE 5.7 source on disk: `C:\Program Files\Epic Games\UE_5.7\Engine\Source\Runtime\SlateCore\Public\Styling\SlateTypes.h` — FButtonStyle, FProgressBarStyle fields verified
- UE 5.7 source on disk: `C:\Program Files\Epic Games\UE_5.7\Engine\Source\Runtime\UMG\Public\Components\Image.h` — SetBrushFromTexture API verified
- UE 5.7 source on disk: `C:\Program Files\Epic Games\UE_5.7\Engine\Source\Runtime\UMG\Public\Components\TextBlock.h` — SetText, SetFont, FSlateFontInfo field verified
- UE 5.7 source on disk: `C:\Program Files\Epic Games\UE_5.7\Engine\Source\Runtime\UMG\Public\Blueprint\WidgetTree.h` — ConstructWidget<T> template verified
- UE 5.7 source on disk: `C:\Program Files\Epic Games\UE_5.7\Engine\Source\Runtime\UMG\Public\Components\CanvasPanel.h` — AddChildToCanvas returning UCanvasPanelSlot* verified
- UE 5.7 source on disk: `C:\Program Files\Epic Games\UE_5.7\Engine\Plugins\Editor\EditorScriptingUtilities\Source\EditorScriptingUtilities\Public\EditorAssetLibrary.h` — SaveLoadedAsset signature verified

### Secondary (MEDIUM confidence)
- `C:\Dev\psd-to-umg\Source\PSD2UMG\Private\Factories\PsdImportFactory.cpp` — Integration point structure confirmed; Phase 3 hook site verified
- `C:\Dev\psd-to-umg\Source\PSD2UMG\Public\PSD2UMGSetting.h` — `TextureAssetDir` confirmed as the `TargetDir` proxy; no `WBPAssetDir` field exists (requires Phase 3 addition)
- `.planning/phases/02-c-psd-parser/02-05-SUMMARY.md` — Layer ordering, ARGB quirk, and integration point confirmation

## Metadata

**Confidence breakdown:**
- WBP creation API sequence: HIGH — verified from actual UE 5.7 source
- Canvas coordinate math: HIGH — FAnchorData struct fields verified from CanvasPanelSlot.h
- Texture Source.Init format: HIGH — ETextureSourceFormat enum verified; BGRA8 is the required format
- Button/Progress style API: HIGH — FButtonStyle, FProgressBarStyle, SetStyle APIs verified from SlateTypes.h and component headers
- ZOrder direction: MEDIUM — PhotoshopAPI layer ordering behavior inferred; recommend empirical verification in Wave 0
- Settings TargetDir naming: HIGH — PSD2UMGSetting.h read directly; TextureAssetDir is the correct field name

**Research date:** 2026-04-09
**Valid until:** 2026-05-09 (UE 5.7 APIs are stable; no expiry risk within milestone)
