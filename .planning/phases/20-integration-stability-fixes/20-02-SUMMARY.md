---
phase: 20-integration-stability-fixes
plan: 02
subsystem: reimport
tags: [font-resolver, cache-invalidation, raii, scope-exit, reimport]

# Dependency graph
requires:
  - phase: 17-automated-font-matching
    provides: FFontResolver::InvalidateDiscoveryCache() and the D-04 unconditional-clear contract
  - phase: 17-automated-font-matching
    provides: PsdImportFactory.cpp:278 call site (the D-04 pattern this plan mirrors)

provides:
  - ON_SCOPE_EXIT RAII guard in FPsdReimportHandler::Reimport — fires InvalidateDiscoveryCache on all 7 exit paths
  - FONT-01 traceability row annotated with Phase 20 reimport hardening note

affects: [reimport-handler, font-resolver, requirements]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "RAII via ON_SCOPE_EXIT for unconditional teardown in multi-exit functions — mirrors D-04 unconditional cache-clear contract"

key-files:
  created: []
  modified:
    - Source/PSD2UMG/Private/Reimport/FPsdReimportHandler.cpp
    - .planning/REQUIREMENTS.md

key-decisions:
  - "D-03 mirror: ON_SCOPE_EXIT chosen over single-exit restructuring — one statement covers all 7 exit paths cleanly"
  - "Two new includes: Mapper/FontResolver.h (project group, after Generator/) and Misc/ScopeExit.h (engine group, after Misc/Paths.h)"
  - "PsdImportFactory.cpp:278 call site explicitly left unchanged — Plan 20-02 only adds the second call site"

patterns-established:
  - "ON_SCOPE_EXIT RAII pattern for multi-exit function teardown: insert at function top, fires on every return path"

requirements-completed:
  - FONT-01

# Metrics
duration: 5min
completed: 2026-04-27
---

# Phase 20 Plan 02: Cache Invalidation Reimport Hardening Summary

**ON_SCOPE_EXIT RAII guard added to FPsdReimportHandler::Reimport so FFontResolver auto-discovery cache is unconditionally cleared on all 7 exit paths, closing the REIMPORT_CACHE_LEAK integration gap**

## Performance

- **Duration:** ~5 min
- **Started:** 2026-04-27T15:20:00Z
- **Completed:** 2026-04-27T15:25:00Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Added `#include "Mapper/FontResolver.h"` and `#include "Misc/ScopeExit.h"` to FPsdReimportHandler.cpp include block (alphabetised into correct groups)
- Inserted ON_SCOPE_EXIT RAII guard at the top of `FPsdReimportHandler::Reimport` calling `PSD2UMG::FFontResolver::InvalidateDiscoveryCache()` — covers all 7 exit paths (4 failure returns, 1 cancel return, 1 success return) with a single statement
- Annotated FONT-01 traceability row in REQUIREMENTS.md with Phase 20 reimport hardening note; FONT-02 and all other rows unchanged

## Task Commits

Each task was committed atomically:

1. **Task 1: Add unconditional cache invalidation to FPsdReimportHandler::Reimport via ON_SCOPE_EXIT** - `89a5322` (feat)
2. **Task 2: Annotate REQUIREMENTS.md FONT-01 with Phase 20 reimport hardening note** - `616386e` (chore)

**Plan metadata:** (docs commit follows)

## Files Created/Modified

- `Source/PSD2UMG/Private/Reimport/FPsdReimportHandler.cpp` - Added 2 includes + ON_SCOPE_EXIT block at top of Reimport function
- `.planning/REQUIREMENTS.md` - FONT-01 traceability row updated with Phase 20 / D-03 annotation

## Decisions Made

- **RAII over single-exit restructuring (D-04 / Claude's Discretion):** ON_SCOPE_EXIT at function top is the cleanest single-statement placement covering all exit paths without restructuring existing return statements. Mirrors the D-04 pattern from PsdImportFactory.cpp:278.
- **PsdImportFactory.cpp untouched:** The existing call at line ~278 is explicitly verified unchanged (`grep -c` returns exactly 1). Plan 20-02 only adds the second call site in the reimport handler.

## Deviations from Plan

### Plan Acceptance Criterion Discrepancy (not a code deviation)

The plan's acceptance criterion stated `grep -c "EReimportResult::Failed"` should return exactly 4. The actual file has 5 `EReimportResult::Failed` returns (4 in the original interfaces listing plus one at line ~202 for the Update() failure path). No return statements were added or removed — the count of 5 is pre-existing and correct. The plan had an off-by-one in the expected count. All return statements are preserved exactly.

None — plan executed exactly as written. No production code changes beyond the two specified edits.

## Grep-able Evidence Anchors

- `PSD2UMG::FFontResolver::InvalidateDiscoveryCache()` — 1 occurrence in FPsdReimportHandler.cpp (new), 1 in PsdImportFactory.cpp (unchanged)
- `ON_SCOPE_EXIT` — 1 occurrence in FPsdReimportHandler.cpp
- `Phase 20 / D-03` — 1 occurrence in FPsdReimportHandler.cpp comment
- `FONT-01 | Phase 17 / Phase 20` — 1 occurrence in REQUIREMENTS.md

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Both Phase 20 integration stability fixes are complete (Plan 01: mapper priority bump, Plan 02: reimport cache invalidation)
- FPsdReimportHandler::Reimport now mirrors PsdImportFactory D-04 pattern — cache state never outlives a single PSD operation
- Font discovery cache is correctly invalidated on reimport, so new UFont assets added between import and reimport are picked up without engine restart

## Self-Check

- [x] `Source/PSD2UMG/Private/Reimport/FPsdReimportHandler.cpp` modified — confirmed (grep hits verified)
- [x] `.planning/REQUIREMENTS.md` modified — confirmed (FONT-01 row updated)
- [x] Task 1 commit `89a5322` — confirmed in git log
- [x] Task 2 commit `616386e` — confirmed in git log

## Self-Check: PASSED

---
*Phase: 20-integration-stability-fixes*
*Completed: 2026-04-27*
