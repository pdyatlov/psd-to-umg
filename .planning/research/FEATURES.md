# Feature Landscape

**Domain:** PSD-to-UMG Widget Blueprint import plugin for Unreal Engine 5.7
**Researched:** 2026-04-07

## Competing Tools

Before defining features, here is the landscape of existing tools that solve similar problems. This informs what is table stakes vs. differentiating.

### PSD2UMG (durswd) - Marketplace, $14.99
**Engine:** UE4 only (no UE5 version found)
**Approach:** Custom `.psdumg` file extension, C++ plugin
**Supported widgets:** Image, Button, ProgressBar, CanvasPanel, Text (Beta)
**Naming convention:** `LayerName@Button`, state variants via `[Normal]`/`[Hovered]`/`[Pressed]`
**Strengths:**
- Automatic anchor assignment
- Reimport preserves manual edits (only updates layers with matching names)
- Pure C++ (no Python dependency)
**Weaknesses:**
- No layer styles or shape layers
- Only 8-bit RGB
- Text is Beta-quality (position and string only, no font/size/color)
- Requires renaming `.psd` to `.psdumg` manually
- No ListView/TileView/ScrollBox support
- No 9-slice support
- UE4 only, appears unmaintained for UE5
**Confidence:** MEDIUM (based on official docs at GitHub + marketplace listing)

### PSD2UI - Marketplace
**Engine:** UE4 (Photoshop 2019-2022)
**Approach:** Batch extraction from PSD
**Supported widgets:** Images with coordinates, text extraction
**Strengths:** Fast batch cutting, extracts coordinates/sizes/fonts/colors
**Weaknesses:** Appears to focus on extraction rather than full Widget Blueprint generation. Limited documentation available.
**Confidence:** LOW (marketplace listing only, sparse details)

### AutoPSDUI (HakimHua) - GitHub, MIT
**Engine:** UE4.26, Python-based
**Approach:** Import hook triggers Python script via PythonScriptPlugin
**Supported widgets:** Canvas, Image, Button (4-state), Text (full properties), ProgressBar, ListView, TileView
**Strengths:**
- Richest widget type coverage of any PSD tool
- Full text property extraction (font, size, color, alignment, stroke, shadow)
- Button state images (normal/hovered/pressed/disabled)
- ListView/TileView with child Widget Blueprint generation
- Color overlay effect support
- Automatic import on PSD drop
**Weaknesses:**
- Python runtime dependency (psd_tools)
- No reimport (overwrites everything)
- No anchor assignment
- No 9-slice
- No import preview/confirmation
- UE4 only
- Hacky font size correction (`size - 2` instead of proper DPI conversion)
**Confidence:** HIGH (we have the full source code)

### Figma2UMG (Buvi Games) - Marketplace + GitHub (recently open-sourced)
**Engine:** UE5
**Approach:** Figma API integration, pulls designs via access token
**Supported widgets:** Frames, text, shapes with basic styling
**Strengths:**
- Works with Figma (industry-standard design tool)
- Preserves hierarchy and nesting
- Active development, UE5 support
**Weaknesses:**
- No PSD support (Figma only)
- Missing advanced layouts (HBox/VBox/Grid)
- No font sync, no text formatting, no outlines/gradients
- Requires Figma API token setup
**Confidence:** MEDIUM (GitHub README + marketplace listing)

### Epic's Native PSD Layer Import (UE 5.5+)
**What it does:** Imports PSD files natively preserving layer structure as textures with "Layer Depth Offset" for animation. This is for 3D compositing and material layers -- NOT for UMG Widget Blueprint generation.
**Relevance:** Does not compete. Solves a completely different problem (texture/material layers, not UI widgets).
**Confidence:** MEDIUM (roadmap entry confirmed shipped in 5.5)

### Summary of Competitive Gap

No existing tool provides all of: UE5 support + full text properties + button states + ListView/TileView + 9-slice + anchor assignment + reimport + no Python dependency. Our project is uniquely positioned to be the first comprehensive PSD-to-UMG tool for UE5.

