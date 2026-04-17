# Phase 6: Advanced Layout - Context

**Gathered:** 2026-04-10
**Status:** Ready for planning

<domain>
## Phase Boundary

Production layout features: 9-slice borders with margin syntax, improved anchor heuristics that auto-detect rows/columns, Smart Object recursive import as child Widget Blueprints, and `_variants` suffix for UWidgetSwitcher.

**In scope:**
- LAYOUT-01: `_9s` / `_9slice` suffix → UImage with Box draw mode
- LAYOUT-02: Margin syntax `MyButton_9s[16,16,16,16]` sets margins; default 16px uniform when omitted
- LAYOUT-03: Improved anchor heuristics: auto-detect horizontal rows → HBox, vertical stacks → VBox
- LAYOUT-04: Smart Object layers → recursive import as child Widget Blueprints (UUserWidget reference)
- LAYOUT-05: Smart Object fallback: rasterize as image if extraction fails
- LAYOUT-06: `_variants` suffix groups → UWidgetSwitcher with one child per variant slot

**Out of scope:**
- Material-based effects or blend modes
- New widget type mappers beyond what's listed
- CommonUI integration (Phase 7)

</domain>

<decisions>
## Implementation Decisions

### 9-Slice (LAYOUT-01, LAYOUT-02)
- **D-01:** Any image layer with `_9s` or `_9slice` suffix gets Box draw mode on its UImage widget. Not limited to standalone images — works on button state brushes and any other image layer.
- **D-02:** Margin syntax `[L,T,R,B]` in the layer name sets margins explicitly. When no margin syntax is present, apply a uniform default of 16px on all sides.
- **D-03:** The `_9s`/`_9slice` suffix AND any `[L,T,R,B]` margin syntax are **stripped from the widget name**. E.g., `MyButton_9s[16,16,16,16]` → widget named `MyButton`. Consistent with how anchor suffixes are stripped.

### Smart Object Import (LAYOUT-04, LAYOUT-05)
- **D-04:** Smart Object layers are imported as separate Widget Blueprint assets, referenced in the parent via UUserWidget. This preserves the "reusable component" benefit.
- **D-05:** Recursion depth is **configurable** via a new setting `MaxSmartObjectDepth` on `UPSD2UMGSettings`, with a **default value of 2**. At depth limit, Smart Objects rasterize as images.
- **D-06:** Both embedded and linked Smart Objects are attempted. For linked Smart Objects, resolve the linked file from the PSD's directory. If the linked file is not found, fall back to rasterized image.
- **D-07:** Extraction failure (corrupt data, unsupported format, etc.) always falls back to rasterized image with a diagnostic warning — same best-effort pattern as Phase 4/5.
- **D-08:** New `EPsdLayerType::SmartObject` value needed in the parser to distinguish Smart Object layers from regular image/group layers.

### Anchor Heuristics Improvement (LAYOUT-03)
- **D-09:** When children of a CanvasPanel are clearly aligned in a horizontal row or vertical column (within tolerance), **automatically wrap them in HBox/VBox** instead of individual CanvasPanel slots.
- **D-10:** Alignment tolerance for row/column detection: **4-8px** (moderate — not too strict, not too relaxed). Exact value at Claude's discretion within this range.
- **D-11:** This is an enhancement to the existing `FAnchorCalculator` / generator logic, not a new mapper. The heuristic runs on groups that would otherwise produce a CanvasPanel.

### Spacing Detection (LAYOUT-03)
- **D-12:** Whether to detect equal spacing between items and auto-apply padding on HBox/VBox is at **Claude's discretion**. Implement if straightforward; skip if it adds significant complexity.

### Variants & Switcher (LAYOUT-06)
- **D-13:** `_variants` is a **suffix** on any group (e.g., `States_variants`). It produces UWidgetSwitcher, same as the existing `Switcher_` prefix mapper. Both coexist — the suffix is a new recognition path, the prefix remains for backward compatibility.
- **D-14:** Variant children (slots) are ordered by **PSD layer order** (top to bottom = slot 0, 1, 2...). Predictable, matches designer's layer panel.
- **D-15:** The `_variants` suffix is **stripped from the widget name**. `States_variants` → widget named `States`. Consistent with D-03.

### Claude's Discretion
- How to implement 9-slice margin parsing (regex, manual parse, etc.)
- Whether SmartObject child WBP naming uses the smart object layer name or the linked file name
- Exact alignment tolerance value within 4-8px range
- Whether to implement spacing/padding detection for auto HBox/VBox
- How to implement the `_variants` suffix mapper (new mapper class vs extending existing Switcher mapper)
- Whether Smart Object pixel extraction uses PhotoshopAPI's composited preview or the linked file data

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Project
- `CLAUDE.md` — UE 5.7.4 / C++20 / no Python / Editor-only / Win64 only
- `.planning/PROJECT.md` — Core value, requirements, key decisions
- `.planning/REQUIREMENTS.md` — LAYOUT-01..LAYOUT-06 acceptance criteria
- `.planning/ROADMAP.md` — Phase 6 goal and success criteria

