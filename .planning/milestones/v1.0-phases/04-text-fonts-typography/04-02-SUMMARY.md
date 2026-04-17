---
phase: 04-text-fonts-typography
plan: 02
subsystem: typography
tags: [font-resolver, typeface, bold, italic, postscript-name, fslatefontinfo]

requires:
  - phase: 03-layer-mapping-widget-blueprint-generation
    provides: FTextLayerMapper baseline with content/DPI/color/alignment
  - phase: 04-text-fonts-typography plan 01
    provides: FPsdTextRun bBold/bItalic/bHasExplicitWidth fields (partially delivered via Rule 3 deviation)
provides:
  - FFontResolver with 4-step fallback chain (Exact > CaseInsensitive > Default > EngineDefault)
  - PostScript suffix parsing for bold/italic detection
  - FTextLayerMapper rewritten to resolve fonts and apply TypefaceFontName
affects: [04-text-fonts-typography plan 03]

tech-stack:
  added: []
  patterns: [font-resolver-pattern, suffix-table-longest-first, typeface-existence-check]

key-files:
  created:
    - Source/PSD2UMG/Private/Mapper/FontResolver.h
    - Source/PSD2UMG/Private/Mapper/FontResolver.cpp
    - Source/PSD2UMG/Tests/FFontResolverSpec.cpp
  modified:
    - Source/PSD2UMG/Private/Mapper/FTextLayerMapper.cpp
    - Source/PSD2UMG/Public/Parser/PsdTypes.h

key-decisions:
  - "Suffix table ordered longest-first for greedy match (-BoldItalicMT before -Bold)"
  - "Typeface existence verified against CompositeFont before applying TypefaceFontName"
  - "Parser bold/italic flags OR-combined with suffix-derived flags for maximum coverage"

patterns-established:
  - "Font resolver pattern: static helper class with Resolve/ParseSuffix/MakeTypefaceName"
  - "Typeface verification: always check CompositeFont.DefaultTypeface.Fonts before setting TypefaceFontName"

requirements-completed: [TEXT-02, TEXT-05]

duration: 8min
completed: 2026-04-10
---

# Phase 4 Plan 02: Font Resolution & Bold/Italic Summary

**FFontResolver with 4-step PostScript-name lookup chain and suffix-based bold/italic via TypefaceFontName**

## Performance

- **Duration:** 8 min
- **Started:** 2026-04-10T08:03:51Z
- **Completed:** 2026-04-10T08:12:00Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments
- FFontResolver implements Exact > CaseInsensitive > DefaultFont > EngineDefault lookup against UPSD2UMGSettings::FontMap
- Suffix table strips -BoldItalicMT/-BoldMT/-Bold/-ItalicMT/-Italic/-Oblique/-It from PostScript names to derive weight/style
- FTextLayerMapper rewired to call FFontResolver::Resolve, apply UFont + TypefaceFontName with existence check
- Phase 3 DPI x0.75, color, and alignment preserved verbatim

## Task Commits

Each task was committed atomically:

1. **Task 1: Implement FFontResolver with resolver spec** - `09405df` (feat)
2. **Task 2: Rewrite FTextLayerMapper to use FFontResolver** - `8ab4555` (feat)

## Files Created/Modified
- `Source/PSD2UMG/Private/Mapper/FontResolver.h` - FFontResolver declaration with Resolve/ParseSuffix/MakeTypefaceName
- `Source/PSD2UMG/Private/Mapper/FontResolver.cpp` - Implementation with suffix table and 4-step fallback
- `Source/PSD2UMG/Tests/FFontResolverSpec.cpp` - 9 test cases covering ParseSuffix, MakeTypefaceName, and Resolve
- `Source/PSD2UMG/Private/Mapper/FTextLayerMapper.cpp` - Rewritten Map() using FFontResolver
- `Source/PSD2UMG/Public/Parser/PsdTypes.h` - Added bBold/bItalic/bHasExplicitWidth/BoxWidthPx/OutlineColor/OutlineSize fields

## Decisions Made
- Suffix table ordered longest-first to avoid partial matches (e.g. -BoldItalicMT matched before -Bold)
- TypefaceFontName verified against CompositeFont entries before applying -- falls back to default typeface with warning if not found
- Parser bold/italic OR-combined with suffix flags so both faux-bold and real bold PostScript names produce correct typeface

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Added FPsdTextRun Phase 4 fields inline (04-01 dependency not yet executed)**
- **Found during:** Task 1 (FFontResolver needs bBold/bItalic fields on FPsdTextRun)
- **Issue:** Plan 04-01 (parser extension) has not been executed yet, but 04-02 depends on the struct fields it would add
- **Fix:** Added bBold, bItalic, bHasExplicitWidth, BoxWidthPx, OutlineColor, OutlineSize to FPsdTextRun directly
- **Files modified:** Source/PSD2UMG/Public/Parser/PsdTypes.h
- **Verification:** Struct compiles, fields are default-initialized, no existing consumers broken
- **Committed in:** 09405df (Task 1 commit)

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** Struct extension was required for compilation. Plan 04-01 can skip the struct changes when it runs and focus on parser population + fixture.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Known Stubs
None - all data flows are wired. Font resolution returns real UFont pointers from settings or nullptr for engine default.

## Next Phase Readiness
- FFontResolver ready for Plan 03 to add outline/wrap/shadow on top
- FTextLayerMapper has clean extension points for outline (FSlateFontInfo::OutlineSettings) and wrap (SetAutoWrapText)
- Plan 04-01 parser work can still run independently -- struct fields already exist, parser just needs to populate them

---
*Phase: 04-text-fonts-typography*
*Completed: 2026-04-10*
