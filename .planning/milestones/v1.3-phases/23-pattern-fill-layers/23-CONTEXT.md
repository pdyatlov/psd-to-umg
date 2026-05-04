# Phase 23: Pattern Fill Layers - Context

**Gathered:** 2026-05-04
**Status:** Ready for planning

<domain>
## Phase Boundary

Two requirements:
1. **PTFL-01** — `ConvertLayerRecursive` detects `adjPattern` (`TaggedBlockKey::adjPattern` / `"PtFl"`) in the first-pass AdjustmentLayer tag scan and sets `EPsdLayerType::PatternFill` (new enum value). Layer must not fall to `Unknown`.
2. **PTFL-02** — `FPatternFillLayerMapper` (priority 101) produces a UImage backed by `RGBAPixels` via `FTextureImporter::ImportLayer`. When `RGBAPixels` is empty, mapper returns `nullptr` with a `UE_LOG Warning` — import completes rather than crashing.

No generator FX blocks. No color overlay. No new descriptor scanner. Mapper is structurally identical to `FFillLayerMapper` but dispatches on `PatternFill` type.

</domain>

<decisions>
## Implementation Decisions

### Detection Placement
- **D-01:** `adjPattern` detection belongs in the **first-pass AdjustmentLayer tag scan** (PsdParser.cpp lines 2161–2194, alongside `adjSolidColor` and `adjGradient`). Pattern fill layers are `AdjustmentLayer<T>` in PhotoshopAPI — the same concrete type as gradient and solid fill. Adding a third `bIsPatternFill` bool to the existing loop keeps the detection block coherent and avoids a fourth RTTI dispatch branch. Claude's discretion.

### Pixel Extraction
- **D-02:** Call `ExtractImagePixels` for `adjPattern` layers (same as the `adjGradient` branch). PhotoshopAPI composites the tiled pattern before we see it. No `ScanPatternFillColor()` required — CP-04: the `PtFl` descriptor has no `Clr ` key; color is embedded in the composited pixels, not a parsed field.

### Mapper Implementation
- **D-03:** New `FPatternFillLayerMapper.cpp` — separate file matching the established one-type-per-file pattern. `CanMap` dispatches on `Layer.Type == EPsdLayerType::PatternFill`. `Map` mirrors `FFillLayerMapper::Map` exactly (priority 101, `FTextureImporter::ImportLayer`, `SetBrushFromTexture`, `DrawAs = Image`). No shared base or helper needed.

### Empty Pixel Fallback
- **D-04:** When `RGBAPixels` is empty (texture import returns nullptr), `FPatternFillLayerMapper::Map` logs `UE_LOG Warning` and returns `nullptr`. The generator's existing "No mapper" warning path handles the skip. No parser changes (`bHasComplexEffects` is NOT set). Consistent with `FFillLayerMapper`'s nullptr-on-failure path. The generator FX-05 flatten path requires both `bHasComplexEffects=true` AND `RGBAPixels.Num() > 0` — setting the flag without pixels would still produce nothing.

### Fixture Strategy
- **D-05:** Spec-only stubs — no PatternFill.psd fixture for this phase. Parser spec (`FPsdParserPatternSpec`) asserts PTFL-01 using a synthetic layer with `adjPattern` tag injection (mirrors FPsdParserGradientSpec pattern). Mapper spec (`FPatternFillLayerMapperSpec`) asserts PTFL-02 via a pre-filled `FPsdLayer` with `Type=PatternFill` and synthetic `RGBAPixels`. Positive fixture PSD deferred until user has a real pattern fill layer handy.

### Priority
- **D-06:** Priority 101 — consistent with `FFillLayerMapper` (101) and `FSolidFillLayerMapper` (101) per Phase 20 D-01. Wins the `FImageLayerMapper` collision (FLayerTagParser maps `PatternFill` → `EPsdTagType::Image`).

