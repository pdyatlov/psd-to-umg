---
phase: 09-unified-layer-naming-convention-tag-based
plan: "01"
subsystem: parser
tags: [parser, grammar, tags, foundation, wave-0]
dependency_graph:
  requires: []
  provides: [TAG-01, TAG-03, TAG-07]
  affects: [PsdParser.cpp, PsdTypes.h]
tech_stack:
  added: []
  patterns:
    - "Pure-function parser with OutDiagnostics out-param (no UE_LOG side effects)"
    - "Static const TMap<FString, EnumT> lookup tables for tag-name dispatch"
    - "FStringView input + FString::ParseIntoArrayWS tokenisation"
    - "Post-pass tree walk to populate ParsedTags after Type is known"
key_files:
  created:
    - Source/PSD2UMG/Private/Parser/FLayerTagParser.h
    - Source/PSD2UMG/Private/Parser/FLayerTagParser.cpp
    - Source/PSD2UMG/Tests/FLayerTagParserSpec.cpp
    - Docs/LayerTagGrammar.md
  modified:
    - Source/PSD2UMG/Public/Parser/PsdTypes.h
    - Source/PSD2UMG/Private/Parser/PsdParser.cpp
decisions:
  - "Spec file placed in Source/PSD2UMG/Tests/ (project convention) instead of Private/Tests/ (plan-specified path); all existing specs live under Tests/."
  - "@9s shorthand defaults to {16,16,16,16, bExplicit=false} matching F9SliceImageLayerMapper's historical default (D-07 planner discretion)."
  - "@smartobject:TypeName also sets Type=SmartObject for uniform mapper dispatch (forward-looking; no current consumer reads SmartObjectTypeName)."
  - "ParsedTags populated as a post-pass after ConvertLayerRecursive completes, so the D-02 default-type inference sees the final EPsdLayerType."
metrics:
  duration: ~25m
  completed: "2026-04-14"
  tasks: 2
  files: 6
requirements: [TAG-01, TAG-03, TAG-07]
---

# Phase 9 Plan 01: FLayerTagParser Foundation Summary

**One-liner:** Central `FLayerTagParser` parses `Name @tag @tag:value` PSD layer names into a strongly-typed `FParsedLayerTags` struct, populated once on `FPsdLayer::ParsedTags` during `PsdParser::ParseFile` and consumable by every downstream mapper, anchor calculator, and animation builder.

## What Was Built

### Task 1 -- Types, ParsedTags field, grammar doc

- `Source/PSD2UMG/Private/Parser/FLayerTagParser.h` (NEW): declares
  `EPsdTagType`, `EPsdAnchorTag`, `EPsdStateTag`, `EPsdAnimTag` enums,
  `FPsdNineSliceMargin` struct, `FParsedLayerTags` value object, and the
  `FLayerTagParser` class (`Parse` + `FindChildByState` static methods).
  Forward-declares `FPsdLayer` / `EPsdLayerType` to avoid circular includes.
- `Source/PSD2UMG/Public/Parser/PsdTypes.h`: adds
  `#include "Parser/FLayerTagParser.h"` and a new `FParsedLayerTags ParsedTags;`
  field on `FPsdLayer` (placed after `Children`).
- `Docs/LayerTagGrammar.md` (NEW, D-22 deliverable): formal EBNF, full tag
  inventory table, case-sensitivity rules (D-04/D-14), conflict resolution
  (D-17/D-19), unknown-tag handling (D-18), name-extraction rules
  (D-20/D-21), `@9s` shorthand default documented, 14 worked examples.

### Task 2 -- Parse implementation, parser integration, unit spec

