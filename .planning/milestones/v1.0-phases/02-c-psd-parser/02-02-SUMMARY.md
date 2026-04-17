---
phase: 02-c-psd-parser
plan: 02
subsystem: parser-contract
tags: [headers, contract, log-category, diagnostics, ustruct]
status: complete
completed: 2026-04-08
requires: [02-01]
provides:
  - "LogPSD2UMG log category usable from any PSD2UMG translation unit"
  - "FPsdDocument / FPsdLayer / FPsdTextRun / EPsdLayerType public contract types"
  - "FPsdParseDiagnostics accumulator with HasErrors / AddInfo / AddWarning / AddError"
affects: []
tech-stack:
  added: []
  patterns: [PIMPL contract headers, USTRUCT POD types, header-only diagnostics struct]
key-files:
  created:
    - Source/PSD2UMG/Public/PSD2UMGLog.h
    - Source/PSD2UMG/Private/PSD2UMGLog.cpp
    - Source/PSD2UMG/Public/Parser/PsdTypes.h
    - Source/PSD2UMG/Public/Parser/PsdDiagnostics.h
  modified: []
decisions:
  - "FPsdParseDiagnostics is plain C++ (no USTRUCT) — never crosses Blueprint boundary"
  - "Diagnostics methods inlined in header (trivial bodies, no extra .cpp)"
  - "ETextJustify pulled from Framework/Text/TextLayout.h (Slate-level enum)"
  - "Parser headers live under Public/Parser/ for namespace-by-folder discoverability"
metrics:
  duration: ~5m
  tasks: 2
  commits: 3
---

# Phase 2 Plan 02: Parser Contract Types Summary

One-liner: Defined the public POD contract (FPsdDocument / FPsdLayer / FPsdTextRun / FPsdParseDiagnostics) and LogPSD2UMG category that plan 02-03's parser will populate — zero third-party types leak through PSD2UMG public headers.

## What Shipped

- **`PSD2UMGLog.h` / `PSD2UMGLog.cpp`** — `DECLARE_LOG_CATEGORY_EXTERN(LogPSD2UMG, Log, All)` + `DEFINE_LOG_CATEGORY`. Available to every PSD2UMG translation unit.
- **`Parser/PsdTypes.h`** — `EPsdLayerType` enum + three USTRUCTs (`FPsdTextRun`, `FPsdLayer`, `FPsdDocument`) exported with `PSD2UMG_API`. Matches the `<interfaces>` block from the plan exactly.
- **`Parser/PsdDiagnostics.h`** — Plain C++ `EPsdDiagnosticSeverity`, `FPsdDiagnostic`, `FPsdParseDiagnostics` with inline `HasErrors / AddInfo / AddWarning / AddError`.

## Commits

| Hash      | Message                                                                       |
| --------- | ----------------------------------------------------------------------------- |
| `b4fbf7d` | feat(02-02): declare LogPSD2UMG log category                                  |
| `7236246` | feat(02-02): add FPsdDocument/FPsdLayer/FPsdTextRun + diagnostics contract types |
| (this)    | docs(02-02): complete parser contract types plan                              |

## Verification

- `grep -q "DECLARE_LOG_CATEGORY_EXTERN(LogPSD2UMG"` → matches in `Public/PSD2UMGLog.h`
- `grep -q "DEFINE_LOG_CATEGORY(LogPSD2UMG"` → matches in `Private/PSD2UMGLog.cpp`
- `grep -q "struct PSD2UMG_API FPsdDocument"` → matches in `Public/Parser/PsdTypes.h`
- `grep -q "struct PSD2UMG_API FPsdParseDiagnostics"` → matches in `Public/Parser/PsdDiagnostics.h`
- No literal `PhotoshopAPI` token anywhere under `Source/PSD2UMG/Public/`
- Build verification deferred to plan 02-03 (which is the first plan that actually exercises these headers from a `.cpp`).

## Deviations from Plan

None — plan executed exactly as written. The plan called for an option to inline `FPsdParseDiagnostics` methods or split into a `.cpp`; chose inline (simpler, plan-sanctioned).

## Requirements Satisfied (partial)

- **PRSR-02** — `FPsdDocument` / `FPsdLayer` shape exists; population comes in 02-03.
- **PRSR-03** — `FPsdLayer::RGBAPixels` field exists; population comes in 02-03.
- **PRSR-04** — `FPsdTextRun` shape exists; population comes in 02-03.
- **PRSR-05** — `FPsdLayer::Children` exists; hierarchy preservation implemented in 02-03.

These requirements are not yet checked off in REQUIREMENTS.md — they flip to complete after plan 02-03 lands the parser implementation that fills these structs.

## Self-Check: PASSED

- All 4 source files exist on disk
- Both task commits present in `git log` (`b4fbf7d`, `7236246`)
- No `PhotoshopAPI` literal under `Source/PSD2UMG/Public/`
