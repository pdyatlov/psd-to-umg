# Roadmap: PSD2UMG

## Overview

PSD2UMG transforms Photoshop files into fully functional UMG Widget Blueprints inside UE 5.7. The roadmap moves from porting the existing UE4 codebase, through building a native C++ PSD parser, to implementing the full layer-to-widget mapping pipeline, then layering on text, effects, layout, editor UI, and CommonUI support, culminating in testing and release. Each phase delivers a verifiable capability on top of the previous one.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [x] **Phase 1: UE5 Port** - Rename plugin, fix deprecated APIs, verify clean load in UE 5.7.4
- [x] **Phase 2: C++ PSD Parser** - Integrate PhotoshopAPI, build FPsdParser, replace Python import pipeline
- [x] **Phase 3: Layer Mapping & Widget Blueprint Generation** - Map PSD layers to UMG widgets, generate valid Widget Blueprints with textures and anchors (completed 2026-04-09)
- [ ] **Phase 4: Text, Fonts & Typography** - Full text layer support with DPI conversion, font mapping, outline, shadow
- [ ] **Phase 5: Layer Effects & Blend Modes** - Opacity, color overlay, drop shadow, flatten fallback for complex effects
- [ ] **Phase 6: Advanced Layout** - 9-slice, improved anchors, Smart Object recursive import, variant switchers
- [ ] **Phase 7: Editor UI, Preview & Settings** - Plugin settings, import preview dialog, reimport, context menu, CommonUI mode, animations
- [ ] **Phase 8: Testing, Documentation & Release** - Unit/integration tests, test PSDs, docs, CI/CD, example project

## Phase Details

### Phase 1: UE5 Port
**Goal**: The plugin compiles and loads cleanly in UE 5.7.4 with no deprecated API warnings and no Python dependency in its module descriptor
**Depends on**: Nothing (first phase)
**Requirements**: PORT-01, PORT-02, PORT-03, PORT-04
**Success Criteria** (what must be TRUE):
  1. Plugin loads in UE 5.7.4 editor without errors or warnings in the Output Log
  2. Plugin appears in Edit > Plugins as "PSD2UMG" with correct description
  3. .uplugin uses PlatformAllowList (not WhitelistPlatforms), no PythonScriptPlugin dependency
  4. All C++ source compiles with zero deprecated-API warnings (FAppStyle, PlatformAllowList, AssetRegistry paths)
**Plans:** 2 plans
Plans:
- [x] 01-01-PLAN.md — Rename plugin to PSD2UMG, update .uplugin/Build.cs for UE 5.7, fix all deprecated APIs
- [x] 01-02-PLAN.md — Clean up Python/ThirdParty legacy files, verify compilation and editor load

### Phase 2: C++ PSD Parser
**Goal**: PSD files are parsed natively in C++ via PhotoshopAPI, producing a correct FPsdDocument with full layer tree -- no Python involved
**Depends on**: Phase 1
**Requirements**: PRSR-01, PRSR-02, PRSR-03, PRSR-04, PRSR-05, PRSR-06, PRSR-07
**Success Criteria** (what must be TRUE):
  1. PhotoshopAPI and all transitive dependencies (blosc2, libdeflate, zlib-ng, simdutf, fmt, OpenImageIO) link successfully in a UE 5.7 build on Win64
  2. FPsdParser::ParseFile() returns correct layer tree from a multi-layer test PSD (names, types, bounds, visibility, opacity, hierarchy verified)
  3. Image layer pixel data extracted as RGBA with correct dimensions (verified by writing to UTexture2D)
  4. Text layer content, font name, size, color, and alignment are extracted via PhotoshopAPI text API
  5. Dragging a .psd into Content Browser triggers UPsdImportFactory (not UTextureFactory) and logs parsed layer tree
