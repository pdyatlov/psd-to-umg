---
phase: 07-editor-ui-preview-settings
plan: "04"
subsystem: Mapper, Animation
tags: [commonui, animation, button, input-action, widget-animation]
dependency_graph:
  requires: [07-01]
  provides: [FCommonUIButtonLayerMapper, FPsdWidgetAnimationBuilder]
  affects: [FLayerMappingRegistry, AllMappers.h]
tech_stack:
  added: [CommonUI, EnhancedInput, MovieScene, MovieSceneTracks]
  patterns: [IPsdLayerMapper priority chain, UMovieSceneFloatTrack keyframing, AnimationBindings runtime binding]
key_files:
  created:
    - Source/PSD2UMG/Private/Mapper/FCommonUIButtonLayerMapper.h
    - Source/PSD2UMG/Private/Mapper/FCommonUIButtonLayerMapper.cpp
    - Source/PSD2UMG/Private/Animation/FPsdWidgetAnimationBuilder.h
    - Source/PSD2UMG/Private/Animation/FPsdWidgetAnimationBuilder.cpp
  modified:
    - Source/PSD2UMG/Private/Mapper/AllMappers.h
    - Source/PSD2UMG/Private/Mapper/FLayerMappingRegistry.cpp
decisions:
  - "FCommonUIButtonLayerMapper at priority 210 (FButtonLayerMapper is 200) — higher number wins in registry sort"
  - "CanMap returns false when bUseCommonUI is false so non-CommonUI projects have zero overhead"
  - "Map returns nullptr if CommonUI module not loaded; registry falls through to FButtonLayerMapper"
  - "AnimationBindings.Add used for possessable resolution (no BindPossessableObject per anti-pattern warning in RESEARCH.md)"
metrics:
  duration: "8m"
  completed: "2026-04-10"
  tasks_completed: 2
  files_changed: 6
---

# Phase 7 Plan 04: CommonUI Button Mapper and Animation Builder Summary

**One-liner:** Optional UCommonButtonBase mapper with [IA_Name] input action binding + MovieScene-based _show/_hide/_hover animation builder.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | FCommonUIButtonLayerMapper with input action binding | 75ee377 | FCommonUIButtonLayerMapper.h/.cpp, AllMappers.h, FLayerMappingRegistry.cpp |
| 2 | FPsdWidgetAnimationBuilder for _show/_hide/_hover animations | b7de39c | FPsdWidgetAnimationBuilder.h/.cpp |

## What Was Built

### Task 1 — FCommonUIButtonLayerMapper

- Derives from `IPsdLayerMapper`, priority 210 (FButtonLayerMapper is 200 — registry sorts descending so 210 wins).
- `CanMap()` returns false immediately when `bUseCommonUI == false` — zero overhead for non-CommonUI projects.
- When active, checks `FModuleManager::Get().IsModuleLoaded("CommonUI")`. If not loaded, logs the exact error message required and returns nullptr — the registry falls through to FButtonLayerMapper.
- Copies button state brush logic from FButtonLayerMapper (Normal/Hovered/Pressed/Disabled child image layers).
- Parses `[IA_Name]` bracket suffix from layer name, queries AssetRegistry under `InputActionSearchPath`, calls `SetTriggeringEnhancedInputAction` if found.
- Registered in FLayerMappingRegistry constructor.

### Task 2 — FPsdWidgetAnimationBuilder

- Static helper class with three public methods.
- `CreateOpacityFade()`: Creates UWidgetAnimation with UMovieScene (24000 tick/s, 30fps display), adds `UMovieSceneFloatTrack` targeting `RenderOpacity`, linear keyframes from->to over DurationSec.
- `CreateScaleAnim()`: Same setup with two float tracks for `RenderTransform.Scale.X` and `RenderTransform.Scale.Y`.
- `ProcessAnimationVariants()`: Detects `_show` (fade in 0.3s), `_hide` (fade out 0.3s), `_hover` (scale 1.0→1.05 over 0.15s) suffixes on LayerName.
- Uses `AnimationBindings.Add(FWidgetAnimationBinding{...})` for runtime widget resolution — no `BindPossessableObject` call.

## Deviations from Plan

**1. [Rule 1 - Convention] GetWidgetTypeName not in base interface**
- Found during: Task 1
- Issue: `IPsdLayerMapper` does not declare `GetWidgetTypeName()` as a virtual method. The plan spec listed it as required.
- Fix: Added as non-virtual public method on `FCommonUIButtonLayerMapper` only — no change to the interface needed since callers do not use this method through the interface.

## Known Stubs

None — all functionality is fully wired.

## Self-Check: PASSED

- `Source/PSD2UMG/Private/Mapper/FCommonUIButtonLayerMapper.h` — created
- `Source/PSD2UMG/Private/Mapper/FCommonUIButtonLayerMapper.cpp` — created
- `Source/PSD2UMG/Private/Animation/FPsdWidgetAnimationBuilder.h` — created
- `Source/PSD2UMG/Private/Animation/FPsdWidgetAnimationBuilder.cpp` — created
- Commits 75ee377 and b7de39c — verified in git log
