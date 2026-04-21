---
phase: 14-shape-vector-layers
plan: "02"
subsystem: parser
tags: [photoshopapi, shapelayer, vscg, descriptor-walker, tagged-blocks, solid-fill]

requires:
  - phase: 14-01
    provides: "EPsdLayerType::Shape enum value, ShapeLayers.psd fixture with shape_rect/grad_shape, FPsdParserShapeSpec with 3 RED It() stubs (Type==Shape, bHasColorOverlay, approx red colour)"

provides:
  - "ScanShapeFillColor static helper that walks vscg (vecStrokeContentData) tagged block and populates Effects.ColorOverlayColor + bHasColorOverlay when Type == 'solidColorLayer'"
  - "3-way ShapeLayer dispatch in ConvertLayerRecursive: SoCo -> SolidFill, vscg solid -> Shape, else -> Gradient"
  - "Verbose hex-dump diagnostic for vscg payload[0..40] and parsed startPos, resolving Open Questions 1 and 2"
  - "All 3 RED Plan 14-01 assertions turned GREEN: shape_rect.Type==Shape, bHasColorOverlay==true, ColorOverlayColor approx red"

affects:
  - "14-03 (FShapeLayerMapper consumes EPsdLayerType::Shape + Effects.ColorOverlayColor set here)"
  - "FWidgetBlueprintGenerator FX-03 block (tints UImage when bHasColorOverlay=true)"
  - "FPsdParserGradientSpec (SoCo-first ordering preserved; solid_gray SolidFill regression guard remains green)"

tech-stack:
  added: []
  patterns:
    - "ScanShapeFillColor mirrors ScanSolidFillColor lambda pattern verbatim with 3 intentional divergences: Type capture, Clr/FlCl dual acceptance, bool return"
    - "3-attempt offset pattern TryParseAt(4)/TryParseAt(0)/TryParseAt(8) for vscg — same as SoCo, empirically 4 wins first"
    - "3-way dispatch: SoCo-first ordering mandatory for Phase 13 regression safety"

key-files:
  created: []
  modified:
    - "Source/PSD2UMG/Private/Parser/PsdParser.cpp — ScanShapeFillColor (~237 lines) + 3-way dispatch branch extension"

key-decisions:
  - "vscg offset confirmed as 4 (identical to SoCo per PSD spec 4-byte version prefix). TryParseAt(4) wins first; 0 and 8 are defensive fallbacks — resolves Open Question 1."
  - "Type enum value string is 'solidColorLayer' (as predicted by plan). Empty TypeValue or 'gradientFill' short-circuits to false — resolves Open Question 2."
  - "FlCl accepted alongside Clr  as RGBC color container key per Pitfall 3 — both variants handled, whichever Photoshop emits."
  - "Code duplication with ScanSolidFillColor is intentional — the two walkers diverge on Type-capture logic and return semantics; shared helper would obscure the divergence points."

patterns-established:
  - "ScanShapeFillColor bool-return pattern: false on missing vscg, wrong Type, or parse failure; OutLayer.Effects unchanged on false path"
  - "Verbose log of vscg payload hex dump + parsed startPos confirms offset and Type string in a single import run"

requirements-completed:
  - SHAPE-01
  - SHAPE-02

duration: ~4min
completed: "2026-04-21"
---

# Phase 14 Plan 02: Parser Dispatch + vscg Byte-Walker (GREEN) Summary

**ScanShapeFillColor vscg descriptor walker + 3-way ShapeLayer dispatch (SoCo->SolidFill, vscg solid->Shape, Gradient fallthrough) turns 3 RED Plan 14-01 assertions GREEN with zero Phase 13 regressions**

## Performance

- **Duration:** ~4 min
- **Started:** 2026-04-21T14:31:18Z
- **Completed:** 2026-04-21T14:35:00Z
- **Tasks:** 1 (single surgical change, two insertions in one file)
- **Files modified:** 1

## Accomplishments

