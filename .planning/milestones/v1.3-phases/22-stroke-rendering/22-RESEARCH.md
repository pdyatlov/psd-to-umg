# Phase 22: Stroke Rendering - Research

**Researched:** 2026-04-29
**Domain:** PSD vstk descriptor parsing + UMG sibling-UImage stroke approximation
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

- **D-01:** Stroke sibling creation for both lfx2 (STROKE-01) and vstk (STROKE-03) paths lives in `FWidgetBlueprintGenerator`, NOT in `FShapeLayerMapper::Map()`. STROKE-03 is a new FX block in the generator that checks `bHasVectorStroke` on Shape-type layers — identical pattern to FX-04 (drop shadow). `FShapeLayerMapper` does not create siblings; no mapper API changes required.
- **D-02:** Always sibling UImage for both STROKE-01 and STROKE-03. No DrawType::Border logic. Shape layers produce UImages tinted via ColorOverlayColor (no texture file); the stroke geometry is a sized+offset sibling UImage with StrokeColor tint at ZOrder = main - 1.
- **D-03:** vstk wins. `ScanVstkStroke()` clears `bHasStroke` when it sets `bHasVectorStroke` on a Shape layer. Guard lives at parse time. When both fields potentially set, vstk value takes precedence. Generator never sees both `bHasStroke` and `bHasVectorStroke` simultaneously on a Shape layer.
- **D-04:** No new stroke fixture PSD. Validation uses ButtonStyles.psd for non-regression (success criterion 4). Positive stroke assertions are specification-only — unit-level assertions against parsed `FPsdLayerEffects` fields if feasible without a fixture, otherwise deferred until a fixture is available.

### Claude's Discretion

- Whether STROKE-01 (lfx2) and STROKE-03 (vstk) are the same FX block with a combined condition, or two sequential blocks that both emit via the same helper.
- Whether to add a shared `EmitStrokeSibling(UCanvasPanel*, UCanvasPanelSlot*, const FPsdLayer*, UWidgetTree*)` helper used by both paths.
- Plan count and split (STROKE-02 must precede STROKE-03, STROKE-01 is independent).

### Deferred Ideas (OUT OF SCOPE)

- Positive stroke fixture (Stroke.psd): deferred until user can provide a PSD.
- DrawType::Border approach for shape strokes.
- frameFXMulti VlLs stroke rendering (FXFMT-01 completed in Phase 21; emission for VlLs-origin vstk data is v1.3+ backlog).
- Inside/outside/center stroke alignment precision.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| STROKE-01 | Image/shape layers with lfx2 `bHasStroke` set emit a stroke sibling UImage (size +2×StrokePx, offset -StrokePx, tinted StrokeColor, ZOrder = main-1). Canvas-only; mirrors drop-shadow pattern. | FX-04 in generator (lines 305-366) is the exact template. `bHasStroke` already populated by `ExtractLfx2Stroke` for all layer types since Phase 4.1. |
| STROKE-02 | `ScanVstkStroke()` parses the `vstk` (vecStrokeData) tagged block at **byte offset 0** and writes `bHasVectorStroke`, `VectorStrokeSize`, `VectorStrokeColor` to `FPsdLayerEffects`. Must NOT write to `bHasStroke`. | `TaggedBlockKey::vecStrokeData` confirmed in Enum.h:822/900. `ScanShapeFillColor` (lines 1445-1661) is the direct model. Descriptor key names in vstk are long-form: `strokeStyleLineWidth`, `strokeEnabled`, `strokeStyleContent` — different from FrFX short keys. CP-01 (offset=0) confirmed by PITFALLS.md and external psd-tools evidence. |
| STROKE-03 | Shape layers with `bHasVectorStroke` set emit stroke geometry (sibling UImage). The two stroke sources (lfx2 and vstk) must not double-emit on the same layer. | D-03 guard (clear `bHasStroke` in `ScanVstkStroke` for Shape layers) prevents double-emit at parse time. Generator checks `bHasVectorStroke` on Shape layers exactly as FX-04 checks `bHasDropShadow`. |
</phase_requirements>

---

## Summary

Phase 22 adds stroke rendering to PSD2UMG through three tightly coupled changes across two source files. The work is straightforward given existing patterns: STROKE-02 is a new `ScanVstkStroke()` static function modeled verbatim on `ScanShapeFillColor`, STROKE-01 and STROKE-03 are new FX blocks in `PopulateChildren` modeled on the FX-04 drop shadow block, and the `FPsdLayerEffects` struct needs three new fields.

