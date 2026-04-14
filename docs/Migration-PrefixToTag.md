# Migration Guide: Prefix Syntax → @-Tag Grammar

Phase 9 of PSD2UMG replaces every legacy prefix, suffix, and bracket convention with a single unified **@-tag grammar** for PSD layer names. This is a **hard cutover** — no backwards compatibility shims exist. Any PSD authored against the pre-Phase-9 conventions will either fail to dispatch or dispatch incorrectly until the layer names are rewritten.

This guide enumerates every removed pattern, shows the exact @-tag replacement with before/after PSD layer-name examples, walks a representative layer tree through the full rewrite, and gives a checklist to lint a PSD after conversion.

The formal grammar (BNF, parser rules, conflict resolution) lives in [LayerTagGrammar.md](LayerTagGrammar.md). That document is the spec; this one is the migration companion.

---

## The One-Line Rewrite Rule

| Pre-Phase-9 | Phase 9+ |
|-------------|----------|
| `<Prefix>_<Name><suffix...>[bracket...]` | `<Name> @tag @tag:value @tag:v1,v2,...` |

Everything that used to live in a prefix, a `_suffix`, or a `[bracket]` now lives in a tag at the **end** of the layer name. The widget name is whatever text appears before the first `@`.

---

## Old → New Mapping Table

Each row is a pattern removed in Phase 9 with its exact tag replacement. Examples are literal PSD layer names — what you would see (and now rename) in Photoshop's layer panel.

### Widget type tags (group layers)

| Old syntax | New syntax | Example old | Example new |
|------------|------------|-------------|-------------|
| `Button_` prefix | `@button` | `Button_Start` | `Start @button` |
| `Btn_` prefix (preview-only) | `@button` | `Btn_OK` | `OK @button` |
| `Progress_` prefix | `@progress` | `Progress_Health` | `Health @progress` |
| `HBox_` prefix | `@hbox` | `HBox_Row` | `Row @hbox` |
| `VBox_` prefix | `@vbox` | `VBox_Column` | `Column @vbox` |
| `Overlay_` prefix | `@overlay` | `Overlay_Popup` | `Popup @overlay` |
| `ScrollBox_` prefix | `@scrollbox` | `ScrollBox_List` | `List @scrollbox` |
| `Slider_` prefix | `@slider` | `Slider_Volume` | `Volume @slider` |
| `CheckBox_` prefix | `@checkbox` | `CheckBox_Mute` | `Mute @checkbox` |
| `Input_` prefix | `@input` | `Input_Name` | `Name @input` |
| `List_` prefix | `@list` | `List_Items` | `Items @list` |
| `Tile_` prefix | `@tile` | `Tile_Grid` | `Grid @tile` |
| `Switcher_` prefix | `@variants` (merged with `_variants`) | `Switcher_Pages` | `Pages @variants` |
| `_variants` suffix | `@variants` | `Menu_variants` | `Menu @variants` |

### Image draw-mode tags (pixel layers)

| Old syntax | New syntax | Example old | Example new |
|------------|------------|-------------|-------------|
| `_9s` / `_9slice` suffix | `@9s` (shorthand — 16px default margins on all sides) | `Panel_9s` | `Panel @9s` |
| `_9s[L,T,R,B]` bracket | `@9s:L,T,R,B` | `Panel_9s[8,16,8,16]` | `Panel @9s:8,16,8,16` |

**Placement note:** `@9s` attaches to the **pixel layer**, not the containing group. If you had `Panel_9s` as a group, move `@9s` onto the background image child inside that group.

### Anchor / sizing tags (CanvasPanel children)

| Old syntax | New syntax | Example old | Example new |
|------------|------------|-------------|-------------|
| `_anchor-tl` / `_anchor-tc` / `_anchor-tr` | `@anchor:tl` / `@anchor:tc` / `@anchor:tr` | `Icon_anchor-tl` | `Icon @anchor:tl` |
| `_anchor-cl` / `_anchor-c` / `_anchor-cr` | `@anchor:cl` / `@anchor:c` / `@anchor:cr` | `Logo_anchor-c` | `Logo @anchor:c` |
| `_anchor-bl` / `_anchor-bc` / `_anchor-br` | `@anchor:bl` / `@anchor:bc` / `@anchor:br` | `Footer_anchor-bc` | `Footer @anchor:bc` |
| `_stretch-h` suffix | `@anchor:stretch-h` | `Bar_stretch-h` | `Bar @anchor:stretch-h` |
| `_stretch-v` suffix | `@anchor:stretch-v` | `Sidebar_stretch-v` | `Sidebar @anchor:stretch-v` |
| `_fill` suffix (anchor) | `@anchor:fill` | `Bg_fill` | `Bg @anchor:fill` |

