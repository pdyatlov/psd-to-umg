---
phase: 01-ue5-port
plan: 01
subsystem: plugin-core
tags: [ue5, port, rename, build-system]
requirements: [PORT-01, PORT-02]
dependency-graph:
  requires: []
  provides: [psd2umg-module-skeleton]
  affects: [Source/AutoPSDUI/*, AutoPSDUI.uplugin]
tech-stack:
  added: []
  patterns: [UE5 DeveloperSettings GetCategoryName/GetSectionName overrides]
key-files:
  created: []
  modified:
    - AutoPSDUI.uplugin
    - Source/AutoPSDUI/AutoPSDUI.Build.cs
    - Source/AutoPSDUI/Public/AutoPSDUI.h
    - Source/AutoPSDUI/Private/AutoPSDUI.cpp
    - Source/AutoPSDUI/Public/AutoPSDUILibrary.h
    - Source/AutoPSDUI/Private/AutoPSDUILibrary.cpp
    - Source/AutoPSDUI/Public/AutoPSDUISetting.h
    - Source/AutoPSDUI/Private/AutoPSDUISetting.cpp
decisions: []
metrics:
  duration: ~5m
  completed: 2026-04-08
  tasks: 2
---

# Phase 01 Plan 01: UE5 Port - Rename & API Migration Summary

Rename AutoPSDUI plugin to PSD2UMG and migrate all C++ source, Build.cs, and .uplugin from UE 4.26 to UE 5.7 APIs in a single atomic pass.

## What Was Done

**Task 1 — .uplugin + Build.cs (commit 12b05ce)**
- `AutoPSDUI.uplugin`: FriendlyName/Module Name `PSD2UMG`, EngineVersion `5.7`, `PlatformAllowList` replaces `WhitelistPlatforms`, removed `PythonScriptPlugin` and `EditorScriptingUtilities` from Plugins array, `Installed: false`, cleaned legacy URLs.
- `AutoPSDUI.Build.cs`: Class renamed to `PSD2UMG : ModuleRules`, removed `PythonScriptPlugin` and `EditorScriptingUtilities` private deps (EditorScriptingUtilities folded into `UnrealEd` in UE5).

**Task 2 — C++ source rename + UE5 API fixes (commit fd86560)**
- `FAutoPSDUIModule` -> `FPSD2UMGModule` (header + impl).
- `UAutoPSDUILibrary` -> `UPSD2UMGLibrary`, `AUTOPSDUI_API` -> `PSD2UMG_API`.
- `UAutoPSDUISetting` -> `UPSD2UMGSettings` with `GetCategoryName`/`GetSectionName` overrides (UE5 pattern) and `meta=(DisplayName="PSD2UMG")`.
- `IMPLEMENT_MODULE(FPSD2UMGModule, PSD2UMG)`.
- AssetRegistry include fixed to UE5 path `AssetRegistry/AssetRegistryModule.h`.
- Removed `IPythonScriptPlugin.h` include; `RunPyCmd` stubbed with deprecation `UE_LOG` warning.
- `StartupModule` guards `GEditor`; `ShutdownModule` properly calls `RemoveAll(this)` on the import delegate.
- `OnPSDImport` strips the Python pipeline call and logs a pending-Phase-2 warning while preserving the hook point.

## Deviations from Plan

None - plan executed exactly as written.

## Commits

- `12b05ce` feat(01-01): update .uplugin and Build.cs for UE 5.7
- `fd86560` feat(01-01): rename C++ classes to PSD2UMG and fix UE5 APIs

## Verification

- `grep -r "FAutoPSDUIModule\|UAutoPSDUILibrary\|UAutoPSDUISetting\|AUTOPSDUI_API\|IPythonScriptPlugin" Source/` returns 0 matches.
- All PSD2UMG rename targets present; IMPLEMENT_MODULE uses new name pair.
- Build/compile not validated here (requires UE 5.7 toolchain); this plan is source-level migration only. Next plan validates compile.

## Known Stubs

- `UPSD2UMGLibrary::RunPyCmd` - deprecation log only; intentional per D-16. Native C++ pipeline lands in Phase 2.
- `UPSD2UMGLibrary::ImportImage` - returns nullptr (pre-existing stub, unchanged).
- `FPSD2UMGModule::OnPSDImport` - detects PSDs but logs pending Phase 2 instead of invoking the removed Python pipeline. Intentional.

## Self-Check: PASSED

- FOUND: AutoPSDUI.uplugin
- FOUND: Source/AutoPSDUI/AutoPSDUI.Build.cs
- FOUND: Source/AutoPSDUI/Public/AutoPSDUI.h
- FOUND: Source/AutoPSDUI/Private/AutoPSDUI.cpp
- FOUND: Source/AutoPSDUI/Public/AutoPSDUILibrary.h
- FOUND: Source/AutoPSDUI/Private/AutoPSDUILibrary.cpp
- FOUND: Source/AutoPSDUI/Public/AutoPSDUISetting.h
- FOUND: Source/AutoPSDUI/Private/AutoPSDUISetting.cpp
- FOUND: commit 12b05ce
- FOUND: commit fd86560