**Plans:** 5 plans
Plans:
- [x] 02-01-PLAN.md — Vendor PhotoshopAPI static libs, wire ThirdParty module, enable RTTI/exceptions/C++20, drop Mac
- [x] 02-02-PLAN.md — Declare LogPSD2UMG + FPsdDocument/FPsdLayer/FPsdTextRun/FPsdParseDiagnostics contract types
- [x] 02-03-PLAN.md — Implement PSD2UMG::Parser::ParseFile (groups, image RGBA, single-run text) behind PIMPL
- [x] 02-04-PLAN.md — Add UPsdImportFactory wrapper (forwards to UTextureFactory + runs parser); remove OnAssetReimport hook
- [x] 02-05-PLAN.md — Hand-craft MultiLayer.psd fixture + FPsdParserSpec automation spec; verification gate

### Phase 3: Layer Mapping & Widget Blueprint Generation
**Goal**: The full PSD-to-Widget-Blueprint pipeline works end-to-end -- designers drop a PSD and get a correctly structured, openable Widget Blueprint with all baseline widget types
**Depends on**: Phase 2
**Requirements**: MAP-01, MAP-02, MAP-03, MAP-04, MAP-05, MAP-06, MAP-07, MAP-08, MAP-09, MAP-10, MAP-11, MAP-12, WBP-01, WBP-02, WBP-03, WBP-04, WBP-05, WBP-06, TEX-01, TEX-02, TEX-03
**Success Criteria** (what must be TRUE):
  1. A PSD with image, text, and group layers produces a Widget Blueprint that opens in UMG Designer without errors, showing correct widget hierarchy and positions
  2. Button_ prefix groups produce UButton with normal/hovered/pressed/disabled state brushes from child layers
  3. All prefix-mapped widget types work: Progress_, HBox_, VBox_, Overlay_, ScrollBox_, Slider_, CheckBox_, Input_, List_, Tile_, Switcher_
  4. Textures are imported as persistent UTexture2D assets organized in {TargetDir}/Textures/{PsdName}/{LayerName}
  5. Anchor heuristics auto-assign reasonable anchors, and _anchor-*/_stretch-*/_fill suffixes override them correctly
**Plans:** 4/4 plans complete
Plans:
- [x] 03-01-PLAN.md — Contracts + infrastructure: IPsdLayerMapper, FLayerMappingRegistry, FTextureImporter, settings field
- [x] 03-02-PLAN.md — All widget mappers: Image, Text, Group, Button_, Progress_, and 10 simple prefix types
- [x] 03-03-PLAN.md — WBP generator + anchor calculator: full Generate(), recursive tree, coordinate math, suffix parsing
- [x] 03-04-PLAN.md — Factory integration, automation spec, E2E human verification
**UI hint**: yes

### Phase 4: Text, Fonts & Typography
**Goal**: Text layers reproduce the designer's intended typography -- correct font, size, color, weight, alignment, outline, and shadow
**Depends on**: Phase 3
**Requirements**: TEXT-01, TEXT-02, TEXT-03, TEXT-04, TEXT-05, TEXT-06
**Success Criteria** (what must be TRUE):
  1. Text font size visually matches the PSD (DPI conversion: PS point size x 0.75 = UMG size, or read actual DPI from header)
  2. Bold and italic text layers use the correct TypefaceFontName on the resolved UFont asset
  3. Text outline (stroke) and drop shadow render correctly on UTextBlock via OutlineSettings and ShadowOffset
  4. Font mapping system resolves Photoshop PostScript font names to UE font assets via plugin settings, with a configurable default fallback
  5. Multi-line text enables AutoWrapText when the text box width is defined in the PSD
**Plans**: TBD

### Phase 5: Layer Effects & Blend Modes
**Goal**: Common Photoshop layer effects translate to UMG equivalents, with a flatten fallback for anything too complex
**Depends on**: Phase 4
**Requirements**: FX-01, FX-02, FX-03, FX-04, FX-05
**Success Criteria** (what must be TRUE):
  1. Layer opacity is applied via SetRenderOpacity and hidden layers are collapsed
  2. Color Overlay effect sets the brush tint color on image widgets
  3. Drop Shadow produces a visible offset duplicate behind the main widget
  4. When bFlattenComplexEffects is true, layers with unsupported effects (inner shadow, gradient overlay) are rasterized as a single PNG and imported as UImage