The critical research findings are: (1) `vstk` descriptor keys are LONG-FORM strings (`strokeStyleLineWidth`, `strokeEnabled`, `strokeStyleContent`) not 4-char short keys like FrFX uses; (2) the descriptor starts at byte offset 0 in `Block->m_Data` (no version prefix), unlike SoCo (offset 4) or vscg (offset 8); (3) `strokeStyleContent` is an `Objc` sub-descriptor that contains the `Clr ` / `RGBC` color — not a top-level `Clr ` key.

**Primary recommendation:** Implement STROKE-02 first (new fields + new parser function), then STROKE-01+03 together (generator FX block). The double-emit guard (D-03) lives in STROKE-02, so STROKE-03 is gated on STROKE-02 being present.

---

## Standard Stack

### Core (no new dependencies)

| Component | Location | Purpose |
|-----------|----------|---------|
| `FPsdLayerEffects` | `Source/PSD2UMG/Public/Parser/PsdTypes.h` | Add 3 new fields: `bHasVectorStroke`, `VectorStrokeSize`, `VectorStrokeColor` |
| `PsdParser.cpp` | `Source/PSD2UMG/Private/Parser/PsdParser.cpp` | Add `ScanVstkStroke()` static function + call site in `ConvertLayerRecursive` |
| `FWidgetBlueprintGenerator.cpp` | `Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp` | Add stroke sibling emission block inside canvas-only section of `PopulateChildren` |
| `TaggedBlockKey::vecStrokeData` | `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/Util/Enum.h:822` | Already present — maps `"vstk"` string to enum value; no header patching needed |

No new third-party dependencies. No new UMG widget types. No new module dependencies.

---

## Architecture Patterns

### Pattern 1: ScanVstkStroke() — new parser function

**What:** A new static function in `PsdParser.cpp`'s anonymous namespace that iterates `InLayer->unparsed_tagged_blocks()`, finds the `vecStrokeData` block, and parses its descriptor.

**Model:** `ScanShapeFillColor()` at PsdParser.cpp:1445–1661. Three structural differences from SoCo/vscg:

| Aspect | SoCo (ScanSolidFillColor) | vscg (ScanShapeFillColor) | vstk (ScanVstkStroke) |
|--------|--------------------------|--------------------------|----------------------|
| TaggedBlockKey | `adjSolidColor` | `vecStrokeContentData` | `vecStrokeData` |
| Class ID prefix at [0..3] | None (descriptor at offset 4) | `'SoCo'`/`'GdFl'` at [0..3] | None — descriptor at offset 0 |
| Primary TryParseAt | `TryParseAt(4)` | `TryParseAt(8)` | `TryParseAt(0)` |
| Top-level key for enable | None (always enabled) | None | `"strokeEnabled"` (long-form, `bool` ostype) |
| Top-level key for size | None | None | `"strokeStyleLineWidth"` (long-form, `UntF` ostype) |
| Top-level key for color | `"Clr "` / `"FlCl"` short-form Objc | `"Clr "` / `"FlCl"` short-form Objc | `"strokeStyleContent"` long-form `Objc`, then `"Clr "` short-form `Objc` with `"RGBC"` sub-descriptor |

**CP-01 confirmation:** The PITFALLS.md (CP-01, derived from direct codebase analysis against PSD spec) states vstk carries a raw descriptor with no class-ID or version prefix. Descriptor begins at offset 0. The PSD format spec (paulbourke.net) says vstk has "Version (=16), then Descriptor" — however PITFALLS.md explicitly resolved this contradiction: the vendored PhotoshopAPI strips/handles the version prefix before exposing `Block->m_Data`, so the application code sees the descriptor starting at offset 0 in `m_Data`. TryParseAt(0) is primary; TryParseAt(4) and TryParseAt(8) as defensive fallbacks.

**Key recognition:** `ReadPsString()` already handles long-form keys correctly. When `Len > 0`, it reads `Len` ASCII bytes as the key string. So `"strokeStyleLineWidth"` (20 bytes) is returned as `std::string("strokeStyleLineWidth")`. The `==` comparison works identically to short keys.

