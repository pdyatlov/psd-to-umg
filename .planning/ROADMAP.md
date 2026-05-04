# Roadmap: PSD2UMG

## Overview

PSD2UMG transforms Photoshop files into fully functional UMG Widget Blueprints inside UE 5.7. The roadmap moves from porting the existing UE4 codebase, through building a native C++ PSD parser, to implementing the full layer-to-widget mapping pipeline, then layering on text, effects, layout, editor UI, and CommonUI support, culminating in testing and release. Each phase delivers a verifiable capability on top of the previous one.

## Shipped Milestones

- [x] **v1.0** (shipped 2026-04-15) — [archive](milestones/v1.0-ROADMAP.md) • [audit](milestones/v1.0-MILESTONE-AUDIT.md)
- [x] **v1.0.1** (shipped 2026-04-17) — [archive](milestones/v1.0.1-ROADMAP.md)
- [x] **v1.1** (shipped 2026-04-17) — [archive](milestones/v1.1-ROADMAP.md)
- [x] **v1.2** (shipped 2026-04-28) — [archive](milestones/v1.2-ROADMAP.md) • [audit](milestones/v1.1-MILESTONE-AUDIT.md)
- [x] **v1.3** (shipped 2026-05-04) — [archive](milestones/v1.3-ROADMAP.md)

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

<details>
<summary>✅ v1.3 Advanced Effects (Phases 21-23) — SHIPPED 2026-05-04</summary>

- [x] Phase 21: Parser Correctness Fixes (4/4 plans) — completed 2026-04-29
- [x] Phase 22: Stroke Rendering (2/2 plans) — completed 2026-04-29
- [x] Phase 23: Pattern Fill Layers (2/2 plans) — completed 2026-05-04

See [milestones/v1.3-ROADMAP.md](milestones/v1.3-ROADMAP.md) for full phase details.

</details>

## v1.4+ Backlog

Candidate work for next milestone:

- LFXC-02: Visual UAT confirm — color overlay/drop shadow RGBC correctness on real UE 5.7 host (code confirmed correct; empirical confirm deferred)
- frameFXMulti VlLs stroke rendering — FXFMT-01 (Phase 21) unlocks parsing; stroke emission for VlLs-origin data is post-v1.3
- PatternFill.psd positive fixture (D-05 user-supplied)
- GitHub Actions CI (D-04)
- CHANGELOG.md (D-06)
- Runtime module (RUN-01)
- Figma export JSON input (FMT-01)
- URichTextBlock mixed-style runs (TEXT-V2-01)
- Custom PSD thumbnail in Content Browser (MKT-01)
- Toolbar button "Re-import from PSD" in Widget Blueprint Editor (MKT-02)
- Fab/Epic Marketplace submission packaging (MKT-03)
- Configurable non-canvas slot properties (alignment, padding, orientation)
- Overlay/Canvas/Nested spec coverage (test-coverage gap; implementation complete)
- Vector path export (SVG / UMG custom shape widget)
- Inside/outside/center stroke precision (full precision post-v1.3)
- Material-based tiling for pattern fills (composited PNG correct for v1.3; tiling deferred)
- CMYK/Lab lrFX full conversion (warn path sufficient for v1.3)
