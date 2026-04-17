# Phase 9 — Unified Layer Naming Convention (tag-based)

**Status:** Design finalized via brainstorming (2026-04-14). Ready for `/gsd:plan-phase 9`.

## Goal

Replace the ad-hoc prefix/suffix naming conventions (`HBox_`, `VBox_`, `_variants`, etc.) with a single tag-based syntax that unifies widget type, sizing, anchors, and image draw-mode under one parser. Remove auto-detection heuristics (e.g. `DetectHorizontalRow`) — behavior must be explicit.

## Core Principle

**Tags encode only what cannot be resolved from the PSD element itself.** Pixel data, bounds, text styles, guides, and layer type come from PSD directly. Tags express designer *intent* that PSD has no way to record.

## Syntax

```
<Semantic Name> [tag1][tag2][tag3=value]
```

- Tags in square brackets, one or more, contiguous at the **end** of the layer name.
- Everything before the trailing `[…]` run is the semantic name.
- Tags are lowercase by convention; parser accepts any case and normalizes.
- Order of tags is not significant.
- If a bracketed block fails to parse as a known tag/value, it is ignored with a warning (layer name may legitimately contain `[WIP]` etc.).
- Unknown tag → warning, layer still processed.

## Tag Vocabulary

### Widget type (groups only; exactly one allowed)

| Tag | Widget |
|---|---|
| *(group, no tag)* | `CanvasPanel` |
| `[hbox]` | `HorizontalBox` |
| `[vbox]` | `VerticalBox` |
| `[hscroll]` | `ScrollBox` (horizontal) |
| `[vscroll]` | `ScrollBox` (vertical) |
| `[list]` | `UListView` (EntryWidgetClass set manually) |
| `[tile]` | `UTileView` (EntryWidgetClass set manually) |
| `[btn]` | `Button` (children become button content) |
| `[checkbox]` | `CheckBox` |
| `[slider]` | `Slider` |
| `[progress]` | `ProgressBar` |
| `[input]` | `EditableTextBox` |

**Root of the Widget Blueprint is always `CanvasPanel`.** Top-level PSD layers become its children. Tags on a wrapper group still apply to that group as a child of the root canvas.

### Sizing in parent (mutually exclusive; applicable to any widget)

| Tag | Behavior |
|---|---|
| *(none)* | Size and position from PSD bounds |
| `[fill]` | Fill parent on both axes |
| `[fillx]` | Fill width; height from PSD |
| `[filly]` | Fill height; width from PSD |

Semantics depend on parent:
- `CanvasPanel` child → anchors stretched on corresponding axes.
- `HBox`/`VBox` child → `FillSize=1.0` + `HAlign/VAlign=Fill` on the relevant axis.

### Anchor preset (CanvasPanel children only; mutually exclusive with `[fill*]`)

`[anchor=tl|t|tr|l|c|r|bl|b|br]` — nine presets. Default (no tag) = `tl` (top-left, matches PSD coordinates).

### Image pixel draw-mode (image layers only; mutually exclusive)

| Tag | Draw-as | Parameter source |
|---|---|---|
| *(none)* | `Image` | — |
| `[9slice]` | `Box` | Margins read from PSD **guides** intersecting the layer bounds; fallback = 25% of dimensions |
| `[tiled]` | `Border` | — |

### Cross-cutting

| Tag | Behavior |
|---|---|
| `[ignore]` | Skip this layer (and all its descendants) during generation |

**PSD layer visibility (the eyeball) === `[ignore]`.** Hiding a layer in Photoshop removes it from generation. No separate `[hidden]` / `[nohit]` tags in this phase.

## Edge Case Rules

1. **Name parsing:** Try to parse every bracketed block `[…]` at the tail as a tag. Blocks that fail to parse are kept as part of the semantic name (so `Panel [WIP] [fill]` → name=`Panel [WIP]`, tag=`fill`). *Implementation note:* parse right-to-left; stop at the first non-tag block.

2. **Case:** Any case accepted; normalized to lowercase internally.

3. **Name sanitization to `FName`:** Strip characters illegal for UMG `FName` (emoji, control chars, leading/trailing whitespace), preserving readable Unicode where UE supports it. Emit a warning when sanitization changes the name.

