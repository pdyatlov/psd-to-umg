---
phase: 16-rich-text-multiple-text-runs
plan: 03
subsystem: ui
tags: [URichTextBlock, UDataTable, FRichTextStyleRow, rich-text, FTextLayerMapper, mapper-pipeline, inline-markup]

# Dependency graph
requires:
  - phase: 16-02
    provides: ExtractSingleRunText multi-run extraction populating FPsdTextRun::Spans (Spans.Num() > 1 for multi-style layers)
  - phase: 12-text-property-fidelity
    provides: FTextLayerMapper FFontResolver + DPI formula (SizePx * 0.75f) reused verbatim for per-span FTextBlockStyle
provides:
  - FRichTextLayerMapper declared in AllMappers.h, defined in FRichTextLayerMapper.cpp — emits URichTextBlock for Spans.Num() > 1
  - Persistent UDataTable companion asset (Default + s0..sN rows, FRichTextStyleRow::StaticStruct) sibling to WBP
  - Inline markup string <s0>...</><s1>...</> with HTML-escaped span text
  - FTextLayerMapper::CanMap narrowed to Spans.Num() <= 1 — mutual exclusion enforced
  - Priority 110 registration in FLayerMappingRegistry (above FTextLayerMapper at 100)
affects: [Phase 17 font matching, any future per-span shadow/outline expansion]

# Tech tracking
tech-stack:
  added: [URichTextBlock (Components/RichTextBlock.h), UDataTable (Engine/DataTable.h), FRichTextStyleRow]
  patterns:
    - FSmartObjectImporter::GetCurrentPackagePath thread-local reused by FRichTextLayerMapper to obtain WBP package path for DataTable sibling placement
    - CreatePackage + NewObject(RF_Public|RF_Standalone) + UEditorAssetLibrary::SaveLoadedAsset — same persistence pattern as FTextureImporter::ImportLayer
    - HTML-escape order: & first then < > to prevent double-escaping
    - Mutual-exclusion mapper pair: CanMap predicates are complementary (Spans.Num() <= 1 vs > 1) regardless of priority ordering

key-files:
  created:
    - Source/PSD2UMG/Private/Mapper/FRichTextLayerMapper.cpp
  modified:
    - Source/PSD2UMG/Private/Mapper/AllMappers.h
    - Source/PSD2UMG/Private/Mapper/FLayerMappingRegistry.cpp
    - Source/PSD2UMG/Private/Mapper/FTextLayerMapper.cpp

key-decisions:
  - "FRichTextLayerMapper at priority 110 (above FTextLayerMapper 100); descending-priority sort in FLayerMappingRegistry ensures multi-run layers always route to FRichTextLayerMapper first"
  - "FTextLayerMapper::CanMap narrowed symmetrically to Spans.Num() <= 1 — mutual exclusion holds even if priorities are changed later"
  - "DataTable package path: {WbpPackagePath}/{CleanName}_RichStyles/DT_{CleanName}_RichStyles — sibling sub-folder avoids Pitfall 6 package collision with WBP asset itself"
  - "Default DataTable row built from FPsdTextRun dominant-run scalars; per-span rows s0..sN built from FPsdTextRunSpan fields — zero-indexed matching Spans array order"
  - "Per-span ShadowOffset/OutlineSize deferred to future phase; FTextBlockStyle Default row captures dominant-run shadow/outline values (out-of-scope Phase 16 closure)"
  - "Row deduplication (Open Question 3) deferred: sequential s0/s1/s2 names sufficient for Phase 16 functional delivery"
  - "FRichTextStyleRow directly in Components/RichTextBlock.h (UE5.7) — no extra include needed"
  - "FSmartObjectImporter::GetCurrentPackagePath() thread-local reused (already declared public in header from Phase 6)"

patterns-established:
  - "Mapper mutual exclusion via complementary CanMap predicates: single-run (Spans.Num() <= 1) -> UTextBlock, multi-run (Spans.Num() > 1) -> URichTextBlock"
  - "DataTable companion asset: created as sibling sub-package, persisted before Map() returns, pointer passed to RTB->SetTextStyleSet()"
  - "Thread-local WBP path (FSmartObjectImporter::GetCurrentPackagePath) used by any mapper that needs to create sibling assets — established pattern from Phase 6"

requirements-completed: [RICH-01, RICH-02]

# Metrics
duration: continuation (Tasks 1-3 prior session; Task 4 human-verify confirmed by user 2026-04-22)
completed: 2026-04-22
---

# Phase 16 Plan 03: FRichTextLayerMapper + Persistent DataTable Asset (Wave 3) Summary

**FRichTextLayerMapper at priority 110 emits URichTextBlock with persistent UDataTable companion asset for multi-run text layers; 9/9 spec It() blocks green; DataTable survives editor restart**

## Performance

- **Duration:** Continuation — Tasks 1-3 in prior session; Task 4 (human-verify) confirmed by user 2026-04-22
- **Started:** 2026-04-22
- **Completed:** 2026-04-22
- **Tasks:** 4 (3 auto + 1 human-verify checkpoint)
- **Files modified:** 4

