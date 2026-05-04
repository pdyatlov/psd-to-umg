# PSD2UMG

## Current State

**v1.3 Advanced Effects shipped 2026-05-04.** Phases 21-23 complete: CJK/emoji text null-sentinel fix (RTXT-01), lrFX HSB ColorSpace conversion (LFXC-01), Photoshop CC 2014+ VlLs effects format parsing (FXFMT-01), vstk vector stroke parser (STROKE-02), lfx2 stroke sibling emission for Image/Shape (STROKE-01 + STROKE-03), and pattern fill layer detection + UImage mapping via composited RGBAPixels (PTFL-01 + PTFL-02). LFXC-02 visual UAT deferred (code correct; empirical confirm on live host pending). PatternFill.psd fixture deferred (D-05).

The plugin is a production-grade Unreal Engine 5.7 editor plugin written entirely in C++20. It delivers a complete one-click PSD-to-Widget-Blueprint import pipeline: a native PhotoshopAPI-backed parser (FPsdParser), a pluggable layer-mapper registry with 16 widget types (including pattern fill), full typography support with Layer-Style stroke and drop shadow routing, layer effects with flatten fallback, 9-slice borders, Smart Object recursive import, row/column anchor heuristics, an SPsdImportPreviewDialog, non-destructive reimport, CommonUI/animation interop, a comprehensive automation spec suite, and a unified `@`-tag grammar. Non-canvas panel child attachment (`@vbox`, `@hbox`, `@scrollbox`, `@overlay`) dispatches via `UPanelWidget::AddChild`. All 75 v1 + 7 v1.0.1 requirements satisfied. v1.3 adds 8 more requirements (RTXT-01, LFXC-01, FXFMT-01, STROKE-01/02/03, PTFL-01/02).

## What This Is

A production-grade Unreal Engine 5.7 editor plugin that imports `.psd` files and converts them into fully functional UMG Widget Blueprints in one click. It preserves layer hierarchy, positions, text properties, images, and effects — so a designer's Photoshop mockup becomes working UI without manual reconstruction. Targets both internal team use and public open-source release.

## Core Value

A designer drops a PSD into Unreal Editor and gets a correctly structured, immediately usable Widget Blueprint — with no Python dependency, no manual tweaking, and no loss of layer intent.

## Requirements

### Validated

<!-- v1.0 baseline — existing AutoPSDUI UE4 capabilities, confirmed in C++ rewrite -->

- ✓ PSD import hook triggers automatically on UE asset import — v1.0
- ✓ Group layers → UCanvasPanel widget hierarchy — v1.0
- ✓ Image layers → UImage with SlateBrush — v1.0
- ✓ Text layers → UTextBlock (content, font, size, color, alignment, outline, shadow) — v1.0
- ✓ `Button_` groups → UButton with normal/hovered/pressed/disabled state brushes — v1.0
- ✓ `Progress_` groups → UProgressBar with background/fill brushes — v1.0
- ✓ `ListView_` / `TileView_` groups → UListView / UTileView with EntryWidgetClass — v1.0
- ✓ Layer images exported as PNG and imported as UTexture2D assets — v1.0
- ✓ Font mapping system (Photoshop font name → UE font asset) via DeveloperSettings — v1.0
- ✓ Plugin enable/disable toggle via Project Settings — v1.0

<!-- v1.0.1 — Panel Child Attachment Hotfix -->

- ✓ `@vbox` group layers generate UVerticalBox with children via UPanelWidget::AddChild in PSD z-order — v1.0.1
- ✓ `@hbox` group layers generate UHorizontalBox with children in PSD z-order — v1.0.1
- ✓ `@scrollbox` group layers generate UScrollBox with children in PSD z-order — v1.0.1
- ✓ `@overlay` group layers generate UOverlay with children in PSD z-order — v1.0.1
- ✓ Canvas group behavior (`@canvas` / default) byte-identical to v1.0 — v1.0.1
- ✓ Unattached children emit UE_LOG Warning (no silent drops) — v1.0.1
- ✓ `Panels.psd` fixture + `FPanelAttachmentSpec` covering VBox/HBox/ScrollBox — v1.0.1

<!-- v1.2 — Layer Fidelity Expansion -->

- ✓ Layer opacity via SetRenderOpacity; Color Overlay → brush tint; Drop Shadow → offset UImage duplicate; flatten fallback — v1.2
- ✓ `_9s` / `_9slice` suffix → Box draw mode with configurable margins — v1.2
- ✓ Gradient fill layers (adjGradient/GdFl) → UImage via composited RGBAPixels at priority 101 — v1.2
- ✓ Solid color fill layers (adjSolidColor/SoCo) → UImage at priority 101 — v1.2
- ✓ Shape/vector layers (vscg) → UImage at priority 101 with solid-color fill — v1.2
- ✓ Button state text animation from _show/_hide/_hover layer variants — v1.2
- ✓ Smart Object layers → recursive child Widget Blueprints — v1.2
- ✓ `_variants` suffix groups → UWidgetSwitcher — v1.2
- ✓ Fill/shape mapper priorities at 101 (eliminates non-stable sort race with FImageLayerMapper) — v1.2
- ✓ FFontResolver auto-discovery cache cleared on all Reimport exit paths — v1.2

