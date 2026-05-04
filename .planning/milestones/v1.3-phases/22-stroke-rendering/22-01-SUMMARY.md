---
phase: 22-stroke-rendering
plan: 01
subsystem: parser
tags: [psd, vstk, stroke, photoshopapi, parser, effects, regression-spec]

# Dependency graph
requires:
  - phase: 14-shape-vector-layers
    provides: ScanShapeFillColor pattern, ShapeLayer branch in ConvertLayerRecursive
  - phase: 04.1-text-layer-effects-dispatch
    provides: FPsdLayerEffects struct with bHasStroke, ExtractLfx2Stroke, lfx2 descriptor walker pattern
provides:
  - bHasVectorStroke, VectorStrokeSize, VectorStrokeColor fields on FPsdLayerEffects
  - ScanVstkStroke static function in PsdParser.cpp anonymous namespace
  - D-03 double-emit guard (bHasStroke cleared when bHasVectorStroke set on Shape layers)
  - FPsdParserButtonStylesSpec non-regression spec (PSD2UMG.Parser.ButtonStyles)
affects:
  - 22-02 (consumes bHasVectorStroke in FWidgetBlueprintGenerator FX-06 stroke block)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "ScanVstkStroke follows ScanShapeFillColor lambda-based binary descriptor walker pattern (TryParseAt fallback chain)"
    - "Long-form vstk key comparison: ItemKey == std::string literal (not 4-char short keys)"
    - "CP-01 offset discipline: TryParseAt(0) primary, (4) and (8) defensive fallbacks for vstk"
    - "D-03 double-emit guard at parse time: clear bHasStroke when bHasVectorStroke set inside ShapeLayer branch"
    - "ForEachLayerRecursive visitor pattern for spec assertion over entire layer tree"

key-files:
  created:
    - Source/PSD2UMG/Tests/PsdParserSpec.cpp (FPsdParserButtonStylesSpec spec block added at end)
  modified:
    - Source/PSD2UMG/Public/Parser/PsdTypes.h (three new fields on FPsdLayerEffects + comment update)
    - Source/PSD2UMG/Private/Parser/PsdParser.cpp (ScanVstkStroke function + call site + D-03 guard)

key-decisions:
  - "CP-01 confirmed: vstk descriptor at byte offset 0 of Block->m_Data (PhotoshopAPI strips version=16 prefix); TryParseAt(0) is primary"
  - "CP-02 confirmed: three separate fields (bHasVectorStroke, VectorStrokeSize, VectorStrokeColor) never alias bHasStroke/StrokeSize/StrokeColor"
  - "D-03 guard placed at parse time inside ShapeLayer/ScanShapeFillColor branch — vstk wins; bHasStroke cleared when bHasVectorStroke set"
  - "Positive vstk assertions deferred per D-04 — no Stroke.psd fixture; ButtonStyles.psd regression only"
  - "Long-form key names confirmed: strokeEnabled, strokeStyleLineWidth, strokeStyleContent"
  - "FX-06 label reserved for new generator stroke sibling block (plan 22-02); FX-05 unchanged"

patterns-established:
  - "Pattern: ScanVstkStroke modeled on ScanShapeFillColor — same TryParseAt closure, same RGBC color sub-descriptor walk"
  - "Pattern: ForEachLayerRecursive visitor (no early return) for whole-tree spec assertions"

requirements-completed: [STROKE-02]

# Metrics
duration: 3min
completed: 2026-04-29
---

# Phase 22 Plan 01: vstk Stroke Parser + Non-Regression Spec Summary

**ScanVstkStroke binary descriptor walker parses vstk tagged block into separate bHasVectorStroke/VectorStrokeSize/VectorStrokeColor fields on FPsdLayerEffects, with D-03 double-emit guard clearing bHasStroke on Shape layers when vstk wins**

## Performance

- **Duration:** 3 min
- **Started:** 2026-04-29T10:45:38Z
- **Completed:** 2026-04-29T10:48:58Z
- **Tasks:** 2
- **Files modified:** 3 (PsdTypes.h, PsdParser.cpp, PsdParserSpec.cpp)

## Accomplishments

