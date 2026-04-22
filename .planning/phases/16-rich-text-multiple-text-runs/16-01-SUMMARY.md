---
phase: 16-rich-text-multiple-text-runs
plan: "01"
subsystem: parser-types, test-spec, fixture
tags: [rich-text, multi-run, data-model, tdd, red-state]
dependency_graph:
  requires: []
  provides:
    - FPsdTextRunSpan USTRUCT (PsdTypes.h)
    - FPsdTextRun::Spans field (TArray<FPsdTextRunSpan>)
    - RichText.psd fixture (2 Type layers)
    - RED FTextPipelineSpec Describe blocks for Plan 16-02/16-03
  affects:
    - Source/PSD2UMG/Public/Parser/PsdTypes.h
    - Source/PSD2UMG/Tests/FTextPipelineSpec.cpp
    - Source/PSD2UMG/Tests/Fixtures/RichText.psd
tech_stack:
  added: []
  patterns:
    - FPsdTextRunSpan USTRUCT-of-USTRUCT inside TArray (reflection-safe)
    - RED TDD spec stubs with per-Describe BeforeEach reset of shared Doc/Diag members
key_files:
  created:
    - Source/PSD2UMG/Tests/Fixtures/RichText.psd
    - .planning/phases/16-rich-text-multiple-text-runs/16-01-SUMMARY.md
  modified:
    - Source/PSD2UMG/Public/Parser/PsdTypes.h
    - Source/PSD2UMG/Tests/FTextPipelineSpec.cpp
decisions:
  - "FPsdTextRunSpan declared above FPsdTextRun so TArray<FPsdTextRunSpan> compiles without forward-declaration hacks"
  - "Spans.Num()==0 preserves legacy single-run path; Plans 16-02/16-03 populate/consume Spans"
  - "Reuse existing FTextPipelineSpec class (same test namespace) rather than new spec — keeps Source.PSD2UMG.Typography.Pipeline single"
requirements-completed: [RICH-01, RICH-02]
metrics:
  duration: ~15m
  completed: "2026-04-22"
  tasks_completed: 4
  tasks_total: 4
  files_modified: 3
---

# Phase 16 Plan 01: Fixture + Data Model + RED Spec Stubs (Wave 0) Summary

**One-liner:** RichText.psd fixture with 2 multi-run Type layers; FPsdTextRunSpan USTRUCT + TArray<FPsdTextRunSpan> Spans added to FPsdTextRun; 6 RED It() stubs in FTextPipelineSpec establishing the TDD contract for Plans 16-02/16-03.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Author RichText.psd fixture | 6f8df5a | Source/PSD2UMG/Tests/Fixtures/RichText.psd |
| 2 | Add FPsdTextRunSpan struct + Spans field | fd3233b | Source/PSD2UMG/Public/Parser/PsdTypes.h |
| 3 | Add RichText.psd Describe blocks (RED stubs) | 5c90491 | Source/PSD2UMG/Tests/FTextPipelineSpec.cpp |
| 4 | Confirm RED state in UE Session Frontend | confirmed | (human-verify checkpoint) |

## Fixture Details

**File:** `Source/PSD2UMG/Tests/Fixtures/RichText.psd`
**Size:** 112,835 bytes (~110 KB)
**SHA1:** `fbc727c5cfd9c9a6c14c4f111acc59cfc8c220a8`
**Canvas:** 512x256, Maximize Compatibility ON

**Layer authoring (per user confirmation):**
- `text_two_colors` (RICH-01 target): single Type layer, text `RedWord GreenWord` — `RedWord` characters in pure red #FF0000, `GreenWord` characters in pure green #00FF00. Both within one text layer with 2 color-distinct style runs.
- `text_bold_normal` (RICH-02 target): single Type layer, text `BoldPart NormalPart` — `BoldPart` in Bold weight, `NormalPart` in Regular weight. Both within one text layer with 2 weight-distinct style runs.
- Both layers have Type (T) icons — NOT rasterised; cursor-editable with the Type tool.
- Background raster layer deleted; document contains exactly 2 root layers.

## Data Model Changes (PsdTypes.h)

**New USTRUCT `FPsdTextRunSpan` declared above `FPsdTextRun`:**
```
FString Text       — UTF-8 substring for this span
FString FontName   — PostScript font name
float SizePx       — per-span point size (0 = inherit from FPsdTextRun)
FLinearColor Color — per-span fill color (linearized from sRGB)
bool bBold         — faux-bold or true-bold face flag
bool bItalic       — faux-italic or true-italic face flag
```