---

## Table Stakes

Features users expect. Missing = product feels incomplete or no better than manual work.

| # | Feature | Why Expected | Complexity | Existing? |
|---|---------|-------------|------------|-----------|
| 1 | **One-click PSD import** (drag PSD into Content Browser, get Widget Blueprint) | This IS the core value proposition. Without it, the tool doesn't exist. | Med | Yes (Python) |
| 2 | **Layer hierarchy preservation** (groups become nested CanvasPanels) | Designers structure PSDs with intent. Flattening that structure makes the output useless for iteration. | Low | Yes (Python) |
| 3 | **Image layer extraction** (pixel/shape/smart object layers become UImage with SlateBrush + imported UTexture2D) | Images are 80%+ of any UI mockup by layer count. | Med | Yes (Python) |
| 4 | **Text layer extraction** (content, font family, size, color, alignment) | Text is the other major layer type. Getting at minimum the string + size + color is mandatory. | Med | Yes (Python) |
| 5 | **Button widget generation** (Button_ prefix group with normal/hovered/pressed/disabled state images) | Buttons are the most common interactive widget. Designers already organize state layers in groups. | Med | Yes (Python) |
| 6 | **Correct positioning** (layer bounds map to CanvasPanel slot position and size) | If elements don't appear where the designer placed them, the output is useless. | Low | Yes (Python) |
| 7 | **Font mapping** (Photoshop font name to UE font asset via settings) | Fonts never match 1:1 between PS and UE. A mapping system is essential. | Low | Yes (Python) |
| 8 | **DPI-correct text sizing** (PS 72 DPI to UE 96 DPI conversion) | Without this, all text is the wrong size and users lose trust immediately. The existing Python code uses a hack (`size - 2`). | Low | Partial (hacky) |
| 9 | **Text stroke/outline and drop shadow** | These are the two most common text effects in game UI. Every HUD has outlined text. | Low | Yes (Python) |
| 10 | **Color overlay effect** (Multiply blend mode on images) | Extremely common PSD technique for tinting reusable UI elements. Already proven valuable in the Python baseline. | Low | Yes (Python) |
| 11 | **No Python dependency** | A C++ UE plugin requiring Python at runtime is a deployment and maintenance nightmare. This is table stakes for a "production-grade" tool. | High | No (this is the core rewrite motivation) |
| 12 | **Layer visibility respected** (hidden layers are skipped) | Designers use hidden layers for reference/notes. Importing them pollutes the widget tree. | Low | Unclear |
| 13 | **Unique widget names** (deduplication of duplicate layer names) | PSD allows duplicate layer names; UMG widget trees do not. Without dedup, the blueprint fails to compile. | Low | Yes (Python) |

---

## Differentiators

Features that elevate the tool from "functional" to "professional-grade." Not expected by users, but valued enough to drive adoption over manual workflow or competing tools.