### Claude's Discretion
- Whether the `PatternFill` enum value is inserted before or after `Shape` in `EPsdLayerType` (ordering is cosmetic; Claude picks adjacent to `Gradient`/`SolidFill`).
- Whether the parser spec uses a mock `unparsed_tagged_blocks()` block or a real `adjPattern` key lookup (spec design detail).
- Plan count and split (single plan likely sufficient given narrow scope; researcher/planner decide).

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### First-pass detection (PTFL-01 home)
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` — first-pass AdjustmentLayer tag scan (~lines 2161–2194): `bIsSolidFill`/`bIsGradientFill` loop — add `bIsPatternFill` here; `adjGradient` pixel-extraction branch as the model for `adjPattern` branch

### Type definitions
- `Source/PSD2UMG/Public/Parser/PsdTypes.h` — `EPsdLayerType` enum (lines 15–25): add `PatternFill` value adjacent to `Gradient`/`SolidFill`

### Mapper model (PTFL-02 must mirror this)
- `Source/PSD2UMG/Private/Mapper/FFillLayerMapper.cpp` — `FFillLayerMapper::Map()`: copy/adapt for `FPatternFillLayerMapper`; `CanMap` change is the only delta

### Mapper registry
- `Source/PSD2UMG/Private/Mapper/AllMappers.h` — add `FPatternFillLayerMapper` declaration here
- `Source/PSD2UMG/Private/Mapper/FLayerMappingRegistry.cpp` — add `FPatternFillLayerMapper` instantiation

### PhotoshopAPI enum
- `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/Util/Enum.h` — `TaggedBlockKey::adjPattern` (line 757); `"PtFl"` string key (line 832)

### Requirements + pitfalls (MANDATORY)
- `.planning/REQUIREMENTS.md` — PTFL-01, PTFL-02 specs; CP-04 (no `Clr` key in `PtFl` descriptor)

### Prior phase decisions (fill mapper pattern)
- `.planning/phases/13-gradient-layers/13-CONTEXT.md` — GRAD-01/GRAD-02: fill mapper pattern; first-pass AdjustmentLayer detection rationale
- `.planning/phases/20-integration-stability-fixes/20-CONTEXT.md` — D-01: priority 101 for fill mappers

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `FFillLayerMapper::Map()` (`Mapper/FFillLayerMapper.cpp`): copy verbatim for `FPatternFillLayerMapper`; only `CanMap` changes
- First-pass AdjustmentLayer detection block (`PsdParser.cpp:2161–2194`): add one bool and one `if` branch; no structural change
- `FTextureImporter::ImportLayer` (`Generator/FTextureImporter.h`): used by `FFillLayerMapper`; reuse unchanged

### Established Patterns
- AdjustmentLayer first-pass detection: `bIsSolidFill` + `bIsGradientFill` loop — extend with `bIsPatternFill`
- `ExtractImagePixels` called for gradient fills (`Adj` cast path) — same for pattern fills
- Priority 101 for specialized fill mappers — `FFillLayerMapper`, `FSolidFillLayerMapper` both at 101
- Separate `.cpp` per mapper type (all 15 mappers follow this)

### Integration Points
- `EPsdLayerType` enum in `PsdTypes.h`: +1 value
- `AllMappers.h`: +1 class declaration
- `FLayerMappingRegistry.cpp`: +1 `MakeUnique<FPatternFillLayerMapper>()` instantiation
- `PsdParser.cpp` first-pass block: +1 bool detection + 1 dispatch branch

</code_context>

<specifics>
## Specific Notes

- CP-04: `PtFl` descriptor has NO `Clr ` key — do not attempt color parsing from the tagged block. The composited `RGBAPixels` are the only data source.
- `TaggedBlockKey::adjPattern` confirmed at `Enum.h:757`; `"PtFl"` string at line 832.
- First-pass block at lines 2161–2194 handles `AdjustmentLayer<T>` types (SoCo, GdFl). `adjPattern` (`PtFl`) is also an `AdjustmentLayer<T>` — same dispatch tier, no new RTTI branch needed.
- The `adjPattern` branch must call `ExtractImagePixels` with the `Adj` cast path (same as `adjGradient`) so PhotoshopAPI composites the tiled pixels before `RGBAPixels` is populated.

</specifics>

<deferred>
## Deferred Ideas

- PatternFill.psd fixture: deferred until user can provide a PSD with a real pattern fill layer. Positive end-to-end assertion (UImage dimensions match layer bounds, texture asset created) is the main gap vs spec-only.
- Material-based tiling for pattern fills: out of scope per REQUIREMENTS.md v1.3 — composited PNG is always visually correct for this milestone.
- Pattern scale/offset metadata from `PtFl` descriptor: not attempted; composited pixels already represent the final tiled result.

</deferred>

---

*Phase: 23-pattern-fill-layers*
*Context gathered: 2026-05-04*
