# Architecture Patterns — v1.3 Advanced Effects Integration

**Domain:** UE 5.7 editor plugin — PSD-to-UMG import pipeline
**Researched:** 2026-04-28
**Scope:** Four v1.3 features: vstk stroke on image/shape layers, PtFl pattern fill, lrFX channel-order fix, UTF-16 rich-text slicing fix

---

## How the Existing Pipeline is Structured

```
FPsdParser::ParseFile()
  └─ ConvertLayerRecursive()
       ├─ ExtractLayerEffects()       // lrFX (sofi, dsdw, isdw, bevl)
       ├─ ExtractLfx2Stroke()         // lfx2/FrFX raw byte scan → Effects.bHasStroke
       ├─ RouteTextEffects()          // moves Effects.* onto Text.* for text layers
       └─ populates FPsdLayer
            ├─ .Type  (EPsdLayerType)
            ├─ .Effects  (FPsdLayerEffects)
            ├─ .Text     (FPsdTextRun + Spans[])
            └─ .RGBAPixels

FLayerMappingRegistry::MapLayer(FPsdLayer)   // sorted by priority descending
  └─ IPsdLayerMapper::CanMap() → Map()       // produces UWidget*

FWidgetBlueprintGenerator::PopulateChildren()
  ├─ Flatten-fallback override
  ├─ Registry.MapLayer() → UWidget*
  ├─ Canvas slot layout / anchor assignment
  ├─ Drop shadow: canvas-only UImage sibling (below main widget)
  └─ FX block: opacity, visibility, color overlay (brush tint or panel child)
```

Data flows one direction: parser writes FPsdLayer, mapper reads it, generator applies residual effects after mapper returns the widget. The generator never calls back into the parser.

---

## Feature 1 — vstk Stroke on Image/Shape Layers

### Where the data lives now

`FPsdLayerEffects.bHasStroke` / `StrokeColor` / `StrokeSize` are already populated for **all** layer types by `ExtractLfx2Stroke` (raw lfx2 byte scan). `RouteTextEffects` then moves those fields onto `FPsdTextRun.OutlineColor/OutlineSize` and clears the Effects flags so the text mapper consumes them. For non-text layers (Image, Shape) `bHasStroke` remains set on `FPsdLayerEffects` but nothing currently reads it.

The comment in `FShapeLayerMapper.cpp` line 13 explicitly names the attachment point:
> "Future stroke rendering (vstk -> UMG border/outline) will attach here, not in FSolidFillLayerMapper."

### Rendering strategy: sibling UBorder vs brush modification

**Option A — Modify the brush (DrawAs = Border)**
`FSlateBrush::DrawAs = ESlateBrushDrawType::Border` renders the brush as a Slate border with `Margin` controlling the border thickness. Setting `DrawAs=Border`, `Margin` to a value derived from `StrokeSize`, and `TintColor` to `StrokeColor` produces a visible border on the existing `UImage`. This is a mapper-internal change: `FImageLayerMapper::Map` and `FShapeLayerMapper::Map` inspect `Layer.Effects.bHasStroke` and set the brush accordingly. No new widgets; no generator changes.

**Option B — Canvas-only sibling UBorder widget (mirrors drop shadow)**
The generator creates a separate `UBorder` widget (identical bounds, `StrokeColor` background brush, placed at same ZOrder) as a sibling to the main widget. Same canvas-only sibling pattern as drop shadow. Generator-internal change: add a `bStrokeSupportedType` block parallel to the drop-shadow block at lines 305-373 of `FWidgetBlueprintGenerator.cpp`.

**Recommended: Option A (brush modification)**

Rationale:
- The UMG Border draw mode is the canonical way to render a framed image. Option A produces a single widget, which is simpler and more robust than a sibling pattern.
- Drop shadow uses a sibling because it needs a spatial offset; stroke has no offset (it wraps the widget boundary). There is no geometric argument for a sibling.
- Option A requires no generator changes, only mapper changes. This is a smaller diff surface and keeps the mapper self-contained.
- FShapeLayerMapper already produces a UImage with `DrawAs=Image`; switching it to `DrawAs=Border` with margin from `StrokeSize` is a direct extension.

**Limitation:** Slate's Border draw mode is a 9-slice border, not a CSS-style outline. For thick strokes the image area shrinks. Acceptable for v1.3 as an approximation. True "outside stroke" (stroke does not consume image area) would require a sibling with negative margin offsets, which is Option B territory. Document this in PITFALLS.

### New vs modified files

