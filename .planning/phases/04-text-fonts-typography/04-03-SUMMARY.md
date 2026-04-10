---
phase: 04-text-fonts-typography
plan: 03
subsystem: text-mapper, widget-generator
tags: [typography, outline, autowrap, text-pipeline, e2e-test]
dependency_graph:
  requires: [04-02]
  provides: [TEXT-01-verified, TEXT-03, TEXT-04-documented, TEXT-06]
  affects: [FTextLayerMapper, FWidgetBlueprintGenerator]
tech_stack:
  added: []
  patterns: [FSlateFontInfo::OutlineSettings, UTextBlock::SetAutoWrapText, BoxWidthPx-slot-sizing]
key_files:
  created:
    - Source/PSD2UMG/Tests/FTextPipelineSpec.cpp
  modified:
    - Source/PSD2UMG/Private/Mapper/FTextLayerMapper.cpp
    - Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp
decisions:
  - "TEXT-04 drop shadow explicitly deferred — PhotoshopAPI v0.9 has no layer effect API"
  - "Outline uses same 0.75 DPI conversion as font size"
  - "BoxWidthPx overrides slot width only for non-stretched text layers"
metrics:
  duration: 8m
  completed: 2026-04-09
  tasks: 3
  files: 3
---

# Phase 4 Plan 03: Outline, Wrap & E2E Pipeline Spec Summary

Text outline applied via FSlateFontInfo::OutlineSettings with 0.75 DPI scaling, paragraph wrap gated on bHasExplicitWidth with BoxWidthPx slot sizing, TEXT-04 drop shadow explicitly deferred with code comment, E2E spec validating full Typography.psd pipeline.

## Requirement Closure

| Req | Status | Detail |
|-----|--------|--------|
| TEXT-01 | Verified preserved | `SizePx * 0.75f` line intact from Phase 3; E2E spec asserts size==18 for 24px source |
| TEXT-02 | Delivered (Plan 02) | FFontResolver + TypefaceFontName for bold/italic |
| TEXT-03 | Delivered | OutlineSettings.OutlineSize + OutlineColor from parser fields, 0.75 DPI conversion |
| TEXT-04 | NOT delivered | PhotoshopAPI v0.9 lacks drop-shadow accessors (no lfx2/effect API); code comment documents gap; entry point for Phase 4.1 |
| TEXT-05 | Delivered (Plan 02) | FontMap + DefaultFont + engine fallback via FFontResolver |
| TEXT-06 | Delivered | AutoWrapText=true only for paragraph text; CanvasPanelSlot width overridden to BoxWidthPx |

## Tasks Completed

### Task 1a: Apply outline + wrap in FTextLayerMapper, document TEXT-04 no-op
- **Commit:** bc8848c
- **Files:** FTextLayerMapper.cpp
- Added OutlineSettings block after SetFont, gated on OutlineSize > 0
- Added SetAutoWrapText(bHasExplicitWidth) for paragraph vs point text
- Added explicit TEXT-04 deferred comment citing PhotoshopAPI v0.9 limitation

### Task 1b: Size text-layer CanvasPanelSlot to BoxWidthPx
- **Commit:** a5a4155
- **Files:** FWidgetBlueprintGenerator.cpp
- Added branch before SetLayout: text layers with bHasExplicitWidth && BoxWidthPx > 0 get slot width overridden
- Only applies to non-stretched anchors (point-anchored text)

### Task 2: E2E text pipeline spec
- **Commit:** 90ddb31
- **Files:** FTextPipelineSpec.cpp (new)
- Drives Typography.psd through ParseFile -> Generate -> assert UTextBlock properties
- Asserts: parse success, regular size==18, stroked outline>0, paragraph autowrap==true, regular autowrap==false

## Deviations from Plan

None - plan executed exactly as written.

## Known Stubs

None.
