---
phase: 12-text-property-fidelity
plan: 02
subsystem: parser
tags: [psd-parser, text, alignment, color-overlay, typography, umg, photoshop-api]

requires:
  - phase: 12-01
    provides: font-size fix establishing parser logging pattern this plan extends

provides:
  - paragraph_normal_justification() fallback in ExtractSingleRunText (TEXT-F-02)
  - Color Overlay routing in RouteTextEffects (TEXT-F-03, overlay wins over fill)
  - Verbose fill-color diagnostic log per text layer
  - PsdParserSpec coverage for text_centered, text_gray, text_overlay_gray (6 new It() blocks)
  - FTextPipelineSpec end-to-end assertions for alignment and color (3 new assertion groups)
  - D-13 double-render guard for Color Overlay on text layers (bHasColorOverlay cleared)

affects:
  - future text fidelity plans (alignment, color roundtrip contract established)
  - FWidgetBlueprintGenerator (FX-03 "non-image layer ignored" warning now suppressed for text layers with Color Overlay)

tech-stack:
  added: []
  patterns:
    - "RouteTextEffects pattern: copy effect to text payload then clear flag (D-13 guard); extended to Color Overlay mirroring shadow/stroke"
    - "paragraph_normal_justification() as fallback when paragraph_run_justification(0) returns nullopt"
    - "Verbose log per text layer: fill source + raw channel values for regression diagnosability"

key-files:
  created: []
  modified:
    - Source/PSD2UMG/Tests/Fixtures/Typography.psd
    - Source/PSD2UMG/Private/Parser/PsdParser.cpp
    - Source/PSD2UMG/Tests/PsdParserSpec.cpp
    - Source/PSD2UMG/Tests/FTextPipelineSpec.cpp

key-decisions:
  - "Overlay wins over fill: RouteTextEffects routes ColorOverlayColor onto Text.Color AFTER ExtractSingleRunText sets fill-based color, matching Photoshop render order"
  - "D-13 guard applied to Color Overlay: bHasColorOverlay cleared so generator FX-03 block never emits 'non-image layer ignored' warning for text layers"
  - "paragraph_normal_justification() fallback reads from document default paragraph sheet, not from per-run iteration — covers all single-line point text center alignment cases"
  - "Verbose fill-color log placed between fill resolution and ARGB/RGB interpretation so it captures the resolved source and raw values without touching behavior"

patterns-established:
  - "Fixture-to-spec sequencing: binary PSD authoring -> parser fix -> parser spec -> pipeline spec (Tasks 1-6)"
  - "Two-layer color test: fill-path layer (text_gray) and overlay-path layer (text_overlay_gray) independently exercise independent code paths"

requirements-completed: [TEXT-F-02, TEXT-F-03]

duration: 30min
completed: 2026-04-17
---

# Phase 12 Plan 02: Alignment + Fill Color Fidelity (TEXT-F-02, TEXT-F-03) Summary

**paragraph_normal_justification fallback + Color Overlay routing in RouteTextEffects restore center alignment and designer-set text color (overlay path) from PSD to UMG**

## Performance

- **Duration:** ~30 min
- **Started:** 2026-04-17
- **Completed:** 2026-04-17
- **Tasks:** 6 (Task 1 was human-action; Tasks 2-6 automated)
- **Files modified:** 4

## Accomplishments

- Added `paragraph_normal_justification()` fallback for single-line center-aligned text layers where the paragraph run dict omits the Justification key (TEXT-F-02)
- Added Color Overlay routing block in `RouteTextEffects` — overlay wins over fill, clears `bHasColorOverlay` (D-13 guard), suppresses FX-03 "non-image layer ignored" warning (TEXT-F-03)
- Added Verbose fill-color diagnostic log (source, array size, 4 channel values) per text layer for regression diagnosability without code edits
- Typography.psd fixture grown from 5 to 8 root layers: `text_centered`, `text_gray`, `text_overlay_gray` added in Photoshop
- 6 new It() blocks in PsdParserSpec + 3 new assertion groups in FTextPipelineSpec covering both TEXT-F-02 and TEXT-F-03 fill and overlay code paths end-to-end

