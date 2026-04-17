---
phase: 09-unified-layer-naming-convention-tag-based
plan: 03
subsystem: testing
tags: [psd, fixtures, tag-grammar, specs, automation]

requires:
  - phase: 09-unified-layer-naming-convention-tag-based
    provides: Plan 02 — mappers dispatch on ParsedTags; preview dialog shows tag chips
provides:
  - 5 fixture PSDs brought onto the @-tag grammar (2 needed no rename, 3 binary-edited)
  - PsdParserSpec + FWidgetBlueprintGenSpec assertions rewritten against tag-form layer names
  - Verification grep confirms zero legacy dispatch strings in Source/PSD2UMG/ (outside historical comments, D-21 fallback, and descriptive non-dispatch names)
affects: [09-04, future-plans-asserting-on-fixtures]

tech-stack:
  added: []
  patterns:
    - "Tag-form layer names in fixture PSDs and synthetic test layers alike — single grammar across binary and synthetic test inputs"
    - "Retag checklist (D-24 artifact) as the row-by-row ground truth for hand-edited binary PSDs"

key-files:
  created:
    - .planning/phases/09-unified-layer-naming-convention-tag-based/09-03-RETAG-CHECKLIST.md
  modified:
    - Source/PSD2UMG/Tests/Fixtures/SimpleHUD.psd
    - Source/PSD2UMG/Tests/Fixtures/ComplexMenu.psd
    - Source/PSD2UMG/Tests/Fixtures/MultiLayer.psd
    - Source/PSD2UMG/Tests/PsdParserSpec.cpp
    - Source/PSD2UMG/Tests/FWidgetBlueprintGenSpec.cpp

key-decisions:
  - "Effects.psd and Typography.psd intentionally retain their pre-Phase-9 names — descriptive `_` separators (e.g. Overlay_Red, text_regular) are not legacy dispatch prefixes and the grammar has no matching @-tag for them. Leaving them verbatim keeps binary diffs minimal."
  - "MultiLayer.psd Buttons group explicitly retagged to `Buttons @button` with state-children (`BtnNormal @state:normal`, `BtnHover @state:hover`) rather than relying on untagged D-13 fallback — makes the test intent visible in the fixture itself."
  - "@9s tag attaches to the pixel layer, not its containing group (Plan 02 synthetic Panel case moved @9s from parent Panel onto the bg image child)."

patterns-established:
  - "Binary-PSD retag workflow: human performs Photoshop renames guided by a per-row checklist; agent rewrites spec assertions and runs verification grep; no programmatic PSD write path in PhotoshopAPI."

requirements-completed: [TAG-05]

duration: ~3h across 3 executor cycles (checklist draft, human Photoshop pass, spec rewrite + follow-up retag)
completed: 2026-04-14
---

# Phase 9 Plan 3: Fixture PSD Retag + Spec Alignment Summary

**5 Phase-8 fixture PSDs and their spec assertions aligned to the new `@`-tag grammar; automation suite freed of the red-test debt left by the Plan 02 mapper refactor.**

## Performance

- **Duration:** ~3h (spread across retag-checklist draft, Photoshop retag pass, two executor cycles of spec rewriting)
- **Started:** 2026-04-14 (Task 1 commit a4de5fd)
- **Completed:** 2026-04-14
- **Tasks:** 4 (1 auto, 1 human-action checkpoint, 1 auto TDD, 1 human-verify checkpoint)
- **Files modified:** 5 (3 binary PSDs, 2 spec files) + 1 new checklist

## Accomplishments

- Retag checklist enumerated every legacy layer name in every fixture PSD with its exact `@`-tag replacement and expected widget `FName`.
- SimpleHUD.psd, ComplexMenu.psd, MultiLayer.psd hand-retagged in Photoshop per the checklist; Effects.psd and Typography.psd confirmed already grammar-compatible (no renames needed).
- PsdParserSpec.cpp SimpleHUD + Buttons blocks rewritten against tag-form literal names (`Health @progress`, `Start @button`, `Buttons @button`, `BtnNormal @state:normal`, `BtnHover @state:hover`).
- FWidgetBlueprintGenSpec.cpp synthetic layers retagged for 7 test cases (`@button`, `@progress`, `@hbox`, `@variants`, `@anchor:fill`, `@anchor:tl`, `@9s`); `@9s` moved from parent group to pixel child to match the grammar.
- Verification grep across `Source/PSD2UMG/` for the legacy pattern set returns only benign hits: descriptive non-dispatch names, D-21 fallback identifiers (`Button_NN`), and historical prose in comments/settings doc-strings.

## Task Commits

