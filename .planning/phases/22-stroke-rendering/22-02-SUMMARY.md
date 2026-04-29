---
phase: 22-stroke-rendering
plan: 02
subsystem: generator
tags: [stroke, fx-06, stroke-01, stroke-03, canvas-sibling, tdd]
dependency_graph:
  requires:
    - "22-01: vstk fields (bHasVectorStroke, VectorStrokeSize, VectorStrokeColor) in FPsdLayerEffects"
    - "Phase 15: FX-04 drop-shadow canvas-sibling pattern (structural model)"
    - "Phase 4.1: lfx2 bHasStroke/StrokeSize/StrokeColor fields in FPsdLayerEffects"
  provides:
    - "FX-06 stroke sibling block in FWidgetBlueprintGenerator::PopulateChildren (canvas-only)"
    - "Four new automation specs covering STROKE-01 Image, STROKE-01 Shape residual, STROKE-03, non-canvas guard"
  affects:
    - "FWidgetBlueprintGenerator.cpp: all canvas-parented imports now check for stroke and emit sibling"
    - "FShapeLayerMapper.cpp: header comment updated to remove obsolete 'Future stroke rendering' sentence"
tech_stack:
  added: []
  patterns:
    - "Canvas-only sibling pattern (FX-06 follows FX-04 drop-shadow structural model)"
    - "bShapeStroke-takes-precedence ternary as defence-in-depth against double-emit"
    - "Pitfall 6 Right/Bottom expansion (non-stretch canvas slot encoding)"
key_files:
  created: []
  modified:
    - "Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp"
    - "Source/PSD2UMG/Private/Mapper/FShapeLayerMapper.cpp"
    - "Source/PSD2UMG/Tests/FWidgetBlueprintGenSpec.cpp"
decisions:
  - "D-01 confirmed: stroke sibling lives in the generator (FWidgetBlueprintGenerator FX-06), NOT in FShapeLayerMapper"
  - "D-02 confirmed: stroke sibling always uses ESlateBrushDrawType::NoDrawType — no texture, no DrawType::Border"
  - "D-03 confirmed: bShapeStroke (vstk) takes precedence ternary added as defence-in-depth even though parser already clears bHasStroke when bHasVectorStroke is set"
  - "Pitfall 5 label: new block is FX-06 (line 107 FX-05 unchanged)"
  - "Pitfall 6: Offsets.Right += 2*StrokePx AND Offsets.Bottom += 2*StrokePx (both expansions, not just Left/Top shift)"
metrics:
  duration: "~10min"
  completed: "2026-04-29"
  tasks: 2
  files: 3
---

# Phase 22 Plan 02: Stroke Sibling Generator (FX-06) Summary

**One-liner:** FX-06 canvas-sibling UImage block for STROKE-01 (Image/Shape lfx2) and STROKE-03 (Shape vstk) with Pitfall-6 Right/Bottom slot expansion and four new automation specs.

## What Was Built

### PART A — FX-06 Stroke Sibling Block (FWidgetBlueprintGenerator.cpp)

Inserted immediately after the FX-04 drop-shadow block inside the `if (UCanvasPanelSlot* CanvasSlot = ...)` canvas-only guard in `PopulateChildren`. The block:

- Computes `bImageStroke = bHasStroke && (Type == Image || Type == Shape)` covering STROKE-01 for both Image AND Shape layers carrying lfx2 bHasStroke
- Computes `bShapeStroke = bHasVectorStroke && Type == Shape` covering STROKE-03 vstk path
- On `bImageStroke || bShapeStroke` (with degenerate-stroke skip for StrokePx == 0 or alpha == 0):
  - Constructs a `UImage` named `{CleanName}_Stroke` via `MakeUniqueObjectName`
  - Sets brush `DrawAs = NoDrawType` (D-02), `TintColor = StrokeColor`, `ImageSize = bounds + 2*StrokePx`
  - Adds to canvas via `CanvasParent->AddChildToCanvas`
  - Copies Anchors + Alignment from main slot
  - Applies Pitfall 6: `StrokeOffsets.Left -= StrokePx`, `StrokeOffsets.Top -= StrokePx`, `StrokeOffsets.Right += 2*StrokePx`, `StrokeOffsets.Bottom += 2*StrokePx`
  - Sets `ZOrder = CanvasSlot->GetZOrder() - 1` (behind main widget, mirrors FX-04)

### PART B — Non-Canvas Warning Block (FWidgetBlueprintGenerator.cpp)

Added after the existing drop-shadow non-canvas warning (after line 385). Emits `UE_LOG Warning` for stroke on Image/Shape layers inside non-canvas parents (UVerticalBox, etc.) — mirrors FX-04 non-canvas behaviour.

### PART C — FShapeLayerMapper.cpp Header Comment Update