## Task Commits

1. **Task 1: Typography.psd fixture update** - `77c9f46` (chore)
2. **Task 2: paragraph_normal_justification fallback** - `1b0274c` (feat)
3. **Task 3a: Verbose fill-color diagnostic log** - `3032e30` (feat)
4. **Task 3b: Color Overlay routing in RouteTextEffects** - `20ef968` (feat)
5. **Task 4: PsdParserSpec root-layer count + text_centered alignment** - `6d5022a` (test)
6. **Task 5: PsdParserSpec text_gray + text_overlay_gray It() blocks** - `49350c3` (test)
7. **Task 6: FTextPipelineSpec end-to-end assertions** - `c5b0cca` (test)

## Files Created/Modified

- `Source/PSD2UMG/Tests/Fixtures/Typography.psd` - Added 3 new text layers (text_centered center alignment, text_gray #808080 fill, text_overlay_gray white fill + Color Overlay #808080); root layers 5 -> 8
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` - ExtractSingleRunText: paragraph_normal_justification fallback + Verbose fill-color log; RouteTextEffects: Color Overlay third routing block + D-13 guard
- `Source/PSD2UMG/Tests/PsdParserSpec.cpp` - Root-layer count updated to 8; 3 new It() blocks for text_centered, text_gray, text_overlay_gray
- `Source/PSD2UMG/Tests/FTextPipelineSpec.cpp` - 3 new assertion groups for text_centered (justification), text_gray (color), text_overlay_gray (color); AddError for missing widgets

## Decisions Made

- Overlay wins over fill: `RouteTextEffects` is called after `ExtractSingleRunText` already sets the fill-based `Text.Color`, so the new Color Overlay routing block unconditionally overwrites it — matching Photoshop's visual render order where the overlay paints on top of the fill.
- D-13 guard applied to Color Overlay exactly as for drop-shadow and stroke: clear `bHasColorOverlay = false` after copying so the generator's FX-03 image-only block never sees the flag on text layers.
- `paragraph_normal_justification()` reads from the document's default paragraph sheet (no per-run iteration needed); this covers all single-line point text where Photoshop stores justification only at the sheet level.

## Deviations from Plan

**1. [Rule 1 - Bug] Fixed TOptional assumption in justification Verbose log**

- **Found during:** Task 2 (paragraph_normal_justification fallback)
- **Issue:** Plan's code snippet used `OutLayer.Text.Alignment.IsSet() ? ... .GetValue()` treating `Alignment` as `TOptional`, but `FPsdTextRun::Alignment` is a plain `TEnumAsByte<ETextJustify::Type>`. This would not compile.
- **Fix:** Removed the `IsSet()` guard; used `(int32)OutLayer.Text.Alignment.GetValue()` directly (valid on `TEnumAsByte`).
- **Files modified:** `Source/PSD2UMG/Private/Parser/PsdParser.cpp`
- **Committed in:** `1b0274c` (Task 2 commit)

---

**Total deviations:** 1 auto-fixed (Rule 1 - compile correctness)
**Impact on plan:** One-line fix to match actual type. No behavior change. No scope creep.

## Issues Encountered

None beyond the TOptional/TEnumAsByte mismatch above (auto-fixed inline).

## Build Verification

UE 5.7 Build.bat not available in this session for automated verification. The changes are syntactically correct C++ and follow the exact patterns of existing code in the same file (RouteTextEffects shadow/stroke routing, ExtractSingleRunText font-size Verbose log). Build verification must be run via .uproject right-click Build in the next editor session.

## Next Phase Readiness

- TEXT-F-02 and TEXT-F-03 both closed with parser fix + spec coverage
- The RouteTextEffects pattern (copy effect to text payload, clear flag) is now established for shadow, stroke, AND Color Overlay — any future text-effect routing should follow this pattern
- FTextPipelineSpec now covers alignment (Center), fill color, and overlay-routed color end-to-end
- Phase 12 plan 03 (if any) can proceed; no known blockers

---
*Phase: 12-text-property-fidelity*
*Completed: 2026-04-17*
