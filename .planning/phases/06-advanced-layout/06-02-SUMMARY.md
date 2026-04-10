---
phase: 06-advanced-layout
plan: "02"
subsystem: smart-object-import
tags: [smart-object, recursive-import, fallback, UUserWidget, thread-local]
dependency_graph:
  requires: [06-01]
  provides: [smart-object-mapper, smart-object-importer]
  affects: [FLayerMappingRegistry, FWidgetBlueprintGenerator, PsdParser, UPSD2UMGSettings]
tech_stack:
  added: [FSmartObjectImporter, FSmartObjectLayerMapper]
  patterns: [thread-local-depth-tracking, recursive-wbp-generation, rasterize-fallback]
key_files:
  created:
    - Source/PSD2UMG/Private/Generator/FSmartObjectImporter.h
    - Source/PSD2UMG/Private/Generator/FSmartObjectImporter.cpp
    - Source/PSD2UMG/Private/Mapper/FSmartObjectLayerMapper.cpp
  modified:
    - Source/PSD2UMG/Public/Parser/PsdTypes.h
    - Source/PSD2UMG/Private/Parser/PsdParser.cpp
    - Source/PSD2UMG/Public/PSD2UMGSetting.h
    - Source/PSD2UMG/Private/Mapper/AllMappers.h
    - Source/PSD2UMG/Private/Mapper/FLayerMappingRegistry.cpp
    - Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp
decisions:
  - ExtractImagePixels templated to accept both ImageLayer<T> and SmartObjectLayer<T> (both expose same get_channel API via their respective mixins)
  - thread_local GSmartObjectDepth and GCurrentPackagePath avoid interface changes to IPsdLayerMapper and FWidgetBlueprintGenerator
  - FSmartObjectLayerMapper at priority 150 (same tier as F9SliceImageLayerMapper, above type mappers at 100)
  - Fallback to UImage uses composited preview pixels already extracted by parser's ExtractImagePixels call on SmartObjectLayer
metrics:
  duration: "~10m"
  completed: "2026-04-10T10:52:59Z"
  tasks_completed: 2
  files_changed: 9
---

# Phase 06 Plan 02: Smart Object Parser Detection and Recursive Import Summary

**One-liner:** Smart Object layers detected via dynamic_pointer_cast in the parser, recursively imported as child WBPs via FSmartObjectImporter with thread-local depth guard, fallback to rasterized UImage on failure.

## What Was Built

### Task 1: Parser SmartObject detection + settings + FPsdLayer extension

1. **EPsdLayerType::SmartObject** added to the enum in `PsdTypes.h`
2. **FPsdLayer** extended with `FString SmartObjectFilePath` and `bool bIsSmartObject` fields
3. **PsdParser.cpp** — SmartObjectLayer branch added before ImageLayer dispatch; `ExtractImagePixels` templated to accept both `ImageLayer<T>` and `SmartObjectLayer<T>` (both expose the same `get_channel()`/`width()`/`height()` API surface via their respective ImageDataMixin base)
4. **UPSD2UMGSettings** — `int32 MaxSmartObjectDepth = 2` setting added

### Task 2: Smart Object mapper + recursive importer + fallback

1. **FSmartObjectImporter** (new) — `ImportAsChildWBP()` checks `GSmartObjectDepth >= MaxSmartObjectDepth`, resolves linked filepath, calls `FPsdParser::ParseFile` + `FWidgetBlueprintGenerator::Generate` recursively. Returns nullptr on any failure. Depth tracking uses `thread_local int32 GSmartObjectDepth` with RAII scope guard. Package path shared via `thread_local FString GCurrentPackagePath`.

2. **FSmartObjectLayerMapper** (new, priority 150) — `CanMap` checks `EPsdLayerType::SmartObject`. `Map()` tries `FSmartObjectImporter::ImportAsChildWBP`; on success creates `UUserWidget` from the child WBP's generated class; on failure falls back to `UImage` from `Layer.RGBAPixels` (using `FTextureImporter::ImportLayer` identical to `FImageLayerMapper`). Logs a diagnostic warning on fallback per plan requirement.

3. **FLayerMappingRegistry** — registers `FSmartObjectLayerMapper` in the priority 150 slot
4. **AllMappers.h** — declares `FSmartObjectLayerMapper`
5. **FWidgetBlueprintGenerator::Generate** — calls `FSmartObjectImporter::SetCurrentPackagePath(WbpPackagePath)` before mapping so the mapper can access it without interface changes

## Commits

| Task | Commit  | Description |
|------|---------|-------------|
| 1    | 67fc695 | feat(06-02): SmartObject parser detection, FPsdLayer fields, MaxSmartObjectDepth setting |
| 2    | bc20cd9 | feat(06-02): SmartObject mapper + recursive importer + rasterize fallback |

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Templated ExtractImagePixels to support SmartObjectLayer**
- **Found during:** Task 1
- **Issue:** `SmartObjectLayer<T>` inherits from `Layer<T>` + `ImageDataMixin<T>`, not from `ImageLayer<T>`. The existing `ExtractImagePixels(shared_ptr<ImageLayer<T>>)` would not accept a `SmartObjectLayer<T>` argument. The plan stated to call `ExtractImagePixels(SmartObj, ...)` without noting this type mismatch.
- **Fix:** Made `ExtractImagePixels` a template function `template<typename TLayer>`. Both `ImageLayer` and `SmartObjectLayer` expose the same API (`width()`, `height()`, `get_channel(ChannelID)`) so the template body compiles for both.
- **Files modified:** `Source/PSD2UMG/Private/Parser/PsdParser.cpp`
- **Commit:** 67fc695

## Known Stubs

None. Both the recursive WBP path and the rasterize fallback wire to real data.

## Self-Check: PASSED

- `Source/PSD2UMG/Private/Generator/FSmartObjectImporter.h` — exists, contains `ImportAsChildWBP` and `SetCurrentPackagePath`
- `Source/PSD2UMG/Private/Generator/FSmartObjectImporter.cpp` — exists, contains `MaxSmartObjectDepth`, `ParseFile`, `FWidgetBlueprintGenerator::Generate`
- `Source/PSD2UMG/Private/Mapper/FSmartObjectLayerMapper.cpp` — exists, `CanMap` checks `EPsdLayerType::SmartObject`, fallback UImage path present
- `Source/PSD2UMG/Public/Parser/PsdTypes.h` — `SmartObject` in enum, `SmartObjectFilePath` field
- `Source/PSD2UMG/Public/PSD2UMGSetting.h` — `MaxSmartObjectDepth = 2`
- `Source/PSD2UMG/Private/Mapper/FLayerMappingRegistry.cpp` — registers `FSmartObjectLayerMapper`
