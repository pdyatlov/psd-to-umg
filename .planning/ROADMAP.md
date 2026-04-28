# Roadmap: PSD2UMG

## Overview

PSD2UMG transforms Photoshop files into fully functional UMG Widget Blueprints inside UE 5.7. The roadmap moves from porting the existing UE4 codebase, through building a native C++ PSD parser, to implementing the full layer-to-widget mapping pipeline, then layering on text, effects, layout, editor UI, and CommonUI support, culminating in testing and release. Each phase delivers a verifiable capability on top of the previous one.

## Shipped Milestones

- [x] **v1.0** (shipped 2026-04-15) — [archive](milestones/v1.0-ROADMAP.md) • [audit](milestones/v1.0-MILESTONE-AUDIT.md)
- [x] **v1.0.1** (shipped 2026-04-17) — [archive](milestones/v1.0.1-ROADMAP.md)
- [x] **v1.1** (shipped 2026-04-17) — [archive](milestones/v1.1-ROADMAP.md)
- [x] **v1.2** (shipped 2026-04-28) — [archive](milestones/v1.2-ROADMAP.md) • [audit](milestones/v1.1-MILESTONE-AUDIT.md)

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

<details>
<summary>✅ v1.1 Import Fidelity Fixes (Phases 11-12) — SHIPPED 2026-04-17</summary>

- [x] Phase 11: Import Dialog Hidden-Layer Filtering (2/2 plans) — completed 2026-04-28
- [x] Phase 12: Text Property Fidelity (2/2 plans) — completed 2026-04-17

See [milestones/v1.1-ROADMAP.md](milestones/v1.1-ROADMAP.md) for full phase details.

</details>

<details>
<summary>✅ v1.2 Layer Fidelity Expansion (Phases 13-20) — SHIPPED 2026-04-28</summary>

- [x] Phase 13: Gradient Layers (3/3 plans) — completed 2026-04-21
- [x] Phase 14: Shape/Vector Layers (3/3 plans) — completed 2026-04-22
- [x] Phase 15: Group Effects (1/1 plan) — completed 2026-04-22
- [x] Phase 16: Rich Text / Multiple Text Runs (3/3 plans) — completed 2026-04-22
- [x] Phase 16.1: LayerTag Fix + Requirements Traceability (1/1 plan) — completed 2026-04-22
- [x] Phase 17: Automated Font Matching (2/2 plans) — completed 2026-04-22
- [x] Phase 17.1: Button+Variants State Wiring Validation (2/2 plans) — completed 2026-04-28
- [x] Phase 17.2: Button State Text Animation (4/4 plans) — completed 2026-04-27
- [x] Phase 18: Hidden-Layer Filtering + 17.1 Close-out (2/2 plans) — completed 2026-04-28
- [x] Phase 19: Text + Layout Correctness Fixes (3/3 plans) — completed 2026-04-27
- [x] Phase 20: Integration Stability Fixes (2/2 plans) — completed 2026-04-27

See [milestones/v1.2-ROADMAP.md](milestones/v1.2-ROADMAP.md) for full phase details.

</details>

### v1.3 Advanced Effects (Phases 21-23)

- [ ] **Phase 21: Parser Correctness Fixes** — Fix Utf8ToFString null-sentinel, lrFX ColorSpace branch, VlLs format branch, and human UAT sign-off
- [ ] **Phase 22: Stroke Rendering** — Parse vstk vector stroke data and emit stroke sibling UImage for image and shape layers
- [ ] **Phase 23: Pattern Fill Layers** — Detect and map adjPattern tagged blocks to UImage via composited RGBAPixels

## Phase Details