| File | Change Type | What Changes |
|------|-------------|--------------|
| `Source/PSD2UMG/Private/Mapper/FImageLayerMapper.cpp` | MODIFIED | After building `FSlateBrush`, check `Layer.Effects.bHasStroke`; if true, set `Brush.DrawAs = Border`, `Brush.Margin` from `StrokeSize`, `Brush.TintColor` from `StrokeColor`. Clear `Effects.bHasStroke` before return (D-13 guard). |
| `Source/PSD2UMG/Private/Mapper/FShapeLayerMapper.cpp` | MODIFIED | Same brush modification block. `FShapeLayerMapper` already sets a zero-texture UImage; switch `DrawAs=Border` when `bHasStroke`. |
| `Source/PSD2UMG/Tests/FWidgetBlueprintGenSpec.cpp` | MODIFIED | Add It() blocks: "Image layer with stroke renders DrawAs=Border", "Shape layer with stroke renders DrawAs=Border". |

No parser changes needed: `ExtractLfx2Stroke` already fires for all layer types including Image/Shape. No generator changes needed for Option A.

### Data path diagram

```
lrFX raw scan → ExtractLfx2Stroke → FPsdLayerEffects.bHasStroke / StrokeSize / StrokeColor
                                                   |
                             RouteTextEffects: if Text → moves to FPsdTextRun, clears flag
                                                   | (non-text: flag remains)
                             FImageLayerMapper::Map() / FShapeLayerMapper::Map()
                               inspect Effects.bHasStroke → set FSlateBrush.DrawAs=Border
```

---

## Feature 1b — frameFXMulti VlLs stroke format (newer Photoshop)

### Context

Newer Photoshop versions (CC 2020+) may store stroke effects in a `frameFXMulti` key inside the lfx2 descriptor instead of (or in addition to) the `FrFX` key. The `frameFXMulti` value is a `VlLs` (value list) containing zero or more `Objc` entries, each with the same internal structure as `FrFX`. The existing `ParseFrFXDescriptor` walker skips any key that is not `FrFX`.

### Integration point

This is a parser-only change in `ParseFrFXDescriptor` (inside `PsdParser.cpp`). The outer key check `if (ItemKey == "FrFX" && FCStringAnsi::Strcmp(OsType, "Objc") == 0)` needs an additional branch for `frameFXMulti VlLs`. Each `Objc` entry in the list is parsed using the same inner `FrFX` logic.

Because `ParseFrFXDescriptor` populates `FPsdStrokeInfo.bEnabled` which then feeds `ExtractLfx2Stroke` → `FPsdLayerEffects`, downstream code (mappers, generator) sees no change at all.

| File | Change Type | What Changes |
|------|-------------|--------------|
| `Source/PSD2UMG/Private/Parser/PsdParser.cpp` | MODIFIED | `ParseFrFXDescriptor`: add `frameFXMulti VlLs` branch alongside existing `FrFX Objc` branch. |

---

## Feature 2 — Pattern Fill Layers (PtFl)

### Where pattern fill appears in the PSD format

A Photoshop "Pattern Fill" adjustment layer stores its fill descriptor under the `TaggedBlockKey::adjPatternFill` (four-byte key `PtFl`) in the layer's additional-layer-info. PhotoshopAPI v0.9 does not expose this key via `unparsed_tagged_blocks()` — it is silently dropped like `lfx2`. However, PhotoshopAPI **does** composite the layer before giving us channel data, so `RGBAPixels` already contains the correct tiled pixel output.

### Recommended representation: no new type; fall through to FImageLayerMapper

Two options were evaluated:

**Option A — New `EPsdLayerType::PatternFill`**
Mirror the `EPsdLayerType::Gradient` / `SolidFill` / `Shape` pattern. In `ConvertLayerRecursive`, when the `adjPatternFill` tagged block is found (or inferred), set `OutLayer.Type = EPsdLayerType::PatternFill`. Add `FPatternFillLayerMapper : public IPsdLayerMapper` that returns a `UImage` built from the already-composited `RGBAPixels`.

**Option B — No new type; fall through to FImageLayerMapper**
Since `RGBAPixels` already contains the tiled fill, pattern fill layers are indistinguishable from rasterized pixel layers from the mapper's point of view. Letting them fall through to `FImageLayerMapper` (which imports `RGBAPixels` as a `UTexture2D`) produces a tiled-texture `UImage` with correct visuals. No new type, no new mapper.

**Recommended: Option B for v1.3**