| # | Feature | Value Proposition | Complexity | Phase |
|---|---------|-------------------|------------|-------|
| 1 | **Automatic anchor assignment** (heuristic-based from position relative to canvas bounds) | Eliminates the #1 most tedious manual post-import task. PSD2UMG (durswd) does this and it is their top marketing point. Without it, every widget needs manual anchor setup. | Med | 2 |
| 2 | **Anchor override via name suffixes** (`_anchor-tl`, `_anchor-c`, `_stretch-h`, `_fill`) | Gives designers explicit control when heuristics are wrong, without requiring them to open UE. This is designer empowerment -- no competing tool offers it. | Low | 5 |
| 3 | **Reimport that preserves manual edits** (update only changed layers, keep user additions) | Critical for iterative design. PSD2UMG (durswd) does this. Without it, every PSD change destroys manual Blueprint work, making the tool useless after initial import. | High | 6 |
| 4 | **ProgressBar widget generation** (Progress_ prefix with background/fill sub-layers) | Common game UI element. Already proven in AutoPSDUI baseline. | Low | 2 |
| 5 | **ListView / TileView with child Widget Blueprint** (List_ / Tile_ prefix groups with auto-generated entry class) | Advanced feature no competing PSD tool offers. Saves significant manual setup for inventory grids, leaderboards, chat panels. | Med | 2 |
| 6 | **9-slice / Box draw mode** (`_9s` or `_9slice` suffix triggers Box draw mode with configurable margins) | Extremely common for scalable UI backgrounds and frames. Manual setup per image is tedious. | Med | 5 |
| 7 | **Import preview dialog** (layer tree with checkboxes, widget type badges, before committing import) | Gives users confidence and control. Prevents "import and pray" syndrome. No competing tool offers this. | Med | 6 |
| 8 | **Smart Object recursive import** (Smart Object layers become child Widget Blueprints, enabling reusable UI components) | Maps directly to how designers organize reusable components in Photoshop. Creates natural UMG component hierarchy. | Med | 5 |
| 9 | **Layer opacity mapping** (layer opacity to SetRenderOpacity) | Simple but visible fidelity improvement. | Low | 4 |
| 10 | **Flatten fallback for complex effects** (rasterize layer+effects as single PNG when effects cannot be mapped to UMG properties) | Ensures no layer is silently dropped. Better to have a correct-looking rasterized image than a broken approximation. | Med | 4 |
| 11 | **CommonUI opt-in mode** (Button_ maps to UCommonButtonBase instead of UButton) | CommonUI is the recommended approach for cross-platform UE5 projects. No existing tool supports it. | Med | 7 |
| 12 | **Widget variant groups** (`_variants` suffix maps to UWidgetSwitcher) | Maps to common PSD pattern of showing different states of the same UI area. | Low | 5 |
| 13 | **ScrollBox content auto-calculation** (detect scrollable areas, set content size) | ScrollBox is one of the most common UMG containers, but hard to auto-detect from PSD. | Med | 7 |
| 14 | **Pluggable layer mapper registry** (IPsdLayerMapper interface for user-registered custom widget types) | Power-user feature. Studios with custom widget types can extend without forking. | Med | 2 |
| 15 | **Input action binding via layer name** (`Button_Confirm[IA_Confirm]`) | Bridges the gap between visual design and interactivity. Designer can specify input bindings in the PSD. | Low | 7 |

---

## Anti-Features

Things that seem useful but add complexity without proportional value. Explicitly NOT building these.

| Anti-Feature | Why It Seems Useful | Why Avoid | What to Do Instead |
|-------------|---------------------|-----------|-------------------|
| **Full PSD effect fidelity** (inner shadow, gradient overlay, bevel, pattern overlay as UMG materials) | "Pixel-perfect import" sounds impressive | Each effect requires a custom material or complex shader. The combinatorial explosion of effects makes this a maintenance nightmare. Results are fragile and often look wrong. | Use the flatten fallback: rasterize layer+effects as a single PNG. Always correct, zero maintenance. |
| **Figma/Sketch/XD parser support in v1** | Figma is more popular than Photoshop for UI design | Each format is a completely different parser with different semantics. The architecture supports future parser swap, but shipping PSD first lets us get the full pipeline right before adding format complexity. | Design IPsdParser interface abstractly enough that FigmaParser could be added later without pipeline changes. |
| **Runtime PSD loading** | "Load UI from PSD at runtime for modding" | PSD parsing is heavyweight (memory, CPU). Runtime UMG construction is fragile and performance-sensitive. The use case is niche. | Editor-only import. Runtime uses the generated Widget Blueprint like any other UMG asset. |
| **Automatic animation generation from layer groups** (beyond simple show/hide) | "Import animations from PSD" | PSD has no animation data. Any "animation" would be pure guessing from layer naming, producing results that rarely match designer intent. | Support simple `_show`/`_hide`/`_hover` variant layers that can drive UWidgetAnimation visibility toggles (Phase 7). Anything more complex should be authored in UMG Designer. |
| **Automatic layout panel detection** (HorizontalBox, VerticalBox, GridPanel from spatial analysis) | "Detect that these items are in a row and use HBox" | Heuristic-based layout detection is unreliable. Items that look like a row may have intentional spacing differences. False positives create worse results than CanvasPanel with manual conversion. | Use CanvasPanel everywhere (as PSD uses absolute positioning). Document how to manually promote groups to layout panels after import. |
| **Two-way sync** (edit Widget Blueprint, push changes back to PSD) | "Round-trip editing" | PSD writing is complex and error-prone. Widget Blueprint edits (adding logic, bindings) have no PSD representation. The workflow is inherently one-directional: design flows from PSD to engine. | One-way import with reimport support. Designers edit PSD, developers edit Blueprint. Clear ownership boundary. |
| **Custom `.psdumg` file extension** (like PSD2UMG does) | Prevents accidental imports | Adds friction (designers must rename files). Feels hacky. Breaks existing workflows. | Use the standard `.psd` extension with an import hook. Provide an enable/disable toggle and optionally a confirmation dialog. |

