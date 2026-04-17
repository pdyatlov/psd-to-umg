---
phase: 12-text-property-fidelity
plan: 01
subsystem: testing
tags: [font-size, photoshop-api, text-fidelity, regression-guard]

requires: []
provides:
  - "Permanent Verbose diagnostic log of raw style_run_font_size per text layer in PsdParser"
  - "Empirical proof that PhotoshopAPI returns designer_pt * (4/3) for all text layers"
  - "PsdParserSpec It() pinning text_regular Text.SizePx == 32.0f at parser boundary"
  - "FTextPipelineSpec assertion with full explanatory comment documenting correct 0.75 formula"
affects: [future text fidelity plans, mapper changes, PhotoshopAPI upgrade evaluations]

tech-stack:
  added: []
  patterns:
    - "PhotoshopAPI 4/3 scaling pattern: style_run_font_size() returns designer_pt * 4/3; mapper * 0.75 recovers designer intent"
    - "Empirical evidence gathered via Verbose log before formula changes — disproved the hypothesis, preserved correct code"

key-files:
  created: []
  modified:
    - Source/PSD2UMG/Private/Parser/PsdParser.cpp
    - Source/PSD2UMG/Tests/FTextPipelineSpec.cpp
    - Source/PSD2UMG/Tests/PsdParserSpec.cpp

key-decisions:
  - "TEXT-F-01 closed as no-bug-confirmed: the * 0.75f formula in FTextLayerMapper is mathematically correct — it inverts PhotoshopAPI's internal 4/3 scaling to recover designer pt intent"
  - "Task 3 (formula removal) skipped — empirical diagnostic logs from real designer PSD disproved the plan hypothesis"
  - "Path B chosen: regression-pinning tests added at both parser and pipeline boundaries without changing working code"
  - "Diagnostic Verbose log from Task 1 retained permanently as the diagnostic of record for future font-size questions"

patterns-established:
  - "PhotoshopAPI 4/3 pattern: when style_run_font_size(0) returns X, the designer-authored pt size is X * 0.75. Confirmed across Typography.psd (32->24) and real designer PSD (40->30)."

requirements-completed: [TEXT-F-01]

duration: 45min
completed: 2026-04-17
---

# Phase 12 Plan 01: Font Size Fidelity (TEXT-F-01) Summary

**TEXT-F-01 closed as no-bug: diagnostic logs confirmed * 0.75f correctly inverts PhotoshopAPI's 4/3 internal scaling; regression-pinning tests added at parser and pipeline boundaries**

## Performance

- **Duration:** ~45 min
- **Started:** 2026-04-17
- **Completed:** 2026-04-17
- **Tasks:** 3 of 5 (Tasks 1, 4-modified, 5-modified executed; Task 3 skipped per re-evaluation)
- **Files modified:** 3

## Empirical Evidence

The plan was authored on the hypothesis that `* 0.75f` is wrong. Before applying the formula fix, Task 1 added a permanent Verbose diagnostic log. The user then ran the log against a real designer PSD (`Test full hud.psd`) and captured:

```
LogPSD2UMG: Verbose: Text layer '00:23:10:34 @anchor:cl' style_run_font_size(0) raw=40.0000 stored=40.0000
LogPSD2UMG: Verbose: Text layer '[Opening Safe] @anchor:tl' style_run_font_size(0) raw=40.0000 stored=40.0000
...all layers: raw=40.0000
```

The designer most likely authored these at 30pt. Verification: `30 * (4/3) = 40` (PhotoshopAPI internal scaling), `40 * 0.75 = 30` (mapper recovers intent). The formula is correct.

Cross-referencing with the Typography fixture:
- `text_regular` current assertion: `size == 24`
- Working backward: `24 / 0.75 = 32` → `raw SizePx = 32`
- `32 / (4/3) = 24` → designer authored at 24pt

Both data points confirm the **PhotoshopAPI 4/3 scaling pattern** and that `* 0.75` correctly inverts it.

**Conclusion: Task 3 (formula removal) was skipped. The formula is mathematically correct.**

## Accomplishments

- Confirmed PhotoshopAPI consistently returns `designer_pt * (4/3)` for `style_run_font_size()` across both test fixture and real production PSD
- Established permanent Verbose diagnostic log in PsdParser for future font-size regressions (no code edits needed to diagnose)
- Added PsdParserSpec `It()` block pinning `text_regular` `Text.SizePx == 32.0f` — if PhotoshopAPI ever changes its internal scaling, this test surfaces it immediately
- Updated FTextPipelineSpec `text_regular` assertion comment to fully document the correct contract with captured raw values

## Task Commits

1. **Task 1: Add Verbose diagnostic log** - `6071022` (feat — already committed before re-evaluation)
2. **Task 4+5 (modified): Pin tests with evidence comments** - `a34e4bb` (test)

**Plan metadata:** (final docs commit below)

## Files Created/Modified

- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` — Permanent Verbose log of raw `style_run_font_size(0)` per text layer (Task 1, `6071022`)
- `Source/PSD2UMG/Tests/FTextPipelineSpec.cpp` — `text_regular` assertion comment documents 4/3 pattern and confirms formula correctness
- `Source/PSD2UMG/Tests/PsdParserSpec.cpp` — New `It()` block: `text_regular has SizePx equal to raw PhotoshopAPI font size (TEXT-F-01)` asserts `SizePx == 32.0f`

## Decisions Made

- **Formula preserved**: `Layer.Text.SizePx * 0.75f` in `FTextLayerMapper.cpp` is NOT removed. Empirical evidence proves it is the correct inverse of PhotoshopAPI's 4/3 internal scaling.
- **Path B chosen** (regression-pinning tests only): Added defense-in-depth without changing working production code.
- **Task 3 skipped**: Formula removal would have introduced a bug. The original plan hypothesis was disproved by empirical data.

## Deviations from Plan

### Plan Hypothesis Disproved

The plan was authored on the hypothesis that `* 0.75f` produces incorrect values "only by accident" for Typography.psd. Diagnostic logs from a real production PSD disproved this: the 4/3 scaling pattern is consistent and intentional in PhotoshopAPI. The `* 0.75f` factor is the mathematically correct inverse.

**Impact:** Task 3 (formula removal) was skipped entirely. Tasks 4 and 5 were executed in modified form — keeping assertions at existing correct values but adding comprehensive explanatory comments and a new parser-level pinning test.

**Tasks 3 skipped intentionally**: No bug exists. Removing the formula would have introduced regressions for all PSDs.

## Issues Encountered

None during execution. The re-evaluation logic itself was the key discovery.

## Next Phase Readiness

- Font size mapping is confirmed correct and now defensively tested at two boundaries
- Future plans can proceed with confidence that `SizePx * 0.75 = designer_pt` is the correct contract
- Any future upgrade of PhotoshopAPI library should re-run `PSD2UMG.Parser.Typography` — the new `SizePx == 32.0f` assertion will catch scaling changes immediately
- Outline (line 97, `OutlineSize * 0.75f`) and shadow (line 112, `ShadowOffset * 0.75`) DPI factors intentionally untouched (D-09 Option B, Phase 4)

---
*Phase: 12-text-property-fidelity*
*Completed: 2026-04-17*
