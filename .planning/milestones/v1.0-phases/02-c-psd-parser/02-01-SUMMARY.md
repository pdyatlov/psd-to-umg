---
phase: 02-c-psd-parser
plan: 01
subsystem: build-linkage
tags: [thirdparty, photoshopapi, build-cs, vendoring]
status: complete
completed: 2026-04-08
requires: []
provides:
  - "ThirdParty PhotoshopAPI external module wired into PSD2UMG"
  - "RTTI + exceptions + C++20 enabled on PSD2UMG module"
  - "Win64-only .uplugin (Mac removed)"
affects:
  - PSD2UMG.uplugin
  - Source/PSD2UMG/PSD2UMG.Build.cs
tech-stack:
  added: [PhotoshopAPI (vendored static lib), openimageio, fmt, libdeflate, stduuid, eigen3]
  patterns: [PIMPL-ready external ThirdParty module, vcpkg x64-windows-static-md triplet]
key-files:
  created:
    - Source/ThirdParty/PhotoshopAPI/PhotoshopAPI.Build.cs
    - Source/ThirdParty/PhotoshopAPI/Win64/include/.gitkeep
    - Source/ThirdParty/PhotoshopAPI/Win64/lib/.gitkeep
    - Source/ThirdParty/PhotoshopAPI/LICENSES/README.md
    - Scripts/build-photoshopapi.bat
  modified:
    - Source/PSD2UMG/PSD2UMG.Build.cs
    - PSD2UMG.uplugin
decisions:
  - "Vendored .libs committed under Win64/lib (resilient enumeration via Directory.GetFiles)"
  - "vcpkg triplet x64-windows-static-md (UE links /MD)"
  - "PhotoshopAPI declared private dep — never leaks through public headers"
metrics:
  duration: ~human-verify cycle
  tasks: 2 + checkpoint
  commits: 4
---

# Phase 2 Plan 01: Vendor PhotoshopAPI + Wire Build Linkage Summary

One-liner: Vendored PhotoshopAPI as a pre-built static lib (vcpkg x64-windows-static-md), wired the PSD2UMG module with RTTI/exceptions/C++20, and dropped Mac from PlatformAllowList — UE 5.7.4 Win64 build links cleanly.

## What Shipped

- **ThirdParty external module** at `Source/ThirdParty/PhotoshopAPI/` exposing include + lib paths via `ModuleType.External` Build.cs that auto-enumerates `*.lib` under `Win64/lib/`.
- **PSD2UMG.Build.cs** updated: `bUseRTTI = true`, `bEnableExceptions = true`, `CppStandard = Cpp20`, `PhotoshopAPI` added to `PrivateDependencyModuleNames`.
- **PSD2UMG.uplugin** PlatformAllowList reduced to `["Win64"]` (Mac removed per CONTEXT.md D-Uplugin-Cleanup).
- **Bootstrap script** `Scripts/build-photoshopapi.bat` documents the one-time CMake/vcpkg rebuild flow.

## Commits

| Hash      | Message                                                                  |
| --------- | ------------------------------------------------------------------------ |
| `b12760b` | chore(02-01): vendor PhotoshopAPI ThirdParty module scaffolding          |
| `a3836df` | feat(02-01): wire PSD2UMG to PhotoshopAPI, enable RTTI/exceptions/C++20  |
| `e8763e2` | fix(02-01): re-add EditorScriptingUtilities dep + bootstrap script fixes |
| (this)    | docs(02-01): complete vendor-photoshopapi plan                           |

## Verification

Human-verified at the blocking checkpoint:
- Build target `PSD2UMGEditor` (Development Editor, Win64) links cleanly with zero unresolved-external errors.
- Editor launches; plugin appears in **Edit → Plugins → PSD2UMG** with no load errors.
- `.psd` import still emits the Phase 1 stub log message — expected, factory comes in 02-04.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Re-added `EditorScriptingUtilities` to PrivateDependencyModuleNames**
- **Found during:** Human-verify checkpoint (linker error)
- **Issue:** `PSD2UMGLibrary::CompileAndSaveBP` calls `UEditorAssetLibrary::SaveLoadedAsset`, which lives in the `EditorScriptingUtilities` module. Phase 1 carry-over: dep was dropped during the AutoPSDUI → PSD2UMG rename and the legacy stub no longer linked.
- **Fix:** Re-added `"EditorScriptingUtilities"` to `PrivateDependencyModuleNames` with a comment noting the stub is preserved through Phase 2 and removed in Phase 6+.
- **Files modified:** `Source/PSD2UMG/PSD2UMG.Build.cs`
- **Commit:** `e8763e2`

**2. [Rule 1 - Bug] Patched `Scripts/build-photoshopapi.bat`**
- **Found during:** Human-verify checkpoint (script execution path)
- **Issue:** Bootstrap script had path / argument issues uncovered when actually run.
- **Fix:** Bundled with the EditorScriptingUtilities fix in the same commit for atomicity.
- **Files modified:** `Scripts/build-photoshopapi.bat`
- **Commit:** `e8763e2`

## Requirements Satisfied

- **PRSR-01** — PhotoshopAPI integrated as pre-built static lib via Build.cs (Win64) with all transitive dependencies linked.
- **PRSR-07** — No Python dependency at plugin runtime (Phase 1 work preserved; nothing reintroduced).

## Self-Check: PASSED

- All 4 commits present in `git log`
- `Source/PSD2UMG/PSD2UMG.Build.cs` contains `PhotoshopAPI`, `bUseRTTI`, `bEnableExceptions`, `Cpp20`, `EditorScriptingUtilities`
- `Source/ThirdParty/PhotoshopAPI/` scaffolding present
- Build verified by user at checkpoint
