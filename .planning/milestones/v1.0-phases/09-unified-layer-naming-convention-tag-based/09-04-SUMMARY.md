---
phase: 09-unified-layer-naming-convention-tag-based
plan: "04"
subsystem: docs
tags: [docs, migration, readme, tag-grammar, designer-facing, wave-4]

requires:
  - phase: 09-unified-layer-naming-convention-tag-based
    provides: "Plans 01-03 — parser + mapper refactor + fixture retag shipped; grammar spec (LayerTagGrammar.md) and retag checklist (09-03-RETAG-CHECKLIST.md) already on disk"
provides:
  - "Designer-facing migration guide (D-23) mapping every removed legacy pattern to its @-tag replacement with before/after PSD layer names"
  - "README.md naming cheat sheet rewritten to show only the @-tag grammar (D-25); cross-references to LayerTagGrammar.md and Migration-PrefixToTag.md wired"
  - "Delete-and-reimport guidance for pre-Phase-9 WBPs (ParsedTags.CleanName identity key, option-a from Plan 02 Task 1)"
affects: [future-designers, public-release, onboarding]

tech-stack:
  added: []
  patterns:
    - "Designer docs split: README.md (terse cheat sheet + examples) + Docs/LayerTagGrammar.md (formal spec) + Docs/Migration-PrefixToTag.md (migration companion). Each page links to the other two — no duplication of tag vocabulary."

key-files:
  created:
    - Docs/Migration-PrefixToTag.md
  modified:
    - README.md

key-decisions:
  - "Migration guide treats the Plan 02 reimport-identity decision (option-a, ParsedTags.CleanName) as a designer-visible concern, not an internal implementation detail: the 'delete and reimport' instruction is spelled out explicitly with rationale. Without this, users hitting mass-orphaning on reimport would have no documented path forward."
  - "`Button_NN` auto-name example retained in README.md rules list. The legacy-pattern grep flags it, but it is Phase-9 parser output (D-21 fallback for empty-name @button layers), not legacy dispatch input. Identical to the 09-03 verification rationale for the same string in FLayerTagParserSpec."
  - "CommonUI section collapsed into the main modifier-tag table (no separate subsection). Grammar is uniform — CommonUI mode only swaps the widget class backing @button; the tag vocabulary is the same. Keeping a dedicated CommonUI table would duplicate @ia / @anim rows already present elsewhere."

patterns-established:
  - "Migration guide per deprecation event: when a syntax is removed via hard cutover, ship a per-row old→new table, a worked-example walkthrough, a lint checklist, and an identity-key warning. This keeps users unblocked without requiring a rewind/deprecation pipeline."

requirements-completed: [TAG-04, TAG-06]

duration: ~5m
completed: "2026-04-14"
---

# Phase 9 Plan 04: Migration Guide + README Rewrite Summary

**Shipped Docs/Migration-PrefixToTag.md (full old→new mapping, 15-layer HUD walkthrough, PSD lint checklist, delete-and-reimport warning) and rewrote README.md Layer Naming section to show only the @-tag grammar with cross-references to the formal spec and migration guide.**

## Performance

- **Duration:** ~5m
- **Started:** 2026-04-14T15:31:48Z
- **Completed:** 2026-04-14T15:34:12Z
- **Tasks:** 2 (both `type="auto"`)
- **Files modified:** 2 (1 new, 1 modified)

## Accomplishments

- `Docs/Migration-PrefixToTag.md` created: header with hard-cutover framing; 4 mapping tables (widget-type tags, image draw-mode, anchor/sizing, state-children) plus @ia and @anim rows covering all 27+ removed legacy patterns; 15-layer HUD before/after PSD walkthrough; 7-point lint checklist; reimport-identity warning box documenting delete-and-reimport as the supported path for pre-Phase-9 WBPs; cross-reference link to `Docs/LayerTagGrammar.md`.
- `README.md` Layer Naming section rewritten from ~60 lines of prefix/suffix tables to ~45 lines of tag-format syntax + type-tag table + modifier-tag table + rules + examples; cross-references to `Docs/LayerTagGrammar.md` (formal spec) and `Docs/Migration-PrefixToTag.md` (migration companion) added.
- Three CommonUI / EnhancedInput prose lines in README.md (optional-plugin note, `bUseCommonUI` setting, `InputActionSearchPath` setting) rewritten to reference `@button` / `@ia:IA_Action` tags instead of `Button_Name[IA_Action]` bracket syntax.
- Legacy-pattern grep over `README.md` returns only one hit (`Button_NN` — documented Phase-9 auto-name fallback per D-21, not legacy dispatch). Grep over `Docs/` returns expected hits only (the migration guide's own "old syntax" column, the grammar spec's deprecation notes, and the `python-reference/` historical source tree that is not layer-naming documentation).

