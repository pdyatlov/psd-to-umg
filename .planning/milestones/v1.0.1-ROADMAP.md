# Roadmap: PSD2UMG

## Overview

PSD2UMG transforms Photoshop files into fully functional UMG Widget Blueprints inside UE 5.7. The roadmap moves from porting the existing UE4 codebase, through building a native C++ PSD parser, to implementing the full layer-to-widget mapping pipeline, then layering on text, effects, layout, editor UI, and CommonUI support, culminating in testing and release. Each phase delivers a verifiable capability on top of the previous one.

## Shipped Milestones

- [x] **v1.0** (shipped 2026-04-15) — [archive](milestones/v1.0-ROADMAP.md) • [audit](milestones/v1.0-MILESTONE-AUDIT.md)

## Current Milestone: v1.0.1 — Panel Child Attachment Hotfix

**Goal:** Close the v1.0 gap where non-canvas group tags (`@vbox`, `@hbox`, `@scrollbox`, `@overlay`) silently drop all children because the generator's recursion is hard-cast to `UCanvasPanel` and `PopulateCanvas` only calls `AddChildToCanvas`.

### Phase 10: Panel Child Attachment

**Goal:** Dispatch child attachment on parent panel type. Canvas keeps `AddChildToCanvas` + slot positioning. Non-canvas panels (VBox, HBox, ScrollBox, Overlay) use `UPanelWidget::AddChild` with children in PSD z-order. Add `Panels.psd` fixture + `FPanelAttachmentSpec` asserting child counts and types. Emit warning diagnostic on any attachment miss (no silent drops). Canvas behavior byte-identical to v1.0.

**Requirements:** PANEL-01, PANEL-02, PANEL-03, PANEL-04, PANEL-05, PANEL-06, PANEL-07
**Depends on:** v1.0 (Phase 3 generator, Phase 9 tag parser)
**Plans:** 3/3 plans executed
- [x] 10-01-PLAN.md — Refactor Generate path (PopulateCanvas → panel-type dispatch + diagnostic)
- [x] 10-02-PLAN.md — Refactor Update/reimport path with clear-and-rebuild for non-canvas
- [x] 10-03-PLAN.md — Panels.psd fixture (human checkpoint) + FPanelAttachmentSpec (VBox/HBox/ScrollBox; Overlay/Canvas/Nested deferred per user direction)

## v1.1+ Backlog

Phases for the next milestone will be planned here when v2 scope is defined.

**Candidate work (from v1.0 deferred registry):**
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
