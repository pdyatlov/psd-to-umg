---
phase: 05-layer-effects-blend-modes
plan: "01"
subsystem: parser
tags: [effects, lrFX, color-overlay, drop-shadow, complex-effects, settings]
dependency_graph:
  requires: []
  provides: [FPsdLayerEffects, lrFX-parsing, bFlattenComplexEffects]
  affects: [PsdParser, PsdTypes, PSD2UMGSetting]
tech_stack:
  added: []
  patterns: [lrFX-binary-parsing, vendored-header-extension]
key_files:
  created: []
  modified:
    - Source/PSD2UMG/Public/Parser/PsdTypes.h
    - Source/PSD2UMG/Public/PSD2UMGSetting.h
    - Source/PSD2UMG/Private/PSD2UMGSetting.cpp
    - Source/PSD2UMG/Private/Parser/PsdParser.cpp
    - Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/LayeredFile/LayerTypes/Layer.h
decisions:
  - "Added public unparsed_tagged_blocks() to vendored Layer.h (minimal non-breaking addition) since m_UnparsedBlocks is protected with no public accessor"
  - "lrFX color channels parsed as R,G,B with D-16 Verbose logging for ARGB verification at test time"
  - "Complex effects (isdw, bevl) set bHasComplexEffects=true; ImageLayer RGBAPixels populated by existing extraction path"
metrics:
  duration: 4m
  completed: "2026-04-10T10:08:46Z"
  tasks: 2
  files: 5
---

# Phase 05 Plan 01: Layer Effects Parser Extension Summary

FPsdLayerEffects struct added with color overlay, drop shadow, and complex-effect presence; lrFX tagged block binary parser extracts all three into FPsdLayer.Effects on every layer; bFlattenComplexEffects setting wired into Project Settings.

## Tasks Completed

| # | Name | Commit | Key Files |
|---|------|--------|-----------|
| 1 | Add FPsdLayerEffects struct and bFlattenComplexEffects setting | a86e147 | PsdTypes.h, PSD2UMGSetting.h, PSD2UMGSetting.cpp |
| 2 | Add public accessor to vendored Layer.h and parse lrFX | bb5b870 | Layer.h, PsdParser.cpp |

## What Was Built

- `FPsdLayerEffects` USTRUCT with 6 fields: `bHasColorOverlay`, `ColorOverlayColor`, `bHasDropShadow`, `DropShadowColor`, `DropShadowOffset`, `bHasComplexEffects`
- `Effects` member added to `FPsdLayer`
- `ExtractLayerEffects()` helper in PsdParser.cpp parses the lrFX binary block
  - `sofi`: color overlay color (R,G,B,A from uint16[4] + opacity byte)
  - `dsdw`: drop shadow color + angle/distance-derived offset vector
  - `isdw`/`bevl`: sets `bHasComplexEffects = true`
  - All wrapped in try/catch, gracefully skips missing blocks
- `unparsed_tagged_blocks()` public accessor added to vendored `Layer<T>` in Layer.h
- `bFlattenComplexEffects` UPROPERTY (config, EditAnywhere, default=true) on `UPSD2UMGSettings`
- Verbose logs for D-16 ARGB channel order verification

## Deviations from Plan

None - plan executed exactly as written.

## Known Stubs

None. FPsdLayerEffects is fully structured; Plan 02 (generator) is the consumer.

## Self-Check: PASSED

- Source/PSD2UMG/Public/Parser/PsdTypes.h — modified (FPsdLayerEffects struct added)
- Source/PSD2UMG/Public/PSD2UMGSetting.h — modified (bFlattenComplexEffects added)
- Source/PSD2UMG/Private/Parser/PsdParser.cpp — modified (ExtractLayerEffects added, called from ConvertLayerRecursive)
- Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/LayeredFile/LayerTypes/Layer.h — modified (unparsed_tagged_blocks accessor added)
- Commits a86e147 and bb5b870 both present
