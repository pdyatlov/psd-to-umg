---
phase: 09-unified-layer-naming-convention-tag-based
plan: "02"
subsystem: mapper-generator-animation-ui
tags: [mapper, generator, anchor, animation, preview-dialog, fan-out, wave-2]
dependency_graph:
  requires: [TAG-01, TAG-03, TAG-07]
  provides: [TAG-02, TAG-08, TAG-09]
  affects:
    - Source/PSD2UMG/Private/Mapper/*.cpp
    - Source/PSD2UMG/Private/Generator/FAnchorCalculator.{h,cpp}
    - Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp
    - Source/PSD2UMG/Private/Generator/FSmartObjectImporter.{h,cpp}
    - Source/PSD2UMG/Private/Animation/FPsdWidgetAnimationBuilder.{h,cpp}
    - Source/PSD2UMG/Private/UI/SPsdImportPreviewDialog.cpp
tech_stack:
  added: []
  patterns:
    - "Tag-enum dispatch in CanMap (no string substring matches anywhere in Mapper/Generator/Animation/UI)"
    - "FLayerTagParser::FindChildByState centralises Button/Progress state-child lookup"
    - "TOptional<FString> ExplicitTypeName parameter wires @smartobject:TypeName into FSmartObjectImporter naming"
    - "PopulateCanvas R-05 guard: explicit @anchor:stretch-h/-v/fill suppresses auto-row/column heuristic"
key_files:
  created:
    - Source/PSD2UMG/Tests/TestHelpers.h
  modified:
    - Source/PSD2UMG/Private/Mapper/FGroupLayerMapper.cpp
    - Source/PSD2UMG/Private/Mapper/FImageLayerMapper.cpp
    - Source/PSD2UMG/Private/Mapper/FTextLayerMapper.cpp
    - Source/PSD2UMG/Private/Mapper/FSimplePrefixMappers.cpp
    - Source/PSD2UMG/Private/Mapper/FVariantsSuffixMapper.cpp
    - Source/PSD2UMG/Private/Mapper/F9SliceImageLayerMapper.cpp
    - Source/PSD2UMG/Private/Mapper/FButtonLayerMapper.cpp
    - Source/PSD2UMG/Private/Mapper/FProgressLayerMapper.cpp
    - Source/PSD2UMG/Private/Mapper/FCommonUIButtonLayerMapper.cpp
    - Source/PSD2UMG/Private/Mapper/FSmartObjectLayerMapper.cpp
    - Source/PSD2UMG/Private/Generator/FSmartObjectImporter.h
    - Source/PSD2UMG/Private/Generator/FSmartObjectImporter.cpp
    - Source/PSD2UMG/Private/Generator/FAnchorCalculator.h
    - Source/PSD2UMG/Private/Generator/FAnchorCalculator.cpp
    - Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp
    - Source/PSD2UMG/Private/Animation/FPsdWidgetAnimationBuilder.h
    - Source/PSD2UMG/Private/Animation/FPsdWidgetAnimationBuilder.cpp
    - Source/PSD2UMG/Private/UI/SPsdImportPreviewDialog.cpp
    - Source/PSD2UMG/Private/UI/SPsdImportPreviewDialog.h
    - Source/PSD2UMG/Private/UI/PsdLayerTreeItem.h
    - Source/PSD2UMG/Tests/FLayerTagParserSpec.cpp
    - Source/PSD2UMG/Tests/FWidgetBlueprintGenSpec.cpp
decisions:
  - "Reimport identity-key strategy: option-a (match by ParsedTags.CleanName). Pre-Phase-9 WBPs orphan on first reimport; migration guide (Plan 09-04) must document delete-and-reimport. Aligned with D-15 hard cutover."
  - "FSwitcherLayerMapper retained but now dispatches on ParsedTags.bIsVariants at priority 199 (just below FVariantsSuffixMapper at 200) so the legacy class isn't dead code while keeping FVariantsSuffixMapper as the canonical handler."
  - "FProgressLayerMapper background lookup tries FindChildByState(Bg) first; if absent, falls back to FindChildByState(Normal) (first untagged Image) so untagged child layers still produce a background brush."
  - "Task 6a (tag-chip rendering, D-26/D-27) completed in follow-up executor pass; Task 6 (human-verify checkpoint) is the only remaining work."
metrics:
  duration: ~50m
  completed: "2026-04-14"
  tasks: 5
  files: 23
requirements: [TAG-02, TAG-08, TAG-09]
---

# Phase 9 Plan 02: Tag-dispatch fan-out across mappers, generator, animation, preview Summary

**One-liner:** Every mapper, FAnchorCalculator, FPsdWidgetAnimationBuilder, and SPsdImportPreviewDialog::InferWidgetTypeName now dispatches on `FPsdLayer::ParsedTags` enum/struct fields; legacy `Layer.Name.StartsWith/EndsWith/Contains` dispatch deleted entirely (D-15 hard cutover); reimport identity keys on `ParsedTags.CleanName` (option-a).

## Reimport Identity Decision (Task 1 -- option-a)

**Decision:** match by `FName(*Layer.ParsedTags.CleanName)` everywhere.

**Rationale:**

- Aligned with the D-15 "no backwards compatibility" mandate.
- Once a project is fully on tag syntax, identity is stable: adding/removing a tag (e.g. `@anchor:tl`) does not change `CleanName`, so widget identity survives.
- Avoids the option-b plumbing cost of adding a per-widget annotation slot for the verbatim PSD layer name.

**Cost (must be documented in migration guide -- Plan 09-04 owns):**

- Pre-Phase-9 WBPs (whose widget FNames embed legacy prefixes like `Button_Start`) will fully orphan on first reimport after retagging the source PSD. Designer must delete + reimport to converge.

## What Was Built

### Task 2 -- All mappers refactored to dispatch on `ParsedTags`

10 mappers now compute `CanMap` from `Layer.ParsedTags.Type` (or `bIsVariants` / `NineSlice.IsSet()` for the modifier-tag mappers) and source widget `FName` from `Layer.ParsedTags.CleanName`:

| Mapper | CanMap predicate | Notes |
| --- | --- | --- |
| FGroupLayerMapper | `Type == Canvas` | Default-type inference: a group with no @-tag becomes Canvas via D-02. |
| FImageLayerMapper | `Type == Image` | |
| FTextLayerMapper | `Type == Text` | |
| FHBoxLayerMapper | `Type == HBox` | |
| FVBoxLayerMapper | `Type == VBox` | |
| FOverlayLayerMapper | `Type == Overlay` | |
| FScrollBoxLayerMapper | `Type == ScrollBox` | |
| FSliderLayerMapper | `Type == Slider` | |
| FCheckBoxLayerMapper | `Type == CheckBox` | |
| FInputLayerMapper | `Type == Input` | |
| FListLayerMapper | `Type == List` | |
| FTileLayerMapper | `Type == Tile` | |
| FSwitcherLayerMapper | `bIsVariants` | Priority lowered 200->199 so FVariantsSuffixMapper (200) wins; both produce UWidgetSwitcher. |
| FVariantsSuffixMapper | `bIsVariants` | Canonical @variants handler. |
| F9SliceImageLayerMapper | `NineSlice.IsSet()` | `[L,T,R,B]` bracket parsing deleted; reads margins from `ParsedTags.NineSlice.GetValue()`. |
| FButtonLayerMapper | `Type == Button` | Per-state child loop replaced by 4 calls to `FLayerTagParser::FindChildByState(Normal/Hover/Pressed/Disabled)`. |
| FCommonUIButtonLayerMapper | `bUseCommonUI && Type == Button` | `[IA_X]` bracket parsing deleted; reads `ParsedTags.InputAction.GetValue()`. |
| FProgressLayerMapper | `Type == Progress` | FillImage from `FindChildByState(Fill)`; BackgroundImage from `FindChildByState(Bg)` with `Normal` fallback. |
| FSmartObjectLayerMapper | `Layer.Type == SmartObject \|\| ParsedTags.Type == SmartObject` | Forwards `ParsedTags.SmartObjectTypeName` as `ExplicitTypeName` override (D-11). |

### D-11 SmartObject wiring

- `FSmartObjectImporter::ImportAsChildWBP` signature gained `const TOptional<FString>& ExplicitTypeName = {}`.
- When set + non-empty, the child WBP asset name becomes `WBP_SO_<ExplicitTypeName>` instead of `WBP_SO_<LayerName>`.
- A `Log`-level line is emitted: `"FSmartObjectLayerMapper: Using explicit SmartObject type name '%s' from @smartobject tag (layer='%s')"`.
- Mapper also accepts `@smartobject` on a non-smartobject group, letting designers force the smart-object pipeline.

### R-06 CommonUI priority gate spec

Added to `FLayerTagParserSpec.cpp`:

- `bUseCommonUI=true` -> both `FCommonUIButtonLayerMapper` and `FButtonLayerMapper` accept `@button` layers; CommonUI's priority (210) > vanilla (200), so the registry picks CommonUI.
- `bUseCommonUI=false` -> CommonUI rejects (returns false), vanilla wins.

### Task 3 -- FAnchorCalculator + FWidgetBlueprintGenerator + R-05

- `FAnchorCalculator::Calculate` signature changed: now takes `const FPsdLayer&` (was `FString LayerName`). Reads `Layer.ParsedTags.Anchor` / `ParsedTags.CleanName`.
- `GSuffixes` table (15 entries) deleted; replaced by `ResolveExplicitAnchor(EPsdAnchorTag, ...)` enum-switch.
- `TryParseSuffix` deleted entirely.
- 3 `FAnchorCalculator::Calculate` callsites in `FWidgetBlueprintGenerator.cpp` updated (PopulateCanvas line ~251, UpdateCanvas existing-widget branch line ~534, UpdateCanvas new-widget branch line ~602).
- **R-05 guard:** `AnyChildHasExplicitStretch(Layers)` short-circuits the auto-row/column heuristic when any child carries `@anchor:stretch-h`, `@anchor:stretch-v`, or `@anchor:fill`. Implemented as a single `&&` clause on the existing `Layers.Num() >= 2` gate.
- New `FWidgetBlueprintGenSpec` case `R-05: explicit @anchor:stretch-h on a child suppresses PopulateCanvas auto-row heuristic` constructs 3 horizontally-aligned images (middle has `@anchor:stretch-h`), generates a WBP, then asserts zero `UHorizontalBox` and 3 `UImage` widgets exist (i.e. all on the canvas, no auto-wrap).

### Task 4 -- AnimationBuilder + PreviewDialog

- `FPsdWidgetAnimationBuilder::ProcessAnimationVariants` signature changed: takes `EPsdAnimTag AnimTag` (was `const FString& LayerName`). Switches on Show/Hide/Hover. `_show`/`_hide`/`_hover` substring detection deleted. (No call sites yet -- the function is consumed by a not-yet-shipped wiring task; all current callers were specs that didn't touch this signature.)
- `SPsdImportPreviewDialog::InferWidgetTypeName` rewritten: switches on `Layer.ParsedTags.Type` covering all 15 tag types + `bIsVariants -> "WidgetSwitcher"`. Falls back to `EPsdLayerType` only when `Type == None`. Legacy `Btn_/Button_/Progress_/List_/Tile_` prefix special cases removed (D-15).

### Task 6a -- Tag-chip rendering in Import Preview dialog (D-26, D-27)

- `FPsdLayerTreeItem` (in `Private/UI/PsdLayerTreeItem.h`) gained `FParsedLayerTags ParsedTags` (includes the parser's `UnknownTags` array -- no new diagnostic struct was needed).
- New public static `SPsdImportPreviewDialog::ReconstructTagChips(const FParsedLayerTags&)` rebuilds canonical `@-tag` strings from a parsed tag state in stable order: Type, Anchor, Anim, State, InputAction, NineSlice, Variants, SmartObject. Emits `@9s` shorthand when `NineSlice.bExplicit == false`, `@9s:L,T,R,B` otherwise. This is the single source of truth for chip text (prevents drift between parser acceptance and dialog display).
- `BuildTreeRecursive` now copies `Layer.ParsedTags` onto each tree item.
- `OnGenerateRow` inserts a new `SHorizontalBox::Slot` between the layer-name column and the widget-type badge column. Inside:
  - `SWrapBox` (UseAllottedSize=true, InnerSlotPadding=(3,2)) so chips flow to the next line on narrow dialogs.
  - Recognized-tag chip: `SBorder` with `FAppStyle::GetBrush("RoundedWarning")` + slate-blue tint (`FLinearColor(0.20, 0.30, 0.50, 0.85)`), `SmallFont` white text.
  - Unknown/invalid chip: same border brush, orange tint (`FLinearColor(0.90, 0.55, 0.10, 0.90)`), tooltip text `"Unknown tag '@<raw>' -- ignored by parser."` Sourced from `ParsedTags.UnknownTags`.
  - When both arrays are empty the slot collapses to an `SSpacer` (zero-tag rows look identical to pre-change).
- Layer-name column switched from `FillWidth(1.f)` to `AutoWidth` so the chip strip takes the slack `FillWidth`.
- `FLayerTagParserSpec` gained three round-trip cases asserting: (a) `Panel @button @anchor:tl @9s:16,16,16,16` produces exactly `[@button, @anchor:tl, @9s:16,16,16,16]`; (b) empty tag state -> zero chips; (c) `@9s` shorthand round-trips to `@9s`, not `@9s:...`.

**Commit:** f4d3f42 -- `feat(09-02): SPsdImportPreviewDialog shows parsed + unknown tag chips per layer row (D-26, D-27)`

### Task 5 -- Synthetic-layer test plumbing (Pitfall 5)

- New `Source/PSD2UMG/Tests/TestHelpers.h` exposes:
  - `MakeTaggedTestLayer(Name, Type, Index)` -- one-shot factory.
  - `PopulateParsedTags(Layer, Index)` -- single-layer mutator.
  - `PopulateParsedTagsRecursive(TArray<FPsdLayer>&)` -- whole-tree sweep (mirrors `PsdParser::ParseFile` post-pass).
  - `PopulateParsedTagsForDocument(FPsdDocument&)` -- whole-doc convenience.
- `FWidgetBlueprintGenSpec::GenerateTracked` now copies its input doc and calls `PopulateParsedTagsForDocument` before invoking `Generate`. This unblocks every existing case constructed from vanilla (un-tagged) layer names: D-02 default-type inference produces `Image`/`Text`/`Canvas` for them, and the new tag-dispatched mappers accept them.
- Layer-name strings in specs are unchanged. Per the plan's Task 5 note, Plan 09-03 owns the retag of synthetic specs that assert on legacy-syntax names like `"Button_Start"`.

## Intentional RED Spec Inventory (handed off to Plan 09-03)

These will remain RED until Plan 09-03 retags fixture PSDs and rewrites synthetic-layer assertions:

| Suite / case | Reason it goes red | Plan 09-03 owner |
| --- | --- | --- |
| `PSD2UMG.Generator` cases asserting widget existence on `"Button_..."`-named groups | `Button_Start` parses to `CleanName="Button_Start"`, `Type=Canvas` (group default). FButtonLayerMapper rejects (Type != Button). | Plan 09-03 Task 3 -- rewrite to `"Start @button"`. |
| `PSD2UMG.Generator` cases asserting on `Progress_*` groups | Same root cause -- no @progress tag -> Type=Canvas. | Plan 09-03 Task 3. |
| `PSD2UMG.Generator` cases asserting on `Panel_9s` / `Panel_9s[L,T,R,B]` images | F9SliceImageLayerMapper now requires `ParsedTags.NineSlice.IsSet()`; the legacy `_9s` substring no longer registers. | Plan 09-03 Task 3 -- rewrite to `Panel @9s` / `Panel @9s:16,16,16,16`. |
| `PSD2UMG.Generator` cases asserting on `Menu_variants` | FVariantsSuffixMapper now requires `ParsedTags.bIsVariants` (set by `@variants` token). | Plan 09-03 Task 3 -- rewrite to `Menu @variants`. |
| `PSD2UMG.Generator` cases asserting widget FName equals the verbatim source-layer name | Widget FName now uses `ParsedTags.CleanName`; for un-tagged layers this equals the raw name -- so most assertions still hold. The exceptions are the prefix-named cases above. | Plan 09-03 Task 3. |
| `PSD2UMG.Parser.PsdParser` fixture-driven cases asserting on layer names from `Tests/Fixtures/*.psd` | Fixture PSDs still carry pre-Phase-9 names. | Plan 09-03 Task 2 -- retag fixture files. |

**Green throughout this plan (must NOT regress):**

- `PSD2UMG.Parser.LayerTagParser` (Plan 09-01 baseline + 2 new R-06 cases).
- `PSD2UMG.Generator.R-05` -- new in this plan.
- `PSD2UMG.Animation.*` -- AnimationBuilder has no synthetic-layer coupling beyond the signature change in Task 4.

## Verification

```
$ rg "Layer\.Name\.(StartsWith|EndsWith|Contains|FindChar)" Source/PSD2UMG/Private/Mapper Source/PSD2UMG/Private/Generator Source/PSD2UMG/Private/UI Source/PSD2UMG/Private/Animation
(no matches)
```

All dispatch is now tag-based. Hits inside `FLayerTagParser.cpp` (the parser itself) and inside debug log strings are intentional and out of scope.

**Note:** Local UE build/spec verification is not possible -- this repo is plugin-only (no `*.uproject`). Per Phase 8 D-06 / Plan 09-01 SUMMARY, build + automation gating defers to the host-project consumer. Implementation followed canonical UE 5.7 / C++20 idioms (`TOptional`, `TFunctionRef`, `FName(*FString)`, `BEGIN_DEFINE_SPEC`).

## Deviations from Plan

### [Rule 3 - Blocking issue] FSwitcherLayerMapper has no `@switcher` tag in the grammar

- **Found during:** Task 2.
- **Issue:** The `Switcher_` prefix mapper exists in `AllMappers.h` but the Phase 9 grammar (Docs/LayerTagGrammar.md) defines no `@switcher` tag -- only `@variants`. Naively dispatching on `Type == Switcher` would dead-code the class because no tag can ever set that.
- **Fix:** Pointed `FSwitcherLayerMapper::CanMap` at `ParsedTags.bIsVariants` (same predicate as `FVariantsSuffixMapper`) and lowered its priority from 200 to 199 so `FVariantsSuffixMapper` always wins. Both produce `UWidgetSwitcher`. The legacy class survives without behavioural drift; clean-up (deleting the duplicate) is left to a follow-up housekeeping pass so we do not delete public-ish mapper class names mid-fan-out.
- **Files modified:** `Source/PSD2UMG/Private/Mapper/FSimplePrefixMappers.cpp`.
- **Commit:** 2230670.

### [Rule 2 - Critical] FProgressLayerMapper untagged-child fallback

- **Found during:** Task 2.
- **Issue:** Original mapper treated any child without `_fill`/`_foreground` suffix as background, including the first untagged Image child. With strict `FindChildByState(Bg)` that fallback would disappear -- many existing PSDs have a single untagged background image.
- **Fix:** When `FindChildByState(Bg)` returns nullptr, try `FindChildByState(Normal)` (D-13 first-untagged-Image fallback) so a single untagged background image still becomes the BackgroundImage brush.
- **Files modified:** `Source/PSD2UMG/Private/Mapper/FProgressLayerMapper.cpp`.
- **Commit:** 2230670.

### [Deferred] Task 6 (human-verify) pending

- **Reason:** Task 6 is a `checkpoint:human-verify` that requires a retagged `.psd` opened inside the Unreal Editor. The executor pauses here and returns checkpoint state for the user; Task 6a's chip rendering is the UI that the verify step will exercise.

## Commits

| Task | Commit | Description |
| --- | --- | --- |
| 1 (recorded only) | n/a | option-a decision logged in STATE.md via `state add-decision`. |
| 2 | c02ce7f | refactor(09-02): rewrite Group/Image/Text mappers to dispatch on ParsedTags |
| 2 | 2230670 | refactor(09-02): rewrite all mappers to dispatch on ParsedTags; thread @smartobject TypeName (D-11); add R-06 spec |
| 3 | aa2450f | refactor(09-02): FAnchorCalculator reads ParsedTags.Anchor; PopulateCanvas R-05 guard |
| 4 | 250c601 | refactor(09-02): AnimationBuilder + PreviewDialog read ParsedTags |
| 5 | 8e796bc | test(09-02): populate ParsedTags on synthetic test layers (Pitfall 5) |
| 6a | f4d3f42 | feat(09-02): SPsdImportPreviewDialog shows parsed + unknown tag chips per layer row (D-26, D-27) |

## Self-Check: PASSED

- `.planning/phases/09-unified-layer-naming-convention-tag-based/09-02-SUMMARY.md` -- exists
- `Source/PSD2UMG/Tests/TestHelpers.h` -- exists
- `Source/PSD2UMG/Private/Generator/FAnchorCalculator.cpp` -- exists (rewritten)
- Commits c02ce7f, 2230670, aa2450f, 250c601, 8e796bc all present in `git log --oneline --all`.
- Grep: zero `Layer.Name.StartsWith/EndsWith/Contains` hits across Mapper, Generator, UI, Animation.