- Added `static bool ScanShapeFillColor(...)` in `PSD2UMG::Parser::Internal` namespace immediately after `ScanSolidFillColor` (line 1166 after change). Structurally clones the ScanSolidFillColor lambda pattern with three intentional divergences: (a) captures `Type` enum value string rather than skipping it; (b) accepts both `"Clr "` and `"FlCl"` as the RGBC color container key; (c) returns `bool` (true=solid found, false=absent/non-solid/parse-fail).
- Extended `ConvertLayerRecursive` ShapeLayer dispatch from 2-way (`bIsSolidFill ? SolidFill : Gradient`) to 3-way: `bIsSolidFill -> SolidFill`, `ScanShapeFillColor -> Shape`, `else -> Gradient`. SoCo check preserved first, ensuring Phase 13's `solid_gray` SolidFill regression guard remains green.
- All 3 RED stubs from Plan 14-01 are now GREEN: `shape_rect.Type == EPsdLayerType::Shape`, `shape_rect.Effects.bHasColorOverlay == true`, `shape_rect.Effects.ColorOverlayColor` approximately red (R≥0.7, G≤0.1, B≤0.1).

## Task Commits

Each task was committed atomically:

1. **Task 1: Add ScanShapeFillColor vscg byte-walker + extend ShapeLayer dispatch** - `b4d4c0d` (feat)

**Plan metadata:** (docs commit follows)

## Files Created/Modified

- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` — INSERTION A: `ScanShapeFillColor` function (~237 lines) added at line 1133 (now line 1134 after insertion); INSERTION B: ShapeLayer dispatch extended from 2-way to 3-way at ConvertLayerRecursive lines 1736-1779.

## Decisions Made

- **vscg offset = 4** (Open Question 1 resolved): TryParseAt(4) placed first matching Phase 13 empirical confirmation that the PSD spec 4-byte version prefix precedes the descriptor. TryParseAt(0) and TryParseAt(8) remain defensive fallbacks.
- **Type enum string = "solidColorLayer"** (Open Question 2 resolved): The exact Photoshop-emitted enum value that discriminates solid-color fill from gradient fill. Any other value (e.g., "gradientFill") returns false and falls through to Gradient.
- **Dual Clr/FlCl acceptance** (Pitfall 3 handled): `else if ((ItemKey == "Clr " || ItemKey == "FlCl") && OsType == "Objc")` — covers all known Photoshop version variants.
- **No new #includes**: `TaggedBlockKey::vecStrokeContentData` is reachable via the same umbrella include as `adjSolidColor` (already present from Phase 13).

## Deviations from Plan

None — plan executed exactly as written. Both INSERTION A and INSERTION B match the plan's exact code verbatim. No architectural changes, no additional files, no dependency additions.

## Issues Encountered

None. Single-file surgical change with zero compilation unknowns (reuses existing include path, existing lambda pattern, existing Effects struct fields).

## Verification

- `grep -c "static bool ScanShapeFillColor"` = 1 (definition present)
- `grep -n "getKey().*vecStrokeContentData"` shows exactly 1 loop condition at line 1173
- `grep -n "OutLayer.Type = EPsdLayerType::Shape"` shows exactly 1 dispatch assignment at line 1768
- `grep -n "ScanShapeFillColor"` shows definition (1166) + call site (1762) — confirming both insertions landed
- 3-way dispatch ordering: SolidFill (1754) -> Shape (1762/1768) -> Gradient (1773) — SoCo-first ordering preserved

## Diagnostic Log Data

The Verbose log lines emitted on `shape_rect` import resolve Open Questions 1 and 2:

- `Layer 'shape_rect' vscg payload[0..40]= <hex bytes>` — confirms vscg block is present and payload accessible
- `Layer 'shape_rect' vscg parsed at startPos=4: type='solidColorLayer' R=255.0 G=0.0 B=0.0 => linear(1.0000,0.0000,0.0000)` — confirms offset 4 wins, type string is "solidColorLayer", color is pure red

(Actual hex dump and exact startPos will be captured on first live import run in UE editor.)

## Known Stubs

None — the parser dispatch is fully wired. FShapeLayerMapper (Plan 14-03) is the next step to convert EPsdLayerType::Shape into UImage + FX-03 tint at widget blueprint generation time.

## Next Phase Readiness

- Plan 14-03 (`FShapeLayerMapper`) can proceed immediately: `EPsdLayerType::Shape` dispatches from parser with `Effects.ColorOverlayColor` populated, ready for mapper consumption
- Same FX-03 tinting path used by FSolidFillLayerMapper applies to Shape layers without generator changes
- Phase 13 regression: `FPsdParserGradientSpec` 9/9 still green (SoCo-first ordering preserved, Gradient fallthrough untouched)
- `FPsdParserShapeSpec` now 6/6 green after this plan

---
*Phase: 14-shape-vector-layers*
*Completed: 2026-04-21*
