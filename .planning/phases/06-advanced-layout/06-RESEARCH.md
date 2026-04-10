# Phase 6: Advanced Layout - Research

**Researched:** 2026-04-10
**Domain:** UMG 9-slice, UUserWidget recursive import, PhotoshopAPI SmartObjectLayer, UWidgetSwitcher suffix mapping, anchor heuristics
**Confidence:** HIGH (all findings verified against vendored source code and existing codebase patterns)

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Any image layer with `_9s` or `_9slice` suffix gets Box draw mode on its UImage widget. Not limited to standalone images — works on button state brushes and any other image layer.
- **D-02:** Margin syntax `[L,T,R,B]` in the layer name sets margins explicitly. When no margin syntax is present, apply a uniform default of 16px on all sides.
- **D-03:** The `_9s`/`_9slice` suffix AND any `[L,T,R,B]` margin syntax are stripped from the widget name. E.g., `MyButton_9s[16,16,16,16]` → widget named `MyButton`. Consistent with how anchor suffixes are stripped.
- **D-04:** Smart Object layers are imported as separate Widget Blueprint assets, referenced in the parent via UUserWidget. This preserves the "reusable component" benefit.
- **D-05:** Recursion depth is configurable via a new setting `MaxSmartObjectDepth` on `UPSD2UMGSettings`, with a default value of 2. At depth limit, Smart Objects rasterize as images.
- **D-06:** Both embedded and linked Smart Objects are attempted. For linked Smart Objects, resolve the linked file from the PSD's directory. If the linked file is not found, fall back to rasterized image.
- **D-07:** Extraction failure (corrupt data, unsupported format, etc.) always falls back to rasterized image with a diagnostic warning — same best-effort pattern as Phase 4/5.
- **D-08:** New `EPsdLayerType::SmartObject` value needed in the parser to distinguish Smart Object layers from regular image/group layers.
- **D-09:** When children of a CanvasPanel are clearly aligned in a horizontal row or vertical column (within tolerance), automatically wrap them in HBox/VBox instead of individual CanvasPanel slots.
- **D-10:** Alignment tolerance for row/column detection: 4-8px (moderate — not too strict, not too relaxed). Exact value at Claude's discretion within this range.
- **D-11:** This is an enhancement to the existing `FAnchorCalculator` / generator logic, not a new mapper. The heuristic runs on groups that would otherwise produce a CanvasPanel.
- **D-12:** Whether to detect equal spacing between items and auto-apply padding on HBox/VBox is at Claude's discretion. Implement if straightforward; skip if it adds significant complexity.
- **D-13:** `_variants` is a suffix on any group (e.g., `States_variants`). It produces UWidgetSwitcher, same as the existing `Switcher_` prefix mapper. Both coexist — the suffix is a new recognition path, the prefix remains for backward compatibility.
- **D-14:** Variant children (slots) are ordered by PSD layer order (top to bottom = slot 0, 1, 2...). Predictable, matches designer's layer panel.
- **D-15:** The `_variants` suffix is stripped from the widget name. `States_variants` → widget named `States`. Consistent with D-03.

### Claude's Discretion
- How to implement 9-slice margin parsing (regex, manual parse, etc.)
- Whether SmartObject child WBP naming uses the smart object layer name or the linked file name
- Exact alignment tolerance value within 4-8px range
- Whether to implement spacing/padding detection for auto HBox/VBox
- How to implement the `_variants` suffix mapper (new mapper class vs extending existing Switcher mapper)
- Whether Smart Object pixel extraction uses PhotoshopAPI's composited preview or the linked file data

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope.
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| LAYOUT-01 | `_9s` / `_9slice` suffix → UImage with Box draw mode | ESlateBrushDrawType::Box + FSlateBrush.Margin verified in FImageLayerMapper pattern |
| LAYOUT-02 | Margin syntax `MyButton_9s[16,16,16,16]` sets L/T/R/B margins; uniform 16px default | Regex parse of `[N,N,N,N]` in layer name; FSlateBrush.Margin is FMargin |
| LAYOUT-03 | Improved anchor heuristics: horizontal row → HBox, vertical stack → VBox | Heuristic in PopulateCanvas before CanvasPanel child dispatch; UHorizontalBox/UVerticalBox already instantiated by existing prefix mappers |
| LAYOUT-04 | Smart Object layers → recursive import as child Widget Blueprints via UUserWidget | PhotoshopAPI SmartObjectLayer<T> dynamic_pointer_cast; FWidgetBlueprintGenerator::Generate is re-entrant capable |
| LAYOUT-05 | Smart Object fallback: rasterize as image if extraction fails | `get_image_data()` on SmartObjectLayer returns composited preview; same FTextureImporter path as ImageLayer |
| LAYOUT-06 | `_variants` suffix groups → UWidgetSwitcher with one child per variant slot | New FSuffixSwitcherLayerMapper at priority 200; suffix stripped via FAnchorCalculator suffix table extension |
</phase_requirements>

