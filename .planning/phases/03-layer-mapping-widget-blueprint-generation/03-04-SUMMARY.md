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
  duration: "~5m"
  completed_date: "2026-04-09"
  tasks_completed: 2
  tasks_total: 3
  files_created: 1
  files_modified: 1
---

# Phase 03 Plan 04: Wire WBP Generator + E2E Spec Summary

Wire FWidgetBlueprintGenerator into PsdImportFactory as a side-effect call after parse, plus an automation spec covering WBP creation, layer skip, ZOrder inversion, and UImage for image layers.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Wire FWidgetBlueprintGenerator into UPsdImportFactory | 41126b1 | PsdImportFactory.cpp |
| 2 | Automation spec for WBP generation pipeline | 12308a9 | Tests/FWidgetBlueprintGenSpec.cpp |

## Task 3 — Pending Human Verification

Task 3 is a `checkpoint:human-verify` gate. See CHECKPOINT REACHED message returned by the executor.

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

None — plan executed exactly as written.

## Self-Check: PASSED

- `Source/PSD2UMG/Private/Factories/PsdImportFactory.cpp` — modified, includes FWidgetBlueprintGenerator
- `Source/PSD2UMG/Tests/FWidgetBlueprintGenSpec.cpp` — created, contains BEGIN_DEFINE_SPEC
- Commits 41126b1 and 12308a9 exist