### State-child tags (button / progress children)

Layers inside a `@button` or `@progress` group that used to be identified by name suffix are now identified by an explicit `@state:*` tag.

| Old syntax | New syntax | Example old | Example new |
|------------|------------|-------------|-------------|
| `_normal` child suffix | `@state:normal` | `Start_normal` | `Normal @state:normal` |
| `_hovered` child suffix | `@state:hover` | `Start_hovered` | `Hover @state:hover` |
| `_pressed` child suffix | `@state:pressed` | `Start_pressed` | `Pressed @state:pressed` |
| `_disabled` child suffix | `@state:disabled` | `Start_disabled` | `Disabled @state:disabled` |
| `_fill` (progress child) | `@state:fill` | `Health_fill` | `Fill @state:fill` |
| `_bg` / `_background` (progress child) | `@state:bg` | `Health_bg` | `Bg @state:bg` |

### Input action binding (CommonUI)

| Old syntax | New syntax | Example old | Example new |
|------------|------------|-------------|-------------|
| `[IA_ActionName]` bracket on button | `@ia:IA_ActionName` | `Button_Confirm[IA_Confirm]` | `Confirm @button @ia:IA_Confirm` |

### Animation-child tags (CommonUI button animations)

| Old syntax | New syntax | Example old | Example new |
|------------|------------|-------------|-------------|
| `_show` child | `@anim:show` | `Panel_show` | `ShowAnim @anim:show` |
| `_hide` child | `@anim:hide` | `Panel_hide` | `HideAnim @anim:hide` |
| `_hover` child (animation) | `@anim:hover` | `Panel_hover` | `HoverAnim @anim:hover` |

---

## Before / After PSD Walkthrough

A small HUD-style PSD, shown as-authored under the legacy conventions and then fully retagged under the @-tag grammar. The layer tree structure does not change — only the names.

### Before (pre-Phase-9)

```
HUD.psd
├── Background                           (pixel, untagged — default UImage)
├── HBox_TopBar                          (group — UHorizontalBox)
│   ├── Logo_anchor-tl                   (pixel — UImage, anchor top-left)
│   └── Title                            (text — UTextBlock)
├── Progress_Health                      (group — UProgressBar)
│   ├── Health_bg                        (pixel — background brush)
│   └── Health_fill                      (pixel — fill brush)
├── Button_Start                         (group — UButton)
│   ├── Start_normal                     (pixel — normal brush)
│   ├── Start_hovered                    (pixel — hovered brush)
│   └── Start_pressed                    (pixel — pressed brush)
├── Button_Confirm[IA_Confirm]           (group — UCommonButtonBase + IA binding)
│   └── Confirm_normal                   (pixel — normal brush)
├── Panel_9s[16,16,16,16]                (pixel — 9-slice background)
└── Menu_variants                        (group — UWidgetSwitcher)
    ├── MainPage                         (group — page 1)
    └── OptionsPage                      (group — page 2)
```

### After (Phase 9+)

```
HUD.psd
├── Background                           (pixel, untagged — default UImage)
├── TopBar @hbox                         (group — UHorizontalBox)
│   ├── Logo @anchor:tl                  (pixel — UImage, anchor top-left)
│   └── Title                            (text — UTextBlock)
├── Health @progress                     (group — UProgressBar)
│   ├── Bg @state:bg                     (pixel — background brush)
│   └── Fill @state:fill                 (pixel — fill brush)
├── Start @button                        (group — UButton)
│   ├── Normal @state:normal             (pixel — normal brush)
│   ├── Hover @state:hover               (pixel — hovered brush)
│   └── Pressed @state:pressed           (pixel — pressed brush)
├── Confirm @button @ia:IA_Confirm       (group — UCommonButtonBase + IA binding)
│   └── Normal @state:normal             (pixel — normal brush)
├── Panel @9s:16,16,16,16                (pixel — 9-slice background)
└── Menu @variants                       (group — UWidgetSwitcher)
    ├── MainPage                         (group — page 1)
    └── OptionsPage                      (group — page 2)
```