**Implementation skeleton:**
```cpp
// Source: PsdParser.cpp anonymous namespace, modeled on ScanShapeFillColor
static bool ScanVstkStroke(
    const std::shared_ptr<PsdLayer>& InLayer,
    FPsdLayer& OutLayer,
    FPsdParseDiagnostics& OutDiag)
{
    for (const auto& Block : InLayer->unparsed_tagged_blocks())
    {
        if (!Block || Block->getKey() != NAMESPACE_PSAPI::Enum::TaggedBlockKey::vecStrokeData)
            continue;

        const auto& Data = Block->m_Data;
        // Hex-dump diagnostic (mirror of ScanSolidFillColor lines 1226-1238)
        // ...

        auto TryParseAt = [&](size_t StartPos) -> bool
        {
            size_t Pos = StartPos;
            // ReadU32BE, ReadDoubleBE, ReadPsString, SkipUnicodeString lambdas
            // SkipValueAfterOsType recursive lambda (mirror of ParseFrFXDescriptor)

            SkipUnicodeString();  // descriptor class name
            ReadPsString();       // classID
            uint32 TopCount = ReadU32BE();
            if (TopCount == 0 || TopCount > 256) return false;

            bool bEnabled = false;
            double SzRaw = 0.0;
            char UnitTag[5] = {};  // MiP-01: read unit tag explicitly
            double Rd = 0.0, Grn = 0.0, Bl = 0.0;
            bool bFoundColor = false;

            for (uint32 i = 0; i < TopCount; ++i)
            {
                std::string Key = ReadPsString();
                char OsType[5] = {};
                // read 4 bytes OsType

                if (Key == "strokeEnabled" && strcmp(OsType, "bool") == 0)
                    bEnabled = (ReadU8() != 0);
                else if (Key == "strokeStyleLineWidth" && strcmp(OsType, "UntF") == 0)
                {
                    // MiP-01: read unit tag, convert #Pnt to px if needed
                    for (int k=0;k<4;++k) UnitTag[k] = Data[Pos+k]; Pos+=4;
                    SzRaw = ReadDoubleBE();
                    // If UnitTag == "#Pnt": SzRaw stays (1pt=1px at 72dpi)
                }
                else if (Key == "strokeStyleContent" && strcmp(OsType, "Objc") == 0)
                {
                    // Sub-descriptor: contains "Clr " -> "RGBC" with Rd/Grn/Bl doubles
                    SkipUnicodeString(); ReadPsString(); // classID
                    uint32 ContentCount = ReadU32BE();
                    for (uint32 j = 0; j < ContentCount; ++j)
                    {
                        std::string CKey = ReadPsString();
                        char COT[5] = {};
                        // read ostype
                        if ((CKey == "Clr " || CKey == "FlCl") && strcmp(COT, "Objc") == 0)
                        {
                            // RGBC sub-descriptor -- same pattern as ScanShapeFillColor lines 1587-1615
                            SkipUnicodeString(); ReadPsString();
                            uint32 ClrCount = ReadU32BE();
                            for (uint32 c = 0; c < ClrCount; ++c) { /* Rd/Grn/Bl */ }
                            bFoundColor = true;
                        }
                        else { SkipValueAfterOsType(COT); }
                    }
                }
                else { SkipValueAfterOsType(OsType); }
            }

            if (!bEnabled || !bFoundColor) return false;

            OutLayer.Effects.bHasVectorStroke = true;
            OutLayer.Effects.VectorStrokeSize = static_cast<float>(SzRaw);
            OutLayer.Effects.VectorStrokeColor = FLinearColor::FromSRGBColor(FColor(R8, G8, B8, 255));
            return true;
        };

        if (TryParseAt(0)) return true;
        if (TryParseAt(4)) return true;
        if (TryParseAt(8)) return true;
        return false;
    }
    return false;
}
```

### Pattern 2: `FPsdLayerEffects` field additions

Three new fields alongside the existing stroke fields (PsdTypes.h lines 120-122):

```cpp
// Phase 22 STROKE-02 -- Vector Shape Stroke (from vstk/vecStrokeData tagged block).
// Separate from bHasStroke (owned by lfx2 for all layer types per D-02 Phase 4.1).
// CP-02: these fields must NOT alias bHasStroke/StrokeSize/StrokeColor.
bool bHasVectorStroke = false;
float VectorStrokeSize = 0.f;
FLinearColor VectorStrokeColor = FLinearColor::Transparent;
```

### Pattern 3: D-03 double-emit guard in ConvertLayerRecursive

In `ConvertLayerRecursive`, after `ScanVstkStroke` is called for Shape layers, clear `bHasStroke`:

```cpp
// STROKE-02 / D-03: call ScanVstkStroke ONLY for ShapeLayer instances.
// If vstk found and enabled, clear bHasStroke so the generator never sees
// both flags simultaneously on a Shape layer (vstk wins per D-03).
if (OutLayer.Type == EPsdLayerType::Shape)
{
    ScanVstkStroke(InLayer, OutLayer, OutDiag);
    if (OutLayer.Effects.bHasVectorStroke)
        OutLayer.Effects.bHasStroke = false;  // D-03 double-emit guard
}
```