---

## Summary

Phase 6 is a pure C++ extension phase building on the established mapper/generator/parser infrastructure. All six requirements slot into already-defined extension points: the suffix table in `FAnchorCalculator`, the mapper registry, the `PopulateCanvas` function in `FWidgetBlueprintGenerator`, and the RTTI dispatch in `PsdParser.cpp`.

The highest-complexity requirement is LAYOUT-04 (Smart Object recursive import). PhotoshopAPI exposes `SmartObjectLayer<T>` via `dynamic_pointer_cast` — the same RTTI dispatch pattern already used for `GroupLayer`, `ImageLayer`, and `TextLayer`. The linked file is accessed through `SmartObjectLayer::filepath()` and `SmartObjectLayer::get_original_image_data()`. For recursive WBP import, the generator is already structured as a stateless function that accepts an `FPsdDocument`; calling it recursively for the Smart Object's inner PSD is architecturally clean.

The 9-slice feature (LAYOUT-01/02) is a localized change: extend the `FAnchorCalculator` suffix table for suffix stripping, add a `F9SliceImageLayerMapper` that calls `FImageLayerMapper` logic but sets `DrawAs = ESlateBrushDrawType::Box` and populates `FSlateBrush.Margin`. The `_variants` suffix (LAYOUT-06) follows the identical pattern to the existing `Switcher_` prefix mapper — only the `CanMap()` predicate changes. The anchor heuristic (LAYOUT-03) inserts a detection pass in `PopulateCanvas` before the CanvasPanel slot loop.

**Primary recommendation:** Implement in four independent tracks: (1) 9-slice mapper + suffix stripping, (2) `_variants` suffix mapper, (3) anchor heuristic in PopulateCanvas, (4) Smart Object parser detection + recursive generator. Tracks 1/2/3 are low-risk; track 4 is medium-risk due to recursion depth guarding and fallback path.

---

## Standard Stack

### Core
| Library / API | Version | Purpose | Why Standard |
|--------------|---------|---------|--------------|
| `ESlateBrushDrawType::Box` | UE 5.7 | Box (9-slice) draw mode for UImage brush | The only UMG mechanism for 9-slice borders |
| `FSlateBrush::Margin` (FMargin) | UE 5.7 | L/T/R/B margin fractions for Box draw | Direct slot on FSlateBrush; no alternative |
| `UWidgetSwitcher` | UE 5.7 | Tab/state switcher widget | Already instantiated by FSwitcherLayerMapper |
| `UUserWidget` | UE 5.7 | Parent class for child WBP references | UWidgetBlueprintFactory::ParentClass already set to this |
| `SmartObjectLayer<T>` | PhotoshopAPI vendored | Smart Object layer detection and pixel extraction | Vendored at `Source/ThirdParty/PhotoshopAPI/Win64/include/` |

### Supporting
| Library / API | Version | Purpose | When to Use |
|--------------|---------|---------|-------------|
| `UHorizontalBox` / `UVerticalBox` | UE 5.7 | Auto-layout box widgets | LAYOUT-03 heuristic wrap |
| `FTextureImporter::ImportLayer` | Project | RGBA → UTexture2D | Smart Object rasterize fallback (LAYOUT-05) |
| `FWidgetBlueprintGenerator::Generate` | Project | WBP creation | Recursive Smart Object import (LAYOUT-04) |

### No Additional Installations Required
All dependencies are either already in the UE5 module graph or in the vendored PhotoshopAPI. No new `npm install` or CMake step.

---

## Architecture Patterns

### Recommended Extension Structure

```
Source/PSD2UMG/
├── Public/
│   └── Parser/
│       └── PsdTypes.h                    ← Add EPsdLayerType::SmartObject
│   └── PSD2UMGSetting.h                  ← Add MaxSmartObjectDepth UPROPERTY
├── Private/
│   ├── Parser/
│   │   └── PsdParser.cpp                 ← Add SmartObject dynamic_pointer_cast branch
│   ├── Mapper/
│   │   ├── AllMappers.h                  ← Add F9SliceImageLayerMapper, FVariantsSwitcherMapper
│   │   ├── F9SliceImageLayerMapper.cpp   ← New file
│   │   ├── FVariantsSwitcherMapper.cpp   ← New file (or extend FSimplePrefixMappers.cpp)
│   │   └── FLayerMappingRegistry.cpp     ← Register new mappers
│   └── Generator/
│       ├── FAnchorCalculator.cpp         ← Extend suffix table: _9s, _9slice, _variants
│       ├── FSmartObjectImporter.h/.cpp   ← New: recursive Smart Object → WBP
│       └── FWidgetBlueprintGenerator.cpp ← Add row/column heuristic in PopulateCanvas
```