## Task Commits

1. **Task 1 — Write Docs/Migration-PrefixToTag.md (D-23 + TAG-04):** `5062777` (docs: add migration guide Prefix→Tag)
2. **Task 2 — Rewrite README.md naming section (D-25 + TAG-06):** `330b9b1` (docs: rewrite README layer-naming section for tag grammar)

**Plan metadata:** _(this commit)_ (docs: complete 09-04 plan)

## Files Created/Modified

- `Docs/Migration-PrefixToTag.md` — NEW (188 lines). Mapping tables, walkthrough, lint checklist, reimport warning, cross-reference.
- `README.md` — naming section fully replaced; three CommonUI/EnhancedInput prose lines updated. Net ~60 insertions / ~60 deletions.

## Decisions Made

- **Migration guide explicitly documents the delete-and-reimport path** for pre-Phase-9 WBPs, surfacing the Plan 02 Task 1 reimport-identity decision (option-a / `ParsedTags.CleanName`) as a designer-facing concern. Without this, users hitting one-time orphaning on reimport of retagged PSDs would have no documented workflow. This is the concrete user manifestation of D-15 ("no backwards compatibility").
- **`Button_NN` auto-name retained in README.** It is Phase-9 parser output (D-21) for empty-name `@button` layers, not a pre-Phase-9 dispatch prefix. Removing or renaming it would lie about the actual parser behavior. Mirrors the benign-hit rationale already used in 09-03's legacy-pattern grep.
- **CommonUI collapsed into the main tag tables.** The grammar is uniform; `bUseCommonUI` only swaps the widget class backing `@button`. A dedicated CommonUI subsection would duplicate `@ia` and `@anim` rows already present in the modifier-tag table.

## Deviations from Plan

None — plan executed exactly as written.

The plan referenced `09-RETAG-CHECKLIST.md`; the on-disk filename is `09-03-RETAG-CHECKLIST.md`. The migration guide links to the correct filename. Not a deviation — filename resolution, not behavioral divergence.

## Issues Encountered

None.

## Verification

**Task 1 gate:**

```
ls Docs/Migration-PrefixToTag.md && grep -q "@button" Docs/Migration-PrefixToTag.md && grep -q "@state:" Docs/Migration-PrefixToTag.md && grep -q "LayerTagGrammar" Docs/Migration-PrefixToTag.md && echo OK
→ OK
```

**Task 2 gate:** legacy-pattern grep over `README.md` returns only `Button_NN` on line 86, which is Phase-9 auto-name documentation, not legacy dispatch. All prior CommonUI prose rewritten.

**Plan-level gate** (`README.md + Docs/` excluding `Migration-PrefixToTag.md`): legacy patterns survive only in:

- `Docs/LayerTagGrammar.md` — grammar spec contains deprecation notes referencing legacy patterns by design (intended reading material for designers who still think in the old syntax).
- `Docs/python-reference/*` — historical AutoPSDUI Python source preserved for reference. Not layer-naming documentation.

All instances are documented, intentional, and not user-facing dispatch instructions.

## User Setup Required

None — no external service configuration required.

## Phase 9 Deliverable Acceptance (D-01..D-27)

Every deliverable from `09-CONTEXT.md` is now satisfied across plans 01-04:

