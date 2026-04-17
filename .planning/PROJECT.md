# PSD2UMG

## Current State

**v1.0.1 shipped 2026-04-17.**

The plugin is a production-grade Unreal Engine 5.7 editor plugin written entirely in C++20. It delivers a complete one-click PSD-to-Widget-Blueprint import pipeline: a native PhotoshopAPI-backed parser (FPsdParser), a pluggable layer-mapper registry with 15 widget types, full typography support including Layer-Style stroke and drop shadow routing, layer effects with flatten fallback, 9-slice borders, Smart Object recursive import, row/column anchor heuristics, an SPsdImportPreviewDialog, non-destructive reimport, CommonUI/animation interop, a comprehensive automation spec suite, and a unified `@`-tag grammar. Non-canvas panel child attachment (`@vbox`, `@hbox`, `@scrollbox`, `@overlay`) now correctly dispatches via `UPanelWidget::AddChild` — the v1.0 silent-drop bug is fixed. All 75 v1 + 7 v1.0.1 requirements satisfied.

See `.planning/milestones/v1.0.1-ROADMAP.md` for the v1.0.1 archive.

## Current Milestone: v1.1 Import Fidelity Fixes

**Goal:** Fix import dialog correctness (hidden layer filtering, checkbox exclusion) and text property fidelity (font size, alignment, base color).

**Target fixes:**
- Hidden PSD layers shown as unchecked by default in import dialog
- Unchecked layers in import dialog actually excluded from WBP output
- Font size: PSD pt value imported correctly into UMG (currently inflated)
- Text alignment: PSD alignment preserved in UTextBlock after import
- Text base color: correct color imported (currently corrupted to red)

## Next Milestone Goals (v1.2+)

_Candidate scope (deferred from earlier milestones):_

- Human UAT pass against a host UE 5.7 project (lrFX visual confirms, 9s/SmartObj/CommonUI/anim runtime checks)
- GitHub Actions CI pipeline (D-04)
- Image-layer stroke rendering (D-12 data already populated)
- frameFXMulti VlLs stroke format (newer Photoshop)
- URichTextBlock mixed-style runs (TEXT-V2-01)
- Configurable panel slot properties (HBox alignment, Overlay padding, ScrollBox orientation)
- Fab/Marketplace submission packaging (MKT-03)
- Overlay/Canvas/Nested spec coverage (test-coverage gap; implementation complete)

---

## What This Is

A production-grade Unreal Engine 5.7 editor plugin that imports `.psd` files and converts them into fully functional UMG Widget Blueprints in one click. It preserves layer hierarchy, positions, text properties, images, and effects — so a designer's Photoshop mockup becomes working UI without manual reconstruction. Targets both internal team use and public open-source release.

## Core Value

A designer drops a PSD into Unreal Editor and gets a correctly structured, immediately usable Widget Blueprint — with no Python dependency, no manual tweaking, and no loss of layer intent.

## Requirements

### Validated

<!-- Shipped and confirmed valuable from existing AutoPSDUI UE4 baseline. -->

- ✓ PSD import hook triggers automatically on UE asset import — existing
- ✓ Group layers → UCanvasPanel widget hierarchy — existing
- ✓ Image layers → UImage with SlateBrush — existing
- ✓ Text layers → UTextBlock (content, font, size, color, alignment, outline, shadow) — existing
- ✓ `Button_` groups → UButton with normal/hovered/pressed/disabled state brushes — existing
- ✓ `Progress_` groups → UProgressBar with background/fill brushes — existing
- ✓ `ListView_` / `TileView_` groups → UListView / UTileView with EntryWidgetClass — existing
- ✓ Layer images exported as PNG and imported as UTexture2D assets — existing
- ✓ Font mapping system (Photoshop font name → UE font asset) via DeveloperSettings — existing
- ✓ Plugin enable/disable toggle via Project Settings — existing

<!-- v1.0.1 — Panel Child Attachment Hotfix -->

- ✓ `@vbox` group layers generate UVerticalBox with children attached via UPanelWidget::AddChild in PSD z-order — v1.0.1
- ✓ `@hbox` group layers generate UHorizontalBox with children in PSD z-order — v1.0.1
- ✓ `@scrollbox` group layers generate UScrollBox with children in PSD z-order — v1.0.1
- ✓ `@overlay` group layers generate UOverlay with children in PSD z-order — v1.0.1
- ✓ Canvas group behavior (`@canvas` / default) byte-identical to v1.0 — no regression — v1.0.1
- ✓ Unattached children emit UE_LOG Warning (no silent drops) — v1.0.1
- ✓ `Panels.psd` fixture + `FPanelAttachmentSpec` covering VBox/HBox/ScrollBox — v1.0.1

### Active

<!-- All 8 phases of PSD2UMG_DEVELOPMENT_PLAN.md — building toward these. -->

**Phase 0 — UE 5.7 Port**
- [ ] Plugin loads in UE 5.7.4 without errors (rename AutoPSDUI → PSD2UMG)
- [ ] .uplugin updated: EngineVersion 5.7, PlatformAllowList, no PythonScriptPlugin dependency
- [ ] All deprecated UE4 APIs fixed (FEditorStyle→FAppStyle, AssetRegistryModule paths, etc.)

**Phase 1 — C++ PSD Parser**
- [ ] PhotoshopAPI integrated as ThirdParty static lib (no CMake at runtime)
- [ ] FPsdParser::ParseFile() returns correct FPsdDocument with full layer tree
- [ ] Layer names, types, bounds, visibility, opacity, pixel data, text content extracted
- [ ] UPsdImportFactory hooks into UE import pipeline (replaces Python trigger)
- [ ] No Python dependency at plugin runtime