Rationale:
- Pattern fills are semantically tiled textures. The composited pixel output from PhotoshopAPI is already correct. Importing it as a plain image asset is lossless from a visual fidelity standpoint.
- A new `EPsdLayerType::PatternFill` would require parser plumbing to detect `adjPatternFill` in `unparsed_tagged_blocks()` — but PhotoshopAPI drops this block, so detection requires a raw-byte scan parallel to `ScanRawLfx2Blocks`. That is significant scope for no functional gain: the output widget is identical to FImageLayerMapper's.
- The v1.3 goal is "tiled texture UImage" not "live tiling with configurable repeat count". Live tiling is out of scope.
- If in a future milestone pattern fill metadata (tile size, repeat mode, phase offset) is needed, a `FPsdLayerEffects.PatternFill` sub-struct or a new type can be added then.

**What actually needs to happen for v1.3:**
1. Verify that `adjPatternFill` layers are reaching `FImageLayerMapper` and not being dropped due to missing `RGBAPixels`. If they are already working, no code change is needed — only a test.
2. If they are being dropped (empty `RGBAPixels`), add pixel extraction for `adjPatternFill` layers in `ConvertLayerRecursive` using the same `ImageLayer` pixel extraction path.

| File | Change Type | What Changes |
|------|-------------|--------------|
| `Source/PSD2UMG/Private/Parser/PsdParser.cpp` | MAYBE MODIFIED | `ConvertLayerRecursive`: ensure `adjPatternFill` layers are classified as `EPsdLayerType::Image` with `RGBAPixels` populated (currently unverified). |
| `Source/PSD2UMG/Tests/PsdParserSpec.cpp` | MODIFIED | Add spec for PatternFill fixture layer round-trips through FImageLayerMapper. |

No new type in `EPsdLayerType`. No new mapper. No `FPsdLayerEffects` field.

---

## Feature 3 — lrFX Channel-Order Fix

### Current state

`ExtractLayerEffects` parses the lrFX v0 binary block for `sofi` (color overlay) and `dsdw` (drop shadow). The color channels are extracted as C0/C1/C2 from a 10-byte color record `[2 colorSpace][8 channel data (4 x uint16)]` then assembled as `FLinearColor(C0, C1, C2, A)`.

The PSD spec says this 10-byte color record uses colorSpace 0 = RGB, where C0=R, C1=G, C2=B. The current code reads them in index order (0, 1, 2) with positional reads — colorSpace 0 (RGB) maps position 0=R, 1=G, 2=B. The current `FLinearColor(C0, C1, C2, A)` constructor is `FLinearColor(float R, float G, float B, float A)`, so this is **correct for colorSpace=0 (RGB)**.

The "lrFX channel-order fix" item in v1.3 scope is a visual confirmation task, not a known code defect. The Verbose logs already emit `C0=%.3f C1=%.3f C2=%.3f` with the colorSpace value, which allows a human to verify against a known-color layer.

### Integration point

This is a **parser-only** verification and potential fix. No mappers, no generator.

Two sub-cases:

**Sub-case A (confirm existing code is correct):** Run against a host project PSD with a layer that has a known solid-color overlay (e.g., pure red `FF0000` overlay on a shape). The Verbose log will show `C0=1.000 C1=0.000 C2=0.000`. If the color renders correctly in the imported widget, no code change is needed.

**Sub-case B (fix if channel order is wrong):** If empirical testing shows the overlay renders the wrong color, the fix is in `ExtractLayerEffects` — swap the assignment order of the three `ReadU16()` calls in the `sofi` and/or `dsdw` branches. This is a 2-line change per branch.

| File | Change Type | What Changes |
|------|-------------|--------------|
| `Source/PSD2UMG/Private/Parser/PsdParser.cpp` | MAYBE MODIFIED | `sofi`/`dsdw` branches: swap channel reads only if empirical visual confirm shows wrong colors. |

No mapper or generator changes regardless of outcome.

---

## Feature 4 — UTF-16 Slicing Fix for Non-ASCII Rich Text

### Where the bug lives

In `ConvertLayerRecursive`, the multi-run span extraction block (lines 469-610 of `PsdParser.cpp`) slices the layer's `Content` string into per-run substrings using `FString::Mid(CharOffset, RunLen)`. The `RunLen` values come from `Text->style_run_lengths()`, which per the PSD spec are **UTF-16 code-unit counts**. The existing TODO in the code notes this directly:

> "For non-ASCII text (CJK, combining marks, emoji), length units may disagree between PhotoshopAPI's UTF-16 counts and the FString's platform-TCHAR indexing; slicing may cut mid-codepoint."