### Parser Foundation (Phase 2)
- `Source/PSD2UMG/Public/Parser/PsdTypes.h` — `EPsdLayerType` enum (needs SmartObject), `FPsdLayer`, `FPsdDocument`
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` — layer extraction, type dispatch (line ~494: Unknown, ~510: Group, ~523: Image, ~530: Text)
- `Source/PSD2UMG/Private/Parser/PsdParserInternal.h` — PhotoshopAPI PIMPL boundary

### Mapper Infrastructure (Phase 3)
- `Source/PSD2UMG/Private/Mapper/AllMappers.h` — all mapper class declarations (FHBoxLayerMapper, FVBoxLayerMapper, FSwitcherLayerMapper already exist)
- `Source/PSD2UMG/Private/Mapper/FSimplePrefixMappers.cpp` — existing HBox_, VBox_, Switcher_ prefix mappers
- `Source/PSD2UMG/Private/Mapper/FLayerMappingRegistry.cpp` — mapper registration

### Generator & Anchors (Phase 3)
- `Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp` — widget tree building, where heuristic HBox/VBox auto-wrap would be added
- `Source/PSD2UMG/Private/Generator/FAnchorCalculator.cpp` — suffix parsing, position-based anchor logic

### Settings
- `Source/PSD2UMG/Public/PSD2UMGSetting.h` — `UPSD2UMGSettings` (add MaxSmartObjectDepth here)

### PhotoshopAPI Smart Objects
- `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/LayeredFile/LayerTypes/SmartObjectLayer.h` — SmartObjectLayer<T> API
- `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/LayeredFile/LinkedData/LinkedLayerData.h` — linked layer data, hash-based dedup

### Prior Phase Context
- `.planning/phases/03-layer-mapping-widget-blueprint-generation/03-CONTEXT.md` — mapper pattern, anchor calculator design
- `.planning/phases/05-layer-effects-blend-modes/05-CONTEXT.md` — best-effort delivery pattern, flatten fallback pattern

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `FLayerMappingRegistry` — pluggable mapper registration; new 9-slice and _variants mappers slot in naturally
- `FSwitcherLayerMapper` — existing Switcher_ prefix mapper producing UWidgetSwitcher; _variants suffix can reuse similar logic
- `FHBoxLayerMapper` / `FVBoxLayerMapper` — already handle HBox_/VBox_ prefixes; heuristic detection creates same widget types
- `FAnchorCalculator` — suffix stripping logic already implemented; extend for _9s/_9slice/_variants suffixes
- `FTextureImporter` — RGBA → UTexture2D pipeline; reusable for Smart Object rasterize fallback
- `FPsdLayerEffects` / flatten fallback — Phase 5 pattern for rasterizing complex layers as images

### Established Patterns
- PIMPL boundary: all PhotoshopAPI calls inside `Private/Parser/` files
- Suffix stripping: anchor suffixes (_anchor-tl, etc.) are stripped from widget names in FAnchorCalculator
- Priority-based mapper dispatch: higher priority mappers checked first
- Best-effort delivery: missing/unsupported data → no-op with warning diagnostic
- AllMappers.h private header: all mapper declarations in one internal header

### Integration Points
- `PsdParser.cpp` — add SmartObject layer type detection (PhotoshopAPI `SmartObjectLayer<T>` dynamic_pointer_cast)
- `FWidgetBlueprintGenerator::BuildWidgetTree` — add row/column detection heuristic before creating CanvasPanel slots
- `UPSD2UMGSettings` — add `MaxSmartObjectDepth` setting
- `FLayerMappingRegistry` — register new 9-slice mapper and _variants suffix mapper
- `FAnchorCalculator` — extend suffix table for _9s/_9slice/_variants stripping

</code_context>

<specifics>
## Specific Ideas

- 9-slice suffix stripping should use the same mechanism as anchor suffix stripping in FAnchorCalculator — extend the suffix table.
- Smart Object depth tracking: pass a `CurrentDepth` parameter through the recursive import call chain; compare against `MaxSmartObjectDepth` setting.
- For row/column detection: sort children by Y (for rows) or X (for columns), check if all Y values (or X values) are within tolerance. If children form a clear row, wrap in HBox; if column, wrap in VBox.
- The `_variants` mapper needs higher priority than the default group→CanvasPanel mapper so the suffix is recognized before the group fallback.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 06-advanced-layout*
*Context gathered: 2026-04-10 via /gsd:discuss-phase*
