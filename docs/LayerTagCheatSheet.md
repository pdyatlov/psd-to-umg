# PSD2UMG — Layer Tag Cheat Sheet

> **Audience:** UI designers authoring PSD files for the PSD2UMG import pipeline.
> **Version:** v1.0 (shipped 2026-04-15)
> **Source of truth:** [`Docs/LayerTagGrammar.md`](LayerTagGrammar.md) — normative spec with full EBNF and design decisions.

---

## How it works

PSD2UMG inspects every PSD layer name. Designers express widget intent — button, 9-slice image, anchor, state brush, animation, input action — using a uniform `@tag` syntax.

**Format:**

```
LayerName @tag @tag:value @tag:v1,v2,v3
```

**Basic rules:**

- The widget name is everything **before the first `@`**.
- Tags are separated by whitespace.
- Tag **order is irrelevant**.
- Tag **names** are case-insensitive (`@button` ≡ `@BUTTON`).
- Tag **values** that are enums are case-insensitive; **asset references** preserve case (`@ia:IA_Confirm`).

---

## Type tags

Each layer becomes exactly one widget type. If no type tag is present, the parser infers a default from the PSD layer kind.

| Tag | UMG widget | Default for |
|---|---|---|
| `@button` | `UButton` (or `UCommonButtonBase` in CommonUI mode) | — |
| `@image` | `UImage` | **pixel layers** |
| `@text` | `UTextBlock` | **text layers** |
| `@progress` | `UProgressBar` | — |
| `@hbox` | `UHorizontalBox` | — |
| `@vbox` | `UVerticalBox` | — |
| `@overlay` | `UOverlay` | — |
| `@scrollbox` | `UScrollBox` | — |
| `@slider` | `USlider` | — |
| `@checkbox` | `UCheckBox` | — |
| `@input` | `UEditableText` | — |
| `@list` | `UListView` | — |
| `@tile` | `UTileView` | — |
| `@canvas` | `UCanvasPanel` | **group layers** |
| `@smartobject` | external WBP via `FSmartObjectImporter` | **smart-object layers** |

---

## Modifier tags

Applied in addition to the type tag.

| Tag | Purpose | Example |
|---|---|---|
| `@anchor:<value>` | Override auto-anchor heuristic. | `Panel @anchor:fill` |
| `@9s` | 9-slice image with default margins `{16, 16, 16, 16}`. | `Frame @9s` |
| `@9s:L,T,R,B` | 9-slice with explicit pixel margins. | `Frame @9s:8,8,16,16` |
| `@variants` | Group becomes `UWidgetSwitcher`; children are switcher slots. | `Menu @variants` |
| `@background` | Mark an image inside a `@state:*` group as the state brush; layer is not exported as a child widget. | `bg @background` |
| `@state:<value>` | Mark a child layer as a state brush on its parent button. | `Hover @state:hover` |
| `@anim:<value>` | Mark a child group as an animation source on its parent. | `FadeIn @anim:show` |
| `@ia:<AssetName>` | Bind an EnhancedInput action by asset name (case preserved). | `Confirm @button @ia:IA_Confirm` |
| `@smartobject:<TypeName>` | Optional type annotation for smart-object layers. | `HUDPanel @smartobject:HUDPanel` |

### `@anchor` values

`tl` · `tc` · `tr` · `cl` · `c` · `cr` · `bl` · `bc` · `br` · `stretch-h` · `stretch-v` · `fill`

### `@state` values

`normal` · `hover` · `pressed` · `disabled` · `fill` · `bg`

### `@anim` values

`show` · `hide` · `hover`

---

## Rules and diagnostics

| Situation | Behavior |
|---|---|
| **Widget name extraction** | Text before first `@` is trimmed; internal spaces become `_`. |
| **Empty clean name** | Falls back to `<TypeTag>_<LayerIndex>` with zero-padded two-digit index (e.g. `@button` at layer 7 → `Button_07`). Warning logged. |
| **Conflicting tags, same family** | Last wins. Warning logged. (`@anchor:tl @anchor:c` → anchor = c) |
| **Multiple type tags** | Last wins. **Error** logged. (`@button @image` → Image) |
| **Unknown tags** | Captured in diagnostics, warning logged, parsing continues. |
| **Malformed tokens** | `@`, `@:value`, `@anchor:` → warning, ignored. |

---

## Worked examples

| Layer name | Widget name | Widget type | Notes |
|---|---|---|---|
| `PlayButton @button @anchor:tl` | `PlayButton` | Button | Anchored top-left |
| `My Widget @button` | `My_Widget` | Button | Internal space → `_` |
| `Background @image @anchor:fill` | `Background` | Image | Fills parent |
| `Panel @9s` | `Panel` | Image / Canvas | NineSlice `{16,16,16,16}` |
| `Panel @9s:8,8,16,16` | `Panel` | (default) | Explicit margins |
| `Title @text` | `Title` | TextBlock | — |
| `Confirm @button @ia:IA_Confirm` | `Confirm` | Button | Bound to `IA_Confirm` |
| `Menu @variants` | `Menu` | WidgetSwitcher | — |
| `Hover @state:hover` | `Hover` | Image | State = Hover |
| `FadeIn @anim:show` | `FadeIn` | Canvas | Animation source |
| `@button` (layer #7) | `Button_07` | Button | Empty-name fallback |
| `Btn @BUTTON @Anchor:TL` | `Btn` | Button | Case-insensitive |
| `X @button @image` | `X` | Image | Last-wins, error logged |
| `X @anchor:tl @anchor:c` | `X` | (default) | Last-wins, warn |
| `X @button @frobnicate` | `X` | Button | `frobnicate` captured as unknown |

---

## Authoring checklist

Before exporting a PSD for import:

- [ ] Every button layer has an `@button` tag (so CommonUI mode picks up `UCommonButtonBase` correctly).
- [ ] State brushes (hover / pressed / disabled) are child layers of their button, each tagged `@state:<value>`.
- [ ] 9-slice frames use `@9s` with explicit margins when the default 16 px is wrong.
- [ ] Input-action bindings use the exact UE asset name: `@ia:IA_Confirm`, not `@ia:ia_confirm`.
- [ ] Widget switchers (`@variants`) sit on a group layer; each child is a switcher slot.
- [ ] Button state groups with multiple images use `@background` on the intended brush layer to avoid ambiguity.
- [ ] Animation sources (`@anim:show|hide|hover`) sit under the widget they animate.
- [ ] No legacy syntax remains (`Button_Name`, `_anchor-tl`, `_9s`, `[IA_X]`, `[L,T,R,B]`, `_show`, `_hide`, `_hover`, `_pressed`, `_fill`, `_bg`, `_variants`) — see the migration guide.

---

## References

| Document | Purpose |
|---|---|
| [`Docs/LayerTagGrammar.md`](LayerTagGrammar.md) | Normative spec — EBNF, design decisions, case rules |
| [`Docs/Migration-PrefixToTag.md`](Migration-PrefixToTag.md) | Old-syntax → new-syntax mapping, PSD lint checklist |
| `Source/PSD2UMG/Private/Parser/FLayerTagParser.{h,cpp}` | Parser implementation |
| `Source/PSD2UMG/Tests/FLayerTagParserSpec.cpp` | Tag grammar test suite |
