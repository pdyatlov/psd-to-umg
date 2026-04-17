---
phase: 03-layer-mapping-widget-blueprint-generation
plan: 02
subsystem: mapper
tags: [mapper, umg, widget, blueprint, image, text, group, button, progress, prefix]
dependency_graph:
  requires: [03-01]
  provides: [03-03]
  affects: [FLayerMappingRegistry, FWidgetBlueprintGenerator]
tech_stack:
  added: []
  patterns: [out-of-line class implementation via shared header, priority-sorted mapper dispatch]
key_files:
  created:
    - Source/PSD2UMG/Private/Mapper/AllMappers.h
    - Source/PSD2UMG/Private/Mapper/FImageLayerMapper.cpp
    - Source/PSD2UMG/Private/Mapper/FTextLayerMapper.cpp
    - Source/PSD2UMG/Private/Mapper/FGroupLayerMapper.cpp
    - Source/PSD2UMG/Private/Mapper/FButtonLayerMapper.cpp
    - Source/PSD2UMG/Private/Mapper/FProgressLayerMapper.cpp
    - Source/PSD2UMG/Private/Mapper/FSimplePrefixMappers.cpp
  modified:
    - Source/PSD2UMG/Private/Mapper/FLayerMappingRegistry.cpp
decisions:
  - AllMappers.h is a private header that declares all 15 mapper classes; individual .cpps provide out-of-line implementations — avoids ODR violation while keeping mappers internal-only (no public headers per D-01)
  - FButtonLayerMapper uses SetStyle(Style) not WidgetStyle= (deprecated in UE 5.2+)
  - FProgressLayerMapper uses SetWidgetStyle(Style) not direct field assignment
  - List_ and Tile_ mappers log an info message that EntryWidgetClass must be set manually — PSD layer data cannot encode a UE class reference
metrics:
  duration: 5m
  completed: 2026-04-09
  tasks: 3
  files: 8
---

# Phase 03 Plan 02: Widget Mapper Implementations Summary

All 15 mapper classes implemented and registered in FLayerMappingRegistry. PSD layers now map to correct UMG widget types with proper priority ordering.

## Tasks Completed

| Task | Description | Commit | Files |
|------|-------------|--------|-------|
| 1 | Image, Text, and Group mappers | 6bd0509 | FImageLayerMapper.cpp, FTextLayerMapper.cpp, FGroupLayerMapper.cpp |
| 2 | Button_ and Progress_ prefix mappers | ac8ecc6 | FButtonLayerMapper.cpp, FProgressLayerMapper.cpp |
| 3 | Simple prefix mappers + registry wiring | 31e4db1 | FSimplePrefixMappers.cpp, AllMappers.h, FLayerMappingRegistry.cpp |

## What Was Built

**15 mapper classes** implementing `IPsdLayerMapper`, each with `GetPriority()`, `CanMap()`, and `Map()`:

| Priority | Class | Trigger | Widget |
|----------|-------|---------|--------|
| 200 | FButtonLayerMapper | `Button_` prefix on Group | UButton with 4-state brushes |
| 200 | FProgressLayerMapper | `Progress_` prefix on Group | UProgressBar with bg/fill brushes |
| 200 | FHBoxLayerMapper | `HBox_` prefix on Group | UHorizontalBox |
| 200 | FVBoxLayerMapper | `VBox_` prefix on Group | UVerticalBox |
| 200 | FOverlayLayerMapper | `Overlay_` prefix on Group | UOverlay |
| 200 | FScrollBoxLayerMapper | `ScrollBox_` prefix on Group | UScrollBox |
| 200 | FSliderLayerMapper | `Slider_` prefix on Group | USlider |
| 200 | FCheckBoxLayerMapper | `CheckBox_` prefix on Group | UCheckBox |
| 200 | FInputLayerMapper | `Input_` prefix on Group | UEditableTextBox |
| 200 | FListLayerMapper | `List_` prefix on Group | UListView |
| 200 | FTileLayerMapper | `Tile_` prefix on Group | UTileView |
| 200 | FSwitcherLayerMapper | `Switcher_` prefix on Group | UWidgetSwitcher |
| 100 | FImageLayerMapper | `EPsdLayerType::Image` | UImage via FTextureImporter |
| 100 | FTextLayerMapper | `EPsdLayerType::Text` | UTextBlock with DPI conversion (x0.75), color, justification |
| 50 | FGroupLayerMapper | `EPsdLayerType::Group` (fallback) | UCanvasPanel |

**FLayerMappingRegistry::RegisterDefaults** registers all 15 and sorts descending by priority so higher-priority prefix mappers are always checked before the generic Group fallback.

## Key Technical Decisions

1. **AllMappers.h pattern**: A single private header declares all 15 mapper classes; each .cpp provides the out-of-line implementations. This avoids ODR violations and keeps class definitions out of public headers (D-01).
2. **SetStyle / SetWidgetStyle**: Used for UButton and UProgressBar respectively — not deprecated direct field assignment (`WidgetStyle =`).
3. **EntryWidgetClass**: UListView and UTileView log an info message that the class must be set manually in the Widget Blueprint, since PSD layer data cannot encode a UE class reference.
4. **DPI conversion**: Text mapper applies `SizePx * 0.75f` (Photoshop 72 DPI → UMG 96 DPI).
5. **Button state detection**: Case-insensitive suffix matching (`_Hovered`, `_Hover`, `_Pressed`, `_Press`, `_Disabled`; anything else → Normal).

## Deviations from Plan

None — plan executed exactly as written.

## Known Stubs

None. All mappers are fully implemented. FTextureImporter integration is real (deferred to FTextureImporter.cpp from Plan 01).

## Self-Check: PASSED

Files created:
- Source/PSD2UMG/Private/Mapper/AllMappers.h — FOUND
- Source/PSD2UMG/Private/Mapper/FImageLayerMapper.cpp — FOUND
- Source/PSD2UMG/Private/Mapper/FTextLayerMapper.cpp — FOUND
- Source/PSD2UMG/Private/Mapper/FGroupLayerMapper.cpp — FOUND
- Source/PSD2UMG/Private/Mapper/FButtonLayerMapper.cpp — FOUND
- Source/PSD2UMG/Private/Mapper/FProgressLayerMapper.cpp — FOUND
- Source/PSD2UMG/Private/Mapper/FSimplePrefixMappers.cpp — FOUND

Commits:
- 6bd0509 — FOUND
- ac8ecc6 — FOUND
- 31e4db1 — FOUND