The text content arrives via `Text->text().value_or("")` as a UTF-8 `std::string`, which is then converted to `FString` (internally stored as UTF-16 TCHAR on Win64). On Win64 where `TCHAR` is `wchar_t` (2 bytes), `FString::Mid` offsets by UTF-16 code units, which matches the PSD spec's `style_run_lengths()` for BMP characters. The bug only manifests on characters outside the BMP (emoji, some CJK extension blocks) that require surrogate pairs (2 UTF-16 code units per codepoint). For BMP CJK (U+4E00..U+9FFF), one codepoint = one UTF-16 code unit, so the slice is currently correct for those characters despite the code-unit/TCHAR mismatch.

### Integration point

This is a **parser-only** change. The fix is inside the span extraction loop in `ConvertLayerRecursive`. No types, no mappers, no generator changes.

**Fix approach:**

1. Convert `FullUtf8` (`std::string` in UTF-8) to a `TArray<uint16>` (UTF-16 code units) via `FTCHARToUTF16` or manual UTF-8 to UTF-16 transcoding before the loop.
2. Slice the UTF-16 array by run lengths to get per-run UTF-16 code-unit sequences.
3. Convert each slice back to `FString` for `Span.Text` using `FString(TArray<uint16>)` or `UTF16_TO_TCHAR`.

This replaces the current `FString::Mid(CharOffset, RunLen)` with a code-unit–accurate slice. The `FString` constructor from UTF-16 data handles surrogate pairs correctly on all platforms.

| File | Change Type | What Changes |
|------|-------------|--------------|
| `Source/PSD2UMG/Private/Parser/PsdParser.cpp` | MODIFIED | Span extraction loop: replace `FString::Mid(CharOffset, RunLen)` slicing with UTF-16 code-unit–accurate slice. Remove the TODO comment when fixed. |
| `Source/PSD2UMG/Tests/PsdParserSpec.cpp` | MODIFIED | Add spec for CJK multi-run text fixture (requires a PSD with non-ASCII characters and multiple runs). |
| `Source/PSD2UMG/Tests/Fixtures/` | NEW FILE | CJK/emoji rich-text `.psd` fixture (or extend existing `RichText.psd` with a CJK layer). |

No mapper or generator changes. `FRichTextLayerMapper` receives `FString` spans after the fix; its markup building is unaffected.

---

## Component Boundaries Summary

| Component | v1.3 Change |
|-----------|-------------|
| `PsdParser.cpp` — `ExtractLayerEffects` | lrFX channel-order: verify only (possible 2-line fix in sofi/dsdw) |
| `PsdParser.cpp` — `ParseFrFXDescriptor` | frameFXMulti VlLs: add alternate stroke key branch alongside FrFX |
| `PsdParser.cpp` — `ConvertLayerRecursive` | PtFl: verify pixel extraction; UTF-16: replace Mid slicing with code-unit slice |
| `PsdParser.cpp` — `ExtractLfx2Stroke` | No change — already fires for all layer types |
| `PsdParser.cpp` — `RouteTextEffects` | No change — already routes stroke to Text.* for text layers |
| `PsdTypes.h` — `FPsdLayerEffects` | No change — bHasStroke / StrokeSize / StrokeColor already present |
| `PsdTypes.h` — `EPsdLayerType` | No change — no new type for pattern fill |
| `FImageLayerMapper.cpp` | MODIFIED: apply brush DrawAs=Border when bHasStroke |
| `FShapeLayerMapper.cpp` | MODIFIED: apply brush DrawAs=Border when bHasStroke |
| `FWidgetBlueprintGenerator.cpp` | No change — stroke handled inside mappers (Option A) |
| `AllMappers.h` | No change — no new mapper classes |
| `FLayerMappingRegistry.cpp` | No change — no new mapper registrations |
| `FRichTextLayerMapper.cpp` | No change — receives fixed FString spans |

---

## Build Order (Phase Sequencing)

Dependencies determine this order: parser must stabilize before types are locked, types must be locked before mappers, mappers before generator.

### Phase A — Parser fixes (no type or mapper dependencies)

1. **UTF-16 slicing fix** — self-contained loop change in `ConvertLayerRecursive`. Requires a CJK/emoji fixture. No downstream consumers change. Spec: add CJK multi-run spec.
2. **lrFX channel-order confirm** — visual UAT against host project PSD. Parser-only potential 2-line fix. No downstream consumers change.
3. **frameFXMulti VlLs** — add branch to `ParseFrFXDescriptor`. Downstream unchanged (same `FPsdLayerEffects.bHasStroke` output path). Spec: add fixture with newer-Photoshop stroke.