---

## Feature Dependencies

```
Layer Hierarchy Parsing ──> All widget types
                        ├── Image extraction ──> Button states (uses images)
                        │                    ├── ProgressBar (uses images)
                        │                    └── 9-slice (modifies image draw mode)
                        ├── Text extraction ──> Font mapping
                        │                   └── DPI conversion
                        ├── Anchor heuristics ──> Anchor override suffixes
                        ├── Color overlay ──> Flatten fallback (for unsupported effects)
                        └── Smart Object detection ──> Recursive child WBP import

Reimport support requires: widget name matching + selective update logic
Import preview requires: full parse before committing widget creation
CommonUI mode requires: all base widget types working first
ListView/TileView requires: child Widget Blueprint generation pipeline
Pluggable mappers require: clean IPsdLayerMapper interface from Phase 2
```

---

## UMG Widget Type Priority

Based on frequency of use in game UI mockups and the effort-to-value ratio:

| Priority | UMG Widget | PSD Source | Rationale |
|----------|------------|------------|-----------|
| **P0** | UCanvasPanel | Group layers | Foundation. Every widget tree needs a root canvas. All positioning depends on it. |
| **P0** | UImage | Pixel/shape/smart object layers | Most numerous layer type in any PSD. |
| **P0** | UTextBlock | Type layers | Second most common layer type. |
| **P0** | UButton | `Button_` prefix groups | Most common interactive widget in game UI. |
| **P1** | UProgressBar | `Progress_` prefix groups | Common in HUDs (health, XP, loading). |
| **P1** | UScrollBox | `Scroll_` prefix groups (new) | Extremely common in menus, inventory, settings. Currently not supported by AutoPSDUI baseline. |
| **P1** | UListView / UTileView | `List_` / `Tile_` prefix groups | Proven valuable in baseline for inventory grids, leaderboards. |
| **P2** | UWidgetSwitcher | `_variants` suffix groups | State management for tab panels, multi-state UI areas. |
| **P2** | UCheckBox | `Check_` prefix groups (new) | Common in settings menus. Could be added as a pluggable mapper. |
| **P2** | USizeBox | `_fixed` suffix (new) | Constraining child dimensions. Useful but can be added post-import. |
| **P3** | UCommonButtonBase | `Button_` in CommonUI mode | For projects using CommonUI framework. |
| **P3** | URichTextBlock | `RichText_` prefix (new) | For text with inline styling. Niche but powerful. |

---

## MVP Recommendation

**Minimum viable product** (covers all table stakes + critical differentiators):