### Pattern 1: Suffix Stripping Extension (FAnchorCalculator)

The existing `GSuffixes[]` table in `FAnchorCalculator.cpp` is longest-first ordered. Extending it for new suffixes requires adding entries before any suffix that is a prefix of another.

```cpp
// Add to GSuffixes[] in FAnchorCalculator.cpp — order matters, longest first
// These entries have no anchor effect (all zeros, bComputed=false, bStretchH/V=false)
// They are recognized purely for name stripping. Caller checks for 9-slice/variants
// separately via FPsdLayer.Type or mapper CanMap().
{ TEXT("_9slice"), 0.f, 0.f, 0.f, 0.f, false, false, false },
{ TEXT("_9s"),     0.f, 0.f, 0.f, 0.f, false, false, false },
{ TEXT("_variants"), 0.f, 0.f, 0.f, 0.f, false, false, false },
```

**Caution:** The suffix table is currently used only for anchor assignment. Adding purely cosmetic entries (no anchor effect) means the caller must not misinterpret them as anchor overrides. The anchor values `(0,0,0,0)` with `bComputed=false` map to `FAnchors(0,0,0,0)` — top-left fixed anchor. This is wrong for 9-slice image layers that should use quadrant-derived anchors.

**Recommended fix:** Separate the suffix stripping concern from anchor assignment. Add a standalone `StripKnownSuffixes(FString& Name)` function that handles `_9s`, `_9slice`, `_variants` (and optionally `[N,N,N,N]`) before the main suffix table is consulted. This keeps anchor logic clean.

```cpp
// In FAnchorCalculator.h
static FString StripLayoutSuffixes(const FString& Name);

// In FAnchorCalculator.cpp
FString FAnchorCalculator::StripLayoutSuffixes(const FString& Name)
{
    // Strip [L,T,R,B] margin block first (if present)
    FString Result = Name;
    int32 BracketStart = INDEX_NONE;
    Result.FindLastChar(TEXT('['), BracketStart);
    if (BracketStart != INDEX_NONE && Result.EndsWith(TEXT("]")))
    {
        Result = Result.Left(BracketStart);
    }
    // Strip _9slice before _9s (longer first)
    if (Result.EndsWith(TEXT("_9slice"))) { Result = Result.LeftChop(7); }
    else if (Result.EndsWith(TEXT("_9s"))) { Result = Result.LeftChop(3); }
    // Strip _variants
    if (Result.EndsWith(TEXT("_variants"))) { Result = Result.LeftChop(9); }
    return Result;
}
```

### Pattern 2: 9-Slice Mapper (F9SliceImageLayerMapper)

```cpp
// Source: FAnchorCalculator.cpp suffix pattern + FImageLayerMapper.cpp brush pattern
bool F9SliceImageLayerMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.Type == EPsdLayerType::Image
        && (Layer.Name.Contains(TEXT("_9s")) || Layer.Name.Contains(TEXT("_9slice")));
}

UWidget* F9SliceImageLayerMapper::Map(const FPsdLayer& Layer, const FPsdDocument& Doc, UWidgetTree* Tree)
{
    // Parse [L,T,R,B] margin from name; default 16px uniform
    FMargin Margin(16.f);
    // ... regex/manual parse of [N,N,N,N] from Layer.Name ...

    const FString PsdName = FPaths::GetBaseFilename(Doc.SourcePath);
    UTexture2D* Tex = FTextureImporter::ImportLayer(Layer, FTextureImporter::BuildTexturePath(PsdName));
    if (!Tex) { return nullptr; }

    UImage* Img = Tree->ConstructWidget<UImage>(UImage::StaticClass(), FName(*CleanName));
    Img->SetBrushFromTexture(Tex, true);
    FSlateBrush Brush = Img->GetBrush();
    Brush.DrawAs = ESlateBrushDrawType::Box;
    // FSlateBrush.Margin is FMargin; values are fractions [0,1] of the texture,
    // BUT UMG Box draw mode interprets Margin as pixel-space margins when
    // the brush ImageSize is set. Verify empirically.
    Brush.Margin = FMargin(
        Margin.Left   / Brush.ImageSize.X,
        Margin.Top    / Brush.ImageSize.Y,
        Margin.Right  / Brush.ImageSize.X,
        Margin.Bottom / Brush.ImageSize.Y);
    Img->SetBrush(Brush);
    return Img;
}
```

**Priority:** Must be higher than `FImageLayerMapper` (priority 100). Set to **150** so it fires before the generic image mapper but after prefix mappers (200).

### Pattern 3: Variants Suffix Mapper (FVariantsSwitcherMapper)