**Call site:** Inside the ShapeLayer dispatch branch (PsdParser.cpp around line 2024–2031), after `ScanShapeFillColor` sets `OutLayer.Type = EPsdLayerType::Shape`. Specifically: after the `DispatchTag = TEXT("Shape")` assignment, before the `return`.

### Pattern 4: Stroke sibling FX block in PopulateChildren (generator)

**Position in code:** After the FX-04 drop shadow block (PsdParser.cpp lines 305–373), before FX-03 (color overlay, line 401). The combined condition handles STROKE-01 (lfx2 `bHasStroke` on Image layers) and STROKE-03 (vstk `bHasVectorStroke` on Shape layers).

```cpp
// FX-NEW: Stroke — canvas-only sibling UImage pattern (STROKE-01/STROKE-03)
// STROKE-01: image layers with lfx2 bHasStroke
// STROKE-03: shape layers with vstk bHasVectorStroke
// D-02: always sibling UImage (no DrawType::Border), ZOrder = main - 1
// D-03: ScanVstkStroke cleared bHasStroke for Shape layers at parse time,
//       so both flags are never simultaneously true here.
const bool bImageStroke = LayerPtr->Effects.bHasStroke
    && LayerPtr->Type == EPsdLayerType::Image;
const bool bShapeStroke = LayerPtr->Effects.bHasVectorStroke
    && LayerPtr->Type == EPsdLayerType::Shape;

if (CanvasParent && (bImageStroke || bShapeStroke))
{
    const FLinearColor StrokeColor = bShapeStroke
        ? LayerPtr->Effects.VectorStrokeColor
        : LayerPtr->Effects.StrokeColor;
    const float StrokePx = bShapeStroke
        ? LayerPtr->Effects.VectorStrokeSize
        : LayerPtr->Effects.StrokeSize;

    const FString BaseName = !LayerPtr->ParsedTags.CleanName.IsEmpty()
        ? LayerPtr->ParsedTags.CleanName : LayerPtr->Name;
    const FName StrokeFName = MakeUniqueObjectName(
        Tree, UImage::StaticClass(),
        FName(*FString::Printf(TEXT("%s_Stroke"), *BaseName)));
    UImage* StrokeImg = Tree->ConstructWidget<UImage>(UImage::StaticClass(), StrokeFName);

    FSlateBrush StrokeBrush;
    StrokeBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
    StrokeBrush.TintColor = FSlateColor(StrokeColor);
    StrokeBrush.ImageSize = FVector2D(
        static_cast<float>(LayerPtr->Bounds.Width())  + 2.f * StrokePx,
        static_cast<float>(LayerPtr->Bounds.Height()) + 2.f * StrokePx);
    StrokeImg->SetBrush(StrokeBrush);
    StrokeImg->SetRenderOpacity(StrokeColor.A);

    UCanvasPanelSlot* StrokeSlot = CanvasParent->AddChildToCanvas(StrokeImg);
    if (StrokeSlot)
    {
        FAnchorData StrokeData;
        StrokeData.Anchors  = CanvasSlot->GetLayout().Anchors;
        FMargin StrokeOffsets = CanvasSlot->GetLayout().Offsets;
        StrokeOffsets.Left -= StrokePx;
        StrokeOffsets.Top  -= StrokePx;
        // Width/Height already expanded via ImageSize; Left/Top offset achieves centering
        StrokeData.Offsets  = StrokeOffsets;
        StrokeData.Alignment = FVector2D(0.f, 0.f);
        StrokeSlot->SetLayout(StrokeData);
        // ZOrder = main - 1 (behind main widget, matching FX-04 drop shadow)
        StrokeSlot->SetZOrder(CanvasSlot->GetZOrder() - 1);
    }
}
```

### FX Block Naming Conflict

**Important:** The current generator already uses the comment label `// FX-05: Flatten fallback` at line 107. The CONTEXT.md uses "FX-05 block" to refer to the new stroke block. The planner must resolve this label conflict — either:
- Label the new stroke block `FX-06` and leave FX-05 as flatten fallback (safest, no existing comment changes needed)
- Relabel the flatten fallback (requires editing line 107 comment)

Recommended: use `// FX-06: Stroke sibling` for the new block. The CONTEXT.md's "FX-05" is a prospective name written before the generator was inspected. The code's existing FX-05 label wins.

### Anti-Patterns to Avoid