<!-- v1.3 — Advanced Effects -->

- ✓ RTXT-01: `Utf8ToFString` null-sentinel stripped; CJK/emoji multi-run text imports without truncation — v1.3
- ✓ LFXC-01: `ConvertLfx2Color` helper routes HSB lrFX colors via HSVToLinearRGB; CMYK/unknown warn+identity — v1.3
- ✓ FXFMT-01: `ParseFrFXDescriptor` adds VlLs branch; Photoshop CC 2014+ stroke effects parsed correctly — v1.3
- ✓ STROKE-02: `ScanVstkStroke` parses vstk tagged block at offset 0 → `bHasVectorStroke` / `VectorStrokeSize` / `VectorStrokeColor` — v1.3
- ✓ STROKE-01: lfx2 stroke sibling UImage emitted for Image/Shape layers (size +2×StrokePx, offset -StrokePx, ZOrder main-1) — v1.3
- ✓ STROKE-03: `FShapeLayerMapper` reads `bHasVectorStroke`; emits vstk stroke sibling with D-03 double-emit guard — v1.3
- ✓ PTFL-01: `ConvertLayerRecursive` detects `adjPattern` → `EPsdLayerType::PatternFill`; no longer falls to Unknown — v1.3
- ✓ PTFL-02: `FPatternFillLayerMapper` (priority 101) → UImage backed by composited RGBAPixels; D-04 nullptr fallback + warning — v1.3

### Active

**Phase 0 — UE 5.7 Port** (legacy tracking; port complete in practice)
- [ ] Plugin loads in UE 5.7.4 without errors (rename AutoPSDUI → PSD2UMG)
- [ ] .uplugin updated: EngineVersion 5.7, PlatformAllowList, no PythonScriptPlugin dependency
- [ ] All deprecated UE4 APIs fixed (FEditorStyle→FAppStyle, AssetRegistryModule paths, etc.)

**Phase 1 — C++ PSD Parser** (legacy tracking; complete in practice)
- [ ] PhotoshopAPI integrated as ThirdParty static lib (no CMake at runtime)
- [ ] FPsdParser::ParseFile() returns correct FPsdDocument with full layer tree
- [ ] Layer names, types, bounds, visibility, opacity, pixel data, text content extracted
- [ ] UPsdImportFactory hooks into UE import pipeline (replaces Python trigger)
- [ ] No Python dependency at plugin runtime

**Phase 2 — Layer Mapping** (legacy tracking; complete in practice)
- [ ] IPsdLayerMapper interface + FLayerMappingRegistry with pluggable mappers
- [ ] All widget types from Phase Validated set reimplemented in C++
- [ ] FWidgetBlueprintGenerator creates UWidgetBlueprint with correct widget hierarchy
- [ ] Anchors auto-assigned from position heuristics
- [ ] Widget Blueprint opens correctly in UMG Designer

**Phase 3 — Text & Typography** (legacy tracking; complete in practice)
- [ ] Full text property extraction (font, size, color, alignment, bold/italic, letter spacing)
- [ ] DPI conversion: Photoshop 72 DPI → UMG 96 DPI (multiply by 0.75)
- [ ] Outline and shadow effects on text
- [ ] Font mapping system (C++ version)
- [ ] Multi-line text with auto-wrap

**Phase 4 — Layer Effects & Blend Modes** — Validated in Phase 5
- [x] Layer opacity applied via SetRenderOpacity
- [x] Color Overlay → brush tint color
- [x] Drop Shadow → approximate UImage offset duplicate
- [x] Flatten fallback for complex effects (rasterize layer+effects as single PNG)
- [x] User-configurable: bFlattenComplexEffects setting

**Phase 5 — 9-Slice, Anchors, Smart Objects** — Validated in Phase 6
- [x] `_9s` / `_9slice` suffix → Box draw mode with configurable margins
- [x] Improved anchor heuristics (row/column detection, edge proximity)
- [ ] Anchor override suffixes (_anchor-tl, _anchor-c, _stretch-h, _fill, etc.) — implemented in Phase 3
- [x] Smart Object layers → recursive import as child Widget Blueprints
- [x] `_variants` suffix groups → UWidgetSwitcher

**Phase 6 — Editor UI & Workflow** — Validated in Phase 7
- [x] Plugin settings in Project Settings → Plugins → PSD2UMG
- [x] Import preview dialog: layer tree with checkboxes, widget type badges
- [x] Reimport support: update changed layers without destroying manual edits
- [x] Right-click context menu in Content Browser ("Import as Widget Blueprint")

**Phase 7 — CommonUI & Interactivity** — Validated in Phase 7
- [x] Optional CommonUI mode: Button_ → UCommonButtonBase
- [x] Input action binding via layer name syntax: `Button_Confirm[IA_Confirm]`
- [x] Animation generation from _show/_hide/_hover layer variants
- [x] ScrollBox content height auto-calculation