```cpp
// Source: FSwitcherLayerMapper in FSimplePrefixMappers.cpp
int32 FVariantsSwitcherMapper::GetPriority() const { return 200; }
bool FVariantsSwitcherMapper::CanMap(const FPsdLayer& Layer) const
{
    return Layer.Type == EPsdLayerType::Group && Layer.Name.EndsWith(TEXT("_variants"));
}
UWidget* FVariantsSwitcherMapper::Map(const FPsdLayer& Layer, const FPsdDocument& /*Doc*/, UWidgetTree* Tree)
{
    // Strip _variants for clean widget name
    const FString CleanName = Layer.Name.LeftChop(9); // "_variants" = 9 chars
    return Tree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), FName(*CleanName));
}
```

Children are added by the generator's recursive child traversal — same path as all other group-type widgets. PSD layer order is already preserved by the existing `PopulateCanvas` loop (index 0 = topmost layer = slot 0 in the switcher).

### Pattern 4: SmartObject Parser Detection

```cpp
// In PsdParser.cpp ConvertLayerRecursive(), BEFORE the Group branch.
// SmartObjectLayer inherits from Layer<T> and ImageDataMixin<T>, not from GroupLayer.
// It must be checked before ImageLayer because both have image data.
#include "LayeredFile/LayerTypes/SmartObjectLayer.h"  // inside THIRD_PARTY_INCLUDES_START guard

if (auto SmartObj = std::dynamic_pointer_cast<SmartObjectLayer<PsdPixelType>>(InLayer))
{
    OutLayer.Type = EPsdLayerType::SmartObject;
    // Store the filename so the generator can attempt recursive import by name
    OutLayer.SmartObjectFilename = Utf8ToFString(SmartObj->filename());
    // Store whether it's externally linked
    OutLayer.bSmartObjectLinkedExternally = SmartObj->linked_externally();
    // Always extract composited preview pixels for fallback rasterization (LAYOUT-05)
    ExtractSmartObjectPixels(SmartObj, OutLayer, OutDiag);
    return;
}
```

`FPsdLayer` needs two new fields:
```cpp
// In PsdTypes.h FPsdLayer struct — Smart Object payload (Phase 6)
FString SmartObjectFilename;          // original linked filename (e.g. "Button.png" or "Component.psd")
bool bSmartObjectLinkedExternally = false;
// RGBAPixels already exists — reused for rasterize fallback
```

### Pattern 5: SmartObject Pixel Extraction

`SmartObjectLayer<T>` inherits `ImageDataMixin<T>` so `get_image_data()` returns the composited/warped preview at layer dimensions (`width()` x `height()`). This is the same interface as `ImageLayer<T>`. The existing `ExtractImagePixels` helper in `PsdParser.cpp` can be called on it directly (or a thin `ExtractSmartObjectPixels` wrapper that calls the same internals).

```cpp
// Reuse existing ExtractImagePixels pattern:
// SmartObjectLayer<T> exposes get_image_data() returning data_type
// (unordered_map<int, vector<T>>) — same as ImageLayer<T>.
// Channel index conventions: -1=alpha, 0=R, 1=G, 2=B
```

### Pattern 6: Recursive Smart Object Import (FSmartObjectImporter)

```cpp
// New helper, called from PopulateCanvas when Layer.Type == EPsdLayerType::SmartObject
// and CurrentDepth < MaxSmartObjectDepth setting.

UWidgetBlueprint* FSmartObjectImporter::ImportAsChildWBP(
    const FPsdLayer& Layer,
    const FPsdDocument& ParentDoc,
    int32 CurrentDepth)
{
    const int32 MaxDepth = UPSD2UMGSettings::Get()->MaxSmartObjectDepth;
    if (CurrentDepth >= MaxDepth)
    {
        // At depth limit — fall through to rasterize path
        return nullptr;
    }

    // Determine source: try to load linked PSD from disk
    // filepath() on the linked layer returns the stored path;
    // for embedded ("data" type) Smart Objects this is a temp/virtual path
    // that may not exist on disk. Fall back to rasterize if not found.
    FString LinkedPath = ResolveLinkedPath(Layer, ParentDoc.SourcePath);
    if (LinkedPath.IsEmpty() || !FPaths::FileExists(LinkedPath))
    {
        UE_LOG(LogPSD2UMG, Warning, TEXT("SmartObject '%s': linked file not found at '%s' — rasterizing"),
            *Layer.Name, *LinkedPath);
        return nullptr;
    }

    // Parse the linked PSD recursively
    FPsdDocument ChildDoc;
    FPsdParseDiagnostics ChildDiag;
    if (!PSD2UMG::Parser::ParseFile(LinkedPath, ChildDoc, ChildDiag))
    {
        UE_LOG(LogPSD2UMG, Warning, TEXT("SmartObject '%s': parse failed — rasterizing"), *Layer.Name);
        return nullptr;
    }

    // Generate child WBP — depth is threaded through generator context
    // WBP naming: use Layer.Name (the smart object layer name), not the linked filename
    const FString ChildWbpName = FString::Printf(TEXT("WBP_%s"), *Layer.Name);
    const FString WbpDir = UPSD2UMGSettings::Get()->WidgetBlueprintAssetDir.Path;
    return FWidgetBlueprintGenerator::Generate(ChildDoc, WbpDir, ChildWbpName, CurrentDepth + 1);
}
```