- **Copying TryParseAt(4) as primary offset from SoCo:** CP-01. vstk primary is TryParseAt(0). See PITFALLS.md CP-01 for full reasoning.
- **Writing to `bHasStroke` from `ScanVstkStroke`:** CP-02. Use `bHasVectorStroke` exclusively.
- **Calling `ScanVstkStroke` for all layer types:** D-03 guard is Shape-only. Image layers use lfx2 stroke; calling vstk scanner on Image layers is incorrect even if they happen to have a vstk block.
- **Treating `strokeStyleLineWidth` as a fixed 4-char key:** It is a long-form key. `ReadPsString` handles it, but an explicit `"Sz  "` comparison will miss it. Always compare against the full string `"strokeStyleLineWidth"`.
- **Skipping the `strokeEnabled` guard:** A vstk block exists on any shape layer that has ever had a stroke configured, even if the stroke was later disabled. Always check `strokeEnabled == true` before setting `bHasVectorStroke`.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Descriptor key iteration | Custom binary reader | `ReadPsString` lambda (already in ParseFrFXDescriptor and ScanShapeFillColor) | Already handles both 0-length (4-char) and N-length (long-form) PSD key strings |
| Sibling widget naming dedup | Manual suffix loop | `MakeUniqueObjectName(Tree, UImage::StaticClass(), BaseFName)` | Already used by FX-04 drop shadow at line 316; handles collision |
| Color sRGB conversion | Manual channel math | `FLinearColor::FromSRGBColor(FColor(R8, G8, B8, A8))` | Already used by ScanShapeFillColor at line 1635; correct gamma handling |
| Canvas child attachment | Direct `AddChild` | `CanvasParent->AddChildToCanvas(StrokeImg)` then `StrokeSlot->SetLayout(...)` | FX-04 pattern at lines 352–364; AddChild alone sets no position/z-order |

---

## Runtime State Inventory

Step 2.5: SKIPPED — Phase 22 is a new feature (stroke rendering). No rename, refactor, or migration. No runtime state affected.

---

## Environment Availability

Step 2.6: SKIPPED — Phase 22 requires no new external tools. All dependencies (UE 5.7, PhotoshopAPI vendored headers) are already present and verified by Phase 21 build.

---

## Common Pitfalls

### Pitfall 1: Wrong vstk descriptor offset (CP-01)
**What goes wrong:** Using TryParseAt(4) as primary (copying from SoCo). `TopCount` sanity check rejects the parse; `bHasVectorStroke` never set; shape layers silently have no stroke.
**Why it happens:** SoCo uses offset 4 (version prefix at 0-3). Copy-paste reflex from ScanSolidFillColor.
**How to avoid:** TryParseAt(0) is primary for vstk. Add hex-dump diagnostic (mirror of ScanSolidFillColor lines 1226-1238) to emit `vstk payload[0..40]=` at Verbose before any parse attempt.
**Warning signs:** ScanVstkStroke returns false for every layer even on PSDs known to have vector strokes. Verbose hex dump shows reasonable TopCount at offset 0 but garbage at offset 4.

### Pitfall 2: bHasStroke reuse collision (CP-02)
**What goes wrong:** Writing vstk stroke to `Effects.bHasStroke`. On a shape layer with both lfx2 stroke and vstk stroke, the values overwrite each other.
**Why it happens:** The existing `bHasStroke` comment says "populated for ALL layer types" — tempting to reuse it.
**How to avoid:** Three new separate fields: `bHasVectorStroke`, `VectorStrokeSize`, `VectorStrokeColor`. The lfx2 fields are exclusively owned by `ExtractLfx2Stroke`; vstk fields are exclusively owned by `ScanVstkStroke`.
**Warning signs:** Spec `TestFalse("Effects.bHasStroke cleared after routing")` in PsdParserSpec.cpp line 290 starts failing — lfx2 stroke routing test broken.

### Pitfall 3: Long-form key comparison mismatch
**What goes wrong:** Comparing against `"Sz  "` (4-char FrFX key) instead of `"strokeStyleLineWidth"` (long-form vstk key). The ReadPsString lambda returns the full string; the 4-char comparison always misses.
**Why it happens:** FrFX uses short keys; vstk uses long keys. Mixing contexts.
**How to avoid:** The inner loop key comparisons must use `Key == "strokeStyleLineWidth"`, `Key == "strokeEnabled"`, `Key == "strokeStyleContent"` — verified from psd-tools source as the canonical key names.
**Warning signs:** `SzRaw` stays 0 even when parsed; `bFoundColor` stays false; stroke renders at 0px.

### Pitfall 4: Calling ScanVstkStroke before OutLayer.Type is Shape
**What goes wrong:** If ScanVstkStroke is called before the RTTI dispatch assigns `OutLayer.Type = EPsdLayerType::Shape`, the D-03 conditional guard `if (OutLayer.Type == EPsdLayerType::Shape)` is never true, so the function is never called on shape layers.
**Why it happens:** Call site placed before the ShapeLayer RTTI branch.
**How to avoid:** Call site must be inside the ShapeLayer branch of ConvertLayerRecursive, after `OutLayer.Type = EPsdLayerType::Shape` is set, before `return`. Current architecture: ScanVstkStroke is called inside the `else if (ScanShapeFillColor(...))` block, after `OutLayer.Type = EPsdLayerType::Shape` assignment.

