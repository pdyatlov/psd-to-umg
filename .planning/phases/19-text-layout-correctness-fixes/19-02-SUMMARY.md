---
phase: 19-text-layout-correctness-fixes
plan: "02"
subsystem: text-mapper
tags: [tdd, text-property, caps, transform-policy, typography]
dependency_graph:
  requires: []
  provides: [TXT-CAPS-01]
  affects: [FPsdTextRun, PsdParser, FTextLayerMapper, Typography fixture]
tech_stack:
  added: []
  patterns: [style_run_font_caps dominant-run pattern, SetTextTransformPolicy]
key_files:
  created: []
  modified:
    - Source/PSD2UMG/Public/Parser/PsdTypes.h
    - Source/PSD2UMG/Private/Parser/PsdParser.cpp
    - Source/PSD2UMG/Private/Mapper/FTextLayerMapper.cpp
    - Source/PSD2UMG/Tests/PsdParserSpec.cpp
    - Source/PSD2UMG/Tests/FTextPipelineSpec.cpp
    - .planning/REQUIREMENTS.md
decisions:
  - bAllCaps on FPsdTextRun is the single source of truth — parser sets it, mapper consumes it, no duplication
  - style_run_font_caps(DominantRunIdx) mirrors FauxBold/FauxItalic pattern exactly (Pitfall 1 avoided)
  - SetTextTransformPolicy only called when bAllCaps is true — avoids overwriting default on non-caps layers
  - FRichTextLayerMapper does NOT get SetTextTransformPolicy — multi-run All Caps deferred (Pitfall 2, out of scope)
metrics:
  duration: "~15min"
  completed: "2026-04-27"
  tasks_completed: 3
  files_modified: 6
---

# Phase 19 Plan 02: TXT-CAPS-01 All Caps Transform Policy Summary

**One-liner:** bAllCaps field on FPsdTextRun wired end-to-end from style_run_font_caps parser read to SetTextTransformPolicy(ToUpper) in FTextLayerMapper.

## What Was Built

TXT-CAPS-01 implements Photoshop's "All Caps" character-panel toggle in the UMG output. When a text layer has All Caps enabled in Photoshop, the source text content is preserved as-authored (e.g., "hello caps") while the generated `UTextBlock` has `TextTransformPolicy = ETextTransformPolicy::ToUpper` — UMG's native locale-correct uppercasing mirrors Photoshop's render-time transform. Layers without All Caps keep the default `ETextTransformPolicy::None`.

## Implementation Chain

```
Typography.psd text_caps layer (All Caps enabled, content "hello caps")
  -> PsdParser::ExtractSingleRunText
       -> Text->style_run_font_caps(DominantRunIdx)    // PhotoshopAPI call
       -> OutLayer.Text.bAllCaps = (*Caps == TextLayerEnum::FontCaps::AllCaps)
  -> FPsdTextRun::bAllCaps = true                      // single source of truth
  -> FTextLayerMapper::Map
       -> if (Layer.Text.bAllCaps)
              TextWidget->SetTextTransformPolicy(ETextTransformPolicy::ToUpper)
  -> UTextBlock::GetTextTransformPolicy() == ETextTransformPolicy::ToUpper
```

Key implementation strings (grep-able):
- `style_run_font_caps(DominantRunIdx)` — in `Source/PSD2UMG/Private/Parser/PsdParser.cpp`
- `OutLayer.Text.bAllCaps = (*Caps == TextLayerEnum::FontCaps::AllCaps)` — same file
- `SetTextTransformPolicy(ETextTransformPolicy::ToUpper)` — in `Source/PSD2UMG/Private/Mapper/FTextLayerMapper.cpp`
- `bool bAllCaps = false;` — in `Source/PSD2UMG/Public/Parser/PsdTypes.h` (after bItalic)

## Spec Assertions (RED -> GREEN)