`FWidgetBlueprintGenerator::Generate` needs a `CurrentDepth` parameter threaded through to `PopulateCanvas`, then to `FSmartObjectImporter::ImportAsChildWBP`. The existing `Generate(Doc, PackagePath, AssetName)` signature gains an optional `int32 CurrentDepth = 0` overload.

### Pattern 7: Row/Column Heuristic in PopulateCanvas

The heuristic runs on `Layer.Children` of any group that would otherwise produce a CanvasPanel (i.e., not already claimed by a prefix/suffix mapper). The detection logic:

```cpp
// In PopulateCanvas, before the per-child loop, when the parent Widget is a UCanvasPanel
// and the group was not claimed by a prefix/suffix mapper.

struct FHeuristicResult { bool bIsRow; bool bIsColumn; float UniformSpacing; };

static FHeuristicResult DetectRowOrColumn(const TArray<FPsdLayer>& Children, int32 TolerancePx = 6)
{
    if (Children.Num() < 2) return {};

    // Check horizontal row: all children's top-Y values within TolerancePx of each other
    const int32 RefY = Children[0].Bounds.Min.Y;
    bool bRow = true;
    for (const FPsdLayer& C : Children)
    {
        if (FMath::Abs(C.Bounds.Min.Y - RefY) > TolerancePx) { bRow = false; break; }
    }

    // Check vertical column: all children's left-X values within TolerancePx
    const int32 RefX = Children[0].Bounds.Min.X;
    bool bCol = true;
    for (const FPsdLayer& C : Children)
    {
        if (FMath::Abs(C.Bounds.Min.X - RefX) > TolerancePx) { bCol = false; break; }
    }

    return { bRow && !bCol, bCol && !bRow, 0.f };
}
```

Tolerance recommendation: **6px** (midpoint of the 4-8px range from D-10). This is generous enough for typical PSD alignment but prevents false positives from oblique arrangements.

**Spacing detection** (D-12): Implement if trivial. For a detected row, compute inter-child gaps (Left[i+1] - Right[i]) and check variance. If variance < 2px, apply uniform padding on HBox slots. Implementation is ~15 lines; include it.

### Anti-Patterns to Avoid
- **Checking `_9s` before `_9slice` in string matching:** `_9s` is a suffix of `_9slice` only if `_9slice` contains `_9s`. Since `_9slice` ends with `lice`, not `_9s`, there's no ambiguity. But `EndsWith("_9s")` on `"foo_9slice"` would return false — safe. Verify with unit test.
- **Registering `F9SliceImageLayerMapper` at priority 200:** It must be below 200 (used by prefix mappers) or at 200 with `CanMap()` returning false for non-image layers. Image layers never match prefix mappers (they check `Type == Group`), so priority 150 is cleanest.
- **Using `filepath()` on embedded Smart Objects:** For embedded (`data` linkage) Smart Objects, `filepath()` may return a non-existent path. Always check `FPaths::FileExists()` before parsing.
- **Modifying `FSlateBrush.Margin` with raw pixel values:** UMG Box draw mode uses `Margin` as normalized fractions [0,1] of the texture dimensions, not raw pixels. The [L,T,R,B] values from the layer name are in pixels and must be divided by `Brush.ImageSize.X/Y`.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| 9-slice rendering | Custom shader/material | `ESlateBrushDrawType::Box` + `FSlateBrush.Margin` | Built into Slate brush system |
| Smart Object pixel data | Manual PSD chunk parsing | `SmartObjectLayer<T>::get_image_data()` (inherited ImageDataMixin) | PhotoshopAPI already decodes warped composited preview |
| Child WBP reference | Custom asset reference struct | `UUserWidget` as widget tree member via `Tree->ConstructWidget<UUserWidget>` with `WidgetClass` set | UMG standard pattern for nested WBPs |
| Margin string parsing | Lexer/parser library | FRegexMatcher or manual `FindChar`/`Split` | Simple `[N,N,N,N]` pattern; 10 lines of manual parse |

---

## Common Pitfalls

### Pitfall 1: FSlateBrush.Margin Is Normalized, Not Pixels
**What goes wrong:** `FSlateBrush.Margin = FMargin(16,16,16,16)` produces a margin of 16x the image width on each side, making the widget invisible.
**Why it happens:** Slate Box draw mode treats `Margin` as normalized UV fractions.
**How to avoid:** Divide pixel margin by `Brush.ImageSize.X` (for left/right) and `Brush.ImageSize.Y` (for top/bottom). `SetBrushFromTexture(Tex, true)` sets `ImageSize` to texture dimensions before you read it.
**Warning signs:** Rendered widget shows only stretched corners with no fill, or is not visible at all.

