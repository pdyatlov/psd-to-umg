# Requirements: PSD2UMG

**Defined:** 2026-04-08
**Core Value:** A designer drops a PSD into Unreal Editor and gets a correctly structured, immediately usable Widget Blueprint — with no Python dependency, no manual tweaking, and no loss of layer intent.

## v1 Requirements

### UE5 Port

- [x] **PORT-01**: Plugin module renamed from AutoPSDUI to PSD2UMG with updated .uplugin descriptor (EngineVersion 5.7, PlatformAllowList, no PythonScriptPlugin)
- [x] **PORT-02**: All deprecated UE4 APIs replaced (FEditorStyle→FAppStyle, WhitelistPlatforms→PlatformAllowList, AssetRegistryModule include paths)
- [x] **PORT-03**: Plugin loads in UE 5.7.4 editor without errors or warnings
- [x] **PORT-04**: Plugin appears in Edit → Plugins with correct name and description

### PSD Parser

- [x] **PRSR-01**: PhotoshopAPI integrated as pre-built static lib via Build.cs (Win64 + Mac), with all transitive dependencies linked (blosc2, libdeflate, zlib-ng, OpenImageIO)
- [x] **PRSR-02**: FPsdParser::ParseFile() returns FPsdDocument with correct layer tree (names, types, bounds, visibility, opacity)
- [x] **PRSR-03**: Image layer pixel data extracted as RGBA TArray<uint8> with correct dimensions
- [x] **PRSR-04**: Text layer content, font name, size, color, and alignment extracted
- [x] **PRSR-05**: Group/folder layers preserved with correct parent-child hierarchy
- [x] **PRSR-06**: UPsdImportFactory intercepts .psd files in UE import pipeline (replaces Python trigger)
- [x] **PRSR-07**: No Python dependency at plugin runtime

### Layer Mapping

- [x] **MAP-01**: IPsdLayerMapper interface and FLayerMappingRegistry implemented with priority-based first-match dispatch
- [x] **MAP-02**: Image layers → UImage with SlateBrush pointing to imported UTexture2D
- [x] **MAP-03**: Text layers → UTextBlock (content, size with DPI conversion, color, alignment)
- [x] **MAP-04**: Group layers → UCanvasPanel (default) with correct child hierarchy
- [x] **MAP-05**: Button_ prefix groups → UButton with normal/hovered/pressed/disabled state brushes
- [x] **MAP-06**: Progress_ prefix groups → UProgressBar with background/fill brushes
- [x] **MAP-07**: HBox_ / VBox_ prefix groups → UHorizontalBox / UVerticalBox
- [x] **MAP-08**: Overlay_ prefix groups → UOverlay
- [x] **MAP-09**: ScrollBox_ prefix groups → UScrollBox
- [x] **MAP-10**: Slider_ / CheckBox_ / Input_ prefix groups → USlider / UCheckBox / UEditableTextBox
- [x] **MAP-11**: List_ / Tile_ prefix groups → UListView / UTileView with EntryWidgetClass
- [x] **MAP-12**: Switcher_ prefix groups → UWidgetSwitcher

### Widget Blueprint Generation

- [x] **WBP-01**: FWidgetBlueprintGenerator creates valid UWidgetBlueprint asset (ConstructWidget → compile → save sequence)
- [x] **WBP-02**: Widget positions and sizes correctly mapped from PSD canvas coordinates to UMG CanvasPanelSlot
- [x] **WBP-03**: Anchor heuristics auto-assign anchors based on layer position relative to canvas (top-left, top-right, bottom-left, bottom-right, centered, stretch)
- [x] **WBP-04**: Anchor override suffixes work (_anchor-tl, _anchor-c, _stretch-h, _stretch-v, _fill)
- [x] **WBP-05**: Layer z-order preserved in widget tree
- [x] **WBP-06**: Widget Blueprint opens correctly in UMG Designer without errors

### Texture Import

- [x] **TEX-01**: Image layer pixel data imported as UTexture2D using Source.Init() (not PlatformData) so textures persist across editor restarts
- [x] **TEX-02**: Textures organized in target directory: {TargetDir}/Textures/{PsdName}/{LayerName}
- [x] **TEX-03**: Duplicate layer names handled by appending index

### Text & Typography

