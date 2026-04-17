# Phase 3: Layer Mapping & Widget Blueprint Generation — Context

**Gathered:** 2026-04-09
**Status:** Ready for planning

<domain>
## Phase Boundary

Take `FPsdDocument` (produced by `FPsdParser::ParseFile` in Phase 2) and generate a valid, openable `UWidgetBlueprint` asset with all baseline widget types, correct canvas-space positions, persistent `UTexture2D` assets for image layers, and sensible auto-assigned anchors.

**In scope:**
- `IPsdLayerMapper` interface + `FLayerMappingRegistry` (internal dispatch, priority-ordered)
- All prefix-mapped widget types: Image, Text, Group (Canvas), Button_, Progress_, HBox_, VBox_, Overlay_, ScrollBox_, Slider_, CheckBox_, Input_, List_, Tile_, Switcher_
- `FWidgetBlueprintGenerator`: constructs `UWidgetBlueprint`, compiles, saves
- Widget positions + sizes from `FPsdLayer.Bounds` → `UCanvasPanelSlot`
- Layer z-order preserved
- Anchor heuristics (quadrant-based) + override suffixes (_anchor-tl, _anchor-c, _stretch-h, _stretch-v, _fill)
- Texture import: `UTexture2D` via `Source.Init()`, organized at `{TargetDir}/Textures/{PsdName}/{LayerName}`
- Duplicate layer name deduplication (append index)
- Basic text properties on `UTextBlock`: content, DPI-converted size (× 0.75), color
- Skip-and-warn behavior for failed layers

**Out of scope (later phases):**
- Font mapping, outline, shadow, bold/italic, multi-line wrap (Phase 4)
- Layer effects, blend modes, opacity (Phase 5)
- 9-slice, smart objects, variant switchers, improved anchor heuristics (Phase 6)
- Editor preview dialog, reimport, CommonUI (Phase 7)
- Mac platform support

</domain>

<decisions>
## Implementation Decisions

### Mapper Architecture
- **D-01:** `IPsdLayerMapper` and `FLayerMappingRegistry` are **internal-only**. No public `Register()` API in Phase 3. The registry holds a priority-ordered list of internal mappers; first match wins. External extensibility is deferred — add it in a future phase if actually needed.
- **D-02:** Mapper dispatch is prefix-based using `FPsdLayer.Name`. Layer names are checked against the full prefix table (Button_, Progress_, HBox_, VBox_, Overlay_, ScrollBox_, Slider_, CheckBox_, Input_, List_, Tile_, Switcher_). Prefix matching is case-sensitive (matches designer convention from Phase 2 context).

### Text Properties in Phase 3
- **D-03:** Phase 3 sets **content, DPI-converted size (× 0.75), and color** on `UTextBlock`. This covers `FPsdTextRun.Content`, `SizePx * 0.75f` → `FSlateFontInfo.Size`, and `FPsdTextRun.Color` → `SetColorAndOpacity`.
- **D-04:** Font mapping (Photoshop PostScript name → UE font asset), outline, shadow, bold/italic, and multi-line wrap are deferred entirely to Phase 4. Phase 3 uses `FSlateFontInfo` with the default engine font.

### Anchor Heuristics
- **D-05:** **Quadrant-based** anchor auto-assignment. Divide the canvas into a 3×3 grid. The layer's center point determines which zone it falls in → maps to the corresponding UE anchor preset (TopLeft, TopCenter, TopRight, CenterLeft, Center, CenterRight, BottomLeft, BottomCenter, BottomRight).
- **D-06:** Layers whose width ≥ 80% of canvas width → horizontal stretch anchor (`FAnchors(0,Y,1,Y)` where Y is the vertical zone midpoint). Layers whose height ≥ 80% of canvas height → vertical stretch. Both conditions → full stretch (`FAnchors(0,0,1,1)`).
- **D-07:** Anchor override suffixes take priority over heuristics: `_anchor-tl`, `_anchor-tc`, `_anchor-tr`, `_anchor-cl`, `_anchor-c`, `_anchor-cr`, `_anchor-bl`, `_anchor-bc`, `_anchor-br`, `_stretch-h`, `_stretch-v`, `_fill`. Suffix is stripped from the widget name after parsing.

### Partial Import Behavior
- **D-08:** **Skip + warn.** When a layer fails (empty pixel data, UE API error, unknown type) the layer is skipped, a `UE_LOG(LogPSD2UMG, Warning, ...)` is emitted naming the layer and reason, and the import continues. The resulting Widget Blueprint is created with the successfully mapped layers. No abort.
- **D-09:** Unknown layer types (not Image/Text/Group and no matching prefix) → log warning + skip. No placeholder widget inserted.