| ID | Deliverable | Plan | Task / Artifact |
|----|-------------|------|-----------------|
| D-01 | Layer name format `WidgetName @tag @tag:value @tag:v1,v2,...` | 09-01 | Task 1 — `FLayerTagParser::Parse` grammar |
| D-02 | One type tag per layer; group→@canvas, pixel→@image, text→@text defaults | 09-01 | Task 2 — post-pass default-type inference |
| D-03 | `:` separator for tag values; comma for multi-value | 09-01 | Task 1 — parser token split |
| D-04 | Case-insensitive tags; asset-name values preserve case | 09-01 | Task 1 — lookup table normalization |
| D-05 | Type tag vocabulary (@button, @image, @text, @progress, @hbox, @vbox, @overlay, @scrollbox, @slider, @checkbox, @input, @list, @tile, @smartobject, @canvas) | 09-01 | Task 1 — `EPsdTagType` enum |
| D-06 | `@anchor:*` consolidation (tl/tc/tr/cl/c/cr/bl/bc/br/stretch-h/stretch-v/fill) | 09-01 + 09-02 | `EPsdAnchorTag` + `FAnchorCalculator` tag-driven |
| D-07 | `@9s:L,T,R,B` with `@9s` shorthand defaulting to 16,16,16,16 | 09-01 + 09-02 | Parser default + F9SliceImageLayerMapper consumption |
| D-08 | `@variants` replaces `Switcher_` and `_variants` | 09-01 + 09-02 | Parser flag + FSwitcherLayerMapper + FVariantsSuffixMapper dispatch on bIsVariants |
| D-09 | `@ia:IA_ActionName` on button | 09-01 + 09-02 | Parser value + FCommonUIButtonLayerMapper |
| D-10 | `@anim:show/hide/hover` on child inside CommonUI button | 09-01 + 09-02 | Parser + FPsdWidgetAnimationBuilder |
| D-11 | `@smartobject:TypeName` | 09-01 | Parser `SmartObjectTypeName` field |
| D-12 | `@state:hover/pressed/disabled/normal/fill/bg` uniform across parents | 09-01 + 09-02 | `EPsdStateTag` + `FLayerTagParser::FindChildByState` + button/progress mappers |
| D-13 | Missing-state fallback to first untagged child | 09-02 | FProgressLayerMapper FindChildByState(Bg→Normal) fallback |
| D-14 | All tags case-insensitive uniformly | 09-01 | Parser normalization |
| D-15 | Hard cutover — no backwards compatibility | 09-01 + 09-02 + 09-04 | Legacy code paths deleted; migration guide documents delete-and-reimport |
| D-16 | Tags order-free | 09-01 | Parser collects all @-tokens regardless of position |
| D-17 | Conflict resolution: last wins + warning | 09-01 | Parser diagnostics (warning-level) |
| D-18 | Unknown tags: warning + continue | 09-01 | Parser diagnostics (warning-level) |
| D-19 | Multiple type tags: error-with-fallback (last wins) | 09-01 | Parser diagnostics (error-level) |
| D-20 | Widget name = text before first `@`, trimmed, spaces→underscores | 09-01 | Parser CleanName extraction |
| D-21 | Empty-name fallback to `<TypeTag>_<LayerIndex>` (e.g. `Button_07`) | 09-01 | Parser CleanName fallback; documented in README |
| D-22 | Formal tag-grammar spec at `Docs/LayerTagGrammar.md` | 09-01 | Task 1 — grammar doc shipped |
| D-23 | Migration guide at `Docs/Migration-PrefixToTag.md` | **09-04** | **Task 1 — this plan** |
| D-24 | Retag existing test fixture PSDs; update specs | 09-03 | Tasks 1-3 — fixtures + specs retagged; checklist shipped |
| D-25 | README layer-naming cheat sheet rewritten | **09-04** | **Task 2 — this plan** |
| D-26 | Import Preview Dialog shows parsed tags per layer | 09-02 | SPsdImportPreviewDialog tag-chip rendering |
| D-27 | `FParsedLayerTags` + diagnostics threaded into `FPsdLayerTreeItem` | 09-02 | Tree-item extension |

All 27 deliverables shipped. Phase 9 complete.

## Next Phase Readiness

- Phase 9 closed. All Phase-9 requirements (TAG-01..TAG-07) completed across plans 01-04.
- **Known stubs:** none. The migration guide and README rewrite contain no placeholder content; every section is substantive and verified against the actual shipped grammar.
- **Known concerns:**
  - Users opening existing pre-Phase-9 WBPs will hit one-time orphaning on reimport. Documented in `Docs/Migration-PrefixToTag.md` with the recommended delete-and-reimport workflow.
  - No automated "retag my PSD" tool exists — designers do the Photoshop rename by hand. Explicit choice given the hard-cutover scope and PhotoshopAPI's read-only status.

## Self-Check: PASSED

- FOUND: Docs/Migration-PrefixToTag.md
- FOUND: README.md (modified, naming section replaced, cross-refs present)
- FOUND: commit 5062777 (Task 1)
- FOUND: commit 330b9b1 (Task 2)

---
*Phase: 09-unified-layer-naming-convention-tag-based*
*Completed: 2026-04-14*