- [ ] **TEXT-01**: DPI conversion applied: Photoshop point size × 0.75 = UMG font size (72→96 DPI)
- [x] **TEXT-02**: Bold and italic weight/style applied via TypefaceFontName
- [ ] **TEXT-03**: Text outline (stroke) applied via FSlateFontInfo.OutlineSettings
- [ ] **TEXT-04**: Text shadow applied via SetShadowOffset + SetShadowColorAndOpacity
- [ ] **TEXT-05**: Font mapping system: Photoshop font name → UE font asset via plugin settings
- [x] **TEXT-06**: Multi-line text with AutoWrapText enabled when text box width is defined

### Layer Effects

- [x] **FX-01**: Layer opacity applied via SetRenderOpacity
- [x] **FX-02**: Layer visibility applied (visible/collapsed)
- [x] **FX-03**: Color Overlay effect → brush tint color
- [x] **FX-04**: Drop Shadow → approximate UImage offset duplicate behind main widget
- [x] **FX-05**: Complex effects (inner shadow, gradient overlay, etc.) fall back to rasterized PNG when bFlattenComplexEffects = true

### Advanced Layout

- [x] **LAYOUT-01**: _9s / _9slice suffix → Box draw mode with configurable margins
- [x] **LAYOUT-02**: Margin syntax in layer name: MyButton_9s[16,16,16,16]
- [x] **LAYOUT-03**: Improved anchor heuristics: horizontal row detection → HorizontalBox suggestion, vertical stack → VerticalBox suggestion
- [x] **LAYOUT-04**: Smart Object layers extracted and imported as child Widget Blueprints (referenced via UUserWidget)
- [x] **LAYOUT-05**: Smart Object fallback: rasterize as image if extraction fails
- [x] **LAYOUT-06**: _variants suffix groups → UWidgetSwitcher with one child per variant slot

### Editor UI & Workflow

- [x] **EDITOR-01**: Plugin settings in Project Settings → Plugins → PSD2UMG (output dirs, font map, source DPI, effect handling, anchor auto-assign, CommonUI mode)
- [x] **EDITOR-02**: Import preview dialog shows layer tree with checkboxes and widget type badges before creating assets
- [x] **EDITOR-03**: User can toggle individual layers on/off before import
- [x] **EDITOR-04**: Reimport support: update changed layers (position, image data, text) without destroying manual Blueprint edits; layer name is stable identity key
- [x] **EDITOR-05**: Right-click context menu in Content Browser: "Import as Widget Blueprint"

### CommonUI & Interactivity

- [x] **CUI-01**: Optional CommonUI mode: Button_ prefix → UCommonButtonBase instead of UButton
- [x] **CUI-02**: Input action binding via layer name syntax: Button_Confirm[IA_Confirm] → bind to UInputAction asset
- [x] **CUI-03**: Animation generation from _show/_hide/_hover layer variants → UWidgetAnimation
- [x] **CUI-04**: ScrollBox content height auto-calculated from children

### Testing & Quality

- [x] **TEST-01**: Unit tests (no UE dependency): FPsdParser, FLayerMappingRegistry, FAnchorCalculator, DPI conversion
- [x] **TEST-02**: Integration tests (FAutomationTestBase): full PSD→WBP pipeline, reimport
- [ ] **TEST-03**: Test PSD files: Test_SimpleHUD.psd, Test_ComplexMenu.psd, Test_MobileUI.psd, Test_Typography.psd, Test_Effects.psd in Plugins/PSD2UMG/TestData/
- [x] **TEST-04**: GitHub Actions CI: compile on Win64 + run unit tests on push

### Documentation & Release

- [x] **DOC-01**: README.md with quick start, installation, basic usage
- [x] **DOC-02**: CONVENTIONS.md with full layer naming convention reference for designers
- [x] **DOC-03**: CHANGELOG.md
- [x] **DOC-04**: Example project: 3-4 pre-imported PSD→WBP demonstrations (HUD, main menu, settings screen, popup)

## v2 Requirements

### Runtime

- **RUN-01**: Runtime module for procedural widget generation from PSD at runtime (not just editor import)

### Extended Format Support

- **FMT-01**: Figma export JSON as alternative input source (Stage 1 swap)
- **FMT-02**: Sketch file format support

### Advanced Text

- **TEXT-V2-01**: URichTextBlock support for mixed-style runs within a single text layer (bold + normal in one layer)
- **TEXT-V2-02**: Letter spacing and line height mapping

### Marketplace Polish

- **MKT-01**: Custom PSD thumbnail in Content Browser (flattened layer preview)
- **MKT-02**: Toolbar button in Widget Blueprint Editor: "Re-import from PSD"
- **MKT-03**: Fab/Epic Marketplace submission packaging