**FPsdTextRun extended:**
- `TArray<FPsdTextRunSpan> Spans` added as the LAST member
- All existing members (Content, FontName, SizePx, Color, Alignment, bBold, bItalic, bHasExplicitWidth, BoxWidthPx, OutlineColor, OutlineSize, ShadowOffset, ShadowColor) preserved in original order
- USTRUCT() count in PsdTypes.h: 4 → 5 (FPsdTextRunSpan added)

## Test Spec Changes (FTextPipelineSpec.cpp)

**New includes:** `Components/RichTextBlock.h`, `Engine/DataTable.h`

**New helper:** `FindRichText(UWidgetTree*, FString)` — declared in spec class, defined out-of-line before Define().

**New Describe block 1: "RichText.psd -> Spans"** (4 It() blocks)
- "parses RichText.psd with no errors (sanity)" — expected GREEN (ParseFile finds fixture, 2 root layers)
- "text_two_colors has Spans.Num() >= 2 (RICH-01 parser)" — expected RED (Spans empty; Plan 16-02 closes)
- "text_two_colors span colors include red and green (RICH-01 parser)" — expected RED (Spans empty → AddError guard; Plan 16-02 closes)
- "text_bold_normal has at least one bold span and one normal span (RICH-02 parser)" — expected RED (Spans empty → AddError guard; Plan 16-02 closes)

**New Describe block 2: "RichText.psd -> URichTextBlock"** (3 It() blocks)
- "generates URichTextBlock for text_two_colors (RICH-01 mapper)" — expected RED (mapper emits UTextBlock; Plan 16-03 closes)
- "URichTextBlock for text_two_colors has non-null TextStyleSet (RICH-01 mapper)" — expected RED (null RTB; Plan 16-03 closes)
- "URichTextBlock markup contains multiple style tags (RICH-01 + RICH-02 mapper)" — expected RED (null RTB; Plan 16-03 closes)

**Existing Describe block "Typography.psd -> WBP":** unchanged, remains GREEN.

**Total Describe blocks:** 1 → 3. Total It() blocks: 2 → 9 (2 existing + 1 sanity + 3 parser RED + 3 mapper RED).

## Task 4: Human-Verify Checkpoint (COMPLETE)

User confirmed UE Session Frontend result for `Source.PSD2UMG.Typography.Pipeline`:
**3 pass / 6 fail** — matches the expected RED baseline exactly.

**Actual UE Session Frontend output:**

| Test | Suite | Result |
|------|-------|--------|
| parses RichText.psd | RichText.psd -> Spans | PASS |
| text_two_colors Spans.Num() >= 2 | RichText.psd -> Spans | FAIL (expected) |
| text_two_colors span colors | RichText.psd -> Spans | FAIL (expected) |
| text_bold_normal bold check | RichText.psd -> Spans | FAIL (expected) |
| generates URichTextBlock | RichText.psd -> URichTextBlock | FAIL (expected) |
| TextStyleSet non-null | RichText.psd -> URichTextBlock | FAIL (expected) |
| markup style tags | RichText.psd -> URichTextBlock | FAIL (expected) |
| parses with no errors | Typography.psd -> WBP | PASS |
| generates UTextBlocks | Typography.psd -> WBP | PASS |

**Full suite regression check:** All other suites (Panels, TextEffects, Parser.*) remain GREEN. No regression from FPsdTextRunSpan addition.

**RED baseline confirmed:** Spans parser assertions fail because no multi-run extraction loop exists yet (Plan 16-02 target). URichTextBlock mapper assertions fail because mapper still emits UTextBlock (Plan 16-03 target). The sanity It() passes — fixture is found and parsed correctly by the parser (0 errors, 2 root layers). This is the correct input contract for Plans 16-02 and 16-03.

## Deviations from Plan

None — plan executed exactly as written. All three auto tasks completed in sequence. FPsdTextRunSpan inserted at the correct location (above FPsdTextRun, below EPsdLayerType), Spans added as last member of FPsdTextRun, two new Describe blocks appended inside Define() after Typography Describe closing brace.

## Known Stubs

None — this plan is purely additive (new struct, empty field, RED test stubs). No data-flow stubs that block the plan's goal; the RED state IS the intended output.

## Self-Check: PASSED

- `Source/PSD2UMG/Tests/Fixtures/RichText.psd` — FOUND (112,835 bytes)
- `Source/PSD2UMG/Public/Parser/PsdTypes.h` — FPsdTextRunSpan USTRUCT present, TArray<FPsdTextRunSpan> Spans present
- `Source/PSD2UMG/Tests/FTextPipelineSpec.cpp` — 3 Describe blocks present, RED stubs present
- Commits 6f8df5a, fd3233b, 5c90491 — FOUND in git log
- UE Session Frontend: 3 pass / 6 fail — CONFIRMED by user (Task 4)
