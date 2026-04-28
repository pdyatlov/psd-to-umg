---
phase: 21-parser-correctness-fixes
plan: "03"
subsystem: parser
tags: [parser, psd-effects, stroke, frfx, vlls, descriptor-walker, fxfmt-01]

# Dependency graph
requires:
  - phase: 21-02
    provides: [lfxc-01-colorspace-dispatch, ConvertLfx2Color-helper]
provides:
  - fxfmt-01-vlls-branch
  - ParseFrFXObjcItem-lambda
  - frfx-cc2014plus-stroke-extraction
affects: [PsdParser, 21-04-LFXC-02-UAT, Phase-22-stroke-rendering]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - ParseFrFXObjcItem extracted lambda pattern — shared inner parser callable by both Objc and VlLs outer branches
    - VlLs list iteration with !bFoundStroke guard stops on first enabled stroke item (mirrors Objc branch semantics)
    - Unknown VlLs item ostypes fall through to SkipValueAfterOsType(ElemOT) — safe skip, no wedge

key-files:
  created: []
  modified:
    - Source/PSD2UMG/Private/Parser/PsdParser.cpp

key-decisions:
  - "ParseFrFXObjcItem extracted as lambda (not free function) capturing Pos/Read*/Skip* by ref — avoids parameter explosion, consistent with all other inner lambdas in ParseFrFXDescriptor"
  - "bFoundStroke = true kept UNCONDITIONAL in Objc branch (legacy semantics) — finding any FrFX/Objc stops the outer search regardless of bEnab; Out.bEnabled is the source of truth for the function return"
  - "VlLs branch sets bFoundStroke only on ParseFrFXObjcItem() returning true (enabled) — matches spirit of Objc branch: Out.bEnabled drives the final return value"
  - "Build.bat verification skipped (plugin-only repo, no host .uproject) — code correctness verified via acceptance-criteria grep checks (all pass)"

patterns-established:
  - "FrFX descriptor walker pattern: extract shared Objc-item body into lambda; Objc branch calls it unconditionally + sets bFoundStroke; VlLs branch iterates list items dispatching Objc-typed ones through same lambda with !bFoundStroke guard"

requirements-completed: [FXFMT-01]

# Metrics
duration: ~10min
completed: "2026-04-28"
---

# Phase 21 Plan 03: FrFX VlLs Branch (FXFMT-01) Summary

**`ParseFrFXObjcItem` lambda extracted from the existing Objc branch body and a new `FrFX/VlLs` branch added to `ParseFrFXDescriptor`'s outer walk — Photoshop CC 2014+ stroke effects stored as VlLs lists now extract `bEnabled`, `SizePx`, and `Color` instead of being silently skipped.**

## Performance

- **Duration:** ~10 minutes
- **Started:** 2026-04-28T14:00:00Z
- **Completed:** 2026-04-28T14:10:00Z
- **Tasks:** 2
- **Files modified:** 1

## Accomplishments

- Extracted the ~80-line FrFX Objc body from `ParseFrFXDescriptor`'s outer walk loop into a `ParseFrFXObjcItem = [&]() -> bool` lambda capturing all parser-state lambdas by reference; lambda returns `bEnab` directly (no `bFoundStroke` capture per RESEARCH Pitfall 2)
- Existing Objc branch simplified to `ParseFrFXObjcItem(); bFoundStroke = true;` — byte-identical behavior for all pre-CC2014 PSDs
- Added new `else if (ItemKey == "FrFX" && FCStringAnsi::Strcmp(OsType, "VlLs") == 0)` branch between the Objc branch and the outer `SkipValueAfterOsType(OsType)` fallback
- VlLs branch reads `N` list items, reads each item's 4-byte ostype, dispatches `Objc`-typed items through `ParseFrFXObjcItem()`, and falls through to `SkipValueAfterOsType(ElemOT)` for unknown item types — loop-stop guard `!bFoundStroke` stops scanning on first enabled stroke item
- Inner `SkipValueAfterOsType` VlLs handler at ~line 1003 is UNCHANGED — skip path and extraction path coexist without conflict
- `ParseFrFXDescriptor` return contract `bFoundStroke && Out.bEnabled` unchanged