### Widget Blueprint Generation
- **D-10:** `FWidgetBlueprintGenerator` is called from inside `UPsdImportFactory::FactoryCreateBinary` after `FPsdParser::ParseFile` succeeds. The factory already wraps `UTextureFactory` for the flat texture; WBP generation runs after that wrapped call returns.
- **D-11:** Claude's discretion for the exact UE API sequence (which of `FWidgetBlueprintFactory`, `FKismetEditorUtilities`, or direct asset construction to use — whichever produces a compilable, openable WBP reliably).

### Texture Import
- **D-12:** Image layer pixels imported as `UTexture2D` using `Source.Init()` (not `PlatformData`) so textures survive editor restarts without re-import.
- **D-13:** Output path: `{TargetDir}/Textures/{PsdName}/{LayerName}`. Duplicate names deduped by appending `_01`, `_02`, etc.
- **D-14:** Layers with zero-size bounds or empty `RGBAPixels` → skip + warn (D-08 applies).

### Claude's Discretion
- Exact UE API sequence for Widget Blueprint creation (FWidgetBlueprintFactory vs FKismetEditorUtilities vs direct)
- Internal structure of `FLayerMappingRegistry` (vector + priority sort, or static dispatch table)
- Whether `FAnchorCalculator` is a standalone class or a free-function namespace
- How `UCanvasPanelSlot` offsets are calculated from `FIntRect` bounds relative to canvas origin
- Slot ZOrder assignment (linear index from layer order, or layer-defined)

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Project
- `CLAUDE.md` — UE 5.7.4 / C++20 / no Python / Editor-only / static-lib linkage / MIT
- `.planning/PROJECT.md` — Core value, requirements, out-of-scope rules
- `.planning/REQUIREMENTS.md` — MAP-01..12, WBP-01..06, TEX-01..03 acceptance criteria
- `.planning/ROADMAP.md` — Phase 3 goal and success criteria

### Phase 2 (foundation)
- `Source/PSD2UMG/Public/Parser/PsdTypes.h` — `FPsdDocument`, `FPsdLayer`, `FPsdTextRun`, `EPsdLayerType` (locked contracts)
- `Source/PSD2UMG/Public/Parser/PsdParser.h` — `FPsdParser::ParseFile()` signature
- `Source/PSD2UMG/Private/Factories/PsdImportFactory.cpp` — integration point; Phase 3 hooks WBP generation here
- `.planning/phases/02-c-psd-parser/02-CONTEXT.md` — Phase 2 decisions (struct shapes, factory wrapper pattern, Win64 only)

### Prior phase summaries
- `.planning/phases/02-c-psd-parser/02-05-SUMMARY.md` — What Phase 2 shipped; deviations

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `FPsdDocument` / `FPsdLayer` / `FPsdTextRun` — direct input to Phase 3 mapper; struct shapes are locked
- `UPsdImportFactory` (`Private/Factories/PsdImportFactory.cpp`) — integration point; Phase 3 adds WBP generation call after the wrapped `UTextureFactory` call
- `LogPSD2UMG` — existing log category; use for all Phase 3 warnings and errors
- `PSD2UMGSetting` — existing settings class; Phase 3 may read `TargetDir` from here for texture output path

### Established Patterns
- PIMPL inside parser (PhotoshopAPI hidden behind `PsdParserInternal.h`) — apply same insulation to any heavy UE WBP APIs in generator
- `TArray<uint8> RGBAPixels` with `PixelWidth`/`PixelHeight` — image layers are fully decoded; no pixel extraction needed in Phase 3
- `Source.Init()` for texture persistence — confirmed pattern from Phase 2 context

### Integration Points
- `UPsdImportFactory::FactoryCreateBinary` — add `FWidgetBlueprintGenerator::Generate(Doc)` call after parser runs
- `UPsdImportFactory` returns `UObject*` — currently returns the flat `UTexture2D`; after Phase 3 it should return the `UWidgetBlueprint` (or keep returning texture and create WBP as a side-effect asset — Claude's discretion)

</code_context>

<specifics>
## Specific Ideas

- The Python baseline (`Content/Python/auto_psd_ui.py`) already handles Canvas/Button/Image/Text/ProgressBar/ListView/TileView — review it for the complete prefix table and any edge cases before planning
- `UCanvasPanelSlot` uses `FAnchors` (normalized 0..1) + `FVector2D` for position and size offsets; position is relative to anchor point, not canvas origin — ensure coordinate math accounts for this
- Quadrant grid thresholds: divide canvas width and height into thirds. Zone center = layer `Bounds` center point.

</specifics>

<deferred>
## Deferred Ideas

- External mapper registration (third-party plugin custom mappers) — future phase if demand exists
- Minimal text properties only (raw size) — rejected; basic DPI + color is worth doing in Phase 3
- Abort-on-failure import behavior — rejected; skip+warn is always better UX for complex PSDs
- Mac platform support — out of scope for this milestone
- Message Log integration (vs Output Log only) — deferred to Phase 7 Editor UI phase

</deferred>

---

*Phase: 03-layer-mapping-widget-blueprint-generation*
*Context gathered: 2026-04-09 via /gsd:discuss-phase*
