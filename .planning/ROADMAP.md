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

- [ ] **Phase 11: Import Dialog Hidden-Layer Filtering** - Hidden PSD layers appear unchecked in the import dialog and unchecked layers are excluded from WBP generation
- [ ] **Phase 12: Text Property Fidelity** - Font size, alignment, and base color correctly preserved from PSD through to UTextBlock

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
**Plans**: TBD

### Phase 12: Text Property Fidelity
**Goal**: Text layers import with correct font size, paragraph alignment, and fill color — what the designer set in Photoshop is what appears in UMG
**Depends on**: Nothing (targets existing FTextLayerMapper)
**Requirements**: TEXT-F-01, TEXT-F-02, TEXT-F-03
**Success Criteria** (what must be TRUE):
  1. A text layer with 24.45 pt font in PSD produces a UTextBlock with the correct UMG font size (not inflated to 30)
  2. A center-aligned paragraph in PSD produces a UTextBlock with ETextJustify::Center after compile+save
  3. A gray text layer in PSD produces a UTextBlock with a gray ColorAndOpacity (not red or another corrupted color)
**Plans**: TBD

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 11. Import Dialog Hidden-Layer Filtering | 0/? | Not started | - |
| 12. Text Property Fidelity | 0/? | Not started | - |

## v1.2+ Backlog

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