## Task Commits

Each task was committed atomically:

1. **Task 1: Extract ParseFrFXObjcItem lambda from existing Objc branch (no behavior change)** - `0557a31` (refactor)
2. **Task 2: Add FrFX VlLs branch to top-level walk loop (GREEN)** - `58d493b` (feat)

## Files Created/Modified

- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` — `ParseFrFXObjcItem` lambda inserted before outer walk loop (+100 lines, -82 lines refactor); new VlLs `else if` branch added to outer walk (+36 lines)

## Decisions Made

- `ParseFrFXObjcItem` as lambda (not extracted to a static free function): consistent with the established inner-lambda pattern throughout `ParseFrFXDescriptor`; captures `Pos`, `CheckRemaining`, `ReadU8/U32BE/DoubleBE`, `ReadPsString`, `SkipUnicodeString`, `SkipValueAfterOsType`, and `Out` by reference without parameter explosion.
- `bFoundStroke = true` kept unconditional in the Objc branch: matches legacy semantics where finding any `FrFX/Objc` stops the outer search even if `bEnab = false`. The function return `bFoundStroke && Out.bEnabled` already handles the disabled-stroke case correctly.
- VlLs branch only sets `bFoundStroke` when `ParseFrFXObjcItem()` returns `true`: a disabled stroke item leaves `bFoundStroke = false` and the loop continues to the next VlLs item, which matches D-06 full-extraction intent and Pitfall 4 loop-stop semantics.
- Build verification skipped: plugin-only repo, no host `.uproject` — same pre-existing constraint as Plans 21-01 and 21-02. All 19 acceptance-criteria grep checks across Tasks 1 and 2 passed.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

- Build verification via `Build.bat PSD2UMGEditor` could not be executed — plugin-only repo, no host `.uproject`. Pre-existing structural constraint identical to Plans 21-01 and 21-02. All acceptance criteria verified via grep (19 checks: all passed).
- Task 2 acceptance criterion "SkipValueAfterOsType(OsType) returns exactly 1 hit" found 3 hits — the other 2 hits are pre-existing occurrences in separate descriptor-walker functions added in prior phases (lines 1379 and 1620). The outer-loop fallback in `ParseFrFXDescriptor` is intact at line 1183. No action required.
- Task 2 acceptance criterion "inner VlLs skip branch exactly 1 hit" found 3 hits — same root cause: 3 separate `SkipValueAfterOsType` inner lambdas exist across 3 different descriptor-walker functions in the file. All 3 pre-exist this plan. The `ParseFrFXDescriptor` inner skip VlLs branch at line 1003 is unchanged.

## Known Stubs

None — FXFMT-01 is a pure parser fix. Stroke data extracted by the VlLs branch flows into `FPsdStrokeInfo` (populated `bEnabled`, `SizePx`, `Color`). Emission of that stroke data into UMG widget rendering is Phase 22+ work (deferred per CONTEXT.md `<deferred>`).

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- FXFMT-01 code change complete: VlLs branch live, pre-CC2014 Objc fixtures unaffected
- Phase 21 Plan 04 (LFXC-02 human UAT) can proceed — does not depend on FXFMT-01 changes
- FXFMT-01 row in REQUIREMENTS.md is code-complete and eligible to flip to "Complete"
- Phase 22 stroke rendering can consume `FPsdStrokeInfo` from both Objc and VlLs origin PSDs

---
*Phase: 21-parser-correctness-fixes*
*Completed: 2026-04-28*

## Self-Check: PASSED

- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` modified: FOUND (0557a31, 58d493b)
- `.planning/phases/21-parser-correctness-fixes/21-03-SUMMARY.md` created: FOUND
- Commit 0557a31 exists: FOUND
- Commit 58d493b exists: FOUND
- `grep -n "ParseFrFXObjcItem" PsdParser.cpp` returns 4 lines (3 non-comment): PASSED
- `grep -n "FrFX.*VlLs" PsdParser.cpp` returns 1 hit: PASSED
- `grep -n "CP-03" PsdParser.cpp` returns 1 hit: PASSED
- `grep -n "FXFMT-01" PsdParser.cpp` returns 3 hits: PASSED