### Pitfall 5: FX block numbered conflict
**What goes wrong:** CONTEXT.md refers to the new stroke generator block as "FX-05" but the generator already has `// FX-05: Flatten fallback` at line 107.
**Why it happens:** CONTEXT.md was written before inspecting the actual generator code labels.
**How to avoid:** Label the new stroke block `// FX-06: Stroke sibling` in the implementation. Do NOT relabel the existing flatten fallback without also updating the CONTEXT.md and related comments.

### Pitfall 6: Stroke Offsets.Right/Bottom not expanded in canvas layout
**What goes wrong:** The stroke sibling slot copies `CanvasSlot->GetLayout().Offsets` which encodes `{Left, Top, Width, Height}` in point-anchor mode (non-stretch). When Left/Top are decremented by StrokePx to shift the sibling, the slot's Right/Bottom (which encode Width/Height) are NOT automatically expanded. The sibling renders at original size, displaced by -StrokePx, looking wrong.
**Why it happens:** `FMargin.Right` and `FMargin.Bottom` in non-stretch canvas slots encode Width and Height, not right-edge margins. Decrementing Left widens the visual offset but not the widget bounds.
**How to avoid:** After copying offsets, ADD `2*StrokePx` to `StrokeOffsets.Right` and `StrokeOffsets.Bottom` (which are width/height in non-stretch mode), and SUBTRACT `StrokePx` from `StrokeOffsets.Left` and `StrokeOffsets.Top`. Alternatively, set `StrokeBrush.ImageSize` and rely on the NoDrawType brush to override visual size — but canvas slot Size still drives layout bounds. Both expansions are required.

---

## Code Examples

### Adding fields to FPsdLayerEffects (PsdTypes.h)

```cpp
// Source: PsdTypes.h lines 117-122 (existing stroke fields shown for context)
// Phase 4.1 TEXT-03 -- Layer-Style Stroke (from lfx2/FrFX descriptor).
bool bHasStroke = false;
FLinearColor StrokeColor = FLinearColor::Transparent;
float StrokeSize = 0.f;

// Phase 22 STROKE-02 -- Vector Shape Stroke (from vstk/vecStrokeData tagged block).
// CP-02: separate fields; must NOT alias bHasStroke/StrokeSize/StrokeColor.
bool bHasVectorStroke = false;
float VectorStrokeSize = 0.f;
FLinearColor VectorStrokeColor = FLinearColor::Transparent;
```

### ScanVstkStroke call site in ConvertLayerRecursive (PsdParser.cpp)

```cpp
// Source: PsdParser.cpp ~line 2024, inside the ShapeLayer RTTI branch,
// after ScanShapeFillColor sets OutLayer.Type = EPsdLayerType::Shape
else if (ScanShapeFillColor(InLayer, OutLayer, OutDiag))
{
    OutLayer.Type = EPsdLayerType::Shape;
    DispatchTag = TEXT("Shape");
    // STROKE-02 / D-03: scan vstk for vector stroke on shape layers.
    // Must be called AFTER Type=Shape is set (D-03 guard is type-conditional).
    ScanVstkStroke(InLayer, OutLayer, OutDiag);
    if (OutLayer.Effects.bHasVectorStroke)
    {
        // D-03: vstk wins; clear lfx2 stroke so generator never sees both simultaneously.
        OutLayer.Effects.bHasStroke = false;
    }
}
```

### FX-04 reference pattern (drop shadow — direct model for stroke sibling)

```cpp
// Source: FWidgetBlueprintGenerator.cpp lines 305-366
// This is the EXACT pattern to mirror for STROKE-01/STROKE-03.
// Key elements: MakeUniqueObjectName, Tree->ConstructWidget<UImage>, 
// FSlateBrush with TintColor, AddChildToCanvas, SetLayout, SetZOrder(main-1).
if (LayerPtr->Effects.bHasDropShadow && bShadowSupportedType)
{
    const FName ShadowFName = MakeUniqueObjectName(Tree, UImage::StaticClass(), ...);
    UImage* ShadowImg = Tree->ConstructWidget<UImage>(UImage::StaticClass(), ShadowFName);
    // ... brush setup ...
    UCanvasPanelSlot* ShadowSlot = CanvasParent->AddChildToCanvas(ShadowImg);
    if (ShadowSlot)
    {
        // ... layout from CanvasSlot + offset delta ...
        ShadowSlot->SetZOrder(CanvasSlot->GetZOrder() - 1);
    }
}
```