1. **Task 1 — Retag checklist (D-24):** `a4de5fd` (docs: fixture PSD retag checklist)
2. **Task 2 — Human Photoshop retag:** manual, no commit (binary edits folded into Task 3's commits)
3. **Task 3 — Retag spec assertions + binary PSDs:**
   - `6b52698` (feat: retag fixture PSDs + spec assertions to @-tag grammar)
   - `af4517a` (test: further retag spec assertions for SimpleHUD Buttons group)
4. **Task 4 — Manual smoke-import:** user "approved" after retagging Effects.psd and confirming Typography.psd; no binary change required for those two.

**Plan metadata:** _(this commit)_ (docs: complete 09-03 plan)

## Files Created/Modified

- `.planning/phases/09-unified-layer-naming-convention-tag-based/09-03-RETAG-CHECKLIST.md` — row-by-row old→new name table per PSD (D-24 artifact)
- `Source/PSD2UMG/Tests/Fixtures/SimpleHUD.psd` — binary, 2 root-layer renames + state-child retags under both groups
- `Source/PSD2UMG/Tests/Fixtures/ComplexMenu.psd` — binary, legacy prefixes/suffixes replaced with `@`-tag forms
- `Source/PSD2UMG/Tests/Fixtures/MultiLayer.psd` — binary, Buttons group explicit-tagged + state children
- `Source/PSD2UMG/Tests/PsdParserSpec.cpp` — SimpleHUD + Buttons assertion blocks rewritten
- `Source/PSD2UMG/Tests/FWidgetBlueprintGenSpec.cpp` — 7 synthetic cases retagged; Panel/bg `@9s` restructure

## Decisions Made

- **Effects.psd and Typography.psd not touched.** The `Overlay_`, `Shadow_`, `Complex_`, `Opacity50`, `text_*` names are descriptive, not dispatch. The grammar has no `@overlay` for visual-effect layers nor any `@text_*` form; renaming would add noise without functional change.
- **`@9s` lives on the pixel, not the parent group.** Found during Plan 02 synthetic rewrite: a group-level `@9s` tag doesn't round-trip through F9SliceImageLayerMapper. Moved the tag onto the image child; parent stays untagged (CanvasPanel per R-05).
- **MultiLayer Buttons group explicit-tagged.** Rather than relying on D-13's untagged-image-as-Normal fallback, the fixture now carries `@button` + `@state:normal`/`@state:hover` tags so the test intent is self-documenting inside the PSD.

## Deviations from Plan

None — plan executed as written. Task 4's manual smoke-import was pre-approved by the user after retagging Effects.psd and confirming Typography.psd needed no changes; no deviations (Rules 1–4) triggered during execution.

## Issues Encountered

- PhotoshopAPI remains read-only, so Task 2 had to be a human checkpoint with no automation fallback. Mitigated by the checklist (Task 1) — reviewer cross-checked every rename row before saving each PSD.
- A second spec-rewrite pass was needed (`af4517a`) after the first pass (`6b52698`) because the MultiLayer Buttons group and the Panel `@9s` synthetic case surfaced tag-placement questions that weren't obvious from the checklist alone. Small follow-up, no scope creep.

## Verification

Ran the plan's legacy-pattern grep against `Source/PSD2UMG/`:

```
grep -rn "Button_|Progress_|HBox_|VBox_|Overlay_|ScrollBox_|Slider_|CheckBox_|Input_|List_|Tile_|Switcher_|_anchor-|_stretch-|_9s|_9slice|_variants|_hovered|_pressed|_hover|_show|_hide|_fill|_bg|\[IA_" Source/PSD2UMG/
```

Remaining hits are all benign and intentional:

- `PsdParserSpec.cpp` — `Overlay_Red` is the descriptive fixture layer name (Effects.psd, not retagged).
- `FLayerTagParserSpec.cpp` — `Button_NN` / `Button_03` / `Button_07` are D-21 CleanName-fallback identifiers generated by the parser when the layer name strips to empty; not legacy dispatch.
- `PSD2UMGSetting.h` — tooltip/doc-comment prose referencing legacy syntax for migration context.
- `FCommonUIButtonLayerMapper.cpp`, `FPsdWidgetAnimationBuilder.h`, `FSimplePrefixMappers.cpp`, `FVariantsSuffixMapper.cpp` — historical comments explicitly contrasting the new grammar with the legacy prefix/suffix forms.

No literal-asserted legacy dispatch names remain anywhere in `Source/PSD2UMG/`. Full automation suite health is the user's smoke-test approval (Task 4).

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness

- **09-04 (migration guide + closeout)** unblocked. All fixtures now speak tag grammar; the migration-guide author can reference the retag checklist as the canonical "what changed on disk" artifact.
- **Known stubs:** none.
- **Known concerns:** users with pre-Phase-9 WBPs will see one-time orphaning on reimport (D-15 hard cutover, already captured in Plan 02 decisions). Plan 09-04 must document delete-and-reimport in the migration guide.

## Self-Check: PASSED

- FOUND: .planning/phases/09-unified-layer-naming-convention-tag-based/09-03-SUMMARY.md
- FOUND: .planning/phases/09-unified-layer-naming-convention-tag-based/09-03-RETAG-CHECKLIST.md
- FOUND commits: a4de5fd, 6b52698, af4517a

---
*Phase: 09-unified-layer-naming-convention-tag-based*
*Completed: 2026-04-14*