4. **Duplicate names at the same tree level:** auto-suffix `_1`, `_2`, … (silent, deterministic order = PSD layer order).

5. **Conflicting tags → error, import aborts with clear diagnostic.** Conflict classes:
   - Two widget-type tags on one group (`[hbox][vbox]`)
   - Two sizing tags (`[fill][fillx]`)
   - `[fill*]` + `[anchor=…]`
   - Two image draw-mode tags (`[9slice][tiled]`)

6. **Smart Objects:** auto-detect by PSD layer type (`EPsdLayerType::SmartObject`); no tag required.

## Codebase Changes (high level)

1. **New `FLayerNameParser`** — single source of truth for `FString → { CleanName, TagMap }`. Used by all mappers and the generator.
2. **Mapping registry switches from prefix to tags:** `CanMap(Layer)` checks `Tags`, not `StartsWith`.
3. **Old mappers deleted (hard cut):** `FHBoxLayerMapper`, `FVBoxLayerMapper`, `FOverlayLayerMapper`, `FScrollBoxLayerMapper`, `FSwitcherLayerMapper`, `FSliderLayerMapper`, `FCheckBoxLayerMapper`, `FInputLayerMapper`, `FListLayerMapper`, `FTileLayerMapper`, `FButtonLayerMapper` (prefix form), `FProgressLayerMapper` (prefix form), `FVariantsSuffixMapper`. Replaced by tag-driven equivalents (some may collapse into a single table-driven mapper).
4. **Drop `Overlay`, `WrapBox`, `Grid`, `Border`, `SizeBox`, `Switcher`** from the convention (not from UMG — designers add in BP manually if needed).
5. **`FWidgetBlueprintGenerator::DetectHorizontalRow` removed.** No auto-HBox wrapping; groups without `[hbox]` stay `CanvasPanel`.
6. **Canvas-child slot setup** (anchors, fill) driven by parsed tags in the generator, not the mapper.
7. **HBox/VBox slot setup** (FillSize, alignment) also tag-driven; no weighted fill in this phase.
8. **New `FPsdGuideReader`** — extracts PSD document guides intersecting a layer's bounds → 9-slice margins. `F9SliceImageLayerMapper` consumes this instead of the `[L,T,R,B]` suffix.
9. **`[ignore]` handled in `FWidgetBlueprintGenerator`** before mapper dispatch; subtree is not recursed.
10. **PSD layer visibility flag treated as implicit `[ignore]`** at the parser level (single switch).

## Re-import Behavior

**Re-import regenerates the entire Widget Blueprint from scratch.** Tag changes (e.g. `[hbox]` → `[vbox]`) swap widget types cleanly. Manual BP edits are lost — this trade-off is accepted for predictability. Re-importer (`FPsdReimportHandler`) must be updated accordingly.

## Migration

**Hard cut — no backward compatibility.** Old PSD files using `HBox_`, `_variants`, `Button_`, etc. will stop working and must be re-tagged. Plugin is not yet published.

## Deferred (YAGNI)

- `[overlay]`, `[wrap]`, `[grid]`, `[border]`, `[sizebox]`
- `[switcher]`, `[page=…]`, `[state=…]` — button states and switcher pages to be authored in UMG directly.
- `[hidden]`, `[nohit]` — visibility flags.
- `[keep]` — non-stretching image inside a fill container.
- `[style=…]`, `[autowrap]`, `[justify=…]` — text properties all read from PSD.
- `[fill=N]` — weighted fill for HBox/VBox.

## Open Items for `/gsd:plan-phase`

- Decide whether the 12 existing prefix-based mappers collapse into a single `FTaggedWidgetMapper` driven by a lookup table, or retain per-widget classes with shared tag parsing.
- Plan order-of-operations: parser first, then mapper rewrite, then slot-application, then guide reader, then re-importer update, then fixture PSD regeneration for tests.
- Existing test fixture PSDs under `Source/PSD2UMG/Tests/Fixtures/` use prefix convention — they must be regenerated or edited.
- README's "Layer Naming" section (`Phase 08` DOC-02) must be rewritten.