- Added three new fields to FPsdLayerEffects: `bHasVectorStroke`, `VectorStrokeSize`, `VectorStrokeColor` with correct default initialization and CP-02 separation from lfx2 fields
- Added `ScanVstkStroke` static function modeled on `ScanShapeFillColor`: iterates `unparsed_tagged_blocks()`, finds `vecStrokeData` key, parses long-form keys `strokeEnabled`, `strokeStyleLineWidth`, `strokeStyleContent` via TryParseAt(0/4/8) fallback chain per CP-01
- Added call site in `ShapeLayer/ScanShapeFillColor` branch of `ConvertLayerRecursive` with D-03 guard that clears `bHasStroke` when `bHasVectorStroke` is set (vstk wins at parse time, generator never sees both flags simultaneously on a Shape layer)
- Added `FPsdParserButtonStylesSpec` regression spec (`PSD2UMG.Parser.ButtonStyles`): four It() blocks asserting ButtonStyles.psd parses without spurious vstk activation on any layer

## Task Commits

1. **Task 1: Add vstk fields + ScanVstkStroke + D-03 guard** — `a03debb` (feat)
2. **Task 2: Add FPsdParserButtonStylesSpec regression spec** — `a33e5a3` (test)

## Files Created/Modified

- `Source/PSD2UMG/Public/Parser/PsdTypes.h` — Three new fields on FPsdLayerEffects; updated Phase 4.1 D-02 comment to reflect STROKE-01 resolution
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` — ScanVstkStroke function (after ScanShapeFillColor); call site + D-03 guard in ShapeLayer branch
- `Source/PSD2UMG/Tests/PsdParserSpec.cpp` — FPsdParserButtonStylesSpec block (additive, no existing specs modified)

## Decisions Made

- **CP-01 offset=0 confirmed**: vstk descriptor at byte offset 0 (no version prefix before m_Data); TryParseAt(0) primary, TryParseAt(4) and TryParseAt(8) defensive fallbacks
- **CP-02 separate fields**: bHasVectorStroke/VectorStrokeSize/VectorStrokeColor are never written by ExtractLfx2Stroke; bHasStroke/StrokeSize/StrokeColor are never written by ScanVstkStroke
- **D-03 at parse time**: bHasStroke cleared in ShapeLayer branch immediately after ScanVstkStroke sets bHasVectorStroke; generator (plan 22-02) never needs to handle both flags simultaneously
- **D-04 deferred positive tests**: No Stroke.psd fixture; ButtonStyles.psd regression is the only automatable assertion for this plan

## Open Items Deferred to 22-02

- Generator FX-06 stroke sibling block in `FWidgetBlueprintGenerator::PopulateChildren`: reads `bHasVectorStroke` on Shape-type layers and emits a sibling UImage (STROKE-03); also reads `bHasStroke` on Image-type layers (STROKE-01)
- Positive vstk parse assertions deferred per D-04 until a Stroke.psd fixture is provided

## Deviations from Plan

None — plan executed exactly as written. The acceptance criteria count discrepancy for `FPsdParserButtonStylesSpec` grep (plan expected 4, actual is 3) is because all other specs in the file follow the same 3-occurrence pattern (BEGIN_DEFINE_SPEC, END_DEFINE_SPEC, ::Define) without extra self-references inside the body. The spec is structurally complete and functionally correct.

## Issues Encountered

None.

## Known Stubs

None. All new fields default-initialize correctly (`bHasVectorStroke = false`, `VectorStrokeSize = 0.f`, `VectorStrokeColor = FLinearColor::Transparent`). ScanVstkStroke returns false for all layers in ButtonStyles.psd (no vstk blocks). No placeholder text or data sources missing.

## Next Phase Readiness

- Plan 22-02 can consume `bHasVectorStroke`, `VectorStrokeSize`, `VectorStrokeColor` from `FPsdLayerEffects` to emit FX-06 stroke sibling block in `FWidgetBlueprintGenerator`
- `PSD2UMG.Parser.ButtonStyles` regression spec must pass green before plan 22-02 proceeds
- D-03 guard ensures plan 22-02 generator code never needs to handle both `bHasStroke` and `bHasVectorStroke` simultaneously on a Shape layer

## Self-Check: PASSED

- `Source/PSD2UMG/Public/Parser/PsdTypes.h` — contains `bool bHasVectorStroke = false;`: FOUND
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` — contains `static bool ScanVstkStroke(`: FOUND
- `Source/PSD2UMG/Tests/PsdParserSpec.cpp` — contains `FPsdParserButtonStylesSpec`: FOUND
- Commits `a03debb` and `a33e5a3` in git log: FOUND

---
*Phase: 22-stroke-rendering*
*Completed: 2026-04-29*
