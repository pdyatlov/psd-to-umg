---
phase: 03-layer-mapping-widget-blueprint-generation
plan: 03
subsystem: generator
tags: [widget-blueprint, anchor-calculator, canvas-panel, ue5, umg]
dependency_graph:
  requires: [03-01]
  provides: [WBP-01, WBP-02, WBP-03, WBP-04, WBP-05, WBP-06]
  affects: [FPsdImportFactory, Phase 4 DPI/font work]
tech_stack:
  added: []
  patterns:
    - UWidgetBlueprintFactory::FactoryCreateNew (canonical WBP creation path)
    - FKismetEditorUtilities::CompileBlueprint (compile after full tree population)
    - UCanvasPanel::AddChildToCanvas + UCanvasPanelSlot::SetLayout/SetZOrder
    - 3x3 quadrant grid anchor heuristics with 80% stretch threshold
    - Anchor suffix override parsing (12 suffixes, longest-first)
key_files:
  created:
    - Source/PSD2UMG/Private/Generator/FAnchorCalculator.h
    - Source/PSD2UMG/Private/Generator/FAnchorCalculator.cpp
    - Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp
  modified: []
decisions:
  - UWidgetBlueprintFactory used over FKismetEditorUtilities::CreateBlueprint directly — matches Content Browser path, ensures correct UMGEditor module hooks
  - FAnchorCalculator is a static-method class (not namespace) for consistency with project style
  - Suffix table ordered longest-first so _anchor-cr/_anchor-cl/_anchor-c do not shadow each other
  - bComputed flag in suffix table distinguishes fixed-anchor vs computed-from-quadrant suffixes (_stretch-h, _stretch-v, _fill)
metrics:
  duration: ~5m
  completed_date: "2026-04-09T13:09:23Z"
  tasks_completed: 2
  files_changed: 3
---

# Phase 03 Plan 03: Widget Blueprint Generator Summary

Widget Blueprint generator and anchor calculator — core of Phase 3. Takes an FPsdDocument and produces a fully compiled, openable UWidgetBlueprint via UWidgetBlueprintFactory, recursive canvas population, and FKismetEditorUtilities::CompileBlueprint.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | FAnchorCalculator — quadrant heuristics + suffix parsing | b3e0834 | FAnchorCalculator.h, FAnchorCalculator.cpp |
| 2 | FWidgetBlueprintGenerator — full WBP creation, recursive tree, coordinate math | ebf0458 | FWidgetBlueprintGenerator.cpp |

## What Was Built

### FAnchorCalculator

- `Calculate(LayerName, Bounds, CanvasSize)` — public entry point
- `TryParseSuffix` — checks all 12 suffix overrides (longest first): `_anchor-tl`, `_anchor-tc`, `_anchor-tr`, `_anchor-cl`, `_anchor-c`, `_anchor-cr`, `_anchor-bl`, `_anchor-bc`, `_anchor-br`, `_stretch-h`, `_stretch-v`, `_fill`
- `QuadrantAnchor` — 3x3 grid from layer center: thirds divide canvas; width >= 80% -> stretch H, height >= 80% -> stretch V
- `FAnchorResult` struct: `Anchors`, `bStretchH`, `bStretchV`, `CleanName` (suffix stripped)

### FWidgetBlueprintGenerator

- `Generate(Doc, WbpPackagePath, WbpAssetName)` — creates UPackage, uses `UWidgetBlueprintFactory::FactoryCreateNew`, constructs `Root_Canvas` UCanvasPanel, calls recursive `PopulateCanvas`, then `FKismetEditorUtilities::CompileBlueprint`, then `SaveLoadedAsset`
- `PopulateCanvas` (file-local static): skips invisible and zero-size layers (D-08/D-14), calls `FLayerMappingRegistry::MapLayer`, adds to canvas via `AddChildToCanvas`, calls `FAnchorCalculator::Calculate`, sets `Slot->SetLayout(FAnchorData)` and `Slot->SetZOrder(TotalLayers - 1 - i)`, recurses into Group children

### Anchor coordinate math (all 4 modes)

| Mode | Left | Top | Right | Bottom |
|------|------|-----|-------|--------|
| Full stretch | margin-left | margin-top | margin-right | margin-bottom |
| H-stretch | margin-left | top - anchorY | margin-right | height |
| V-stretch | left - anchorX | margin-top | width | margin-bottom |
| Point | left - anchorX | top - anchorY | width | height |

## Deviations from Plan

None — plan executed exactly as written.

## Known Stubs

None — FLayerMappingRegistry::MapLayer dispatches to concrete mapper implementations completed in 03-01/03-02.

## Self-Check: PASSED

- Source/PSD2UMG/Private/Generator/FAnchorCalculator.h — FOUND
- Source/PSD2UMG/Private/Generator/FAnchorCalculator.cpp — FOUND
- Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp — FOUND
- Commit b3e0834 — FOUND
- Commit ebf0458 — FOUND