**Phase 8 — Testing, Docs & Release** — Validated in Phase 8 (2026-04-13)
- [x] Expanded FWidgetBlueprintGenSpec to 22 It() blocks
- [x] FPsdParserSpec expanded with SimpleHUD and Effects fixture classes
- [x] Test PSD fixtures: SimpleHUD, ComplexMenu, Effects
- [x] README.md with layer naming cheat sheet, settings reference, test instructions
- [x] CI/CD: deferred to post-v1

### Out of Scope

- **Figma / Sketch / XD support** — PSD only for v1; architecture allows future parser swap
- **Runtime plugin** — Editor-only; UMG widgets run at runtime but the import tool doesn't
- **Full PSD effect fidelity** — Complex effects fall back to rasterized PNG; pixel-perfect parity is not a goal
- **Python as core dependency** — Plugin core must not require Python
- **UE4 compatibility** — Targeting UE 5.7+ only
- **Material-based tiling for pattern fills** — Composited PNG always visually correct for v1.3
- **Inside/outside/center stroke precision** — Sibling-image approximation acceptable
- **CMYK/Lab lrFX full conversion** — Warn+identity path sufficient
- **frameFXMulti VlLs stroke rendering** — FXFMT-01 unlocks parsing; emission for VlLs-origin data is post-v1.3

## Context

**Codebase state:** Fork of HakimHua/AutoPSDUI (UE4, Python-based). v1.3 shipped with ~38 files changed in the Advanced Effects milestone (5,744 insertions, 1,697 deletions). Full C++20 plugin running in UE 5.7.

**PhotoshopAPI (EmilDohne/PhotoshopAPI):** C++20 library (MIT license) handles 8/16/32-bit PSD/PSB, groups, text, masks, smart objects, tagged blocks. Key tagged block keys used: adjSolidColor, adjGradient, adjPattern, vscg, vstk.

**Architecture:**
```
Stage 1: PSD Parser (PhotoshopAPI) → FPsdDocument
Stage 2: Layer Mapper (pluggable IPsdLayerMapper) → Widget hierarchy
Stage 3: Widget Builder (UE UMG API) → UWidgetBlueprint .uasset
```

**Platform priority:** Win64 primary, Mac secondary.

## Constraints

- **Engine**: UE 5.7.4 — must use UE5 APIs (FAppStyle not FEditorStyle, etc.)
- **Language**: C++20 — CppStandard = CppStandardVersion.Cpp20 in Build.cs
- **No Python at runtime**: PythonScriptPlugin dependency removed from plugin core
- **PhotoshopAPI linkage**: Pre-built static lib via CMake (not compiled inside UE build system)
- **Editor-only**: Module type "Editor", LoadingPhase "PostEngineInit"
- **License**: MIT (same as original AutoPSDUI fork)

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Fork AutoPSDUI as base | Proven concept, existing C++ UE wrappers reusable, MIT license | ✓ Good |
| PhotoshopAPI for C++ PSD parsing | 327 stars, actively maintained, MIT, handles all PSD features | ✓ Good |
| Pre-build PhotoshopAPI as static lib | CMake + UBT integration is painful; pre-built .lib links cleanly | ✓ Good |
| Pluggable IPsdLayerMapper strategy | Allows user-registered mappers for custom layer→widget rules without modifying core | ✓ Good |
| Layer name prefix convention | Designer-controlled — zero config required for common widgets | ✓ Good — evolved to @-tag grammar in Phase 9 |
| Anchor heuristics from position | Automatic reasonable defaults; override via name suffixes for precision | ✓ Good |
| Flatten fallback for complex effects | Full material-based effect fidelity is fragile; rasterize is always correct | ✓ Good |
| CommonUI as opt-in | CommonUI requires extra project setup; don't force it | ✓ Good |
| UPanelWidget::AddChild for non-canvas groups | Fixed silent child-drop bug in v1.0.1 | ✓ Good |
| Priority 101 for fill/shape mappers | Eliminates non-stable sort race with FImageLayerMapper (priority 100) | ✓ Good — v1.2 |
| Separate bHasVectorStroke field (vstk) | CP-02: vstk must not overwrite bHasStroke (lfx2); separate fields prevent double-emit | ✓ Good — v1.3 |
| Adj-cast ExtractImagePixels for PatternFill | CP-04: PtFl has no Clr key; composited RGBAPixels via AdjustmentLayer<T> cast is the only data source | ✓ Good — v1.3 |
| Defer Overlay/Canvas/Nested spec cases | Implementation is generic; test coverage gap only | ⚠ Revisit in v1.4 |
| Defer LFXC-02 visual UAT | Code confirmed correct; live host UAT requires physical UE 5.7 project setup | ⚠ Pending — v1.4 |

## Evolution

This document evolves at phase transitions and milestone boundaries.

---
*Last updated: 2026-05-04 — v1.3 Advanced Effects milestone shipped. 8 requirements validated (RTXT-01, LFXC-01, FXFMT-01, STROKE-01/02/03, PTFL-01/02). Phases 21-23 archived.*