## Accomplishments

- Declared `FRichTextLayerMapper` in `AllMappers.h` and implemented 249-line `FRichTextLayerMapper.cpp` — new mapper emits `URichTextBlock` + persistent `UDataTable` for any text layer with `Spans.Num() > 1`
- Registered `FRichTextLayerMapper` at priority 110 in `FLayerMappingRegistry::RegisterDefaults`; narrowed `FTextLayerMapper::CanMap` to `Spans.Num() <= 1` for explicit mutual exclusion
- UE Session Frontend `Source.PSD2UMG.Typography.Pipeline`: 9/9 green (2 Typography + 4 RichText Spans + 3 RichText URichTextBlock); full `Source.PSD2UMG` suite green with no regression
- Visual verification in WBP Designer: `text_two_colors` renders `RedWord` in red + `GreenWord` in green; `text_bold_normal` renders `BoldPart` bold + `NormalPart` normal weight — both as `URichTextBlock`
- DataTable persistence confirmed: mixed-color/mixed-weight rendering survives editor close + reopen (`RF_Public|RF_Standalone` + `UEditorAssetLibrary::SaveLoadedAsset` pattern)

## Task Commits

Each task was committed atomically:

1. **Task 1: Declare FRichTextLayerMapper in AllMappers.h + narrow FTextLayerMapper predicate** - `fb6f0f3` (feat)
2. **Task 2: Implement FRichTextLayerMapper.cpp (URichTextBlock + DataTable asset)** - `c7c40a5` (feat)
3. **Task 3: Register FRichTextLayerMapper in FLayerMappingRegistry** - `97c23a5` (feat)
4. **Task 4: Confirm GREEN state + visual URichTextBlock rendering** - human-verify checkpoint (no code commit; confirmed by user)

## Files Created/Modified

- `Source/PSD2UMG/Private/Mapper/FRichTextLayerMapper.cpp` (249 lines, new) — `GetPriority()`=110, `CanMap()` (Spans.Num()>1), `Map()` constructing `URichTextBlock` via `Tree->ConstructWidget`, creating persistent `UDataTable` (Default + s0..sN rows), building `<s0>...</><s1>...</>` markup with HTML-escape, setting `SetTextStyleSet` + `SetText`
- `Source/PSD2UMG/Private/Mapper/AllMappers.h` (209 lines) — `FRichTextLayerMapper` class declaration added alongside `FTextLayerMapper` (priority 110 comment, IPsdLayerMapper override signatures)
- `Source/PSD2UMG/Private/Mapper/FLayerMappingRegistry.cpp` (69 lines) — `MakeUnique<FRichTextLayerMapper>()` registered immediately before `MakeUnique<FTextLayerMapper>()`; descending-priority sort places FRichTextLayerMapper (110) before FTextLayerMapper (100) in MapLayer iteration
- `Source/PSD2UMG/Private/Mapper/FTextLayerMapper.cpp` (138 lines) — `CanMap` narrowed to `Layer.ParsedTags.Type == EPsdTagType::Text && Layer.Text.Spans.Num() <= 1`

## Decisions Made

- FRichTextLayerMapper registered at priority 110, FTextLayerMapper remains at 100; CanMap predicates are complementary so the two mappers are mutually exclusive regardless of registry order
- DataTable placed at `{WbpPackagePath}/{CleanName}_RichStyles/DT_{CleanName}_RichStyles` (sibling sub-folder) to prevent Pitfall 6 package-path collision with the WBP asset
- Default DataTable row carries dominant-run scalar values; s0..sN rows carry per-span FPsdTextRunSpan values; all use the same `FFontResolver::Resolve` + `SizePx * 0.75f` DPI formula as `FTextLayerMapper`
- `FSmartObjectImporter::GetCurrentPackagePath()` thread-local reused to pass WBP package path into the mapper without changing the `Map()` signature

## Deviations from Plan

None — plan executed exactly as written. The `GetCurrentPackagePath` accessor was already public (established in Phase 6); no additional exposure was needed.

## Issues Encountered

None — build was clean, all 9 spec It() blocks green on first run after Task 3 registration, DataTable persistence confirmed without any retry.

## Known Stubs

None — both `text_two_colors` and `text_bold_normal` are fully wired: DataTable created and saved, `SetTextStyleSet` called, markup string set on the URichTextBlock, rendering confirmed visually and post-restart.

## Deferred Items

- **Per-span ShadowOffset / OutlineSize**: `FTextBlockStyle` per-span rows carry only Font + Color for Phase 16 scope. Shadow/outline remain on the dominant-run `Default` row. A future phase can extend `FRichTextStyleRow` rows with `ShadowOffset`/`ShadowColor`/`OutlineSettings` if per-span effects are required.
- **DataTable row deduplication** (Open Question 3 from 16-RESEARCH): Two spans with identical style produce two separate s0/s1 rows. Low priority; functional delivery is complete. Future plan can deduplicate rows and merge tag references in the markup string.

## Implementation Details

### FLayerMappingRegistry Registration Order (post-sort by priority descending)

