---
phase: 03-layer-mapping-widget-blueprint-generation
plan: "04"
subsystem: factory-wiring
tags: [factory, wbp-generation, automation-spec, e2e]
dependency_graph:
  requires: ["03-02", "03-03"]
  provides: ["e2e-pipeline", "wbp-gen-spec"]
  affects: ["PsdImportFactory", "FWidgetBlueprintGenerator"]
tech_stack:
  added: []
  patterns: ["side-effect asset generation from factory", "automation spec with programmatic FPsdDocument"]
key_files:
  modified:
    - Source/PSD2UMG/Private/Factories/PsdImportFactory.cpp
  created:
    - Source/PSD2UMG/Tests/FWidgetBlueprintGenSpec.cpp
decisions:
  - "WBP generation is a side-effect from the factory; UTexture2D result returned unchanged for backward compat"
  - "WbpDir falls back to /Game/UI/Widgets if WidgetBlueprintAssetDir is empty"
metrics:
  duration: "~5m + ~45m verification/fixes"
  completed_date: "2026-04-09"
  tasks_completed: 3
  tasks_total: 3
  files_created: 1
  files_modified: 5
---

# Phase 03 Plan 04: Wire WBP Generator + E2E Spec Summary

Wire FWidgetBlueprintGenerator into PsdImportFactory as a side-effect call after parse, plus an automation spec covering WBP creation, layer skip, ZOrder inversion, and UImage for image layers.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Wire FWidgetBlueprintGenerator into UPsdImportFactory | 41126b1 | PsdImportFactory.cpp |
| 2 | Automation spec for WBP generation pipeline | 12308a9 | Tests/FWidgetBlueprintGenSpec.cpp |

## Task 3 — Human Verification: APPROVED

Verified manually in UE 5.7.4 Editor by dragging `MultiLayer.psd` into the Content Browser:
- `WBP_MultiLayer` created under `/Game/UI/Widgets/` — opens in UMG Designer without errors
- Title, BtnNormal, Background rendered at correct PSD positions
- Textures imported as persistent `UTexture2D` assets under `{TextureAssetDir}/Textures/PSD2UMG_<hash>/`
- Invisible `BtnHover` layer correctly skipped (D-08)
- All `PSD2UMG.Generator.*` and `PSD2UMG.Parser.MultiLayer.*` automation specs pass (Session Frontend)

### Post-Initial-Implementation Fixes (caught during verification)

| # | Issue | Fix | Commit |
|---|-------|-----|--------|
| 1 | `TUniquePtr<IPsdLayerMapper>` copy error in implicit `operator=` | Include `IPsdLayerMapper.h`; `=delete` copy, `=default` move on `FLayerMappingRegistry` | 6e692eb, 166e6f4 |
| 2 | `TestNotNull(TObjectPtr<T>)` template deduction fails in UE 5.7 | Call `.Get()` on `WidgetTree`/`RootWidget` | 6e692eb |
| 3 | UImage rendered as thin border instead of filled rect | `SetBrushFromTexture(Tex, bMatchSize=true)` + explicit `DrawAs=Image` | fddec19 |
| 4 | Group layers had zero bounds → children clipped inside 0×0 parent canvas | Groups always full-stretch with zero margins (transparent containers) | 7a98b78 |
| 5 | Auto-stretch heuristic (80% threshold) surprised designers | Disabled auto-stretch; point anchors by default; explicit `_stretch-*`/`_fill` suffixes still work | 424c15c |

## What Was Built

**Task 1 — Factory wiring:**
- Added `#include "Generator/FWidgetBlueprintGenerator.h"` and `#include "PSD2UMGSetting.h"` to PsdImportFactory.cpp
- After successful parse (`bOk == true`), reads `WidgetBlueprintAssetDir` from `UPSD2UMGSettings::Get()`, falls back to `/Game/UI/Widgets` if empty
- Calls `FWidgetBlueprintGenerator::Generate(Doc, WbpDir, WbpAssetName)` with name `WBP_<PsdBaseName>`
- Logs success or warning; returns original `UTexture2D` result unchanged

**Task 2 — Automation spec:**
- `Source/PSD2UMG/Tests/FWidgetBlueprintGenSpec.cpp` with `BEGIN_DEFINE_SPEC(FWidgetBlueprintGenSpec, "PSD2UMG.Generator", ...)`
- 4 test cases: minimal doc creates valid WBP with 2 children; invisible layer skipped; ZOrder inverted (layer 0 gets Z=2 for 3-layer doc); image layer produces UImage child

## Deviations from Plan

- **Auto-stretch heuristic disabled** — the plan specified quadrant-based auto-stretch at ≥80% canvas dimension, but field testing showed surprising results (layers barely crossing the threshold stretched unexpectedly). Changed to point-anchor-only by default per user direction; `_stretch-h`, `_stretch-v`, `_fill` suffixes still work as specified.
- **Group layers always fill parent** — the plan did not account for PhotoshopAPI leaving group layer bounds unset (all zeros). Groups are now transparent containers with full-stretch anchors and zero margins.
- **Debug layout log added** — not in the plan, but `FWidgetBlueprintGenerator::PopulateCanvas` now logs each layer's bounds, anchors, stretch flags, and computed offsets. Very useful for diagnosing layout issues and worth keeping.

## Self-Check: PASSED

- `Source/PSD2UMG/Private/Factories/PsdImportFactory.cpp` — modified, includes FWidgetBlueprintGenerator
- `Source/PSD2UMG/Tests/FWidgetBlueprintGenSpec.cpp` — created, contains BEGIN_DEFINE_SPEC
- Commits 41126b1 and 12308a9 exist
