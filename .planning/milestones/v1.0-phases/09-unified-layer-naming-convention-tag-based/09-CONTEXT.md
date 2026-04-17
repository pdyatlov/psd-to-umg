# Phase 9: Unified Layer Naming Convention (Tag-Based) - Context

**Gathered:** 2026-04-14
**Status:** Ready for planning

<domain>
## Phase Boundary

Replace the current ad-hoc PSD layer-name parsing (prefixes `Button_`, `Image_`, `Progress_`, …; suffix modifiers `_anchor-tl`, `_stretch-h`, `_9s`, `_variants`, `_show`/`_hide`/`_hover`; child-state suffixes `_hovered`, `_pressed`, `_fill`, `_bg`; bracketed parameters `[IA_X]`, `[L,T,R,B]`) with a single unified `@`-tag grammar. Hard cutover — old syntax stops working. All importers, mappers, animation builder, anchor calculator, and tests must speak the new grammar. Documentation and existing test-fixture PSDs (Phase 8) are migrated.

Out of scope: adding any new widget capabilities, new state types, or new modifiers beyond what the current system supports (this is a refactor + grammar consolidation, not a feature expansion).

</domain>

<decisions>
## Implementation Decisions

### Tag Syntax
- **D-01:** Layer name format is `WidgetName @tag @tag:value @tag:v1,v2,…` — clean name first, then `@`-prefixed tags separated by whitespace.
- **D-02:** Exactly one **type tag** is required per layer (`@button`, `@image`, `@text`, `@progress`, `@hbox`, `@vbox`, `@overlay`, `@scrollbox`, `@slider`, `@checkbox`, `@input`, `@list`, `@tile`, `@smartobject`, `@canvas`). Group layers without a type tag default to `@canvas`. Pixel layers without a type tag default to `@image`. Text layers without a type tag default to `@text`.
- **D-03:** Tag values use `:` separator. Multi-value tags use comma: `@9s:16,16,16,16`, `@anchor:tl`. No spaces inside a single tag.
- **D-04:** Tags are **case-insensitive** (both tag names and string values). Tag values that reference UE asset names (e.g., `@ia:IA_Confirm`) preserve case for the lookup.

### Tag Inventory (Refactored & Consolidated)
- **D-05:** Type tags: `@button @image @text @progress @hbox @vbox @overlay @scrollbox @slider @checkbox @input @list @tile @smartobject @canvas`. **Dropped:** `@switcher` is replaced by `@variants` modifier on a group (one way to do it).
- **D-06:** Anchor/layout tags **consolidated** under `@anchor:*`: `@anchor:tl|tc|tr|cl|c|cr|bl|bc|br|stretch-h|stretch-v|fill`. The old separate `_stretch-h`, `_stretch-v`, `_fill` suffixes collapse into `@anchor:stretch-h`, `@anchor:stretch-v`, `@anchor:fill`. Auto-anchor heuristics still apply when `@anchor` is absent.
- **D-07:** 9-slice tag: `@9s:L,T,R,B` (e.g., `@9s:16,16,16,16`). The old `_9s` / `_9slice` (no margins) collapses to `@9s` (zero-margin shorthand) — to be confirmed by planner; fallback is to require explicit margins.
- **D-08:** Variants/switcher tag: `@variants` on a group makes it a `UWidgetSwitcher` over its children (replaces both `Switcher_` prefix and `_variants` suffix).
- **D-09:** Input action tag (CommonUI): `@ia:IA_ActionName` on a button.
- **D-10:** Animation tags (CommonUI): `@anim:show`, `@anim:hide`, `@anim:hover` on a child group inside a CommonUI button — replaces old `_show`/`_hide`/`_hover` suffix.
- **D-11:** SmartObject tag carries the type identifier as value: `@smartobject:TypeName` (current code reads SmartObject type from the layer name; planner verifies exact mapping during research).

### State-Brush Convention (Centralized)
- **D-12:** Child layers that act as state brushes use `@state:name` uniformly across all parent widget types: `@state:hover`, `@state:pressed`, `@state:disabled`, `@state:normal`, `@state:fill`, `@state:bg`. The button mapper, progress mapper, and any future stateful widget all read the same `@state:*` tag from children. Centralized state lookup, not per-mapper hardcoded suffixes.
- **D-13:** When a parent widget needs a state child but none has the matching `@state:*`, fall back to the first child with no state tag (preserves "first child = normal" current behavior).

### Case Sensitivity & Backwards Compatibility
- **D-14:** **All tags case-insensitive.** No exceptions. (Resolves the Phase 3 vs Phase 7 inconsistency by picking case-insensitive uniformly.)
- **D-15:** **No backwards compatibility.** Old `Button_X`, `_anchor-tl`, `_9s`, `[IA_X]`, `[L,T,R,B]`, `_show`/`_hide`/`_hover`, `_hovered`/`_pressed`, `_fill`/`_bg` syntax is **removed entirely**. Code paths that detect old syntax are deleted, not deprecated. Designer PSDs must be retagged.

