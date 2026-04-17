---
phase: 06-advanced-layout
plan: "03"
subsystem: layout-heuristic
tags: [row-detection, column-detection, HorizontalBox, VerticalBox, heuristic]
dependency_graph:
  requires: [06-02]
  provides: [row-column-heuristic]
  affects: [FWidgetBlueprintGenerator, PopulateCanvas]
tech_stack:
  added: [DetectHorizontalRow, DetectVerticalColumn, UHorizontalBox, UVerticalBox]
  patterns: [alignment-tolerance-6px, conservative-all-children-guard]
key_files:
  created: []
  modified:
    - Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp
decisions:
  - AlignmentTolerancePx = 6 (within D-10 design-decision range of 4-8px)
  - Heuristic is conservative: requires ALL children to align; single misaligned child causes fall-through
  - HBox children sorted left-to-right by Bounds.Min.X; VBox children sorted top-to-bottom by Bounds.Min.Y
  - HBox/VBox slot fills parent canvas (stretch anchors, zero offsets) so children remain in correct coordinate space
metrics:
  duration: "~5m"
  completed: "2026-04-10T11:05:00Z"
  tasks_completed: 1
  files_changed: 1
---

# Phase 06 Plan 03: Row/Column Detection Heuristic Summary

**One-liner:** Conservative row/column heuristic in PopulateCanvas wraps vertically-aligned children in UHorizontalBox and horizontally-aligned children in UVerticalBox with 6px tolerance.

## What Was Built

One detection pass added at the **start** of `PopulateCanvas`, before the main CanvasPanel slot loop:

1. **`AlignmentTolerancePx = 6`** — compile-time constant (satisfies D-10 range 4–8px).

2. **`DetectHorizontalRow`** — iterates all visual layers (skips Unknown type and zero-size non-groups), computes each layer's vertical center `(Min.Y + Max.Y) / 2`, and returns `true` only when every visual layer's center is within `Tolerance` of the first. Requires `Layers.Num() >= 2`.

3. **`DetectVerticalColumn`** — same pattern but compares horizontal centers `(Min.X + Max.X) / 2`.

4. **Integration in PopulateCanvas** — guard fires only when `Layers.Num() >= 2`. Row path creates `UHorizontalBox`, sorts layers left-to-right, adds each child via `AddChildToHorizontalBox`; column path creates `UVerticalBox`, sorts top-to-bottom, adds via `AddChildToVerticalBox`. Both paths set fill anchors on the wrapping slot and return early, bypassing the normal CanvasPanel loop. When neither fires, execution falls through unchanged.

## Commits

| Task | Commit  | Description |
|------|---------|-------------|
| 1    | aa62b8f | feat(06-03): add row/column detection heuristic to PopulateCanvas |

## Deviations from Plan

None — plan executed exactly as written.

## Known Stubs

None. Heuristic reads real `FPsdLayer.Bounds` data.

## Self-Check: PASSED

- `Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp` — contains `DetectHorizontalRow`, `DetectVerticalColumn`, `AlignmentTolerancePx = 6`, `UHorizontalBox`, `UVerticalBox`
- Commit `aa62b8f` confirmed in git log