### Non-regression spec pattern (ButtonStyles.psd)

```cpp
// Source: PsdParserSpec.cpp pattern (BEGIN_DEFINE_SPEC / BeforeEach / It / TestFalse)
// ButtonStyles.psd has NO stroke layers -- regression test asserts:
// 1. All root layers parse without diagnostics.AddErrors()
// 2. No layer has bHasVectorStroke == true (correct: no vstk blocks present)
// 3. Layer count / types match known ButtonStyles.psd structure.
It("ButtonStyles: no bHasVectorStroke on any layer (STROKE-03 regression)", [this]()
{
    for (const FPsdLayer& L : Doc.RootLayers)
        TestFalse(TEXT("No vector stroke"), L.Effects.bHasVectorStroke);
});
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| lfx2 stroke deferred ("rendering on non-text deferred") | Phase 22 adds STROKE-01 rendering for image layers | Phase 22 | `bHasStroke` on Image layers finally emits a sibling UImage |
| vstk stroke not parsed or rendered | Phase 22 adds STROKE-02 (parser) + STROKE-03 (generator) | Phase 22 | Shape layer vector strokes produce visible border approximation |

**Deprecated/outdated:**
- Comment `// rendering on non-text is deferred to a future phase` in PsdTypes.h line 119 (D-02 Phase 4.1): obsolete after STROKE-01 lands; update the comment.
- Comment `// Future stroke rendering (vstk -> UMG border/outline) will attach here` in FShapeLayerMapper.cpp line 13: correct location was moved to generator (D-01); update to reflect Phase 22 resolution.

---

## Open Questions

1. **vstk `strokeStyleContent` sub-descriptor class ID**
   - What we know: The color lives inside `strokeStyleContent` → `Clr ` → `RGBC` sub-descriptor, per psd-tools source evidence (HIGH).
   - What's unclear: Whether `strokeStyleContent` ostype is always `Objc` or could be `VlLs` for gradient strokes.
   - Recommendation: Only extract color when ostype == `Objc`. For `VlLs` (gradient stroke), skip and log a Verbose warning; stroke color will default to transparent and no sibling will be emitted (MiP-01 acceptable for v1.3).

2. **MiP-01 unit tag handling for strokeStyleLineWidth**
   - What we know: FrFX parser skips the unit tag (`Pos += 4`) and assumes pixels. vstk may write `#Pnt` (points) for shape strokes at high DPI (MiP-01 from PITFALLS.md).
   - What's unclear: Whether test PSDs used in development are all 72 DPI (1pt=1px).
   - Recommendation: Read and branch on the 4-byte unit tag. For `#Pxl` and `#Pt ` at 72 DPI, the value is identical. For now treating `#Pnt == #Pxl` (72 DPI assumption) is acceptable and matches the existing `ParseFrFXObjcItem` behavior.

3. **Non-stretch canvas anchor mode for stroke offset**
   - What we know: The drop shadow block (FX-04 model) adds a fixed pixel delta to `CanvasSlot->GetLayout().Offsets.Left/Top`. In non-stretch (point-anchor) mode, Offsets.Right and Offsets.Bottom are Width and Height.
   - What's unclear: Does the stroke sibling need to explicitly expand Width/Height in addition to shifting Left/Top by -StrokePx?
   - Recommendation: YES, expand both. Set `StrokeOffsets.Right += 2*StrokePx` and `StrokeOffsets.Bottom += 2*StrokePx` alongside `StrokeOffsets.Left -= StrokePx` and `StrokeOffsets.Top -= StrokePx`. This is distinct from FX-04 (drop shadow) which does not resize, only translates.

---

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | UE Automation Test (Spec API via `BEGIN_DEFINE_SPEC`) |
| Config file | None — engine discovers specs via EAutomationTestFlags |
| Quick run command | `Tests/PSD2UMG.Parser.* -ExecCmds="Automation RunTests PSD2UMG.Parser"` (editor CLI) |
| Full suite command | Run all `PSD2UMG.*` specs in editor Automation window or via command line |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| STROKE-01 | Image layer with `bHasStroke` emits stroke sibling UImage | Unit (spec-level) | `PSD2UMG.Parser.Effects` or new `PSD2UMG.Generator.StrokeSibling` spec | No fixture PSD for positive assert (D-04); deferred |
| STROKE-02 | `ScanVstkStroke` parses vstk block → `bHasVectorStroke`/`VectorStrokeSize`/`VectorStrokeColor` | Unit (parser spec) | New spec against future Stroke.psd fixture | ❌ Wave 0 — deferred per D-04; spec structure can be stubbed |
| STROKE-03 | Shape layer with `bHasVectorStroke` emits stroke sibling; no double-emit | Unit (generator spec) | `PSD2UMG.Generator.StrokeSibling` | ❌ Wave 0 |
| Non-regression | ButtonStyles.psd imports cleanly; no spurious `bHasVectorStroke` on any layer | Regression (parser spec) | New `PSD2UMG.Parser.ButtonStyles` spec | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** Run `PSD2UMG.Parser.*` to catch parser regressions
- **Per wave merge:** Full `PSD2UMG.*` suite
- **Phase gate:** Full suite green before `/gsd:verify-work`