**Parser spec** (PSD2UMG.Parser.Typography):
- `It("text_caps has bAllCaps=true (TXT-CAPS-01 parser)")` — asserts `L->Text.bAllCaps == true`
- `It("text_regular has bAllCaps=false (TXT-CAPS-01 default)")` — asserts `L->Text.bAllCaps == false`

**Pipeline spec** (PSD2UMG.TextPipeline / PSD2UMG.Typography.Pipeline):
- `text_caps TextTransformPolicy is ToUpper` — asserts `GetTextTransformPolicy() == ETextTransformPolicy::ToUpper`
- `text_regular TextTransformPolicy is None (default)` — asserts `GetTextTransformPolicy() == ETextTransformPolicy::None`

**Root count updated:** Typography fixture root-count assertion bumped from 8 to 9 (text_caps is the 9th layer).

## Typography.psd Fixture Update

Typography.psd binary now contains `text_caps` as the ninth root layer (committed in bf96c10 as part of the human-action checkpoint). Layer properties:
- Name: `text_caps`
- Text content: `hello caps` (lowercase source — All Caps is a render-time transform)
- Character panel: All Caps enabled

## Deviations from Plan

### Pre-existing Structural Constraint

**Build + test automation skipped** — this repository is a plugin-only repo with no host `.uproject` file. `Build.bat PSD2UMGEditor` and `UnrealEditor-Cmd.exe Automation RunTests` cannot execute. This is a known pre-existing constraint (documented in 19-01-SUMMARY.md and 19-03-SUMMARY.md). All implementation correctness was verified by:
- Pattern matching against the existing FauxBold/FauxItalic precedent in PsdParser.cpp
- Verified PhotoshopAPI interface signatures from TextLayerStyleRunMixin.h and TextLayerEnum.h
- Verified UE 5.7 UTextBlock API from plan interfaces block
- Manual grep-based acceptance criteria checks confirming exact placement of all new code

### Task 1 Fixture (Already Resolved)

Typography.psd edit was a human-action checkpoint resolved before this execution: commit bf96c10 added the `text_caps` layer. Execution of this plan started from Task 1's spec-only remainder.

## Known Gap: FRichTextLayerMapper

`FRichTextLayerMapper` (multi-run text, `Spans.Num() > 1`) does **not** call `SetTextTransformPolicy`. This is intentional and out-of-scope per the plan (Pitfall 2). `FTextLayerMapper::CanMap` is gated to `Spans.Num() <= 1`, so single-run All Caps is fully covered. Multi-run All Caps is a deferred gap — no deferred-items entry created as this is documented in the plan scope-out.

## Commits

| Task | Commit | Description |
|------|--------|-------------|
| 1 (specs) | 2e39972 | test(19-02): add TXT-CAPS-01 RED specs for text_caps bAllCaps + TextTransformPolicy |
| 2 (impl) | 1ad4ec0 | feat(19-02): implement TXT-CAPS-01 — bAllCaps field + parser + mapper wiring |
| 3 (req) | 4176851 | chore(19-02): mark TXT-CAPS-01 complete in REQUIREMENTS.md |

## Self-Check: PASSED

- `Source/PSD2UMG/Public/Parser/PsdTypes.h` — contains `bool bAllCaps = false;`
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` — contains `style_run_font_caps(DominantRunIdx)` and `OutLayer.Text.bAllCaps`
- `Source/PSD2UMG/Private/Mapper/FTextLayerMapper.cpp` — contains `SetTextTransformPolicy(ETextTransformPolicy::ToUpper)`
- `Source/PSD2UMG/Tests/PsdParserSpec.cpp` — contains 2x `TXT-CAPS-01`, root count asserts 9
- `Source/PSD2UMG/Tests/FTextPipelineSpec.cpp` — contains 2x `GetTextTransformPolicy`, `TXT-CAPS-01` markers
- `.planning/REQUIREMENTS.md` — `[x] TXT-CAPS-01` and traceability row Complete
- Commits 2e39972, 1ad4ec0, 4176851 confirmed in git log
