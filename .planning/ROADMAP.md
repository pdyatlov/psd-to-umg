# Roadmap: PSD2UMG

## Overview

PSD2UMG transforms Photoshop files into fully functional UMG Widget Blueprints inside UE 5.7. The roadmap moves from porting the existing UE4 codebase, through building a native C++ PSD parser, to implementing the full layer-to-widget mapping pipeline, then layering on text, effects, layout, editor UI, and CommonUI support, culminating in testing and release. Each phase delivers a verifiable capability on top of the previous one.

## Shipped Milestones

- [x] **v1.0** (shipped 2026-04-15) — [archive](milestones/v1.0-ROADMAP.md) • [audit](milestones/v1.0-MILESTONE-AUDIT.md)
- [x] **v1.0.1** (shipped 2026-04-17) — [archive](milestones/v1.0.1-ROADMAP.md)

## Phases

<details>
<summary>✅ v1.0 MVP (Phases 1-9) — SHIPPED 2026-04-15</summary>

See [milestones/v1.0-ROADMAP.md](milestones/v1.0-ROADMAP.md) for full phase details.

</details>

<details>
<summary>✅ v1.0.1 Panel Child Attachment Hotfix (Phase 10) — SHIPPED 2026-04-17</summary>

- [x] Phase 10: Panel Child Attachment (3/3 plans) — completed 2026-04-17

See [milestones/v1.0.1-ROADMAP.md](milestones/v1.0.1-ROADMAP.md) for full phase details.

</details>

## v1.1 Import Fidelity Fixes (Phases 11-12)

- [x] **Phase 11: Import Dialog Hidden-Layer Filtering** - Skipped/deferred (2026-04-21)
- [x] **Phase 12: Text Property Fidelity** - Font size, alignment, and base color correctly preserved from PSD through to UTextBlock (completed 2026-04-17)

## Phase Details

### Phase 11: Import Dialog Hidden-Layer Filtering
**Goal**: Users can see which PSD layers are hidden and control which layers are included in the generated Widget Blueprint via import dialog checkboxes
**Depends on**: Nothing (targets existing SPsdImportPreviewDialog + FWidgetBlueprintGenerator)
**Requirements**: HIDDEN-01, HIDDEN-02, FILTER-01, FILTER-02
**Success Criteria** (what must be TRUE):
  1. Importing a PSD with hidden layers shows those layers with their checkbox unchecked by default in the preview dialog
  2. Hidden layers are visually distinct from visible layers in the dialog tree (dimmed row, eye-closed icon, or [hidden] label)
  3. Unchecking a layer in the dialog results in that layer's widget not appearing in the generated WBP
  4. Unchecking a group layer also excludes all of its children from the generated WBP
**Plans**: 2 plans
Plans:
- [ ] 11-01-PLAN.md — Data model + bVisible init fix (FPsdLayerTreeItem fields, BuildTreeRecursive, LayerName bug)
- [ ] 11-02-PLAN.md — UI eye-icon column in OnGenerateRow

### Phase 12: Text Property Fidelity
**Goal**: Text layers import with correct font size, paragraph alignment, and fill color — what the designer set in Photoshop is what appears in UMG
**Depends on**: Nothing (targets existing FTextLayerMapper)
**Requirements**: TEXT-F-01, TEXT-F-02, TEXT-F-03
**Success Criteria** (what must be TRUE):
  1. A text layer with 24.45 pt font in PSD produces a UTextBlock with the correct UMG font size (not inflated to 30)
  2. A center-aligned paragraph in PSD produces a UTextBlock with ETextJustify::Center after compile+save
  3. A gray text layer in PSD produces a UTextBlock with a gray ColorAndOpacity (not red or another corrupted color)
**Plans**: 2 plans
Plans:
- [x] 12-01-PLAN.md — Font size fidelity: confirm raw SizePx, remove * 0.75f, update test expectations (TEXT-F-01)
- [x] 12-02-PLAN.md — Alignment + fill color: fixture update, parser fallbacks, new spec assertions (TEXT-F-02, TEXT-F-03)

## v1.2 Layer Fidelity Expansion (Phases 13-17)

- [x] **Phase 13: Gradient Layers** - Gradient fill layers imported as pre-rendered TC_BC7 UImage widgets; solid fills as zero-texture UImage tinted via FX-03 (completed 2026-04-21)
- [ ] **Phase 14: Shape/Vector Layers** - Photoshop solid-color shape layers imported as UImage with solid-color brush
- [ ] **Phase 15: Group Effects** - Effects applied to group layers (drop shadow, color overlay) propagated to the group's container widget
- [ ] **Phase 16: Rich Text / Multiple Text Runs** - Text layers with mixed styles (bold/italic/color spans) imported as URichTextBlock with inline style definitions
- [ ] **Phase 17: Automated Font Matching** - Photoshop font names resolved to UE font assets automatically via a configurable name-mapping table with fuzzy fallback

### Phase 13: Gradient Layers
**Goal**: Gradient fill layers from Photoshop import as usable UMG widgets — either pre-rendered as Texture2D or as a native gradient widget if available
**Depends on**: Phase 2 (parser), Phase 3 (mapper pipeline)
**Requirements**: GRAD-01, GRAD-02
**Success Criteria** (what must be TRUE):
  1. A linear gradient fill layer in PSD produces a valid UMG widget (UImage with pre-rendered texture or equivalent) after import
  2. Gradient color stops and direction are preserved with no visible banding at standard resolutions
