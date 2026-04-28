---
phase: 21-parser-correctness-fixes
plan: "02"
subsystem: parser
tags: [parser, lrfx, color-space, hsb, hsv, color-overlay, drop-shadow, psd-effects]

# Dependency graph
requires:
  - phase: 21-01
    provides: [rtxt-01-null-sentinel-strip]
provides:
  - lfxc-01-colorspace-dispatch
  - ConvertLfx2Color-helper
  - sofi-hsb-conversion
  - dsdw-hsb-conversion
affects: [PsdParser, FTextEffectsSpec, FPsdParserEffectsSpec, 21-04-LFXC-02-UAT]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - ConvertLfx2Color shared helper in Parser::Internal namespace for ColorSpace dispatch
    - FLinearColor(H*360, S, V, A).HSVToLinearRGB() for Photoshop HSB lrFX channels
    - warn+identity pass-through for unsupported ColorSpace values (CMYK, Lab, etc.)

key-files:
  created: []
  modified:
    - Source/PSD2UMG/Private/Parser/PsdParser.cpp

key-decisions:
  - "ConvertLfx2Color shared helper chosen over inlined dispatch — both sofi and dsdw use identical conversion math; shared helper is cleaner per RESEARCH Pattern 2 and CONTEXT.md Claude's Discretion"
  - "RESEARCH Pitfall 1 confirmed: HSVToLinearRGB preserves alpha pass-through; FLinearColor(C0*360.f, C1, C2, A).HSVToLinearRGB() is correct — A does not corrupt the H/S/V conversion"
  - "RESEARCH Pitfall 5 confirmed: C0/C1/C2 arrive already divided by 65535.f; C0*360.f maps [0,1] to [0,360] degrees as expected by UE HSVToLinearRGB"
  - "warn+identity contract locked per D-05: CMYK and unknown ColorSpace values produce UE_LOG Warning plus FLinearColor(C0,C1,C2,A) — never crash, never zero out designer color"
  - "Build verification via Build.bat skipped (plugin-only repo, no host .uproject) — code correctness verified via acceptance-criteria grep checks (all pass)"

patterns-established:
  - "ConvertLfx2Color pattern: static helper in Parser::Internal namespace dispatches on Photoshop lrFX ColorSpace word; ColorSpace==0 RGB byte-identical, ColorSpace==1 HSB via HSVToLinearRGB, default warn+identity"

requirements-completed: [LFXC-01]

# Metrics
duration: ~10min
completed: "2026-04-28"
---

# Phase 21 Plan 02: lrFX ColorSpace Branch (LFXC-01) Summary

**`ConvertLfx2Color` helper added to `Parser::Internal` namespace; sofi and dsdw callsites routed through it — HSB lrFX colors now convert correctly via `FLinearColor::HSVToLinearRGB`, RGB path byte-identical, CMYK/unknown warn+identity.**

## Performance

- **Duration:** ~10 minutes
- **Started:** 2026-04-28T13:20:00Z
- **Completed:** 2026-04-28T13:28:59Z
- **Tasks:** 2
- **Files modified:** 1

## Accomplishments

- Added `ConvertLfx2Color(uint16 ColorSpace, float C0, float C1, float C2, float A, const FString& LayerName) -> FLinearColor` static helper immediately after `Utf8ToFString` inside `PSD2UMG::Parser::Internal` namespace
- ColorSpace==0 (RGB): returns `FLinearColor(C0, C1, C2, A)` — byte-identical to the pre-fix legacy path; no regression for existing fixtures
- ColorSpace==1 (HSB): returns `FLinearColor(C0 * 360.f, C1, C2, A).HSVToLinearRGB()` — Photoshop H is [0..65535]/65535 = [0,1], multiplied by 360 to degrees as required by UE `HSVToLinearRGB`; alpha preserved per UE Color.cpp:411
- ColorSpace==2+ (CMYK, Lab, unknown): emits `UE_LOG(LogPSD2UMG, Warning, ...)` naming the layer and the ColorSpace integer, then returns identity `FLinearColor(C0, C1, C2, A)` — no crash, no zero-out
- Routed `sofi` (Color Overlay) callsite through `ConvertLfx2Color` — single-line ternary expanded to 3-line form
- Routed `dsdw` (Drop Shadow) callsite through `ConvertLfx2Color` — identical change; `ShadowA` passed as alpha
- All Verbose UE_LOG diagnostic lines at sofi/dsdw parse sites preserved (debugging aids unchanged)
- Disabled-state fallbacks (`FLinearColor::White` for sofi, `FLinearColor(0,0,0,0)` for dsdw) unchanged

## Task Commits

Each task was committed atomically:

1. **Task 1: Add ConvertLfx2Color helper inside Parser::Internal namespace** - `47d200d` (feat)
2. **Task 2: Route sofi and dsdw callsites through ConvertLfx2Color — LFXC-01 GREEN** - `7d0da1f` (feat)

## Files Created/Modified

- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` — `ConvertLfx2Color` helper (+37 lines after `Utf8ToFString`); sofi ColorOverlayColor rewired (+2 lines); dsdw DropShadowColor rewired (+2 lines, -2 lines)

## Decisions Made

- Shared `ConvertLfx2Color` helper preferred over inlined dispatch per RESEARCH Pattern 2 and CONTEXT.md Claude's Discretion — both sofi and dsdw use identical HSB conversion math; a single helper prevents drift.
- Build.bat verification skipped because this repository is a plugin-only repo with no host `.uproject` — same pre-existing constraint documented in Phase 21-01-SUMMARY.md. Code correctness verified via acceptance-criteria grep checks (all 19 criteria across Tasks 1 and 2 passed).
- LFXC-02 human UAT (visual confirm on real UE 5.7 host) logged as standalone task in Plan 21-04 per D-08 — does NOT gate Phase 22 start.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

- Build verification via `Build.bat PSD2UMGEditor` could not be executed — plugin-only repo, no host `.uproject`. Pre-existing structural constraint identical to Plan 21-01 finding. All acceptance criteria verified via grep instead (19 grep checks: all passed).

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- LFXC-01 code change complete: `ConvertLfx2Color` helper live, both sofi and dsdw callsites routed through it
- Existing `Effects.psd` fixture (ColorSpace=0 RGB) will parse byte-identically — no regression
- HSB layers (ColorSpace=1) now produce correct hue via `HSVToLinearRGB` — ready for LFXC-02 visual UAT in Plan 21-04
- Phase 21 Plan 03 (FXFMT-01 VlLs branch) can proceed immediately — no dependency on this plan's changes
- LFXC-01 row in REQUIREMENTS.md is code-complete; full closure requires Plan 21-04 UAT sign-off per D-08

---
*Phase: 21-parser-correctness-fixes*
*Completed: 2026-04-28*

## Self-Check: PASSED

- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` modified: FOUND (47d200d, 7d0da1f)
- Commit 47d200d exists: FOUND
- Commit 7d0da1f exists: FOUND
- `grep -n "ConvertLfx2Color" PsdParser.cpp` returns 3 hits: PASSED
- sofi callsite wired: PASSED (line 779)
- dsdw callsite wired: PASSED (line 836)
- Legacy sofi path removed: PASSED (0 hits)
- Legacy dsdw path removed: PASSED (0 hits)
- FLinearColor::White preserved: PASSED (line 780)
- FLinearColor(0,0,0,0) preserved: PASSED (line 837)
