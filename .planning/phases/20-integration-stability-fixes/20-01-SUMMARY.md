---
phase: 20-integration-stability-fixes
plan: "01"
subsystem: mapper-priority
tags: [mapper, priority, bug-fix, determinism, sort-race]
dependency_graph:
  requires: []
  provides: [mapper-priority-101-fill-shape]
  affects: [FLayerMappingRegistry, FFillLayerMapper, FSolidFillLayerMapper, FShapeLayerMapper]
tech_stack:
  added: []
  patterns: [priority-delta-over-sort-stability]
key_files:
  created: []
  modified:
    - Source/PSD2UMG/Private/Mapper/FFillLayerMapper.cpp
    - Source/PSD2UMG/Private/Mapper/FSolidFillLayerMapper.cpp
    - Source/PSD2UMG/Private/Mapper/FShapeLayerMapper.cpp
    - Source/PSD2UMG/Private/Mapper/FLayerMappingRegistry.cpp
    - Source/PSD2UMG/Tests/FButtonLayerMapperSpec.cpp
    - .planning/REQUIREMENTS.md
decisions:
  - "Priority delta (100→101) chosen over sort-stability workaround: deterministic, minimal-diff, and CI-verifiable via assertions"
  - "Assertions added to existing FButtonLayerMapperSpec.cpp (already includes AllMappers.h) to avoid new file/include overhead"
metrics:
  duration: "~5 minutes"
  completed: "2026-04-27T15:21:27Z"
  tasks_completed: 3
  files_modified: 6
---

# Phase 20 Plan 01: Mapper Priority Collision Fix Summary

**One-liner:** Priority bump (100 → 101) on FFillLayerMapper/FSolidFillLayerMapper/FShapeLayerMapper eliminates the FImageLayerMapper sort-race caused by Phase 16.1 D-02's EPsdTagType::Image aliasing.

## What Was Done

### Root Cause

Phase 16.1 D-02 made `FLayerTagParser` map `EPsdLayerType::Gradient`, `SolidFill`, and `Shape` to `EPsdTagType::Image`. This meant `FImageLayerMapper::CanMap` also returns `true` for these layers (it tests `ParsedTags.Type == EPsdTagType::Image`). With all four mappers returning `GetPriority() == 100`, `TArray::Sort`'s introsort is non-stable at equal keys — the wrong mapper could win non-deterministically.

### Fix (Task 1)

Three one-line changes: `GetPriority()` returns `101` instead of `100` in:
- `FFillLayerMapper.cpp` — gradient fill layers (`return 101`)
- `FSolidFillLayerMapper.cpp` — solid color fill layers (`return 101`)
- `FShapeLayerMapper.cpp` — drawn vector shape layers (`return 101`)

`FImageLayerMapper.cpp` baseline unchanged at `return 100`.

`FLayerMappingRegistry.cpp` comment block updated to document the new `priority 101` for fill/shape mappers. The sort lambda (`return A->GetPriority() > B->GetPriority()`) is untouched — it already does the right thing with the delta in place.

**Evidence:**
- `grep -c "return 101" FFillLayerMapper.cpp` → 1
- `grep -c "return 101" FSolidFillLayerMapper.cpp` → 1
- `grep -c "return 101" FShapeLayerMapper.cpp` → 1
- `grep -c "priority 101" FLayerMappingRegistry.cpp` → 5
- `grep -c "return A->GetPriority() > B->GetPriority()" FLayerMappingRegistry.cpp` → 1

### Spec Assertions (Task 2)

Three `TestTrue` assertions added to `FButtonLayerMapperSpec.cpp` under a new `Describe("Phase 20: Mapper Priority Hardening")` block:

```cpp
TestTrue(TEXT("FFillLayerMapper().GetPriority() > FImageLayerMapper().GetPriority()"),
    FFillLayerMapper().GetPriority() > FImageLayerMapper().GetPriority());
TestTrue(TEXT("FSolidFillLayerMapper().GetPriority() > FImageLayerMapper().GetPriority()"),
    FSolidFillLayerMapper().GetPriority() > FImageLayerMapper().GetPriority());
TestTrue(TEXT("FShapeLayerMapper().GetPriority() > FImageLayerMapper().GetPriority()"),
    FShapeLayerMapper().GetPriority() > FImageLayerMapper().GetPriority());
```

No new `#include` directives needed — `AllMappers.h` already included at line 9. Existing BTN-STATE-01 and BTN-STATE-02 Describe blocks preserved verbatim.

**Evidence:** `grep -c "Phase 20: Mapper Priority Hardening" FButtonLayerMapperSpec.cpp` → 1

### REQUIREMENTS.md Annotations (Task 3)

- **GRAD-01** traceability row updated to `Phase 13 / Phase 16.1 / Phase 20` with note: `Phase 20 hardened with priority 101 on FFillLayerMapper to eliminate sort-race vs FImageLayerMapper at priority 100`
- **SHAPE-01** traceability row updated to `Phase 14 / Phase 16.1 / Phase 20` with note: `Phase 20 hardened with priority 101 on FShapeLayerMapper / FSolidFillLayerMapper to eliminate sort-race vs FImageLayerMapper at priority 100`
- Footer updated: `*Last updated: 2026-04-27 (Phase 20 mapper priority hardening)*`
- `[x]` checkbox status preserved for GRAD-01, GRAD-02, SHAPE-01, SHAPE-02

**Evidence:**
- `grep -c "Phase 20 hardened with priority 101 on FFillLayerMapper" REQUIREMENTS.md` → 1
- `grep -c "Phase 20 hardened with priority 101 on FShapeLayerMapper / FSolidFillLayerMapper" REQUIREMENTS.md` → 1

## Commits

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Bump three mapper priorities to 101 + update registry comment | 72bd904 | FFillLayerMapper.cpp, FSolidFillLayerMapper.cpp, FShapeLayerMapper.cpp, FLayerMappingRegistry.cpp |
| 2 | Add three priority-hardening assertions in FButtonLayerMapperSpec | 9e3a6ab | FButtonLayerMapperSpec.cpp |
| 3 | Annotate REQUIREMENTS.md GRAD-01 / SHAPE-01 with Phase 20 hardening note | ac1eb57 | .planning/REQUIREMENTS.md |

## Deviations from Plan

None - plan executed exactly as written.

## Known Stubs

None.

## Self-Check: PASSED

- `Source/PSD2UMG/Private/Mapper/FFillLayerMapper.cpp` — FOUND (modified)
- `Source/PSD2UMG/Private/Mapper/FSolidFillLayerMapper.cpp` — FOUND (modified)
- `Source/PSD2UMG/Private/Mapper/FShapeLayerMapper.cpp` — FOUND (modified)
- `Source/PSD2UMG/Private/Mapper/FLayerMappingRegistry.cpp` — FOUND (modified)
- `Source/PSD2UMG/Tests/FButtonLayerMapperSpec.cpp` — FOUND (modified)
- `.planning/REQUIREMENTS.md` — FOUND (modified)
- Commits 72bd904, 9e3a6ab, ac1eb57 — FOUND in git log