### Tag Chaining, Conflict, Unknown-Tag Rules
- **D-16:** Tags are **order-free** within a layer name. Parser collects all `@`-tokens regardless of position.
- **D-17:** **Conflict resolution:** when two mutually-exclusive tags appear (e.g., `@anchor:tl @anchor:c`, or two type tags), the **last one wins** and a warning is logged with the layer path.
- **D-18:** **Unknown tags:** the importer logs a warning ("Unknown tag `@foo` on layer `…` — ignored") and continues. Import does not fail.
- **D-19:** **Multiple type tags = error-with-fallback:** log error, use the last type tag (per D-17), continue import.

### Widget Name Extraction
- **D-20:** Widget name = verbatim text **before the first `@`-token**, with **leading/trailing whitespace trimmed** and **internal spaces replaced with underscores**. Special characters are passed through as-is (UMG name validation happens downstream — invalid names produce a clear error pointing back at the offending layer).
- **D-21:** Empty name (layer named only `@button`) → fallback to `<TypeTag>_<LayerIndex>` (e.g., `Button_07`). Warning logged.

### Import Preview Dialog — Tag Visibility (D-26)
- **D-26:** The Import Preview Dialog (`SPsdImportPreviewDialog`) MUST show, on each layer row, the tags that were parsed from that layer's name. Two visual groups:
  - **Recognized tags** — rendered as small neutral chips next to the widget-type badge, e.g. `@button` `@anchor:tl` `@9s:16,16,16,16`. Color: subtle (grey/blue), readable.
  - **Unknown / conflicting tags** — rendered as warning chips (orange/yellow) with a tooltip explaining the diagnostic ("Unknown tag — ignored", "Conflicts with earlier `@anchor:c` — last wins"). Source: the `Diag` string returned by `FLayerTagParser::Parse` for that layer.
  - Layer rows with no tags show no chips (clean name only — still readable at a glance).
  - Chips are informational only; they do NOT add checkboxes or per-tag include/exclude controls (out of scope).
- **D-27:** The per-layer `FParsedLayerTags` + parser diagnostics must be threaded into the `FPsdLayerTreeItem` that backs each tree row so the UI can render without re-parsing.

### Migration & Documentation Deliverables
- **D-22:** Ship a formal **tag-grammar spec** (EBNF or equivalent) at `Docs/LayerTagGrammar.md` covering syntax, full tag inventory, case rules, conflict/unknown handling, name extraction, examples.
- **D-23:** Ship a **migration guide** at `Docs/Migration-PrefixToTag.md` mapping every old prefix/suffix/bracket pattern to its new `@`-tag equivalent, with before/after examples.
- **D-24:** **Retag the existing test-fixture PSDs from Phase 8** (`Tests/Fixtures/*.psd` — SimpleHUD, Effects, etc.) to use the new grammar. Existing parser/generator specs updated to assert against the new layer-name parsing.
- **D-25:** **Update README.md** layer-naming cheat sheet (currently lines ~39-96) to show only the new grammar — remove the old prefix/suffix tables.

### Claude's Discretion
- Internal C++ representation of a parsed layer name (struct vs map of tags). Planner picks the cleanest API.
- Where the central tag parser lives (`Source/PSD2UMG/Private/Parser/FLayerTagParser.{h,cpp}` is the obvious home, but the planner can pick a different layout).
- Whether `@9s` shorthand without margins is supported, or margins are required (D-07).
- Exact wording of warning/error messages.
- Performance characteristics — the parser runs once per layer at import time, no special optimization required.
- Whether to expose the tag registry to Blueprint/Python (probably not — internal C++).
- How `@smartobject:TypeName` value maps to current `FSmartObjectImporter` behavior (research output drives this).

</decisions>

<specifics>
## Specific Ideas

- "Remove the prefixes completely. Replace them with tags." — user, 2026-04-14. This is a hard refactor, not a layered system.
- "PlayButton @button @anchor:tl" — user-supplied canonical example of the syntax shape.
- "@state:name" — user-confirmed unified state-brush tag, applied uniformly across button/progress/future stateful widgets.
- The grammar must look natural when written by a designer in Photoshop's Layers panel — terse, no quotes, no nested structures, easy to type.

</specifics>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Roadmap / Project
- `.planning/ROADMAP.md` — Phase 9 entry (currently goal "[To be planned]" — this CONTEXT.md is the source of truth for the goal)
- `.planning/PROJECT.md` — overall project vision and constraints
- `.planning/REQUIREMENTS.md` — project-level requirements (no naming-specific REQs to date)

### Prior Phase Decisions That This Phase Supersedes
- `.planning/phases/03-layer-mapping-widget-blueprint-generation/03-CONTEXT.md` — D-02 (case-sensitive prefix dispatch), D-05/D-06 (anchor heuristics), D-07 (anchor suffix list). All prefix/suffix conventions here are **replaced**.
- `.planning/phases/06-advanced-layout/06-CONTEXT.md` — D-01 (`_9s`/`_9slice`), D-02 (`[L,T,R,B]` margin), D-03 (suffix stripping), D-13/D-14/D-15 (`_variants`). **Replaced** by `@9s:L,T,R,B` and `@variants`.
- `.planning/phases/07-editor-ui-preview-settings/07-CONTEXT.md` — D-09 (CommonUI button mapping), D-10 (`[IA_X]` syntax), D-11 (`_show`/`_hide`/`_hover` animation suffixes). **Replaced** by `@ia:…` and `@anim:show|hide|hover`.