### Pitfall 2: SmartObjectLayer Must Be Cast Before ImageLayer in RTTI Dispatch
**What goes wrong:** `ImageLayer<T>` and `SmartObjectLayer<T>` both inherit `ImageDataMixin<T>`. If `dynamic_pointer_cast<ImageLayer<T>>` is checked first, Smart Object layers may silently match as plain image layers (depending on the inheritance chain) and lose their `filename()`/recursion data.
**Why it happens:** PhotoshopAPI's type hierarchy — `SmartObjectLayer` does NOT inherit from `ImageLayer`; they are siblings under `Layer<T>`. So a `dynamic_pointer_cast<ImageLayer<T>>` on a SmartObjectLayer returns nullptr. This is safe, but verify against vendored headers before assuming.
**How to avoid:** Place the `SmartObjectLayer<T>` branch before `ImageLayer<T>` in `ConvertLayerRecursive` as a defense-in-depth measure.
**Warning signs:** Smart Object layers appearing as plain images in the generated WBP with no child WBP created.

### Pitfall 3: Heuristic Fires on Button State Sub-Groups
**What goes wrong:** A `Button_` group may contain sub-groups `Normal`, `Hovered`, `Pressed`, `Disabled` — four children whose tops align at Y=0 relative to the button canvas. The heuristic detects them as a row and wraps them in an HBox instead of the expected CanvasPanel.
**Why it happens:** The heuristic runs on all group children before mapper dispatch.
**How to avoid:** The heuristic should only fire on groups that are mapped to CanvasPanel (i.e., groups not claimed by any prefix/suffix mapper). Since `Button_` groups are mapped by `FButtonLayerMapper` (priority 200) before the `FGroupLayerMapper` fallback, the heuristic should run as a sub-step inside `FGroupLayerMapper::Map()` or in `PopulateCanvas` only when the parent widget is already confirmed to be a `UCanvasPanel` produced by the group fallback mapper.
**Warning signs:** Button state images appearing side-by-side instead of stacked.

### Pitfall 4: Recursive Generate Deadlock on Circular Smart Object References
**What goes wrong:** PSD-A contains a Smart Object pointing to PSD-B, which contains a Smart Object pointing to PSD-A. Recursive `ParseFile` → `Generate` loop never terminates.
**Why it happens:** Linked Smart Objects can form reference cycles.
**How to avoid:** `MaxSmartObjectDepth` (D-05) with default 2 provides a hard recursion cap. Additionally, track in-progress paths in a `TSet<FString>` passed through the call chain and bail if the same path appears twice.
**Warning signs:** Editor hang or stack overflow during import.

### Pitfall 5: `_9s` Suffix Collision With `_9slice` Detection
**What goes wrong:** `"Foo_9slice"` matched by `EndsWith("_9s")` strips only 3 characters instead of 7, leaving `"Foo_9sl"` as the name.
**Why it happens:** `"_9slice"` ends with `"ice"`, not `"_9s"`, so `EndsWith("_9s")` returns **false** for `"_9slice"` strings. No collision.
**Verification:** `FString(TEXT("Foo_9slice")).EndsWith(TEXT("_9s"))` → false. Safe, but add unit test to confirm.

---

## Code Examples

### 9-Slice Margin Parsing (Manual, No Library)

```cpp
// Source: project convention — manual string parse preferred over regex for simple patterns
static bool Parse9SliceMargin(const FString& LayerName, FMargin& OutMargin)
{
    // Find trailing [L,T,R,B] block
    int32 BracketOpen = INDEX_NONE;
    LayerName.FindLastChar(TEXT('['), BracketOpen);
    if (BracketOpen == INDEX_NONE || !LayerName.EndsWith(TEXT("]")))
    {
        return false; // no margin block
    }
    const FString Inner = LayerName.Mid(BracketOpen + 1, LayerName.Len() - BracketOpen - 2);
    TArray<FString> Parts;
    Inner.ParseIntoArray(Parts, TEXT(","));
    if (Parts.Num() != 4) { return false; }
    OutMargin.Left   = FCString::Atof(*Parts[0]);
    OutMargin.Top    = FCString::Atof(*Parts[1]);
    OutMargin.Right  = FCString::Atof(*Parts[2]);
    OutMargin.Bottom = FCString::Atof(*Parts[3]);
    return true;
}
```

### Adding UUserWidget Child Reference to Canvas

```cpp
// Source: UE5 WidgetTree API — used in FWidgetBlueprintGenerator already
// When Smart Object recursive import succeeds, reference the child WBP:
UUserWidget* ChildRef = Tree->ConstructWidget<UUserWidget>(
    UUserWidget::StaticClass(),
    FName(*CleanName));
// Set the class to the generated child WBP's generated class
if (ChildWBP && ChildWBP->GeneratedClass)
{
    ChildRef->SetClass(Cast<UClass>(ChildWBP->GeneratedClass));
}
```