1. **C++ PSD parser** via PhotoshopAPI -- replaces Python dependency (table stakes #11)
2. **CanvasPanel hierarchy** from groups (table stakes #2)
3. **Image extraction** with texture import (table stakes #3)
4. **Text extraction** with font/size/color/alignment + DPI conversion (table stakes #4, #7, #8)
5. **Button generation** with 4-state images (table stakes #5)
6. **Correct positioning** in CanvasPanel slots (table stakes #6)
7. **Text stroke and shadow** (table stakes #9)
8. **Color overlay** (table stakes #10)
9. **Automatic anchor heuristics** (differentiator #1 -- this is what makes the output immediately usable vs. requiring hours of manual anchor setup)

**Defer to post-MVP:**
- Reimport (differentiator #3): High complexity, needs careful design. Phase 6.
- Import preview dialog (differentiator #7): Valuable but not blocking adoption. Phase 6.
- 9-slice (differentiator #6): Useful but niche per-project. Phase 5.
- CommonUI (differentiator #11): Opt-in for projects that need it. Phase 7.
- Smart Object recursive import (differentiator #8): Powerful but complex. Phase 5.

This MVP matches Phases 0-3 of the existing development plan, with anchor heuristics pulled into Phase 2 as a critical differentiator.

---

## Painful Manual Steps This Tool Should Automate

Ranked by time wasted per PSD import when done manually:

| Rank | Manual Step | Time per PSD | Our Solution |
|------|-------------|-------------|--------------|
| 1 | **Exporting every layer as PNG, importing into UE, creating textures** | 30-120 min for a complex UI | Automatic layer-to-texture pipeline |
| 2 | **Creating Widget Blueprint, adding every widget, setting position/size** | 30-60 min | Full widget tree generation from PSD structure |
| 3 | **Setting up anchors for every widget** | 15-30 min | Automatic anchor heuristics |
| 4 | **Configuring button styles** (4 brush states per button) | 5-10 min per button | Button_ prefix with state sub-layers |
| 5 | **Font setup** (finding matching UE font, setting size, color, alignment per text widget) | 2-5 min per text | Full text property extraction + font mapping |
| 6 | **Rebuilding after design changes** (re-export, re-position, re-configure) | 50-100% of initial time | Reimport preserving manual edits |
| 7 | **9-slice margin setup** per scalable background | 2-3 min per image | `_9s` suffix with auto margins |
| 8 | **ListView/TileView entry class creation** (separate WBP, interface application) | 10-15 min per list | Automatic child WBP + ListEntryInterface |

**Total manual time for a moderately complex UI screen:** 2-4 hours.
**Target time with PSD2UMG:** Under 30 seconds (PSD drag and drop).

---

## Sources

- [PSD2UMG (durswd) documentation](https://github.com/durswd/ue4psd2umg_web/blob/master/docs/Index.md)
- [PSD2UMG Marketplace listing](https://www.unrealengine.com/marketplace/en-US/product/psd2umg)
- [PSD2UI Marketplace listing](http://marketplace-website-node-launcher-prod.ol.epicgames.com/ue/marketplace/en-US/product/psd2ui)
- [AutoPSDUI GitHub](https://github.com/HakimHua/AutoPSDUI)
- [Figma2UMG GitHub](https://github.com/Buvi-Games/figma2umg)
- [Figma2UMG Figma Plugin](https://www.figma.com/community/plugin/1368487806996965174/figma2umg-unreal-importer)
- [Epic PSD Layer Import roadmap](https://portal.productboard.com/epicgames/1-unreal-engine-public-roadmap/c/1969-psd-layer-import)
- [UE5.7 UMG Best Practices](https://dev.epicgames.com/documentation/en-us/unreal-engine/umg-best-practices-in-unreal-engine)
- [UE4 Widget Type Reference](https://docs.unrealengine.com/4.27/en-US/InteractiveExperiences/UMG/UserGuide/WidgetTypeReference)
- AutoPSDUI source code: `Content/Python/auto_psd_ui.py`, `Content/Python/AutoPSDUI/psd_utils.py` (direct analysis)
