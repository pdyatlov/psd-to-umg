---
phase: 07-editor-ui-preview-settings
plan: 05
subsystem: reimport
tags: [reimport, widget-blueprint, change-detection, preview-dialog]
dependency_graph:
  requires: [07-03, 07-04]
  provides: [FPsdReimportHandler, FWidgetBlueprintGenerator::Update]
  affects: [FPsdContentBrowserExtensions, FPSD2UMGModule]
tech_stack:
  added: [FReimportHandler, FReimportManager]
  patterns: [name-based widget matching, orphan preservation, annotated reimport dialog]
key_files:
  created:
    - Source/PSD2UMG/Private/Reimport/FPsdReimportHandler.h
    - Source/PSD2UMG/Private/Reimport/FPsdReimportHandler.cpp
  modified:
    - Source/PSD2UMG/Public/Generator/FWidgetBlueprintGenerator.h
    - Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp
    - Source/PSD2UMG/Public/PSD2UMG.h
    - Source/PSD2UMG/Private/PSD2UMG.cpp
    - Source/PSD2UMG/Private/ContentBrowser/FPsdContentBrowserExtensions.cpp
decisions:
  - "FReimportManager auto-registers on FReimportHandler construction; module stores TUniquePtr for lifetime"
  - "DetectChange uses widget name lookup + bounds/text/opacity comparison for annotations"
  - "Orphan widgets (deleted PSD layers) are kept in WidgetTree per D-07, never removed"
  - "Content browser Reimport action delegates to FReimportManager::Instance()->Reimport() for handler dispatch"
metrics:
  duration: 15m
  completed_date: "2026-04-10"
  tasks: 2
  files: 7
---

# Phase 07 Plan 05: Reimport Handler Summary

**One-liner:** Non-destructive PSD reimport via FReimportHandler with annotated preview dialog and name-based widget update preserving orphans.

## What Was Built

### Task 1: FWidgetBlueprintGenerator::Update() and DetectChange()

Added two new static methods to `FWidgetBlueprintGenerator`:

- `Update(ExistingWBP, NewDoc, SkippedLayerNames)` — walks the new PSD document, matches existing widgets by name, updates PSD-sourced properties (position/size via CanvasPanelSlot, text content/font/color for UTextBlock, opacity and visibility). New layers are created and added to the canvas. Widgets from deleted PSD layers remain as orphans (D-07). Compiles and saves the blueprint on completion.

- `DetectChange(NewLayer, ExistingWidget)` — returns `EPsdChangeAnnotation::New` if no existing widget, `Changed` if bounds/text/opacity differ, `Unchanged` otherwise. Used by the reimport handler to annotate tree items before showing the preview dialog.

### Task 2: FPsdReimportHandler and Module Registration

- `FPsdReimportHandler` subclasses `FReimportHandler` with full `CanReimport` / `SetReimportPaths` / `Reimport` implementation.
- `CanReimport()` casts to UWidgetBlueprint and reads `PSD2UMG.SourcePsdPath` package metadata.
- `Reimport()` pipeline: file existence check → `ParseFile` → `BuildTreeFromDocument` → annotate items → show `SPsdImportPreviewDialog` with `bIsReimport=true` → collect unchecked names → `FWidgetBlueprintGenerator::Update()` → update metadata → return Succeeded/Cancelled/Failed.
- `FPSD2UMGModule` stores `TUniquePtr<FPsdReimportHandler>`, created in `StartupModule` and reset in `ShutdownModule`.
- Content browser "Reimport from PSD" action updated to call `FReimportManager::Instance()->Reimport(Obj)` instead of placeholder log.

## Deviations from Plan

None - plan executed exactly as written.

## Known Stubs

None — all reimport flow is wired end-to-end.

## Self-Check: PASSED

- Source/PSD2UMG/Private/Reimport/FPsdReimportHandler.h: FOUND
- Source/PSD2UMG/Private/Reimport/FPsdReimportHandler.cpp: FOUND
- Commits 2ad5cb5 and 9f816e3: FOUND
