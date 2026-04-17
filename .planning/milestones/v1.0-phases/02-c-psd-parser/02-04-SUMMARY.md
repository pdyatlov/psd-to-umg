---
phase: 02-c-psd-parser
plan: 04
subsystem: factory
tags: [factory, ufactory, import, wrapper-mode]
requirements: [PRSR-06, PRSR-07]
dependency_graph:
  requires:
    - Source/PSD2UMG/Public/Parser/PsdParser.h
    - Source/PSD2UMG/Public/Parser/PsdTypes.h
    - Source/PSD2UMG/Public/Parser/PsdDiagnostics.h
    - Source/PSD2UMG/Public/PSD2UMGLog.h
  provides:
    - "UPsdImportFactory as the .psd import entry point (ImportPriority = DefaultImportPriority + 100)"
  affects:
    - Phase 02 plan 05 (automation spec will drive the factory path indirectly via manual drop test)
    - Phase 03 (WBP generation plugs into UPsdImportFactory::FactoryCreateBinary)
tech-stack:
  added:
    - "UTextureFactory forwarded via NewObject<UTextureFactory>() (wrapper mode)"
  patterns:
    - "Wrapper-mode UFactory: outranks UTextureFactory, forwards to it, then runs parser on same bytes"
    - "In-memory import buffer spilled to ProjectIntermediate/PSD2UMG/*.psd for PhotoshopAPI path-only API"
key-files:
  created:
    - Source/PSD2UMG/Public/Factories/PsdImportFactory.h
    - Source/PSD2UMG/Private/Factories/PsdImportFactory.cpp
  modified:
    - Source/PSD2UMG/Private/PSD2UMG.cpp
    - Source/PSD2UMG/Public/PSD2UMG.h
decisions:
  - "ImportPriority delta = +100 over UTextureFactory's DefaultImportPriority. Gives us clear precedence without claiming INT32_MAX (which could mask future higher-priority third parties)."
  - "Buffer is copied once into a TArray<uint8>, written via FFileHelper::SaveArrayToFile to FPaths::CreateTempFilename under FPaths::ProjectIntermediateDir()/PSD2UMG. UE's intermediate dir is gitignored and writable; we create the subdir on demand and delete the spill file immediately after ParseFile returns."
  - "Empty/null buffer short-circuits parser and returns the wrapped UTexture2D result -- the factory must never fail the import if the parser can't run."
  - "PSD2UMGLibrary.{h,cpp} preserved untouched per 02-CONTEXT.md <specifics>; only the #include was dropped from PSD2UMG.cpp along with Editor.h, Subsystems/ImportSubsystem.h, EditorFramework/AssetImportData.h, PSD2UMGSetting.h, and AssetRegistry/AssetRegistryModule.h -- all were consumed only by the removed OnPSDImport hook."
  - "Factory is auto-discovered by UE via UFactory CDO reflection; no explicit registration needed in StartupModule. StartupModule now only emits a single info log line."
metrics:
  duration: "~8m"
  tasks: 2
  completed: "2026-04-08"
---

# Phase 2 Plan 4: UPsdImportFactory Summary

One-liner: Wrapper-mode UFactory claims .psd at ImportPriority+100, forwards to UTextureFactory for the flat UTexture2D, spills bytes to an Intermediate temp file, and runs PSD2UMG::Parser::ParseFile for a LogPSD2UMG summary — removing the Phase 1 OnAssetReimport hook entirely.

## What Shipped

- `UPsdImportFactory : UFactory` in `Source/PSD2UMG/Public/Factories/PsdImportFactory.h` + `Source/PSD2UMG/Private/Factories/PsdImportFactory.cpp`.
  - `SupportedClass = UTexture2D::StaticClass()`, `Formats = { "psd;Photoshop Document" }`, `bEditorImport = true`, `bText = false`, `ImportPriority = DefaultImportPriority + 100`.
  - `FactoryCreateBinary`:
    1. Captures original `Buffer` / `BufferEnd` before forwarding.
    2. Instantiates an inner `UTextureFactory` via `NewObject<UTextureFactory>(GetTransientPackage(), ...)`, roots it for the duration of the forwarded call, forwards with a local advancing pointer so the inner factory can consume without clobbering our copy.
    3. Creates `ProjectIntermediate/PSD2UMG/` on demand, writes the bytes via `FFileHelper::SaveArrayToFile` to a temp path from `FPaths::CreateTempFilename`.
    4. Calls `PSD2UMG::Parser::ParseFile(TempPath, Doc, Diag)`.
    5. Deletes the temp file.
    6. Tallies `WarningCount` / `ErrorCount` from `Diag.Entries` and emits a single `LogPSD2UMG` summary line on success (canvas size + root layer count + counts). On failure, logs the error plus every diagnostic entry.
    7. Returns the wrapped `UTexture2D*` so the engine sees no behavioural regression vs Phase 1.