**Phase 2 — Layer Mapping**
- [ ] IPsdLayerMapper interface + FLayerMappingRegistry with pluggable mappers
- [ ] All widget types from Phase Validated set reimplemented in C++
- [ ] FWidgetBlueprintGenerator creates UWidgetBlueprint with correct widget hierarchy
- [ ] Anchors auto-assigned from position heuristics
- [ ] Widget Blueprint opens correctly in UMG Designer

**Phase 3 — Text & Typography**
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
- [ ] Anchor override suffixes (_anchor-tl, _anchor-c, _stretch-h, _fill, etc.) — already implemented in Phase 3
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
- [x] Expanded FWidgetBlueprintGenSpec to 22 It() blocks (DPI, anchors, prefixes, DetectChange)
- [x] FPsdParserSpec expanded with SimpleHUD and Effects fixture classes (26 total It() blocks)
- [x] Test PSD fixtures: SimpleHUD, ComplexMenu, Effects (MobileUI deferred; Typography pre-existing)
- [x] README.md with layer naming cheat sheet, settings reference, test instructions
- [x] CI/CD: deferred to post-v1 (no GitHub Actions for v1)
- [x] Example PSDs bundled in Tests/Fixtures/ (no separate project)

### Out of Scope

- **Figma / Sketch / XD support** — PSD only for v1; architecture allows future parser swap
- **Runtime plugin** — Editor-only (no runtime module); UMG widgets run at runtime but the import tool doesn't
- **Full PSD effect fidelity** — Complex effects (inner shadow, gradient overlay, etc.) fall back to rasterized PNG; pixel-perfect parity is not a goal
- **Python as core dependency** — Python scripting remains available to users but the plugin core must not require it
- **UE4 compatibility** — Targeting UE 5.7+ only; no backward compat shims

## Context

**Codebase state:** Fork of HakimHua/AutoPSDUI (UE4, Python-based). Module currently named `AutoPSDUI`, engine target 4.26. All active requirements involve a full rewrite to C++20 targeting UE 5.7.4.

**Existing Python baseline proves the concept works:** The `Content/Python/auto_psd_ui.py` script already handles Canvas/Button/Image/Text/ProgressBar/ListView/TileView conversion correctly. The C++ rewrite in Phases 1-2 must match this feature coverage before adding new capabilities.

**PhotoshopAPI (EmilDohne/PhotoshopAPI):** C++20 library (MIT license) that handles 8/16/32-bit PSD/PSB, groups, text, masks, smart objects. Text layer support tracked in issue #126 — may need manual `TySh` descriptor parsing as fallback.

**Architecture target (from development plan):**
```
Stage 1: PSD Parser (PhotoshopAPI) → FPsdDocument
Stage 2: Layer Mapper (pluggable) → FWidgetTreeModel  
Stage 3: Widget Builder (UE UMG API) → UWidgetBlueprint .uasset
```
Each stage is independently testable and replaceable.

**Platform priority:** Win64 primary, Mac secondary.

## Constraints

- **Engine**: UE 5.7.4 — must use UE5 APIs (FAppStyle not FEditorStyle, PlatformAllowList not WhitelistPlatforms, etc.)
- **Language**: C++20 — CppStandard = CppStandardVersion.Cpp20 in Build.cs
- **No Python at runtime**: PythonScriptPlugin dependency removed from plugin core
- **PhotoshopAPI linkage**: Pre-built static lib via CMake (not compiled inside UE build system) — avoids CMake/UBT conflicts
- **Editor-only**: Module type "Editor", LoadingPhase "PostEngineInit" — no shipping module
- **License**: MIT (same as original AutoPSDUI fork)

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Fork AutoPSDUI as base | Proven concept, existing C++ UE wrappers reusable, MIT license | ✓ Good |
| PhotoshopAPI for C++ PSD parsing | 327 stars, actively maintained, MIT, handles all PSD features including 32-bit | ✓ Good |
| Pre-build PhotoshopAPI as static lib | CMake + UBT integration is painful; pre-built .lib links cleanly via Build.cs | ✓ Good |
| Pluggable IPsdLayerMapper strategy | Allows user-registered mappers for custom layer→widget rules without modifying core | ✓ Good |
| Layer name prefix convention | Designer-controlled (Button_, Progress_, etc.) — zero config required for common widgets | ✓ Good — evolved to @-tag grammar in Phase 9 |
| Anchor heuristics from position | Automatic reasonable defaults; override via name suffixes for precision | ✓ Good |
| Flatten fallback for complex effects | Full material-based effect fidelity is complex and fragile; rasterize is always correct | ✓ Good |
| CommonUI as opt-in | CommonUI requires extra project setup; don't force it on projects that don't use it | ✓ Good |
| UPanelWidget::AddChild for non-canvas groups | PopulateCanvas hard-cast to UCanvasPanel silently dropped all non-canvas children; UPanelWidget dispatch fixes this generically | ✓ Good — v1.0.1 |
| Clear-and-rebuild for non-canvas reimport | Non-canvas slots carry no PSD state; ClearChildren+PopulateChildren is simpler and correct vs diff-and-patch | ✓ Good — v1.0.1 |
| Defer Overlay/Canvas/Nested spec cases | Implementation is generic; test coverage gap only — triaged as v1.1+ work | ⚠ Revisit in v1.1 |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd:transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `/gsd:complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-04-17 — v1.1 milestone started (import fidelity fixes)*