- `Source/PSD2UMG/Private/Parser/FLayerTagParser.cpp` (NEW): full
  `Parse` implementation with static `TMap<FString, EnumT>` lookup tables
  for type/anchor/state/anim names, `ApplyToken` per-tag dispatch, conflict
  + unknown diagnostic capture, `@9s` shorthand (16px default) + 4-comma
  explicit form, `@ia` / `@smartobject` value case-preservation,
  `@variants` modifier, D-02 default-type inference, D-21 empty-name
  fallback. `FindChildByState` does D-12 explicit match then D-13
  first-untagged-Image fallback for Normal.
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp`: adds
  `Internal::PopulateParsedTagsRecursive` post-pass invoked from
  `ParseFile` after `ConvertLayerRecursive` completes for the root layers.
  Diagnostics emitted to `LogPSD2UMG` with the slash-delimited layer path
  and added to `FPsdParseDiagnostics` as warnings.
- `Source/PSD2UMG/Tests/FLayerTagParserSpec.cpp` (NEW): pure-unit spec
  `PSD2UMG.Parser.LayerTagParser` covering D-01..D-21 plus `@variants`,
  `@state`, `@anim`, `@ia`, and `FindChildByState` (D-12/D-13). Headless
  -- no fixture PSD or UE asset deps.

## API Shipped

```cpp
struct FParsedLayerTags {
    FString CleanName;
    EPsdTagType Type = EPsdTagType::None;
    EPsdAnchorTag Anchor = EPsdAnchorTag::None;
    EPsdStateTag State = EPsdStateTag::None;
    EPsdAnimTag Anim = EPsdAnimTag::None;
    TOptional<FPsdNineSliceMargin> NineSlice;
    TOptional<FString> InputAction;
    TOptional<FString> SmartObjectTypeName;
    bool bIsVariants = false;
    TArray<FString> UnknownTags;
    bool HasType() const;
};

class FLayerTagParser {
public:
    static FParsedLayerTags Parse(FStringView LayerName, EPsdLayerType SourceLayerType,
                                  int32 LayerIndex, FString& OutDiagnostics);
    static const FPsdLayer* FindChildByState(const TArray<FPsdLayer>& Children,
                                             EPsdStateTag DesiredState);
};
```

## Spec Suite

- Suite name: `PSD2UMG.Parser.LayerTagParser`
- Run: `UnrealEditor-Cmd.exe <HostProject> -ExecCmds="Automation RunTests PSD2UMG.Parser.LayerTagParser; quit" -unattended -nopause -log`

## Commits

| Task | Commit | Description |
|------|--------|-------------|
| 1 | 29162b6 | feat(09-01): add FLayerTagParser types, ParsedTags field, grammar doc |
| 1 | 09dc52b | docs(09-01): add LayerTagGrammar.md (D-22 deliverable) |
| 2 | 788f6e9 | test(09-01): add FLayerTagParserSpec covering D-01..D-21 grammar |
| 2 | 5e02922 | feat(09-01): implement FLayerTagParser::Parse + FindChildByState |
| 2 | 9526d0d | feat(09-01): populate FPsdLayer.ParsedTags in PsdParser::ParseFile |

## Deviations from Plan

### [Rule 3 - Blocking issue] Spec file location

- **Found during:** Task 2
- **Issue:** Plan specified `Source/PSD2UMG/Private/Tests/FLayerTagParserSpec.cpp`, but no `Private/Tests/` directory exists in the module; all existing specs live under `Source/PSD2UMG/Tests/` (PsdParserSpec, FWidgetBlueprintGenSpec, FFontResolverSpec, FTextPipelineSpec). UBT auto-discovers Tests/ as a module source folder.
- **Fix:** Placed new spec at `Source/PSD2UMG/Tests/FLayerTagParserSpec.cpp` to follow the established project convention.
- **Files modified:** `Source/PSD2UMG/Tests/FLayerTagParserSpec.cpp`
- **Commit:** 788f6e9

### [Out of scope -- documented, not blocking] No host project for build/test verification

- **Found during:** Task 1 verification step.
- **Issue:** This repo is plugin-only -- no `*.uproject` exists. The plan's `verify` commands assume a host UE project (`-Project="C:/Dev/psd-to-umg/PSD2UMG.uproject"`) which does not exist. Prior plans (08-02, 08-03) ran without local build/test verification as well.
- **Resolution:** Implementation followed the canonical UE 5.7 + C++20 idioms (`FStringView`, `TOptional`, `FString::ParseIntoArrayWS`, `BEGIN_DEFINE_SPEC`) used throughout the codebase. Build and test verification will run when this branch is consumed by a host project (CI deferred to post-v1 per Phase 8 D-06).
- **Tracking:** Surfaced as a known limitation; not a behavioural deviation.

## Self-Check: PASSED

- Source/PSD2UMG/Private/Parser/FLayerTagParser.h -- exists
- Source/PSD2UMG/Private/Parser/FLayerTagParser.cpp -- exists
- Source/PSD2UMG/Tests/FLayerTagParserSpec.cpp -- exists
- Docs/LayerTagGrammar.md -- exists
- Source/PSD2UMG/Public/Parser/PsdTypes.h -- modified (ParsedTags field + include)
- Source/PSD2UMG/Private/Parser/PsdParser.cpp -- modified (PopulateParsedTagsRecursive post-pass)
- Commits 29162b6, 09dc52b, 788f6e9, 5e02922, 9526d0d all present in git log