## Out of Scope

| Feature | Reason |
|---------|--------|
| Figma / Sketch / XD support | PSD only for v1; 3-stage architecture allows future parser swap without changing Stage 2+3 |
| Runtime plugin module | Editor-only import tool; UMG widgets themselves run at runtime but the importer doesn't |
| Full PSD effect fidelity | Pixel-perfect parity with Photoshop renderer is not achievable in UMG; flatten fallback is the correct approach |
| Python as core dependency | Replaced by PhotoshopAPI; Python remains available for user scripting only |
| UE4 compatibility | Targeting UE 5.7+ only; no backward-compat shims |
| Manual TySh PSD descriptor parsing | PhotoshopAPI v0.9.0 provides full text layer API; manual parsing not needed |
| CMake at runtime | PhotoshopAPI pre-built as static lib; no CMake in UBT build |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| PORT-01 | Phase 1 | Complete |
| PORT-02 | Phase 1 | Complete |
| PORT-03 | Phase 1 | Complete |
| PORT-04 | Phase 1 | Complete |
| PRSR-01 | Phase 2 | Complete |
| PRSR-02 | Phase 2 | Complete |
| PRSR-03 | Phase 2 | Complete |
| PRSR-04 | Phase 2 | Complete |
| PRSR-05 | Phase 2 | Complete |
| PRSR-06 | Phase 2 | Complete |
| PRSR-07 | Phase 2 | Complete |
| MAP-01 | Phase 3 | Complete |
| MAP-02 | Phase 3 | Complete |
| MAP-03 | Phase 3 | Complete |
| MAP-04 | Phase 3 | Complete |
| MAP-05 | Phase 3 | Complete |
| MAP-06 | Phase 3 | Complete |
| MAP-07 | Phase 3 | Complete |
| MAP-08 | Phase 3 | Complete |
| MAP-09 | Phase 3 | Complete |
| MAP-10 | Phase 3 | Complete |
| MAP-11 | Phase 3 | Complete |
| MAP-12 | Phase 3 | Complete |
| WBP-01 | Phase 3 | Complete |
| WBP-02 | Phase 3 | Complete |
| WBP-03 | Phase 3 | Complete |
| WBP-04 | Phase 3 | Complete |
| WBP-05 | Phase 3 | Complete |
| WBP-06 | Phase 3 | Complete |
| TEX-01 | Phase 3 | Complete |
| TEX-02 | Phase 3 | Complete |
| TEX-03 | Phase 3 | Complete |
| TEXT-01 | Phase 4 | Pending |
| TEXT-02 | Phase 4 | Complete |
| TEXT-03 | Phase 4 | Pending |
| TEXT-04 | Phase 4 | Pending |
| TEXT-05 | Phase 4 | Pending |
| TEXT-06 | Phase 4 | Complete |
| FX-01 | Phase 5 | Complete |
| FX-02 | Phase 5 | Complete |
| FX-03 | Phase 5 | Complete |
| FX-04 | Phase 5 | Complete |
| FX-05 | Phase 5 | Complete |
| LAYOUT-01 | Phase 6 | Complete |
| LAYOUT-02 | Phase 6 | Complete |
| LAYOUT-03 | Phase 6 | Complete |
| LAYOUT-04 | Phase 6 | Complete |
| LAYOUT-05 | Phase 6 | Complete |
| LAYOUT-06 | Phase 6 | Complete |
| EDITOR-01 | Phase 7 | Complete |
| EDITOR-02 | Phase 7 | Complete |
| EDITOR-03 | Phase 7 | Complete |
| EDITOR-04 | Phase 7 | Complete |
| EDITOR-05 | Phase 7 | Complete |
| CUI-01 | Phase 7 | Complete |
| CUI-02 | Phase 7 | Complete |
| CUI-03 | Phase 7 | Complete |
| CUI-04 | Phase 7 | Complete |
| TEST-01 | Phase 8 | Complete |
| TEST-02 | Phase 8 | Complete |
| TEST-03 | Phase 8 | Pending |
| TEST-04 | Phase 8 | Complete |
| DOC-01 | Phase 8 | Complete |
| DOC-02 | Phase 8 | Complete |
| DOC-03 | Phase 8 | Complete |
| DOC-04 | Phase 8 | Complete |

**Coverage:**
- v1 requirements: 66 total
- Mapped to phases: 66
- Unmapped: 0

---
*Requirements defined: 2026-04-08*
*Last updated: 2026-04-07 after roadmap creation (phase assignments populated)*
