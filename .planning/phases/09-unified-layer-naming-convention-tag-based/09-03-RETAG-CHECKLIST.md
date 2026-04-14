# 09-03 Fixture PSD Retag Checklist (D-24)

**Purpose:** Per-PSD, per-layer rename list for the 5 Phase 8 fixture PSDs. Each row is
an exact `old layer name` → `new tag-form name` mapping, plus the expected widget
`FName` that post-refactor specs will assert against.

**Identity strategy (per Plan 09-02 Task 1, option-a):** widget `FName` =
`ParsedTags.CleanName` (the raw name with all `@...` tokens stripped, trimmed). So for
`Start @button`, the widget `FName` is `Start`; for `Menu @variants`, it is `Menu`.

**Grammar reference:** `Docs/LayerTagGrammar.md`.

## General Lint (apply to every PSD before saving)

Per D-22 / D-23:

- [ ] No trailing `_` segments on any layer name (e.g. `Button_`, `Background_fill`).
- [ ] No `[...]` brackets in any layer name (neither `[IA_X]` input-action brackets
      nor `[L,T,R,B]` 9-slice margin brackets).
- [ ] At most one type tag per layer (no `@button @progress` on the same layer).
- [ ] Modifier tags use colon value form where applicable: `@anchor:tl`,
      `@state:hover`, `@anim:show`, `@9s:16,16,16,16`, `@ia:IA_Confirm`,
      `@smartobject:Foo`.
- [ ] Type tags use bare form: `@button`, `@image`, `@text`, `@hbox`, `@vbox`,
      `@overlay`, `@scrollbox`, `@slider`, `@checkbox`, `@input`, `@list`, `@tile`,
      `@progress`, `@canvas`, `@smartobject`.
- [ ] `@variants` is a bare modifier (no value), attaches to a Group; turns the
      group into a `UWidgetSwitcher`.
- [ ] `@9s` (shorthand) uses 16px default margins on all sides.

---

## 1. MultiLayer.psd (Phase 02-05 baseline)

Current asserted layer inventory (from `PsdParserSpec.cpp` MultiLayer block, lines
~97-165):

| # | Old name | New name | Widget FName | Rationale |
|---|----------|----------|--------------|-----------|
| 1 | `Title` | `Title` (no change) | `Title` | Plain text layer; no tag needed. Type inferred via D-02 (Text). |
| 2 | `Buttons` | `Buttons` (no change) | `Buttons` | Group with no dispatch prefix; stays a Canvas via D-02. |
| 3 | `BtnNormal` | `BtnNormal` (no change) | `BtnNormal` | Image child; no legacy dispatch prefix. Name is descriptive, not dispatch. |
| 4 | `BtnHover` | `BtnHover` (no change) | `BtnHover` | Image child; no legacy dispatch prefix. |
| 5 | `Background` | `Background` (no change) | `Background` | Plain image; no tag needed. |

**Net changes for MultiLayer.psd: NONE.** Fixture is already tag-grammar-compatible.
Save a no-op overwrite to keep the binary in git (optional; skip if diff is empty).

---

## 2. Typography.psd (Phase 04-01 baseline)

Current asserted layer inventory (from `PsdParserSpec.cpp` Typography block, lines
~233-282):

| # | Old name | New name | Widget FName | Rationale |
|---|----------|----------|--------------|-----------|
| 1 | `text_regular` | `text_regular` (no change) | `text_regular` | Plain text layer; no dispatch prefix. |
| 2 | `text_bold` | `text_bold` (no change) | `text_bold` | Same. |
| 3 | `text_italic` | `text_italic` (no change) | `text_italic` | Same. |
| 4 | `text_stroked` | `text_stroked` (no change) | `text_stroked` | Same. |
| 5 | `text_paragraph` | `text_paragraph` (no change) | `text_paragraph` | Same. |

**Net changes for Typography.psd: NONE.** Fixture is already tag-grammar-compatible
(names use `_` as a descriptive separator, not a dispatch prefix).

---

## 3. SimpleHUD.psd (Phase 08-02 baseline)

Current asserted layer inventory (from `PsdParserSpec.cpp` SimpleHUD block, lines
~342-386). Note: spec asserts 5 root layers; two carry legacy dispatch prefixes.

| # | Old name | New name | Widget FName | Rationale |
|---|----------|----------|--------------|-----------|
| 1 | `Progress_Health` | `Health @progress` | `Health` | `Progress_` prefix removed; `@progress` tag drives FProgressLayerMapper. |
| 2 | `Button_Start` | `Start @button` | `Start` | `Button_` prefix removed; `@button` tag drives FButtonLayerMapper. |
| 3 | `Score` | `Score` (no change) | `Score` | Plain text layer. |
| 4 | *(4th root, if present)* | unchanged unless legacy prefix present | — | Inspect in Photoshop. |
| 5 | *(5th root, if present)* | unchanged unless legacy prefix present | — | Inspect in Photoshop. |

**State children inside `Button_Start`** (not individually asserted in specs but
present in the PSD per Phase 8 fixture design):

| Old child name | New child name | Rationale |
|----------------|----------------|-----------|
| `*_normal` | `* @state:normal` or leave untagged (first untagged Image child is picked as Normal via D-13 fallback) | Either form works; prefer untagged for the default state. |
| `*_hovered` | `* @state:hover` | `@state:hover` is the explicit state tag (D-12). |
| `*_pressed` | `* @state:pressed` | Same. |
| `*_disabled` | `* @state:disabled` | Same. |