These three can run in parallel or in any order. None produces type-level changes that mappers depend on.

### Phase B — Mapper changes (depends on Phase A stability for full stroke coverage)

4. **Image-layer stroke** — modify `FImageLayerMapper::Map` to set `DrawAs=Border` when `bHasStroke`. Depends on parser already setting `bHasStroke` for image layers (it does; this is verified existing behavior from Phase 4.1). Can start before A3 is done if testing against old-format PSDs only.
5. **Shape-layer stroke** — modify `FShapeLayerMapper::Map` for same brush change. Can run in parallel with Phase B-4.

### Phase C — Pattern fill verify (independent, can overlap Phase A or B)

6. **PtFl verify** — check whether `adjPatternFill` layers arrive at `FImageLayerMapper` correctly. If pixel data is already present (PhotoshopAPI composites it), this is a test-only task. If pixel extraction is missing, add the extraction branch in `ConvertLayerRecursive`.

### Dependency graph

```
Phase A (parser fixes — parallel)
  A1: UTF-16 slicing fix     [PsdParser.cpp]
  A2: lrFX channel-order UAT [PsdParser.cpp — maybe no code change]
  A3: frameFXMulti VlLs      [PsdParser.cpp — ParseFrFXDescriptor]
        |
        v (parser stable, all stroke data flows correctly)
Phase B (mapper changes — parallel)
  B1: FImageLayerMapper stroke brush
  B2: FShapeLayerMapper stroke brush

Phase C (independent)
  C1: PtFl verify + test
```

Recommended PRs:
- PR 1: A1 + A3 (both parser-internal, no type/mapper surface)
- PR 2: A2 is a UAT step (no PR until visual confirm determines if a code fix is needed)
- PR 3: B1 + B2 (both mappers touch the same brush modification pattern, small diff)
- PR 4: C1 (verification + test, possibly no code change)

---

## Files: New vs Modified

**NEW files:**
- `Source/PSD2UMG/Tests/Fixtures/CjkRichText.psd` (or extend `RichText.psd`) — test fixture for UTF-16 slicing spec.

**MODIFIED files:**
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` — UTF-16 span slicing (A1), frameFXMulti VlLs branch (A3), possible lrFX channel-order fix (A2).
- `Source/PSD2UMG/Private/Mapper/FImageLayerMapper.cpp` — stroke brush DrawAs=Border (B1).
- `Source/PSD2UMG/Private/Mapper/FShapeLayerMapper.cpp` — stroke brush DrawAs=Border (B2).
- `Source/PSD2UMG/Tests/PsdParserSpec.cpp` — CJK multi-run spec (A1), frameFXMulti spec (A3).
- `Source/PSD2UMG/Tests/FWidgetBlueprintGenSpec.cpp` — image/shape stroke brush mode spec (B1/B2).

**UNCHANGED files (confirmed no changes needed):**
- `Source/PSD2UMG/Public/Parser/PsdTypes.h` — no new fields or enum values.
- `Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp` — stroke handled in mappers.
- `Source/PSD2UMG/Private/Mapper/AllMappers.h` — no new mapper classes.
- `Source/PSD2UMG/Private/Mapper/FLayerMappingRegistry.cpp` — no new mapper registrations.
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp::RouteTextEffects` — text stroke routing already correct.

---

## Key Architectural Decisions

| Decision | Rationale |
|----------|-----------|
| Stroke on Image/Shape as brush modification (Option A), not sibling widget (Option B) | Drop shadow needs offset so it uses a sibling. Stroke wraps bounds with no offset, making brush modification sufficient. Simpler, fewer widgets, mapper-internal. |
| No new `EPsdLayerType::PatternFill` | PhotoshopAPI composites pixels; the output is indistinguishable from raster. Adding a type requires raw-byte scanning of a block PhotoshopAPI drops, for zero functional gain at v1.3 scope. |
| UTF-16 fix via `TArray<uint16>` slice not `FString::Mid` | Directly mirrors PSD spec code-unit model; platform-portable; eliminates the surrogate-pair slicing bug without relying on Win64 TCHAR width coincidence. |
| lrFX channel-order as UAT-first, code-change-if-needed | The existing C0/C1/C2 positional reads are consistent with colorSpace=0=RGB per spec. A visual confirm against a known-color layer is sufficient before writing a fix that may be unnecessary. |
| frameFXMulti VlLs as parser-internal only | Same `FPsdStrokeInfo` output path; zero impact on downstream types, mappers, generator. |
