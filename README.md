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
- **EnhancedInput** — Required for `Button_Name[IA_Action]` input action binding.

---

## Layer Naming Cheat Sheet

PSD layer names control which UMG widget class is generated. Prefixes are matched first (highest priority wins), then suffixes modify anchor and layout behavior.

### Prefixes

| Prefix | Widget Class | Notes |
|--------|-------------|-------|
| `Button_` | `UButton` | Normal/Hovered/Pressed/Disabled child layers mapped to state brushes |
| `Progress_` | `UProgressBar` | Background and Fill child layers |
| `HBox_` | `UHorizontalBox` | Children laid out left-to-right |
| `VBox_` | `UVerticalBox` | Children laid out top-to-bottom |
| `Overlay_` | `UOverlay` | Z-stacked children |
| `ScrollBox_` | `UScrollBox` | Scrollable container |
| `Slider_` | `USlider` | |
| `CheckBox_` | `UCheckBox` | |
| `Input_` | `UEditableTextBox` | |
| `List_` | `UListView` | |
| `Tile_` | `UTileView` | |
| `Switcher_` | `UWidgetSwitcher` | |

Layers without a recognized prefix are mapped by type: pixel layers become `UImage`, text layers become `UTextBlock`, group layers become `UCanvasPanel`.

### Suffixes

Suffixes are appended to any layer name (after the prefix if present). Longest suffix takes priority.

| Suffix | Effect |
|--------|--------|
| `_9slice` | 9-slice draw mode (Box). Optional margins: `LayerName_9slice[L,T,R,B]` |
| `_9s` | Short form of `_9slice` |
| `_variants` | Wrap children in `UWidgetSwitcher` (one child per variant) |
| `_anchor-tl` | Force top-left anchor (0, 0) |
| `_anchor-tc` | Force top-center anchor (0.5, 0) |
| `_anchor-tr` | Force top-right anchor (1, 0) |
| `_anchor-cl` | Force center-left anchor (0, 0.5) |
| `_anchor-c` | Force center anchor (0.5, 0.5) |
| `_anchor-cr` | Force center-right anchor (1, 0.5) |
| `_anchor-bl` | Force bottom-left anchor (0, 1) |
| `_anchor-bc` | Force bottom-center anchor (0.5, 1) |
| `_anchor-br` | Force bottom-right anchor (1, 1) |
| `_stretch-h` | Stretch horizontally (anchor min X=0, max X=1); vertical anchor from quadrant |
| `_stretch-v` | Stretch vertically (anchor min Y=0, max Y=1); horizontal anchor from quadrant |
| `_fill` | Fill entire parent (anchor 0,0 to 1,1) |

Without a suffix, anchor is inferred from layer center position within the canvas (quadrant heuristic).

### CommonUI Mode

When `bUseCommonUI` is enabled in plugin settings:

| Syntax | Effect |
|--------|--------|
| `Button_Name` | Generates `UCommonButtonBase` instead of `UButton` |
| `Button_Confirm[IA_Confirm]` | Generates `UCommonButtonBase` and binds to `UInputAction` asset |
| `_show` / `_hide` child layers | Generate `UWidgetAnimation` for show/hide transitions |
| `_hover` child layers | Generate `UWidgetAnimation` for hover state |

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
| `bUseCommonUI` | `false` | Generate `UCommonButtonBase` instead of `UButton` for `Button_` layers |
| `InputActionSearchPath` | (empty) | Content path searched for `UInputAction` assets referenced by `Button_Name[IA_Action]` |

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