**Plans**: TBD

### Phase 6: Advanced Layout
**Goal**: Production layout features -- 9-slice borders, intelligent anchor detection, Smart Object recursive import, and variant switchers
**Depends on**: Phase 5
**Requirements**: LAYOUT-01, LAYOUT-02, LAYOUT-03, LAYOUT-04, LAYOUT-05, LAYOUT-06
**Success Criteria** (what must be TRUE):
  1. Layers with _9s or _9slice suffix produce UImage with Box draw mode, and margin syntax MyButton_9s[16,16,16,16] sets correct margins
  2. Improved anchor heuristics detect horizontal rows and vertical stacks, suggesting HBox/VBox layout
  3. Smart Object layers are extracted and recursively imported as child Widget Blueprints referenced via UUserWidget; extraction failure falls back to rasterized image
  4. _variants suffix groups produce UWidgetSwitcher with one child per variant slot
**Plans**: TBD
**UI hint**: yes

### Phase 7: Editor UI, Preview & Settings
**Goal**: The import workflow is user-friendly with preview, settings, reimport, and optional CommonUI/animation support
**Depends on**: Phase 6
**Requirements**: EDITOR-01, EDITOR-02, EDITOR-03, EDITOR-04, EDITOR-05, CUI-01, CUI-02, CUI-03, CUI-04
**Success Criteria** (what must be TRUE):
  1. Plugin settings appear in Project Settings > Plugins > PSD2UMG with configurable output dirs, font map, DPI, effect handling, anchor mode, and CommonUI toggle
  2. Import preview dialog shows layer tree with checkboxes and widget type badges; user can toggle layers on/off before import
  3. Reimport updates changed layers (position, image data, text) without destroying manual Blueprint edits, using layer name as stable identity key
  4. Right-click context menu "Import as Widget Blueprint" works in Content Browser
  5. When CommonUI mode is enabled, Button_ prefix produces UCommonButtonBase; input action binding via Button_Confirm[IA_Confirm] syntax works; _show/_hide/_hover variants generate UWidgetAnimation
**Plans**: TBD
**UI hint**: yes

### Phase 8: Testing, Documentation & Release
**Goal**: The plugin is tested, documented, and ready for public open-source release with CI and example projects
**Depends on**: Phase 7
**Requirements**: TEST-01, TEST-02, TEST-03, TEST-04, DOC-01, DOC-02, DOC-03, DOC-04
**Success Criteria** (what must be TRUE):
  1. Unit tests pass for FPsdParser, FLayerMappingRegistry, FAnchorCalculator, and DPI conversion (no UE dependency required)
  2. Integration tests pass for the full PSD-to-WBP pipeline and reimport flow using FAutomationTestBase
  3. Five test PSD files exist in TestData/ covering SimpleHUD, ComplexMenu, MobileUI, Typography, and Effects scenarios
  4. README.md, CONVENTIONS.md, and CHANGELOG.md are complete; example project with 3-4 pre-imported demonstrations works out of the box
  5. GitHub Actions CI compiles on Win64 and runs unit tests on push

**Plans**: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 1 > 2 > 3 > 4 > 5 > 6 > 7 > 8

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. UE5 Port | 2/2 | Complete | 2026-04-08 |
| 2. C++ PSD Parser | 5/5 | Complete | 2026-04-08 |
| 3. Layer Mapping & WBP Generation | 4/4 | Complete   | 2026-04-09 |
| 4. Text, Fonts & Typography | 0/? | Not started | - |
| 5. Layer Effects & Blend Modes | 0/? | Not started | - |
| 6. Advanced Layout | 0/? | Not started | - |
| 7. Editor UI, Preview & Settings | 0/? | Not started | - |
| 8. Testing, Documentation & Release | 0/? | Not started | - |
