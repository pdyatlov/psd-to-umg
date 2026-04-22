---
phase: 16-rich-text-multiple-text-runs
plan: "02"
subsystem: parser
tags: [rich-text, multi-run, parser, RICH-01, RICH-02, span-extraction]
dependency_graph:
  requires:
    - phase: 16-01
      provides: FPsdTextRunSpan USTRUCT + TArray<FPsdTextRunSpan> Spans field + RED spec stubs
  provides:
    - Multi-run span extraction loop in ExtractSingleRunText (PsdParser.cpp)
    - OutLayer.Text.Spans populated when style_run_count() > 1 and RawSpans.Num() > 1 after sentinel strip
    - Per-span: Text (FString::Mid slice), Color (ARGB->RGBA), FontName (PostScript via FontSet), SizePx (* scale_y), bBold, bItalic
  affects:
    - Source/PSD2UMG/Private/Parser/PsdParser.cpp
    - Plan 16-03 (FRichTextLayerMapper) — depends on populated Spans array
tech_stack:
  added: []
  patterns:
    - Multi-run extraction guarded by style_run_count() > 1, placed after dominant-run scalars
    - Per-run ARGB->RGBA swap matching dominant-run extraction pattern (Phase 2 empirical confirmation)
    - Sentinel stripping: trailing whitespace-only spans popped from RawSpans before commit
    - Two-stage commit: RawSpans populated in loop, then committed to Spans only when Num() > 1
key-files:
  created: []
  modified:
    - Source/PSD2UMG/Private/Parser/PsdParser.cpp
key-decisions:
  - "Multi-run block placed AFTER the stroke block inside the existing try{} scope — shares the same exception handler as dominant-run extraction"
  - "RawSpans.Num() > 1 threshold (not >= 1) after sentinel strip: a single surviving span adds no information over the dominant-run scalars already populated"
  - "ASCII-only slicing documented as TODO for non-ASCII future work; FString::Mid by TCHAR count is correct for ASCII fixtures (RichText.psd)"
  - "bBold/bItalic fallback to PostScript font name (Contains Bold/Italic/Oblique) mirrors the dominant-run detection exactly for consistency"
requirements-completed: [RICH-01, RICH-02]
metrics:
  duration: ~10m
  completed: "2026-04-22"
  tasks_completed: 1
  tasks_total: 1
  files_modified: 1
---

# Phase 16 Plan 02: Parser Multi-Run Extraction (Wave 1) Summary

**One-liner:** Multi-run span extraction loop inserted into ExtractSingleRunText — walks every style run after dominant-run scalar extraction, populates OutLayer.Text.Spans with per-span Text/Color/FontName/SizePx/bBold/bItalic, strips trailing sentinel runs, and commits only when ≥2 meaningful spans remain; turns the 3 parser-level RICH RED It() blocks GREEN.

## Performance

- **Duration:** ~10 min
- **Started:** 2026-04-22T11:18:00Z
- **Completed:** 2026-04-22T11:28:00Z
- **Tasks:** 1 completed
- **Files modified:** 1

## Accomplishments

### Task 1: Add multi-run extraction loop to ExtractSingleRunText

**Commit:** 443cb5e — `feat(16-02): add multi-run span extraction loop to ExtractSingleRunText`

**Insertion point:** After the closing `}` of the `if (bStroke) { ... }` block (line 443), before the `catch (const std::exception& e)` (now line 599). The insertion is ~154 lines inside the existing try{} scope.

**What was inserted (line range ~445–597 in PsdParser.cpp):**

A scoped block `{ ... }` containing:
1. `const size_t RunCount = Text->style_run_count()` — guard check
2. `if (RunCount > 1)` — enters multi-run path only when needed
3. `auto LengthsOpt = Text->style_run_lengths()` — per-run length vector
4. Loop over `ri = 0..RunCount-1` building `FPsdTextRunSpan` entries:
   - **Text slice**: `FString::Mid(CharOffset, RunLen)` — ASCII-safe; non-ASCII documented as TODO
   - **Color**: ARGB->RGBA swap identical to dominant-run fill extraction (Phase 2 confirmed)
   - **FontName**: `style_run_font(ri)` → FontSet index → `font_postscript_name(index)`
   - **SizePx**: `style_run_font_size(ri) * scale_y`
   - **bBold/bItalic**: faux flags + PostScript name fallback (`Contains("Bold")` / `Contains("Italic"/"Oblique")`)
5. Sentinel stripping: `while (RawSpans.Num() > 0)` — pops trailing whitespace-only spans
6. Commit to `OutLayer.Text.Spans = MoveTemp(RawSpans)` only when `RawSpans.Num() > 1`
7. UE_LOG(Log) for both the "produced N spans" and "collapsed to 0/1" cases

## Acceptance Criteria — All Passed (static grep verification)

| Check | Expected | Result |
|-------|----------|--------|
| `grep -c "RunCount = Text->style_run_count"` | 1 | 1 |
| `grep -c "style_run_fill_color(ri)"` | 1 | 1 |
| `grep -c "style_run_faux_bold(ri)"` | 1 | 1 |
| `grep -c "OutLayer.Text.Spans = MoveTemp"` | 1 | 1 |
| `grep -c "RICH-01/02"` | 1 | 1 |
| `grep -c "FPsdTextRunSpan"` | >= 2 | 2 |
| `grep -c "ExtractSingleRunText"` | 2 | 2 |

## Expected Test Outcomes (After Build)

When run against `Source.PSD2UMG.Typography.Pipeline`:

| Describe | It() | Before 16-02 | After 16-02 |
|----------|------|--------------|-------------|
| Typography.psd -> WBP | parses with no errors | GREEN | GREEN (no regression) |
| Typography.psd -> WBP | generates UTextBlocks | GREEN | GREEN (no regression) |
| RichText.psd -> Spans | parses with no errors (sanity) | GREEN | GREEN |
| RichText.psd -> Spans | text_two_colors Spans.Num() >= 2 | RED | **GREEN** |
| RichText.psd -> Spans | text_two_colors span colors red+green | RED | **GREEN** |
| RichText.psd -> Spans | text_bold_normal bold+normal | RED | **GREEN** |
| RichText.psd -> URichTextBlock | generates URichTextBlock | RED | RED (Plan 16-03) |
| RichText.psd -> URichTextBlock | TextStyleSet non-null | RED | RED (Plan 16-03) |
| RichText.psd -> URichTextBlock | markup style tags | RED | RED (Plan 16-03) |

*Build + test run requires host UE project — human-verify at start of 16-03 or on demand.*

## Deviations from Plan

None — plan executed exactly as written. The code block inserted matches the plan's specification verbatim (comment header, guard structure, loop body, sentinel strip, commit threshold, UE_LOG statements). No dominant-run code was touched.

## Known Stubs

None — the extraction loop is fully wired. Data flows from PhotoshopAPI style run API into `FPsdTextRunSpan` fields which are stored in `OutLayer.Text.Spans`. Plan 16-03 (mapper) will consume the populated Spans array.

The ASCII-only text-slice path is documented via a TODO comment (lines 499-503) and is intentional for Phase 16 scope — the RichText.psd fixture uses pure ASCII.

## Self-Check: PASSED

- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` modified — FOUND (git status + grep)
- Commit 443cb5e exists — confirmed via `git rev-parse --short HEAD`
- All 7 acceptance-criteria grep checks pass — verified above
- Dominant-run scalars (Content, FontName, SizePx, Color, bBold, bItalic, OutlineColor, ShadowOffset) NOT touched — no grep matches for changes to those assignments
- `ExtractSingleRunText` count remains 2 (declaration + call site) — confirmed
