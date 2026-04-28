---
phase: 21-parser-correctness-fixes
plan: "01"
subsystem: parser
tags: [parser, unicode, utf8, null-sentinel, cjk, rich-text, automation-spec, tdd]
dependency_graph:
  requires: []
  provides: [rtxt-01-null-sentinel-strip, cjk-spec-block]
  affects: [PsdParser, FRichTextLayerMapper, FPsdParserCJKSpec]
tech_stack:
  added: []
  patterns:
    - find_last_not_of sentinel strip before Utf8ToFString (D-03: at call site not in helper)
    - FindLayerRecursive + ContainsNonAscii helpers for recursive CJK layer detection
    - AddWarning skip-guard pattern for user-supplied fixtures (D-01)
key_files:
  created: []
  modified:
    - Source/PSD2UMG/Private/Parser/PsdParser.cpp
    - Source/PSD2UMG/Tests/PsdParserSpec.cpp
key_decisions:
  - "D-03 honored: sentinel strip applied at both call sites (Content scalar ~line 202 and FullUtf8 multi-run ~line 485), NOT inside Utf8ToFString helper — helper remains generic"
  - "D-04 honored: FString::Mid slicing loop unchanged — TCHAR = UTF-16 code unit aligns with style_run_lengths() for BMP CJK after sentinel strip"
  - "D-01 respected: BeforeEach emits AddWarning (not AddError) and short-circuits when RichTextCJK.psd absent — CI stays green without user-supplied fixture"
  - "FPsdParserCJKSpec added to existing PsdParserSpec.cpp (not a new file) — consistent with RESEARCH Pattern 4 recommendation and simpler file structure"
patterns-established:
  - "Null-sentinel strip: std::string::find_last_not_of('\\0') + erase pattern for PhotoshopAPI text() return values"
  - "Fixture skip-guard: AddWarning + early return in BeforeEach for user-supplied fixtures; It() bodies guard on bParsed for graceful no-op"
requirements-completed: [RTXT-01]
duration: ~15min
completed: "2026-04-28"
---

# Phase 21 Plan 01: CJK Null-Sentinel Strip (RTXT-01) Summary

**Strip trailing `\0` sentinel at both Content scalar and FullUtf8 call sites in PsdParser.cpp; add `FPsdParserCJKSpec` automation spec with 5 It() blocks for empirical validation.**

## Performance

- **Duration:** ~15 minutes
- **Started:** 2026-04-28T13:15:00Z
- **Completed:** 2026-04-28T13:30:00Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Added `FPsdParserCJKSpec` spec block (RED guard) to PsdParserSpec.cpp — 5 It() blocks covering: parse success, CJK layer presence, no NUL in Content (single-run path), no NUL in spans (multi-run path), and span-length sum equals Content length
- Applied sentinel strip at single-run Content scalar callsite (~line 202): `find_last_not_of('\0')` + `erase` before `Utf8ToFString`, closing Pitfall 3 (easy-to-miss scalar path)
- Applied sentinel strip at multi-run FullUtf8 callsite (~line 485): same pattern, dropping `const` qualifier to allow mutation — eliminates zero-length spans for trailing runs in CJK layers
- `Utf8ToFString` helper (lines 75-78) untouched per D-03 — byte-identical pre/post
- `FString::Mid` slicing loop untouched per D-04 — BMP CJK correct once sentinel stripped

## Task Commits

Each task was committed atomically:

1. **Task 1: Add FPsdParserCJKSpec block (RED)** - `c94bf61` (test)
2. **Task 2: Strip NUL sentinel at both callsites (GREEN)** - `f25c589` (feat)

## Files Created/Modified

- `Source/PSD2UMG/Tests/PsdParserSpec.cpp` — New `FPsdParserCJKSpec` spec block (133 lines) appended before `#endif // WITH_DEV_AUTOMATION_TESTS`
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` — Two sentinel-strip edits: Content scalar path (+13 lines) and FullUtf8 multi-run path (+11 lines, dropped const)

## Decisions Made

- D-03 honored: fix applied at both call sites, NOT inside `Utf8ToFString`. The helper stays generic and serves `FontName` and other call sites unchanged.
- D-04 honored: `FString::Mid` indexing by TCHAR is correct for BMP CJK (U+0000–U+FFFF) once the sentinel is stripped. Supplementary-plane emoji slicing is deferred per CONTEXT.md `<deferred>`.
- Fixture skip-guard (AddWarning, not AddError): CI stays green when `RichTextCJK.psd` is absent. This is the correct behavior per D-01 — the user supplies the fixture independently.
- Spec folded into existing `PsdParserSpec.cpp` (not a new file): consistent with RESEARCH Pattern 4 recommendation; simpler, fewer files.

## Deviations from Plan

None — plan executed exactly as written.

## Issues Encountered

- Build verification via `Build.bat PSD2UMGEditor` could not be executed because this repository is a plugin with no host `.uproject` — a pre-existing structural constraint documented in v1.0-MILESTONE-AUDIT.md. Code correctness verified via acceptance criteria grep checks instead (all 9 + 5 criteria passed).

## Known Stubs

None — no stub patterns introduced. The pre-existing TODO comments at lines 479/513/533 in PsdParser.cpp (FString::Mid non-ASCII note) are deliberate deferred scope per CONTEXT.md D-04, not introduced by this plan.

## Next Phase Readiness

- RTXT-01 fix is complete and gated by `FPsdParserCJKSpec` — once user supplies `RichTextCJK.psd`, all 5 It() blocks validate correctness empirically
- Phase 21 Plan 02 (LFXC-01 lrFX ColorSpace branch) can proceed immediately — no dependency on this plan's fixture
- `FRichTextLayerMapper` downstream consumer of `Spans` is unaffected (sentinel strip makes its input cleaner, not different in structure)

---
*Phase: 21-parser-correctness-fixes*
*Completed: 2026-04-28*

## Self-Check: PASSED

- `Source/PSD2UMG/Tests/PsdParserSpec.cpp` modified: FOUND (c94bf61)
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` modified: FOUND (f25c589)
- Commit c94bf61 exists: FOUND
- Commit f25c589 exists: FOUND