### Wave 0 Gaps

- [ ] `Source/PSD2UMG/Tests/PsdParserSpec.cpp` — add `FPsdParserButtonStylesSpec` for ButtonStyles.psd non-regression (STROKE-03 criterion 4)
- [ ] `Source/PSD2UMG/Tests/FWidgetBlueprintGenSpec.cpp` — add `StrokeSibling` spec group for STROKE-01/STROKE-03 generator assertions (positive tests deferred per D-04; stub with TODO comment until Stroke.psd fixture arrives)

*(Existing `FPsdParserEffectsSpec` and `FPsdParserSpec` require no changes — they test unrelated fixtures.)*

---

## Project Constraints (from CLAUDE.md)

| Constraint | Applies to Phase 22 |
|------------|---------------------|
| UE 5.7.4 — use UE5 APIs (FAppStyle not FEditorStyle) | Generator uses `CanvasPanel->AddChildToCanvas`, `Tree->ConstructWidget` — already UE5 |
| C++20 standard | No new C++ features required; existing `std::shared_ptr`, lambdas, `std::string` patterns sufficient |
| No Python at runtime | Not applicable — parser/generator only |
| PhotoshopAPI static lib linkage | No new linkage; `vecStrokeData` enum already in vendored headers |
| Editor-only, LoadingPhase PostEngineInit | Not affected |
| MakeUniqueObjectName for widget deduplication | Required for stroke sibling (as done for shadow sibling at line 316) |
| WidgetTree->ConstructWidget<T>() absolute rule | Stroke sibling UImage must use ConstructWidget, not NewObject |
| GSD workflow enforcement | Phase 22 work must proceed through `/gsd:execute-phase` |

---

## Sources

### Primary (HIGH confidence)
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` — `ScanShapeFillColor` (lines 1445-1661), `ExtractLfx2Stroke` (lines 1803-1814), `ConvertLayerRecursive` ShapeLayer branch (lines 1996-2042), `ParseFrFXObjcItem` lambda (lines 1038-1120)
- `Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp` — FX-04 drop shadow block (lines 305-373)
- `Source/PSD2UMG/Public/Parser/PsdTypes.h` — `FPsdLayerEffects` struct (lines 102-123)
- `Source/PSD2UMG/Private/Mapper/FShapeLayerMapper.cpp` — confirms mapper returns single UWidget*, no side-channel
- `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/Util/Enum.h:822` — `vecStrokeData` (`"vstk"`) confirmed present
- `.planning/research/PITFALLS.md` — CP-01 (vstk offset=0), CP-02 (separate field), MiP-01 (unit tag), MiP-02 (structured API for vstk)

### Secondary (MEDIUM confidence)
- psd-tools source `psd_tools/api/shape.py` — Stroke class uses `strokeStyleLineWidth`, `strokeEnabled`, `strokeStyleContent` as long-form descriptor key names (MEDIUM: Python library; keys confirmed consistent with PSD spec)
- PSD format spec via paulbourke.net — vstk "Version(=16) + Descriptor" structure (MEDIUM: PhotoshopAPI may pre-strip version before `Block->m_Data`; PITFALLS.md CP-01 provides resolution)

### Tertiary (LOW confidence)
- Adobe community forum posts (bjango.com CS6 strokes article) — stroke alignment behavior (outside/inside/center); v1.3 is center-aligned approximation only

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — no new dependencies; all referenced code exists and was read directly
- Architecture: HIGH — ScanShapeFillColor and FX-04 are exact models; differences documented
- Pitfalls: HIGH — CP-01/CP-02 from PITFALLS.md (prior research); MiP-01/Pitfall 6 from code inspection
- vstk descriptor key names: MEDIUM — from psd-tools Python source (cross-language evidence); would increase to HIGH with a real vstk PSD hex dump

**Research date:** 2026-04-29
**Valid until:** Stable (PSD format and PhotoshopAPI vendored headers don't change between phases; valid until PhotoshopAPI version bump)