- `FPSD2UMGModule`:
  - `StartupModule`: reduced to a single `UE_LOG(LogPSD2UMG, Log, TEXT("PSD2UMG module loaded"));`. No delegate subscription.
  - `ShutdownModule`: empty.
  - `OnPSDImport` method deleted from both header and cpp.
  - Includes removed from `PSD2UMG.cpp`: `Editor.h`, `Subsystems/ImportSubsystem.h`, `EditorFramework/AssetImportData.h`, `PSD2UMGSetting.h`, `PSD2UMGLibrary.h`, `AssetRegistry/AssetRegistryModule.h`. Added `PSD2UMGLog.h` for the startup log line.

## ImportPriority Rationale

`DefaultImportPriority + 100` was chosen over `INT32_MAX` (per Claude's discretion in the plan). UTextureFactory sits at `DefaultImportPriority`, so +100 is a comfortable margin without permanently crowding out any future higher-priority importer (engine or third party). If another plugin ever wants to supersede us, they can simply add +200.

## Temp-File IO vs In-Memory Parser Input

Plan 02-03 established that `PhotoshopAPI::LayeredFile<bpp8_t>::read` only takes `const std::filesystem::path&` — no stream/buffer overload exists. Plan 02-04's factory receives `const uint8*& Buffer, const uint8* BufferEnd` from UE. The bridge:

- Target dir: `FPaths::ConvertRelativePathToFull(FPaths::ProjectIntermediateDir() / TEXT("PSD2UMG"))`. Intermediate is gitignored and writable; using a subdir keeps collision risk low.
- Temp filename via `FPaths::CreateTempFilename(*TempDir, TEXT("PSD2UMG_"), TEXT(".psd"))`.
- `TArray<uint8>` copy is O(n) but negligible relative to PhotoshopAPI's parse cost on any real PSD.
- Spill file deleted immediately after `ParseFile` returns (both success and failure paths). Best-effort — a crash in the parser would leak the file, which is acceptable since the Intermediate dir is disposable.

## Includes Removed From PSD2UMG.cpp

Exact list (all were consumed solely by the deleted `OnPSDImport` hook):

1. `Editor.h`
2. `Subsystems/ImportSubsystem.h`
3. `EditorFramework/AssetImportData.h`
4. `PSD2UMGSetting.h`
5. `PSD2UMGLibrary.h`
6. `AssetRegistry/AssetRegistryModule.h`

Added: `PSD2UMGLog.h` (for `LogPSD2UMG` used by the new startup log line).

`PSD2UMGLibrary.{h,cpp}` themselves are **untouched** — the deprecated `RunPyCmd` stub lives on per 02-CONTEXT.md `<specifics>` until Phase 6+.

## Buffer Forwarding Subtlety

UE's `FactoryCreateBinary` takes `const uint8*& Buffer` by reference and the inner factory may advance it. We capture `OriginalBegin`/`OriginalEnd` up front and forward a *local* `ForwardBuffer = OriginalBegin` pointer into `Inner->FactoryCreateBinary`, so our subsequent temp-file spill always sees the full buffer regardless of what UTextureFactory did to its copy. This is easy to miss and would otherwise produce a silently-truncated spill file.

## Deviations from Plan

None. Structure matches the plan's pseudo-code verbatim. The one clarification worth recording is the forwarding-pointer copy described above, which the plan left implicit.

## Commits

- `c92db5b` feat(02-04): add UPsdImportFactory wrapper-mode factory
- `38565fc` refactor(02-04): remove OnAssetReimport hook; factory replaces it
- `631da2b` chore(02-04): tighten PSD2UMG.cpp comment to avoid stale symbol references

## Verification

- `grep -q "class UPsdImportFactory" Source/PSD2UMG/Public/Factories/PsdImportFactory.h` — matches.
- `grep -q "UTextureFactory" Source/PSD2UMG/Private/Factories/PsdImportFactory.cpp` — matches.
- `grep -q "PSD2UMG::Parser::ParseFile" Source/PSD2UMG/Private/Factories/PsdImportFactory.cpp` — matches.
- `grep -q "ImportPriority" Source/PSD2UMG/Private/Factories/PsdImportFactory.cpp` — matches.
- `grep "OnAssetReimport\|OnPSDImport" Source/PSD2UMG/Private/PSD2UMG.cpp` — no match after comment tightening.
- Runtime verification (automation spec + manual drop) is owned by plan 02-05.

## Self-Check: PASSED

- FOUND: Source/PSD2UMG/Public/Factories/PsdImportFactory.h
- FOUND: Source/PSD2UMG/Private/Factories/PsdImportFactory.cpp
- FOUND: Source/PSD2UMG/Private/PSD2UMG.cpp (modified)
- FOUND: Source/PSD2UMG/Public/PSD2UMG.h (modified)
- FOUND commit c92db5b
- FOUND commit 38565fc
- FOUND commit 631da2b