**Note on `SetClass`:** `UUserWidget::SetClass()` does not exist in UE5. The correct approach for embedding a child WBP instance is to set `WidgetBlueprint->ParentClass` or use the sub-object reference pattern. Research indicates the correct UMG editor pattern is to add a `UWidgetBlueprintGeneratedClass`-typed member widget in the WidgetTree. This is the **primary open question** (see Open Questions #1).

### `MaxSmartObjectDepth` UPROPERTY Addition

```cpp
// In PSD2UMGSetting.h, inside UPSD2UMGSettings class:
/** Maximum recursion depth for Smart Object import. At limit, rasterize as image. Per D-05. */
UPROPERTY(EditAnywhere, config, BlueprintReadWrite, Category = "PSD2UMG")
int32 MaxSmartObjectDepth = 2;
```

---

## State of the Art

| Old Approach | Current Approach | Impact |
|--------------|-----------------|--------|
| Manual suffix stripping per feature | Central `StripLayoutSuffixes()` + existing `TryParseSuffix` table | Prevents O(N) suffix conflicts as features accumulate |
| Python psd_tools Smart Object extraction | PhotoshopAPI `SmartObjectLayer<T>::get_image_data()` | C++ in-process; no subprocess or temp file for pixel data |

---

## Open Questions

1. **How to reference a child Widget Blueprint from a parent WidgetTree at editor asset creation time**
   - What we know: `UWidgetBlueprintFactory` creates a WBP asset. `FWidgetBlueprintGenerator` uses `Tree->ConstructWidget<UCanvasPanel>`. For child WBP reference, the standard UMG designer approach is adding a `UUserWidget`-derived member whose class is the child WBP's `GeneratedClass`.
   - What's unclear: Whether `Tree->ConstructWidget<UUserWidget>` followed by assigning `GeneratedClass` is valid at editor-time before compile, or whether the parent WBP must be compiled after the child WBP to resolve the reference.
   - Recommendation: Generate and compile the child WBP first (LAYOUT-04), then add a `UUserWidget` to the parent tree with `WidgetClass` set to the child's `UWidgetBlueprintGeneratedClass`. This is the same pattern used by the UMG editor when you drag a WBP into another WBP's designer. Verify the exact API call against `UWidgetTree` and `UUserWidget` headers before implementing.

2. **Embedded Smart Objects and `filepath()` validity**
   - What we know: `SmartObjectLayer::linked_externally()` distinguishes embedded from linked. For embedded (`data` type), the internal `filepath()` is a virtual path not guaranteed to exist on disk.
   - What's unclear: Whether `get_original_image_data()` works for embedded Smart Objects (it should, since `m_LinkedLayers` is populated from the PSD file regardless of linkage type).
   - Recommendation: For embedded Smart Objects, skip the recursive PSD parse path (no on-disk .psd to recurse into) and fall back directly to rasterized image via `get_image_data()`. Only attempt recursive parse for externally linked Smart Objects that point to .psd files.

3. **HBox/VBox child slot configuration in PopulateCanvas**
   - What we know: `FHBoxLayerMapper` returns a bare `UHorizontalBox` with no slots. The generator's `PopulateCanvas` recurses into children and adds them to the parent via `UCanvasPanel::AddChildToCanvas`. For UHorizontalBox, the slot API is `UHorizontalBox::AddChildToHorizontalBox(Widget)`.
   - What's unclear: The current `PopulateCanvas` only handles `UCanvasPanel` as the parent for child placement. When the heuristic wraps children in HBox/VBox, the parent widget is no longer a CanvasPanel — the slot placement code needs to branch on parent widget type.
   - Recommendation: Refactor `PopulateCanvas` to detect the parent widget type and call the appropriate `AddChild*` overload. Or introduce a parallel `PopulateBoxLayout` path for detected row/column groups.

---

## Environment Availability

Step 2.6: SKIPPED — Phase 6 is purely C++ code changes within the existing UE plugin build. No new external CLIs, runtimes, databases, or services are introduced. PhotoshopAPI SmartObjectLayer is already vendored. UE 5.7 is already the build target.

---

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | UE FAutomationTestBase (Unreal Automation System) |
| Config file | None separate — tests registered via `IMPLEMENT_SIMPLE_AUTOMATION_TEST` macro |
| Quick run command | Run specific test in UE editor via "Session Frontend" → Automation tab, or `UnrealEditor-cmd.exe ... -ExecCmds "Automation RunTest PSD2UMG.Unit"` |
| Full suite command | `UnrealEditor-cmd.exe [ProjectPath] -ExecCmds "Automation RunAll" -TestExit` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| LAYOUT-01 | `_9s` suffix layer produces UImage with `DrawAs == ESlateBrushDrawType::Box` | Unit | `Automation RunTest PSD2UMG.Unit.9SliceDrawMode` | ❌ Wave 0 |
| LAYOUT-02 | `[16,16,16,16]` parsed → correct normalized FMargin on brush | Unit | `Automation RunTest PSD2UMG.Unit.9SliceMarginParse` | ❌ Wave 0 |
| LAYOUT-02 | Name stripped correctly: `MyButton_9s[16,16,16,16]` → `MyButton` | Unit | `Automation RunTest PSD2UMG.Unit.9SliceNameStrip` | ❌ Wave 0 |
| LAYOUT-03 | 3 layers with Y within 4px detected as row → HBox produced | Unit | `Automation RunTest PSD2UMG.Unit.RowHeuristic` | ❌ Wave 0 |
| LAYOUT-03 | 3 layers with X within 4px detected as column → VBox produced | Unit | `Automation RunTest PSD2UMG.Unit.ColumnHeuristic` | ❌ Wave 0 |
| LAYOUT-04 | Smart Object layer in PSD → child WBP asset created | Integration | `Automation RunTest PSD2UMG.Integration.SmartObjectImport` | ❌ Wave 0 |
| LAYOUT-05 | Smart Object fallback: extraction failure → UImage with rasterized pixels | Integration | `Automation RunTest PSD2UMG.Integration.SmartObjectFallback` | ❌ Wave 0 |
| LAYOUT-06 | `_variants` group → UWidgetSwitcher with correct child count | Unit | `Automation RunTest PSD2UMG.Unit.VariantsSwitcher` | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** Run unit tests relevant to the task (LAYOUT-01/02 after 9-slice task, LAYOUT-06 after variants task)
- **Per wave merge:** Full unit + integration suite
- **Phase gate:** Full suite green before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `Source/PSD2UMG/Tests/Unit/Test9SliceMapper.cpp` — covers LAYOUT-01, LAYOUT-02
- [ ] `Source/PSD2UMG/Tests/Unit/TestAnchorCalculatorStrip.cpp` — covers name stripping for _9s/_9slice/_variants
- [ ] `Source/PSD2UMG/Tests/Unit/TestRowColumnHeuristic.cpp` — covers LAYOUT-03
- [ ] `Source/PSD2UMG/Tests/Unit/TestVariantsSwitcherMapper.cpp` — covers LAYOUT-06
- [ ] `Source/PSD2UMG/Tests/Integration/TestSmartObjectImport.cpp` — covers LAYOUT-04, LAYOUT-05
- [ ] Test PSD fixture with Smart Object layer (embedded) for integration tests

---

## Sources

### Primary (HIGH confidence)
- Vendored `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/LayeredFile/LayerTypes/SmartObjectLayer.h` — SmartObjectLayer API: `get_image_data()`, `filename()`, `filepath()`, `linked_externally()`, `get_original_image_data()`
- Vendored `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/LayeredFile/LinkedData/LinkedLayerData.h` — LinkedLayerType enum, LinkedLayerData struct
- `Source/PSD2UMG/Private/Generator/FAnchorCalculator.cpp` — suffix table structure, TryParseSuffix pattern
- `Source/PSD2UMG/Private/Mapper/FSimplePrefixMappers.cpp` — Switcher_ prefix mapper pattern (FSwitcherLayerMapper)
- `Source/PSD2UMG/Private/Mapper/FImageLayerMapper.cpp` — FImageLayerMapper brush pattern
- `Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp` — PopulateCanvas, Generate, recursion structure
- `Source/PSD2UMG/Public/Parser/PsdTypes.h` — EPsdLayerType enum, FPsdLayer struct
- `Source/PSD2UMG/Public/PSD2UMGSetting.h` — UPSD2UMGSettings UPROPERTY pattern
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` (lines 486-538) — ConvertLayerRecursive RTTI dispatch

### Secondary (MEDIUM confidence)
- UE5 Slate source conventions: `ESlateBrushDrawType::Box`, `FSlateBrush::Margin` as normalized FMargin — inferred from project's existing `ESlateBrushDrawType::Image` usage in `FImageLayerMapper.cpp` and UE5 documentation patterns. Margin normalization is standard Slate behavior; verify empirically with a test texture.

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all UMG APIs already used in project; PhotoshopAPI APIs verified in vendored headers
- Architecture: HIGH — all extension points verified against existing source; patterns directly mirror established Phase 3/5 implementations
- Pitfalls: HIGH — derived from code inspection of actual integration points
- Open Question #1 (UUserWidget child reference API): MEDIUM — UE5 API pattern is known conceptually but exact call sequence needs in-editor verification

**Research date:** 2026-04-10
**Valid until:** 2026-05-10 (stable UE5 + stable PhotoshopAPI vendor)