Points worth calling out:

- The widget display name (what appears in UMG's hierarchy panel) is the text **before** the first `@`. `Start @button` produces a widget named `Start`, not `Start @button`.
- `@9s` moved off a group onto the pixel layer — the parser consumes it where the pixels live.
- State children no longer need cryptic name suffixes; they read like what they are (`Hover @state:hover`).
- IA-binding moved out of square brackets into a peer tag on the same button layer.

---

## Lint Your PSD — Post-Rename Checklist

After retagging, walk the PSD once more and confirm:

1. **No legacy `_` dispatch suffixes remain.** Search layer names for `_normal`, `_hovered`, `_pressed`, `_disabled`, `_fill`, `_bg`, `_background`, `_9s`, `_9slice`, `_variants`, `_anchor-*`, `_stretch-*`, `_show`, `_hide`, `_hover`. Every hit is either a rename you missed or descriptive text that happens to use underscores (verify case-by-case).
2. **No legacy `Prefix_` dispatch prefixes remain.** Search for `Button_`, `Progress_`, `HBox_`, `VBox_`, `Overlay_`, `ScrollBox_`, `Slider_`, `CheckBox_`, `Input_`, `List_`, `Tile_`, `Switcher_`, `Btn_`.
3. **Every widget-producing layer has exactly one type tag** (or zero — untagged groups stay `UCanvasPanel`, untagged pixel layers stay `UImage`, untagged text layers stay `UTextBlock`).
4. **State-child layers carry `@state:*`.** Inside a `@button` group, the child brushes that used to be `_normal`/`_hovered`/etc. now read `@state:normal`/`@state:hover`/etc. Without the tag the mapper cannot find them.
5. **No `[...]` square-bracket params remain.** `[L,T,R,B]` and `[IA_X]` both moved into tag values (`@9s:L,T,R,B`, `@ia:IA_X`). The parser no longer looks at brackets.
6. **Widget display names are FName-valid.** The text before the first `@` must not contain `/`, `.`, `\`, `:` — those are illegal in UMG widget names. Rename if needed.
7. **No conflicting tags on a single layer.** Two type tags (`@button @hbox`) or two anchor tags (`@anchor:tl @anchor:c`) trigger a warning and last-wins resolution. Pick one.

A quick ripgrep over an exported layer-name listing is the fastest way to run the checklist against a large PSD.

---

## Reimport Identity — Delete and Reimport Legacy WBPs

Phase 9 ships a hard cutover (D-15). The reimport-identity key used to match PSD layers to existing Widget Blueprint nodes is now **`ParsedTags.CleanName`** — the widget display name portion of the new grammar (option-a from Plan 09-02 Task 1).

**Consequence for pre-Phase-9 Widget Blueprints:** Reimporting a retagged PSD over a WBP that was first generated before Phase 9 will typically orphan every widget, because the pre-Phase-9 WBP nodes are keyed by the old full layer name (`Button_Start`) while the reimport is looking for the new `CleanName` (`Start`).

**Recommended migration path:**

1. Rename the layers in the PSD per the table above.
2. **Delete the existing Widget Blueprint asset** generated before Phase 9.
3. Reimport the PSD. A fresh WBP is generated with the new grammar in place.
4. Redo any manual UMG edits — they are not preserved across this one-time transition.

This is a one-time cost per asset. Post-cutover, reimport behaves normally: rename a layer in the PSD, reimport, and the matching widget is updated in place.

---

## See Also

- [LayerTagGrammar.md](LayerTagGrammar.md) — formal @-tag grammar, BNF, parser rules, tag vocabulary, conflict resolution.
- `.planning/phases/09-unified-layer-naming-convention-tag-based/09-03-RETAG-CHECKLIST.md` — row-by-row ground truth for the bundled fixture PSDs; a worked example of exactly this migration applied to real files.
