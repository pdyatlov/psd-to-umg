# PSD2UMG Layer Tag Grammar (Phase 9)

> Status: Normative spec for the PSD2UMG plugin's layer-name parser.
> Implementation: `Source/PSD2UMG/Private/Parser/FLayerTagParser.{h,cpp}`.
> Tests: `Source/PSD2UMG/Tests/FLayerTagParserSpec.cpp`.

PSD2UMG inspects every PSD layer name. Designers express widget intent (button,
9-slice image, anchor, state brush, animation, input action...) using a uniform
`@tag` syntax. This document is the single source of truth for that syntax.

---

## 1. EBNF

```ebnf
layer-name      = clean-name { whitespace tag } ;
clean-name      = { non-at-char } ;          (* trimmed; internal spaces -> '_' downstream *)
non-at-char     = ? any Unicode codepoint except '@' ? ;

tag             = "@" tag-name [ ":" tag-value ] ;
tag-name        = identifier ;
identifier      = letter { letter | digit | "-" } ;
letter          = "A".."Z" | "a".."z" ;
digit           = "0".."9" ;

tag-value       = value-atom { "," value-atom } ;
value-atom      = { value-char } ;
value-char      = ? any non-whitespace, non-comma char except '@' ? ;

whitespace      = " " | "\t" ;               (* one or more *)
```

Notes:
- The first `@` ends the clean name.
- Tags are whitespace-separated. Order is irrelevant (D-16).
- Colon (`:`) separates name from value; further colons inside a value are
  forbidden (LL(1) keeps the grammar simple).

---

## 2. Tag Inventory

### 2.1 Type tags (D-02 -- exactly one per layer; defaults applied if absent)

| Tag | Becomes | Default for |
|---|---|---|
| `@button` | `UButton` (or `UCommonButtonBase` when CommonUI mode is on) | -- |
| `@image` | `UImage` | pixel layers |
| `@text` | `UTextBlock` | text layers |
| `@progress` | `UProgressBar` | -- |
| `@hbox` | `UHorizontalBox` | -- |
| `@vbox` | `UVerticalBox` | -- |
| `@overlay` | `UOverlay` | -- |
| `@scrollbox` | `UScrollBox` | -- |
| `@slider` | `USlider` | -- |
| `@checkbox` | `UCheckBox` | -- |
| `@input` | `UEditableText` | -- |
| `@list` | `UListView` | -- |
| `@tile` | `UTileView` | -- |
| `@smartobject` | external WBP via `FSmartObjectImporter` | smart-object layers |
| `@canvas` | `UCanvasPanel` | group layers |

If no type tag is present, the parser fills in the default for the PSD layer
kind: Group -> Canvas, Image -> Image, Text -> Text.

### 2.2 Modifier tags

| Tag | Effect |
|---|---|
| `@anchor:tl\|tc\|tr\|cl\|c\|cr\|bl\|bc\|br\|stretch-h\|stretch-v\|fill` | Override auto-anchor heuristic (D-06). |
| `@9s` | 9-slice image, default 16px on all sides (shorthand). |
| `@9s:L,T,R,B` | 9-slice with explicit margins (D-07). |
| `@variants` | Group becomes `UWidgetSwitcher` (D-08). |
| `@ia:IA_Confirm` | Bind input action by asset name -- case preserved (D-09). |
| `@smartobject:TypeName` | Optional type-name annotation for smart-object layers (D-11). |
| `@state:normal\|hover\|pressed\|disabled\|fill\|bg` | Mark a child layer as a state brush (D-12). |
| `@anim:show\|hide\|hover` | Mark a child group as an animation source (D-10). |

### 2.3 Default 9-slice margin

`@9s` without a value resolves to `{16,16,16,16}` (`bExplicit=false`). This
matches the historical default in `F9SliceImageLayerMapper`. Use `@9s:L,T,R,B`
to override.

---

## 3. Case Sensitivity (D-04, D-14)

- Tag **names** are case-insensitive: `@BUTTON`, `@Button`, `@button` are equal.
- Enum-valued **values** are case-insensitive: `@anchor:TL` == `@anchor:tl`.
- Asset-reference **values** preserve case verbatim: `@ia:IA_Confirm`,
  `@smartobject:HUDPanel`. The parser does not lower-case these.

---

## 4. Conflict & Unknown-Tag Rules

- **D-17:** Conflicting tags of the same family -> last wins, diagnostic emitted.
  Example: `X @anchor:tl @anchor:c` -> `Anchor=C`, warning logged.
- **D-18:** Unknown tags -> captured in `FParsedLayerTags::UnknownTags`,
  warning emitted, parsing continues.
- **D-19:** Multiple type tags -> error logged, last wins.
  Example: `X @button @image` -> `Type=Image`, error logged.
- Malformed tokens (`@`, `@:value`, `@anchor:`) -> warning, ignored.

---

## 5. Name Extraction

- **D-20:** Widget name = text before the first `@`-token.
  - Trim leading/trailing whitespace.
  - Replace internal spaces with `_`.
  - Other special characters pass through verbatim (downstream
    `FName::IsValidXName` may sanitize further).
- **D-21:** Empty clean name -> fallback to `<TypeTag>_<LayerIndex>` with
  zero-padded two-digit index. Warning emitted.
  - Example: `@button` at LayerIndex 7 -> `Button_07`.

---

## 6. Worked Examples

| Input | CleanName | Type | Notes |
|---|---|---|---|
| `PlayButton @button @anchor:tl` | `PlayButton` | Button | Anchor=TL |
| `My Widget @button` | `My_Widget` | Button | Internal space -> `_` |
| `Background @image @anchor:fill` | `Background` | Image | Anchor=Fill |
| `Panel @9s` | `Panel` | Canvas (default for group) or Image (default for pixel) | NineSlice={16,16,16,16, bExplicit=false} |
| `Panel @9s:8,8,16,16` | `Panel` | (default) | NineSlice={8,8,16,16, bExplicit=true} |
| `Title @text` | `Title` | Text | -- |
| `Confirm @button @ia:IA_Confirm` | `Confirm` | Button | InputAction="IA_Confirm" |
| `Menu @variants` | `Menu` | Canvas (group default) | bIsVariants=true |
| `Hover @state:hover` | `Hover` | Image (pixel default) | State=Hover |
| `FadeIn @anim:show` | `FadeIn` | Canvas | Anim=Show |
| `@button` (LayerIndex=7) | `Button_07` | Button | D-21 fallback, warn |
| `Btn @BUTTON @Anchor:TL` | `Btn` | Button | Case-insensitive |
| `X @button @image` | `X` | Image | D-19, last-wins, error logged |
| `X @anchor:tl @anchor:c` | `X` | (default) | D-17, last-wins, warn |
| `X @button @frobnicate` | `X` | Button | UnknownTags=["frobnicate"], warn |

---

## 7. Migration

Phase 9 hard-removes the old prefix/suffix syntax (`Button_`, `_anchor-tl`,
`_9s`, `[IA_X]`, `[L,T,R,B]`, `_show`, `_hide`, `_hover`, `_hovered`,
`_pressed`, `_fill`, `_bg`, `_variants`). See `Docs/Migration-PrefixToTag.md`
(shipped in Plan 09-04) for the old->new mapping table and per-PSD checklist.
