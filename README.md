# PSD2UMG

![UE 5.7](https://img.shields.io/badge/UE-5.7-blue) ![Win64](https://img.shields.io/badge/platform-Win64-lightgrey) ![MIT License](https://img.shields.io/badge/license-MIT-green)

Unreal Engine 5.7 editor plugin that imports `.psd` files and converts them into UMG Widget Blueprints. Drop a PSD, get a working UI.

---

## Quick Start

1. Copy the `PSD2UMG/` folder into your project's `Plugins/` directory.
2. Right-click your `.uproject` file and select **Generate Visual Studio project files**.
3. Open the UE editor; enable **PSD2UMG** in **Edit > Plugins** if it is not auto-enabled.
4. Drag a `.psd` file into the Content Browser — a Widget Blueprint is created automatically.

---

## Installation

**Requirements:** UE 5.7+, Win64, Visual Studio 2022+

**From source:**

1. Clone or download this repository.
2. Copy the `PSD2UMG/` folder to `<YourProject>/Plugins/`.
3. Add to your `.uproject` `Plugins` array:
   ```json
   { "Name": "PSD2UMG", "Enabled": true }
   ```
4. Rebuild from source (compile via Visual Studio or `UnrealBuildTool`).

**Optional plugins** (enabled automatically if present):

- **CommonUI** — Required for `bUseCommonUI` mode (`UCommonButtonBase` generation).
- **EnhancedInput** — Required for `@ia:IA_Action` input action binding on `@button` layers.

---

## Layer Naming

PSD layer names drive widget generation via `@`-tags. Format:

```
WidgetName @tag @tag:value @tag:v1,v2,...
```

The widget name is whatever text appears before the first `@`. Everything after is parsed as tags.

### Type tags (one per layer)

| Tag | Widget |
|-----|--------|
| `@button` | `UButton` (or `UCommonButtonBase` in CommonUI mode) |
| `@image` | `UImage` |
| `@text` | `UTextBlock` |
| `@progress` | `UProgressBar` |
| `@hbox` / `@vbox` | `UHorizontalBox` / `UVerticalBox` |
| `@overlay` | `UOverlay` |
| `@scrollbox` | `UScrollBox` |
| `@slider` | `USlider` |
| `@checkbox` | `UCheckBox` |
| `@input` | `UEditableTextBox` |
| `@list` / `@tile` | `UListView` / `UTileView` |
| `@variants` | `UWidgetSwitcher` |
| `@smartobject[:TypeName]` | child `UUserWidget` |
| `@canvas` | `UCanvasPanel` (default for groups) |

Default types: group layers → `@canvas`, pixel layers → `@image`, text layers → `@text`.

### Modifier tags

| Tag | Effect |
|-----|--------|
| `@anchor:tl` / `tc` / `tr` / `cl` / `c` / `cr` / `bl` / `bc` / `br` | Explicit anchor corner/center |
| `@anchor:stretch-h` / `stretch-v` / `fill` | Stretch modes |
| `@9s` / `@9s:L,T,R,B` | 9-slice (default 16px on all sides if no margins) |
| `@ia:IA_ActionName` | Bind CommonUI input action |
| `@state:hover` / `pressed` / `disabled` / `normal` / `fill` / `bg` | State-brush child |
| `@anim:show` / `hide` / `hover` | Animation variant (inside CommonUI button) |

### Rules

- Tags are case-insensitive. Values that reference asset names (`@ia:IA_Confirm`) preserve case.
- Tags are order-free. Conflicting tags: last wins + warning logged.
- Unknown tags: warning + ignored.
- Empty widget name (`@button` alone): auto-named `Button_NN`.

### Examples

- `PlayButton @button @anchor:tl`
- `Health @progress`  (with children `Fill @state:fill` and `Bg @state:bg`)
- `Confirm @button @ia:IA_Confirm`
- `Panel @9s:16,16,16,16`
- `Menu @variants`  (children are variant pages)

Formal grammar: [Docs/LayerTagGrammar.md](Docs/LayerTagGrammar.md).
Migration from pre-Phase-9 syntax: [Docs/Migration-PrefixToTag.md](Docs/Migration-PrefixToTag.md).

---

## Plugin Settings

**Project Settings > Plugins > PSD2UMG**

### General

| Setting | Default | Description |
|---------|---------|-------------|
| `bEnabled` | `true` | Master switch — disabling skips all PSD import processing |
| `bShowPreviewDialog` | `true` | Show layer preview dialog before every import. Disable for batch workflows |

### Effects

| Setting | Default | Description |
|---------|---------|-------------|
| `bFlattenComplexEffects` | `true` | Rasterize layers with inner shadow, gradient, pattern, or bevel effects as a single PNG |
| `MaxSmartObjectDepth` | `2` | Recursion depth limit for Smart Object import; deeper Smart Objects rasterize as images |

### Output

| Setting | Description |
|---------|-------------|
| `TextureSrcDir` | Source directory for texture files on disk |
| `TextureAssetDir` | Content Browser path for generated `Texture2D` assets |
| `WidgetBlueprintAssetDir` | Content Browser path for generated Widget Blueprint assets |

### Typography

| Setting | Default | Description |
|---------|---------|-------------|
| `FontMap` | (empty) | Map of Photoshop font name to `UFont` asset. Unmapped fonts fall back to `DefaultFont` |
| `DefaultFont` | (none) | Fallback `UFont` when the PS font name is not in `FontMap` |
| `SourceDPI` | `72` | Photoshop document DPI for pt-to-px conversion. At 72 DPI, 1pt = 1px in UMG |

### CommonUI

| Setting | Default | Description |
|---------|---------|-------------|
| `bUseCommonUI` | `false` | Generate `UCommonButtonBase` instead of `UButton` for `@button` layers |
| `InputActionSearchPath` | (empty) | Content path searched for `UInputAction` assets referenced by `@ia:IA_Action` tags |

---

## Running Tests

Tests use the Unreal Automation Framework. The plugin registers tests under `PSD2UMG.*`.

**Session Frontend:**

1. Open **Window > Developer Tools > Session Frontend**.
2. Switch to the **Automation** tab.
3. Filter by `PSD2UMG` and click **Start Tests**.

**Command line (headless):**

```
UnrealEditor.exe MyProject.uproject -ExecCmds="Automation RunTests PSD2UMG;Quit" -log -nosplash -nullrhi
```

**Test filter groups:**

| Filter | Coverage |
|--------|----------|
| `PSD2UMG.Parser.*` | PSD file parsing, layer extraction |
| `PSD2UMG.Generator.*` | Widget Blueprint generation, anchor calculation |
| `PSD2UMG.Typography.*` | Text properties, font mapping, DPI conversion |

---

## Test Fixtures

Test PSDs live in `Source/PSD2UMG/Tests/Fixtures/`. They can also be dragged into the Content Browser for manual end-to-end testing.

| File | Purpose |
|------|---------|
| `MultiLayer.psd` | Mixed layer types — images, text, groups |
| `Typography.psd` | Text layers with varied font, size, color, alignment |
| `SimpleHUD.psd` | Minimal HUD with buttons and health bar |
| `ComplexMenu.psd` | Nested groups, scroll boxes, overlays |
| `Effects.psd` | Drop shadow, color overlay, opacity, blend modes |

---

## Known Limitations

- Win64 only — macOS support deferred.
- UE 5.7+ required — no backward compatibility.
- Effect fidelity is approximate: color overlay, drop shadow, and opacity are supported; complex effects (inner shadow, gradient overlay, pattern, bevel) flatten to PNG when `bFlattenComplexEffects` is enabled.
- Text shadow is not supported (PhotoshopAPI limitation).
- Smart Object recursive import has a depth limit (`MaxSmartObjectDepth`); deeper nesting rasterizes.
- Editor-only plugin — no runtime module, cannot be packaged into a shipping build.

---

## License

MIT. See [LICENSE](LICENSE).