Replaced obsolete `"Future stroke rendering (vstk -> UMG border/outline) will attach here, not in FSolidFillLayerMapper."` with `"Stroke rendering (vstk) is emitted by FWidgetBlueprintGenerator's FX-06 stroke-sibling block (Phase 22 STROKE-03, D-01) — NOT in this mapper."` Class body and Map() implementation unchanged.

### PART D — Verified FX-05 Unchanged

`// FX-05: Flatten fallback` at line 107 confirmed present and unmodified.

### Task 2 — Four New It() Blocks (FWidgetBlueprintGenSpec.cpp)

Inserted after the existing drop-shadow It() block at line 299, before `should assign ZOrder inversely from layer index`:

1. **STROKE-01 Image**: Image layer `bHasStroke=true, StrokeSize=4, StrokeColor=red` → 2 children, `_Stroke` UImage at ZOrder = main - 1, Left/Top shifted -4, Right/Bottom expanded +8, red tint, NoDrawType brush
2. **STROKE-01 Shape residual**: Shape layer `bHasStroke=true, bHasVectorStroke=false, StrokeSize=2, StrokeColor=green` → 2 children, green tint (not VectorStrokeColor), Right expanded +4
3. **STROKE-03**: Shape layer `bHasVectorStroke=true, VectorStrokeSize=3, VectorStrokeColor=blue` → 2 children, blue tint sourced from VectorStrokeColor, Right expanded +6
4. **Non-canvas guard**: Image `bHasStroke` inside `@vbox` group → zero `_Stroke` widgets anywhere in tree

All four tests build `FPsdDocument` in-memory (no fixture PSDs introduced, per D-04).

## Decisions Confirmed

| Decision | Status | Notes |
|----------|--------|-------|
| D-01: sibling in generator, not mapper | Confirmed | FX-06 in PopulateChildren; FShapeLayerMapper header updated |
| D-02: NoDrawType brush always | Confirmed | `StrokeBrush.DrawAs = ESlateBrushDrawType::NoDrawType` |
| D-03: parser clears bHasStroke on vstk-win | Confirmed | Defence-in-depth ternary added in generator regardless |
| Pitfall 5: FX label | Confirmed | New block is FX-06; FX-05 at line 107 unchanged |
| Pitfall 6: Right/Bottom expansion | Confirmed | Both `+= 2*StrokePx` expansions present; assertions in all three positive specs |

## STROKE-01 Coverage Clarification

REQUIREMENTS.md says "Image/shape layers with lfx2 bHasStroke". The `bImageStroke` condition now accepts `EPsdLayerType::Image || EPsdLayerType::Shape` so both paths are satisfied:

- **Image + lfx2**: the original Phase 4.1 path — spec Test 1
- **Shape + lfx2 (residual)**: a Shape layer with no vstk block retains bHasStroke (D-03 only fires when vstk wins) — spec Test 2 is the regression guard that would fail loudly if the condition were ever narrowed back to Image-only

## Open Items Deferred

- **Positive Stroke.psd fixture**: D-04 deferred positive PSD fixture until a real Stroke.psd is available. Synthetic in-memory tests are the spec-level substitute.
- **Inside/outside/center stroke alignment**: current implementation uses the lfx2/vstk StrokeSize as a symmetric expansion (Pitfall 6 pattern). Photoshop's inside/center/outside modes affect offset differently. Deferred to v1.3+ when alignment field is parsed from the descriptor.
- **frameFXMulti VlLs stroke emission**: Plan 22-01 parsed the legacy lfx2 path; VlLs branch (newer Photoshop format) is a separate parser task deferred beyond Phase 22.

## Anomalies Encountered

- `AddExpectedError` with `EAutomationExpectedErrorFlags::Contains` — used in the non-canvas test to declare the expected Warning. If the project's automation harness does not surface `UE_LOG Warning` to the test error channel, the `AddExpectedError` line is a no-op; the structural assertion (`StrokeWidgetCount == 0`) is the load-bearing check and will still pass.
- `grep -c "FX-05"` returns 2 (line 107 + the reference comment inside FX-06 body) — both occurrences are correct; acceptance criterion requires `>= 1`.

## Deviations from Plan

None — plan executed exactly as written. All PART A/B/C/D actions applied verbatim. All four It() blocks inserted at the specified location.

## Self-Check: PASSED

- `Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp` — contains FX-06, FX-05 unchanged, StrokeOffsets Right/Bottom expansions, `_Stroke` substring, non-canvas warning
- `Source/PSD2UMG/Private/Mapper/FShapeLayerMapper.cpp` — contains FX-06, no `Future stroke rendering`
- `Source/PSD2UMG/Tests/FWidgetBlueprintGenSpec.cpp` — contains all four It() blocks, 8 `_Stroke` occurrences
- Commits: `dd63348` (RED tests), `df35615` (GREEN implementation)