**State children inside `Progress_Health`:**

| Old child name | New child name | Rationale |
|----------------|----------------|-----------|
| `*_bg` / `*_background` | `* @state:bg` | Becomes FProgressLayerMapper's BackgroundImage via `FindChildByState(Bg)`. |
| `*_fill` / `*_foreground` | `* @state:fill` | Becomes FillImage via `FindChildByState(Fill)`. |

**Net changes for SimpleHUD.psd:**
- 2 root layers renamed (`Progress_Health` → `Health @progress`; `Button_Start` → `Start @button`).
- State children under those 2 groups retagged with `@state:*`.

**Spec impact:**
- `PsdParserSpec.cpp` lines 360-373 must assert on `Health @progress` + `Start @button`
  as the literal layer names (parser preserves the full name; `CleanName` is the
  stripped form, but `Layer.Name` is verbatim).

---

## 4. ComplexMenu.psd (Phase 08-02 baseline)

No spec-level layer-name assertions exist for ComplexMenu.psd in the current test
suite, so retag is guided by the PSD's own contents rather than broken asserts.
Inspect in Photoshop and rename any layer that matches these legacy patterns:

| Pattern | Replacement |
|---------|-------------|
| `Button_X` | `X @button` |
| `Progress_X` | `X @progress` |
| `HBox_X` | `X @hbox` |
| `VBox_X` | `X @vbox` |
| `Overlay_X` | `X @overlay` (only if used as layout overlay; if `Overlay_Red` is just a descriptive image name, LEAVE AS-IS — see Effects.psd note below) |
| `ScrollBox_X` | `X @scrollbox` |
| `Slider_X` | `X @slider` |
| `CheckBox_X` | `X @checkbox` |
| `Input_X` | `X @input` |
| `List_X` | `X @list` |
| `Tile_X` | `X @tile` |
| `Switcher_X` / `X_variants` | `X @variants` |
| `Panel_9s` (shorthand) | `Panel @9s` |
| `Panel_9s[16,16,16,16]` | `Panel @9s:16,16,16,16` |
| `Icon_anchor-tl` | `Icon @anchor:tl` |
| `Icon_anchor-c` | `Icon @anchor:c` |
| `Thing_stretch-h` | `Thing @anchor:stretch-h` |
| `Thing_fill` | `Thing @anchor:fill` |
| `Button_Confirm[IA_Confirm]` | `Confirm @button @ia:IA_Confirm` |
| `FadeIn_show` | `FadeIn @anim:show` |
| `Panel_hide` | `Panel @anim:hide` |
| `Hover_hover` | `Hover @anim:hover` |
| `*_hovered` (state child) | `* @state:hover` |
| `*_pressed` (state child) | `* @state:pressed` |
| `*_disabled` (state child) | `* @state:disabled` |
| `*_normal` (state child) | `* @state:normal` or leave untagged |
| `*_fill` / `*_foreground` (progress child) | `* @state:fill` |
| `*_bg` / `*_background` (progress child) | `* @state:bg` |

**Widget FName after retag:** clean name component (everything before the first
`@`), trimmed.

---

## 5. Effects.psd (Phase 08-02 baseline)

Current asserted layer inventory (from `PsdParserSpec.cpp` Effects block, lines
~445-500):

| # | Old name | New name | Widget FName | Rationale |
|---|----------|----------|--------------|-----------|
| 1 | `Overlay_Red` | `Overlay_Red` (no change) | `Overlay_Red` | Despite the `Overlay_` prefix, this is a descriptive image name, not a dispatch prefix — and the Phase 9 grammar has no `@overlay` type that would match on a single image layer's visual overlay effect. Leave verbatim. |
| 2 | `Shadow_Box` | `Shadow_Box` (no change) | `Shadow_Box` | Descriptive name; no legacy dispatch prefix in the grammar. |
| 3 | `Complex_Inner` | `Complex_Inner` (no change) | `Complex_Inner` | Descriptive name. |
| 4 | `Opacity50` | `Opacity50` (no change) | `Opacity50` | Descriptive name. |

**Net changes for Effects.psd: NONE** (assuming only the 4 asserted layers plus
descriptive filler). Inspect in Photoshop to confirm no other legacy-prefixed
layers exist; if any do (e.g. an unasserted `Button_*` child), rename per the
ComplexMenu.psd table above.

---

## Summary

| PSD | Root-layer renames | State-child retags | Binary change expected |
|-----|--------------------|--------------------|------------------------|
| MultiLayer.psd | 0 | 0 | No |
| Typography.psd | 0 | 0 | No |
| SimpleHUD.psd | 2 (`Progress_Health`, `Button_Start`) | Yes (inside 2 groups) | Yes |
| ComplexMenu.psd | Depends on inventory; apply pattern table in §4 | Yes | Yes |
| Effects.psd | 0 (asserted names) | Depends on unasserted children | Likely no |

**Reviewer gate before approving Task 2:** diff each saved PSD by opening in
Unreal and inspecting the import-preview dialog tree. Every `@`-containing name
must render tag chips in the preview dialog (per Plan 09-02 Task 6a). No
`Button_`, `Progress_`, `HBox_`, `_9s`, `[IA_*]`, `_anchor-*`, `_stretch-*`,
`_variants`, `_hovered`, `_pressed`, `_fill`, `_bg`, `_show`, `_hide`, `_hover`
segments should remain on any layer name.
