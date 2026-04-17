---
phase: 06-advanced-layout
plan: "01"
subsystem: layer-mapping
tags: [9-slice, variants, suffix-mapper, UWidgetSwitcher, ESlateBrushDrawType]
dependency_graph:
  requires: []
  provides: [9-slice-image-mapper, variants-suffix-mapper]
  affects: [FLayerMappingRegistry, FAnchorCalculator]
tech_stack:
  added: [F9SliceImageLayerMapper, FVariantsSuffixMapper]
  patterns: [suffix-mapper-pattern, priority-150-slot]
key_files:
  created:
    - Source/PSD2UMG/Private/Mapper/F9SliceImageLayerMapper.cpp
    - Source/PSD2UMG/Private/Mapper/FVariantsSuffixMapper.cpp
  modified:
    - Source/PSD2UMG/Private/Generator/FAnchorCalculator.cpp
    - Source/PSD2UMG/Private/Mapper/AllMappers.h
    - Source/PSD2UMG/Private/Mapper/FLayerMappingRegistry.cpp
decisions:
  - F9SliceImageLayerMapper priority 150 (between type mappers at 100 and prefix mappers at 200)
  - FVariantsSuffixMapper priority 200 (same tier as prefix mappers; EndsWith beats prefix mappers for _variants groups)
  - Default 9-slice margin 16px uniform normalized to texture dimensions
  - Bracket stripping in TryParseSuffix so [L,T,R,B] is removed from clean widget name
metrics:
  duration: "~10m"
  completed: "2026-04-10T10:47:57Z"
  tasks_completed: 2
  files_changed: 5
---

# Phase 06 Plan 01: 9-Slice and Variants Suffix Mappers Summary

**One-liner:** 9-slice image mapper (Box draw mode, [L,T,R,B] margins) and _variants suffix mapper (UWidgetSwitcher) with suffix table and name stripping extended.

## What Was Built

Two new mapper classes and supporting infrastructure for designer-facing naming conventions:

1. **F9SliceImageLayerMapper** (priority 150) — Image layers whose name contains `_9slice` or `_9s` (case-sensitive, `_9slice` checked first) produce a `UImage` with `ESlateBrushDrawType::Box`. Optional `[L,T,R,B]` pixel margin syntax in the layer name is parsed and normalized to 0–1 against texture dimensions. When margin syntax is absent, a 16 px uniform default is applied.

2. **FVariantsSuffixMapper** (priority 200) — Group layers whose name ends with `_variants` produce a `UWidgetSwitcher`. Children are added by the generator in PSD layer order (index 0 = first child = slot 0). Coexists with the existing `FSwitcherLayerMapper` (`Switcher_` prefix).

3. **FAnchorCalculator suffix table** — Three strip-only entries added (`_9slice`, `_9s`, `_variants`; bComputed=true, longest-first). A secondary pass in `TryParseSuffix` now also strips `[L,T,R,B]` bracket syntax from `OutCleanName`.

## Commits

| Task | Commit | Description |
|------|--------|-------------|
| 1 | 8517346 | feat(06-01): add F9SliceImageLayerMapper with Box draw mode and margin parsing |
| 2 | b2da595 | feat(06-01): add FVariantsSuffixMapper for _variants suffix groups |

## Deviations from Plan

None — plan executed exactly as written.

## Known Stubs

None. Both mappers wire data from real PSD layer properties.

## Self-Check: PASSED

- `Source/PSD2UMG/Private/Mapper/F9SliceImageLayerMapper.cpp` — created, contains `ESlateBrushDrawType::Box` and `FMargin`
- `Source/PSD2UMG/Private/Mapper/FVariantsSuffixMapper.cpp` — created, contains `UWidgetSwitcher` and `EndsWith(_variants)`
- `Source/PSD2UMG/Private/Generator/FAnchorCalculator.cpp` — contains `_9slice`, `_9s`, `_variants` suffix entries and bracket stripping
- `Source/PSD2UMG/Private/Mapper/AllMappers.h` — declares both new mapper classes
- `Source/PSD2UMG/Private/Mapper/FLayerMappingRegistry.cpp` — registers both new mappers
