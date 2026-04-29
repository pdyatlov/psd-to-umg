---
phase: quick
plan: 260429-juj
subsystem: text-layout
tags: [text, layout, overflow, autosize, wrap]
dependency_graph:
  requires: []
  provides: [TXT-OVERFLOW-01]
  affects: [FTextLayerMapper, FWidgetBlueprintGenerator]
tech_stack:
  added: []
  patterns: [SetWrapTextAt, SetAutoSize]
key_files:
  created: []
  modified:
    - Source/PSD2UMG/Private/Mapper/FTextLayerMapper.cpp
    - Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp
decisions:
  - "TXT-OVERFLOW-01: SetAutoSize(true) on non-stretch text canvas slots; SetWrapTextAt(BoxWidthPx) replaces SetAutoWrapText for paragraph text"
metrics:
  duration: "~5m"
  completed: "2026-04-29T12:20:11Z"
  tasks: 2
  files: 2
---

# Quick Task 260429-juj: Fix text bottom overflow — WrapTextAt + AutoSize for non-stretch text slots

**One-liner:** Replaced SetAutoWrapText with SetWrapTextAt(BoxWidthPx) and added SetAutoSize(true) on non-stretch text canvas slots to fix Slate font metric height mismatch against PhotoshopAPI tight pixel bounds.

## Objective

Fix UTextBlock bottom overflow caused by Slate font metrics being taller than PhotoshopAPI's tight pixel bounds. Two coordinated changes remove the height mismatch.

## Tasks Completed

| # | Task | Commit | Files |
|---|------|--------|-------|
| 1 | Replace SetAutoWrapText with SetWrapTextAt in FTextLayerMapper | 4f4a521 | FTextLayerMapper.cpp |
| 2 | Add SetAutoSize(true) for non-stretch text slots in FWidgetBlueprintGenerator | b8eb7f7 | FWidgetBlueprintGenerator.cpp |

## Changes Made

### Task 1 — FTextLayerMapper.cpp (line 111-121)

- Removed: `TextWidget->SetAutoWrapText(Layer.Text.bHasExplicitWidth);`
- Added: Conditional `TextWidget->SetWrapTextAt(Layer.Text.BoxWidthPx)` guarded by `bHasExplicitWidth && BoxWidthPx > 0.f`
- Rationale: SetAutoWrapText relies on slot width which becomes undefined when AutoSize=true. SetWrapTextAt provides a fixed pixel boundary independent of slot size.

### Task 2 — FWidgetBlueprintGenerator.cpp (after line 278 SetLayout call)

- Added: `CanvasSlot->SetAutoSize(true)` block guarded by `LayerPtr->Type == EPsdLayerType::Text && !AnchorResult.bStretchH && !AnchorResult.bStretchV`
- Inserted between `CanvasSlot->SetLayout(Data)` and `CanvasSlot->SetZOrder(...)`
- Rationale: PhotoshopAPI layer bounds are tight visual bounds; Slate line height (font metrics + leading) is typically taller. AutoSize lets the canvas slot use UTextBlock's desired size from Slate metrics instead of the tight PSD bounds.
- Stretch-anchored slots excluded to preserve margin-fill behaviour.

## Verification

- `SetAutoWrapText(` call: absent from FTextLayerMapper.cpp (only mentioned in comment)
- `SetWrapTextAt`: present at FTextLayerMapper.cpp line 119 inside bHasExplicitWidth guard
- `SetAutoSize(true)`: present at FWidgetBlueprintGenerator.cpp line 296 inside EPsdLayerType::Text + !bStretchH + !bStretchV guard, positioned after SetLayout call

## Deviations from Plan

None — plan executed exactly as written.

## Known Stubs

None.

## Self-Check: PASSED

- 4f4a521 confirmed in git log
- b8eb7f7 confirmed in git log
- FTextLayerMapper.cpp: SetWrapTextAt present, SetAutoWrapText call absent
- FWidgetBlueprintGenerator.cpp: SetAutoSize(true) present with correct guard