```
FCommonUIButtonLayerMapper  priority 210
FVariantsSuffixMapper       priority 200
FButtonLayerMapper          priority 200
FProgressLayerMapper        priority 200
FHBoxLayerMapper            priority 200
FVBoxLayerMapper            priority 200
FOverlayLayerMapper         priority 200
FScrollBoxLayerMapper       priority 200
FSliderLayerMapper          priority 200
FCheckBoxLayerMapper        priority 200
FInputLayerMapper           priority 200
FListLayerMapper            priority 200
FTileLayerMapper            priority 200
FSwitcherLayerMapper        priority 200
F9SliceImageLayerMapper     priority 150
FSmartObjectLayerMapper     priority 150
FRichTextLayerMapper        priority 110  <-- NEW (multi-run text)
FTextLayerMapper            priority 100  <-- narrowed to Spans.Num() <= 1
FImageLayerMapper           priority 100
FFillLayerMapper            priority 100
FSolidFillLayerMapper       priority 100
FShapeLayerMapper           priority 100
FGroupLayerMapper           priority 50
```

### DataTable Asset Path Pattern

```
{WbpPackagePath}/{CleanName}_RichStyles/DT_{CleanName}_RichStyles
```

Example: WBP at `/Game/_Test/WBPGen/WBP_RichText_Spec`, text layer `text_two_colors`:
```
/Game/_Test/WBPGen/WBP_RichText_Spec/text_two_colors_RichStyles/DT_text_two_colors_RichStyles
```

### Markup Format

```
<s0>RedWord</><s1>GreenWord</>
```

- `&`, `<`, `>` are HTML-escaped (`&` first to avoid double-escaping)
- Tag names (`s0`, `s1`, ...) match DataTable row names

### DataTable Row Structure

| Row Name | Contents |
|----------|----------|
| Default | FTextBlockStyle from FPsdTextRun dominant-run scalars (FontName, SizePx*0.75, Color) |
| s0 | FTextBlockStyle from Spans[0] (per-span font/size/color via FFontResolver) |
| s1 | FTextBlockStyle from Spans[1] |
| ... | ... |

## Session Frontend Results

`Source.PSD2UMG.Typography.Pipeline` — 9/9 green:

| Describe block | It() blocks | Result |
|---|---|---|
| Typography.psd -> WBP | 2/2 | GREEN (pre-existing, no regression) |
| RichText.psd -> Spans | 4/4 | GREEN (parser assertions from Plan 16-02) |
| RichText.psd -> URichTextBlock | 3/3 | GREEN (newly passing after Plan 16-03) |

Full `Source.PSD2UMG` suite: all specs green — no regression in Panels, TextEffects, Parser.MultiLayer, Parser.Typography, Parser.GradientLayers, Parser.ShapeLayers.

## Visual Verification

- `WBP_RichText_Spec` Designer: `text_two_colors` — type `URichTextBlock`, `Text Style Set` points to DataTable at sibling sub-folder path, Designer preview shows `RedWord` in red + `GreenWord` in green
- `WBP_RichText_Spec` Designer: `text_bold_normal` — type `URichTextBlock`, Designer preview shows `BoldPart` in bold weight + `NormalPart` in normal weight
- DataTable opened in editor: rows `Default`, `s0`, `s1` present with correct font/color/size values per span
- Typography.psd widgets (`text_regular`, `text_gray`, etc.) remain `UTextBlock` — mutual exclusion working correctly

## Persistence Confirmation

- Saved all assets (Ctrl+Shift+S), closed editor, reopened — `text_two_colors` still renders with mixed red/green colors; `text_bold_normal` still renders with mixed bold/normal weights
- Confirmed DataTable asset is NOT transient: `RF_Public | RF_Standalone` flags + `UEditorAssetLibrary::SaveLoadedAsset` pattern (same as `FTextureImporter::ImportLayer`) ensures persistence

## Next Phase Readiness

- Phase 16 is fully closed: RICH-01 (two colors) and RICH-02 (bold + normal) delivered end-to-end with visual + persistence confirmation
- Phase 17 (Automated Font Matching) can proceed; `FFontResolver::Resolve` is already the single dispatch point used by both `FTextLayerMapper` and `FRichTextLayerMapper` — font matching improvements will benefit both automatically
- Deferred per-span shadow/outline is tracked above and does not block Phase 17

## Self-Check: PASSED

- `Source/PSD2UMG/Private/Mapper/FRichTextLayerMapper.cpp` — exists (confirmed by prior agent commits)
- `Source/PSD2UMG/Private/Mapper/AllMappers.h` — FRichTextLayerMapper declaration present (commit fb6f0f3)
- `Source/PSD2UMG/Private/Mapper/FLayerMappingRegistry.cpp` — MakeUnique<FRichTextLayerMapper>() present (commit 97c23a5)
- `Source/PSD2UMG/Private/Mapper/FTextLayerMapper.cpp` — Spans.Num() <= 1 present (commit fb6f0f3)
- Task commits: fb6f0f3, c7c40a5, 97c23a5 all in git log

---
*Phase: 16-rich-text-multiple-text-runs*
*Completed: 2026-04-22*