### Phase 21: Parser Correctness Fixes
**Goal**: All parser-level defects blocking correct effects and text output are resolved and empirically confirmed
**Depends on**: Phase 20
**Requirements**: RTXT-01, LFXC-01, LFXC-02, FXFMT-01
**Success Criteria** (what must be TRUE):
  1. A PSD layer containing CJK or emoji text imports without truncation or garbage characters (null-sentinel stripped before Utf8ToFString, explicit size passed — CP-05)
  2. A layer with ColorSpace=1 (HSB) lrFX color overlay or drop shadow renders the correct hue in UMG — not a channel-scrambled artifact (HSVToLinearRGB conversion applied)
  3. A PSD saved by Photoshop CC 2014+ with effects stored under `VlLs` (not bare `Objc`) imports those effects rather than silently discarding them (VlLs branch in ParseFrFXDescriptor — CP-03)
  4. Human UAT on a real UE 5.7 host project confirms color overlay and drop shadow visual output is correct for RGB ColorSpace=0 layers (LFXC-02 empirical close-out)
**Plans**: 4 plans
- [x] 21-01-PLAN.md — RTXT-01 NUL sentinel strip + CJK fixture spec (Wave 1) — completed 2026-04-28
- [ ] 21-02-PLAN.md — LFXC-01 ConvertLfx2Color helper + sofi/dsdw ColorSpace dispatch (Wave 2)
- [ ] 21-03-PLAN.md — FXFMT-01 ParseFrFXObjcItem extraction + VlLs branch (Wave 3)
- [ ] 21-04-PLAN.md — LFXC-02 human UAT sign-off on real UE 5.7 host (Wave 4, checkpoint)

### Phase 22: Stroke Rendering
**Goal**: Image and shape layers with stroke effects defined in PSD produce a visible stroke approximation in the generated widget
**Depends on**: Phase 21
**Requirements**: STROKE-01, STROKE-02, STROKE-03
**Success Criteria** (what must be TRUE):
  1. A shape layer with a `vstk` block parses stroke width and color into `bHasVectorStroke`, `VectorStrokeSize`, `VectorStrokeColor` on `FPsdLayerEffects` — `bHasStroke` (lfx2) is not overwritten (CP-01: offset=0; CP-02: separate field)
  2. An image layer with `bHasStroke` set (lfx2 path — STROKE-01) emits a sibling UImage sized `+2×StrokePx`, offset `-StrokePx`, tinted `StrokeColor`, at `ZOrder = main - 1`
  3. A shape layer with `bHasVectorStroke` set (vstk path — STROKE-03) emits equivalent stroke geometry; the two stroke sources (lfx2 and vstk) do not double-emit on the same layer
  4. Importing the existing ButtonStyles.psd fixture produces no regression on non-stroke layers
**Plans**: TBD

### Phase 23: Pattern Fill Layers
**Goal**: Pattern fill adjustment layers are imported as UImage widgets backed by the layer's composited pixel data rather than silently skipped
**Depends on**: Phase 22
**Requirements**: PTFL-01, PTFL-02
**Success Criteria** (what must be TRUE):
  1. A PSD containing an `adjPattern` tagged block is assigned `EPsdLayerType::PatternFill` during `ConvertLayerRecursive` — it does not fall to `Unknown` and is not skipped
  2. `FPatternFillLayerMapper` (priority 101) produces a UImage populated from `RGBAPixels`; the widget is positioned and sized correctly relative to its parent canvas
  3. When `RGBAPixels` is empty, the mapper falls back to the `bFlattenComplexEffects` path and emits a `UE_LOG Warning` — import completes rather than crashing (CP-04: no `Clr ` key; dedicated scan path required)
**Plans**: TBD

## Progress Table

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 21. Parser Correctness Fixes | 0/4 | Planned | - |
| 22. Stroke Rendering | 0/? | Not started | - |
| 23. Pattern Fill Layers | 0/? | Not started | - |

## v1.3+ Backlog

Candidate work (from deferred registry):
- frameFXMulti VlLs stroke rendering — FXFMT-01 (Phase 21) unlocks parsing; stroke emission for VlLs-origin data is post-v1.3
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
- Vector path export (SVG / UMG custom shape widget) — deferred from Phase 14
- Inside/outside/center stroke precision (sibling-image approximation in v1.3; full precision post-v1.3)
- Material-based tiling for pattern fills (composited PNG correct for v1.3; tiling deferred)
- CMYK/Lab lrFX full conversion (warn path sufficient for v1.3; full conversion deferred)
