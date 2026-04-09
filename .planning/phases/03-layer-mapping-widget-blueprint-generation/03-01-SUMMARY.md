---
phase: 03-layer-mapping-widget-blueprint-generation
plan: 01
subsystem: mapper-infrastructure
tags: [mapper, texture-import, settings, interfaces]
dependency_graph:
  requires: [02-05]
  provides: [IPsdLayerMapper, FLayerMappingRegistry, FTextureImporter, FWidgetBlueprintGenerator-skeleton, WidgetBlueprintAssetDir]
  affects: [03-02, 03-03]
tech_stack:
  added: []
  patterns: [priority-dispatch, BGRA-swap, Source.Init, DeveloperSettings]
key_files:
  created:
    - Source/PSD2UMG/Public/Mapper/IPsdLayerMapper.h
    - Source/PSD2UMG/Public/Mapper/FLayerMappingRegistry.h
    - Source/PSD2UMG/Private/Mapper/FLayerMappingRegistry.cpp
    - Source/PSD2UMG/Public/Generator/FWidgetBlueprintGenerator.h
    - Source/PSD2UMG/Public/Generator/FTextureImporter.h
    - Source/PSD2UMG/Private/Generator/FTextureImporter.cpp
  modified:
    - Source/PSD2UMG/Public/PSD2UMGSetting.h
    - Source/PSD2UMG/Private/PSD2UMGSetting.cpp
decisions:
  - D-01: Registry is internal-only (no public Register API); mappers added in Plan 02
  - D-12/D-13/D-14: Texture import uses Source.Init with TSF_BGRA8 after R/B channel swap; skip+warn on empty pixels
metrics:
  duration: 10m
  completed: 2026-04-09
  tasks: 3
  files: 8
---

# Phase 3 Plan 1: Mapper Infrastructure & Texture Importer Summary

**One-liner:** Priority-dispatch mapper interface, BGRA-swapping texture importer, and FWidgetBlueprintGenerator skeleton with WidgetBlueprintAssetDir settings field.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | IPsdLayerMapper + FLayerMappingRegistry + FWidgetBlueprintGenerator skeleton | 29bf272 | 4 created |
| 2 | FTextureImporter with RGBA-to-BGRA swap and persistent Source.Init | 3067fb5 | 2 created |
| 3 | Add WidgetBlueprintAssetDir to UPSD2UMGSettings | 5bfd332 | 2 modified |

## What Was Built

### IPsdLayerMapper (interface)
- `GetPriority()`, `CanMap()`, `Map()` pure virtuals
- Forward-declares FPsdLayer, FPsdDocument, UWidgetTree, UWidget, UPanelWidget

### FLayerMappingRegistry
- `MapLayer()` iterates `TArray<TUniquePtr<IPsdLayerMapper>>` in priority order
- `RegisterDefaults()` empty stub — Plan 02 adds concrete mappers
- Logs `Warning` when no mapper matches

### FTextureImporter
- `ImportLayer()`: validates pixels, swaps R↔B channels (RGBA→BGRA), calls `Source.Init(TSF_BGRA8)`, `PreEditChange`/`PostEditChange`, `SaveLoadedAsset`
- `BuildTexturePath()`: reads `TextureAssetDir` from settings, returns `{Dir}/Textures/{PsdName}`
- `DeduplicateName()`: appends `_01`.._`99` for collisions

### FWidgetBlueprintGenerator (skeleton)
- `static UWidgetBlueprint* Generate(doc, packagePath, assetName)` declared only
- Implementation comes in Plan 03

### UPSD2UMGSettings
- New `FDirectoryPath WidgetBlueprintAssetDir` with default `/Game/UI/Widgets`

## Deviations from Plan

None — plan executed exactly as written.

## Known Stubs

- `FWidgetBlueprintGenerator::Generate` — declared only, no implementation. Plan 03 implements it. This is intentional per the plan spec (skeleton-only).
- `FLayerMappingRegistry::RegisterDefaults` — empty body. Plan 02 adds concrete mappers. This is intentional.

## Self-Check: PASSED

Files exist:
- Source/PSD2UMG/Public/Mapper/IPsdLayerMapper.h — FOUND
- Source/PSD2UMG/Public/Mapper/FLayerMappingRegistry.h — FOUND
- Source/PSD2UMG/Private/Mapper/FLayerMappingRegistry.cpp — FOUND
- Source/PSD2UMG/Public/Generator/FWidgetBlueprintGenerator.h — FOUND
- Source/PSD2UMG/Public/Generator/FTextureImporter.h — FOUND
- Source/PSD2UMG/Private/Generator/FTextureImporter.cpp — FOUND

Commits:
- 29bf272 — feat(03-01): IPsdLayerMapper interface...
- 3067fb5 — feat(03-01): FTextureImporter...
- 5bfd332 — feat(03-01): add WidgetBlueprintAssetDir...