**Plans**: 3 plans
Plans:
- [x] 13-01-PLAN.md — Fixture + RED spec stubs + EPsdLayerType::Gradient/SolidFill enum (Wave 1)
- [x] 13-02-PLAN.md — ShapeLayer.h get_channel shim + ConvertLayerRecursive dispatch + ScanSolidFillColor (Wave 2)
- [x] 13-03-PLAN.md — FFillLayerMapper + FSolidFillLayerMapper + registry + TC_BC7 + visual verify (Wave 3)

### Phase 14: Shape/Vector Layers
**Goal**: Photoshop drawn vector/shape layers (Rectangle/Ellipse/Pen Tool) with solid-color fill import as UImage widgets with a matching solid SlateBrush — preserving position, size, and fill color. Refines the Phase 13 ShapeLayer dispatch with a vscg (vecStrokeContentData) Type-field byte-walk to distinguish solid-fill drawn shapes from gradient-fill drawn shapes; stroke rendering and pattern fills deferred to later phases.
**Depends on**: Phase 2 (parser), Phase 3 (mapper pipeline), Phase 13 (ShapeLayer branch + FSolidFillLayerMapper template)
**Requirements**: SHAPE-01, SHAPE-02
**Success Criteria** (what must be TRUE):
  1. A solid-color rectangle shape layer in PSD produces a UImage with a solid SlateBrush of the correct color after import
  2. Position and size of the shape widget match the PSD layer bounds within 1px
**Plans**: 3 plans
Plans:
- [x] 14-01-PLAN.md — ShapeLayers.psd fixture + EPsdLayerType::Shape enum + RED FPsdParserShapeSpec stubs (Wave 1)
- [x] 14-02-PLAN.md — ScanShapeFillColor vscg byte-walker + 3-way ShapeLayer dispatch (SoCo / vscg / Gradient) (Wave 2)
- [ ] 14-03-PLAN.md — FShapeLayerMapper + registry registration + SPsdImportPreviewDialog label + visual verify (Wave 3)

### Phase 15: Group Effects
**Goal**: Layer effects (drop shadow, color overlay) applied to Photoshop group layers are reflected on the generated container widget
**Depends on**: Phase 4.1 (effects dispatch), Phase 3 (group mapping)
**Requirements**: GRPFX-01, GRPFX-02
**Success Criteria** (what must be TRUE):
  1. A group layer with a drop shadow effect produces a container widget with a matching drop shadow applied
  2. A group layer with a color overlay produces a container widget with the overlay color applied
**Plans**: TBD

### Phase 16: Rich Text / Multiple Text Runs
**Goal**: Text layers containing multiple style runs (mixed font size, weight, color within one layer) import as URichTextBlock with correct inline markup
**Depends on**: Phase 12 (text property fidelity)
**Requirements**: RICH-01, RICH-02
**Success Criteria** (what must be TRUE):
  1. A text layer with two differently-colored spans produces a URichTextBlock with both colors represented in inline markup
  2. A text layer with bold and normal weight runs produces a URichTextBlock with correct weight styling per span
**Plans**: TBD

### Phase 17: Automated Font Matching
**Goal**: Photoshop font family names are automatically resolved to UE font assets via a configurable mapping table, with fuzzy fallback for common name variants
**Depends on**: Phase 12 (text pipeline)
**Requirements**: FONT-01, FONT-02
**Success Criteria** (what must be TRUE):
  1. A font named "Roboto-Bold" in PSD resolves to the correct UE font asset without any manual configuration if the asset exists in the project
  2. A font with no exact match logs a warning and falls back to the project default font rather than silently producing no font
**Plans**: TBD

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 11. Import Dialog Hidden-Layer Filtering | 0/2 | Skipped | 2026-04-21 |
| 12. Text Property Fidelity | 2/2 | Complete   | 2026-04-17 |
| 13. Gradient Layers | 3/3 | Complete | 2026-04-21 |
| 14. Shape/Vector Layers | 2/3 | In Progress|  |
| 15. Group Effects | 0/? | Not started | - |
| 16. Rich Text / Multiple Text Runs | 0/? | Not started | - |
| 17. Automated Font Matching | 0/? | Not started | - |

## v1.3+ Backlog

Candidate work (from deferred registry):
- frameFXMulti VlLs stroke format (newer Photoshop)
- Image-layer stroke rendering (D-12 — data already populated)
- lrFX channel-order visual confirm for color overlay/shadow (needs host .uproject)
- GitHub Actions CI (D-04)
- CHANGELOG.md (D-06)
- Phase 2 VERIFICATION.md backfill (optional)
- Runtime module (RUN-01)
- Figma export JSON input (FMT-01)
- URichTextBlock mixed-style runs (TEXT-V2-01)
- Custom PSD thumbnail in Content Browser (MKT-01)
- Toolbar button "Re-import from PSD" in Widget Blueprint Editor (MKT-02)
- Fab/Epic Marketplace submission packaging (MKT-03)
- Configurable non-canvas slot properties (alignment, padding, orientation) — deferred from v1.0.1
- Overlay/Canvas/Nested spec coverage — test-coverage gap only (implementation is complete)
- Phase 14 deferred: Stroke rendering (`vstk` -> UMG border/outline effect); Pattern fill layers (`PtFl`); Vector path export (SVG / UMG custom shape widget)
