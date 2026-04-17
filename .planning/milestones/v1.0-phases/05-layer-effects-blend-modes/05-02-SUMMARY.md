---
phase: 05-layer-effects-blend-modes
plan: "02"
subsystem: generator
tags: [effects, opacity, visibility, color-overlay, drop-shadow, flatten]
dependency_graph:
  requires: ["05-01"]
  provides: ["FX-01", "FX-02", "FX-03", "FX-04", "FX-05"]
  affects: ["FWidgetBlueprintGenerator", "FWidgetBlueprintGenSpec"]
tech_stack:
  added: []
  patterns: ["LayerPtr indirection for flatten", "sibling widget pattern for drop shadow"]
key_files:
  modified:
    - Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp
    - Source/PSD2UMG/Tests/FWidgetBlueprintGenSpec.cpp
decisions:
  - "Invisible layers no longer skipped — created with Collapsed visibility (D-01/D-02)"
  - "Drop shadow uses sibling UImage sharing main image brush tinted to shadow color"
  - "Flatten copies layer, sets type to Image, empties children before mapper call"
metrics:
  duration: "10m"
  completed_date: "2026-04-10"
  tasks: 2
  files_modified: 2
---

# Phase 05 Plan 02: Layer Effects Generator Application Summary

**One-liner:** PopulateCanvas now applies opacity, collapsed visibility, color overlay tint, drop shadow sibling UImage, and complex-effects flatten before passing layers to the widget mapper.

## What Was Built

All five layer effects (FX-01 through FX-05) are now applied in `FWidgetBlueprintGenerator::PopulateCanvas`:

- **FX-01 (Opacity):** `SetRenderOpacity` called when `Layer.Opacity < 1.0f`
- **FX-02 (Visibility):** Hidden layers are now created (not skipped); `SetVisibility(Collapsed)` called post-creation instead of `continue`
- **FX-03 (Color Overlay):** `FSlateBrush::TintColor` set on `UImage` widgets with `bHasColorOverlay`; non-image layers log a warning and skip
- **FX-04 (Drop Shadow):** A sibling `UImage` is constructed in the same canvas, sharing the main image brush tinted to `DropShadowColor`, positioned with `DropShadowOffset`, and given `ZOrder - 1` so it appears behind
- **FX-05 (Flatten):** Layers with `bHasComplexEffects` and non-empty `RGBAPixels` are copied into a local `FlattenedLayer` with `Type=Image` and empty children before mapping; `bFlattened` flag guards against double-recursing into group children

The `LayerPtr` indirection pattern ensures all slot calculations and effect application use the correct (possibly flattened) layer data while the group-recursion guard still uses the original `Layer`.

## Tests Updated/Added

- **"should create invisible layers as Collapsed"** — replaces old "should skip invisible layers"; asserts 1 child created with `GetVisibility() == Collapsed`
- **"should apply opacity via SetRenderOpacity"** — text layer at 0.5 opacity; asserts `GetRenderOpacity() ~= 0.5`
- **"should apply color overlay tint to UImage brush"** — image layer with red overlay; asserts tint RGBA on `FSlateBrush`
- **"should flatten layer with complex effects when setting enabled"** — group with child and `bHasComplexEffects`; asserts 1 `UImage` child (no group recursion)
- **"should create shadow sibling for drop shadow layers"** — image layer with drop shadow; asserts 2 canvas children with differing ZOrders

## Deviations from Plan

None — plan executed exactly as written.

## Known Stubs

None.

## Self-Check: PASSED

- `Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp` — exists, contains `SetRenderOpacity`, `SetVisibility`, `bFlattenComplexEffects`
- `Source/PSD2UMG/Tests/FWidgetBlueprintGenSpec.cpp` — exists, contains all 5 new/updated test cases
- Commits: 91cb0e5 (generator), 7315ed9 (tests)