### Code Files That Embody The Old Conventions (To Be Rewritten)
- `Source/PSD2UMG/Private/Mapper/FButtonLayerMapper.cpp` — `Button_` prefix + child state suffixes
- `Source/PSD2UMG/Private/Mapper/FCommonUIButtonLayerMapper.cpp` — case-insensitive `Button_` + `[IA_X]` parsing
- `Source/PSD2UMG/Private/Mapper/FProgressLayerMapper.cpp` — `Progress_` prefix + `_fill`/`_bg` children
- `Source/PSD2UMG/Private/Mapper/FSimplePrefixMappers.cpp` — 12 prefix mappers (HBox, VBox, Overlay, ScrollBox, Slider, CheckBox, Input, List, Tile, Switcher, +9s suffix detection)
- `Source/PSD2UMG/Private/Mapper/F9SliceImageLayerMapper.cpp` — `_9s`/`_9slice` + `[L,T,R,B]` parsing
- `Source/PSD2UMG/Private/Mapper/FVariantsSuffixMapper.cpp` — `_variants` suffix
- `Source/PSD2UMG/Private/Mapper/FSmartObjectLayerMapper.cpp` — SmartObject type lookup from name
- `Source/PSD2UMG/Private/Mapper/FImageLayerMapper.cpp`, `FTextLayerMapper.cpp`, `FGroupLayerMapper.cpp` — generic fallbacks
- `Source/PSD2UMG/Private/Generator/FAnchorCalculator.cpp` — `_anchor-*`, `_stretch-*`, `_fill` suffix table (lines ~23-36)
- `Source/PSD2UMG/Private/Animation/FPsdWidgetAnimationBuilder.cpp` — `_show`/`_hide`/`_hover` suffix detection (lines ~147-155)
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` — current name parsing (verify whether tag parsing belongs here or in a new dedicated module)

### Existing Test Fixtures To Retag
- `Tests/Fixtures/*.psd` (all of them — SimpleHUD, Effects, others from Phase 8) — content needs new tag syntax in layer names. Generation scripts (if any) updated.
- `Source/PSD2UMG/Private/Tests/` — parser/generator/integration spec files that assert against layer names — all updated to the new grammar.

### Documentation To Update / Create
- `README.md` — naming cheat sheet section (~lines 39-96) replaced.
- `Docs/LayerTagGrammar.md` — **new**, formal grammar spec (D-22).
- `Docs/Migration-PrefixToTag.md` — **new**, migration guide (D-23).

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- The mapper-priority architecture (each mapper has a `CanMap`/`Priority`/`Map` interface) survives unchanged — only each mapper's `CanMap` logic is rewritten to read the parsed tag struct instead of doing its own substring matching.
- `FAnchorCalculator` already separates "explicit override" from "auto-heuristic" cleanly — the override path just changes its input source from suffix-strip to tag-lookup.
- The animation builder's CommonUI gating (`bUseCommonUI`) stays the same — only the tag detection is swapped.

### Established Patterns
- All current layer-name parsing is **scattered across mappers** (each mapper does its own `StartsWith`/`EndsWith`/`FindLastChar`). The new design should **centralize tag parsing in one module** and pass a parsed-tag-struct to mappers — this is the architectural win of Phase 9, not just a syntax change.
- Suffix stripping is currently per-mapper (each one trims its known suffix off the widget name). New design strips ALL `@`-tokens once during parsing — name extraction is unified.

### Integration Points
- `FPsdLayer` (or wherever the parsed PSD layer struct lives) gains a `FParsedLayerTags` field populated by the new parser.
- Mappers consume `FParsedLayerTags` instead of `Layer.Name` for dispatch.
- `FAnchorCalculator`, `FPsdWidgetAnimationBuilder`, `FSmartObjectImporter`, `FCommonUIButtonLayerMapper` all read from the same parsed struct.

</code_context>

<deferred>
## Deferred Ideas

- **Tag registry exposed to Blueprint/Python** — out of scope. C++-internal only for this phase.
- **Custom designer-defined tags** (project-specific extensions) — out of scope; phase ships a fixed inventory.
- **PSD layer metadata / custom properties** as an alternative to name parsing — explicitly rejected for Phase 9 (user picked name-based tags). Could be a future phase if PhotoshopAPI exposes the metadata.
- **Linter/auto-fixer tool** that converts old prefixed PSDs to new tag syntax automatically — nice-to-have, not in this phase. Migration guide is manual.
- **Auto-anchor heuristic vs explicit `@anchor` conflict resolution review** — current behavior (explicit overrides heuristic) is preserved as-is. A formal review of the heuristic itself is deferred.
- **Adding new widget capabilities** during this refactor — explicitly out of scope. Phase 9 = grammar swap only.

</deferred>

---

*Phase: 09-unified-layer-naming-convention-tag-based*
*Context gathered: 2026-04-14*
