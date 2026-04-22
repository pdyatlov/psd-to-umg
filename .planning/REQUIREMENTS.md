# PSD2UMG — v1.1 / v1.2 Requirements

**Milestone:** v1.1 + v1.2 (active)
**Parent:** v1.0.1 (archived 2026-04-17)
**Scope:** Import dialog fidelity (hidden layers, checkbox filtering) + text property correctness + layer fidelity expansion (gradients, shapes, group effects, rich text).

## Active Requirements

### Import Dialog — Hidden Layers (HIDDEN-*)

- [ ] **HIDDEN-01**: PSD layers with `bVisible = false` are displayed in the import dialog with their checkbox **unchecked** by default.
- [x] **HIDDEN-02**: The import dialog visually distinguishes hidden layers from visible ones (e.g., dimmed row, eye-closed icon, or `[hidden]` label).

### Import Dialog — Checkbox Filtering (FILTER-*)

- [ ] **FILTER-01**: Layers whose checkbox is unchecked in the import dialog are **not added** to the generated Widget Blueprint.
- [ ] **FILTER-02**: Children of an unchecked group layer are also excluded from the WBP (unchecking a group implicitly excludes its subtree).

### Text Fidelity — Font Size (TEXT-F-*)

- [x] **TEXT-F-01**: Font point size from PhotoshopAPI is converted to UMG font size using the correct formula so `FSlateFontInfo.Size` matches PSD design intent (24.45 pt PSD → correct UMG value, not inflated to 30).

### Text Fidelity — Alignment (TEXT-F-*)

- [x] **TEXT-F-02**: PSD paragraph justification (left/center/right) maps to `ETextJustify::Left/Center/Right` on the generated `UTextBlock` and survives compile+save.

### Text Fidelity — Base Color (TEXT-F-*)

- [x] **TEXT-F-03**: PSD text fill color is imported as the correct `FSlateColor` on `UTextBlock::ColorAndOpacity` — gray in PSD imports as gray (not red); channel order must be verified.

### Gradient Layers (GRAD-*)

- [x] **GRAD-01**: A linear gradient fill layer in PSD produces a valid UMG widget (UImage with pre-rendered texture) after import.
- [x] **GRAD-02**: Gradient color stops and direction are preserved with no visible banding at standard resolutions.

### Shape/Vector Layers (SHAPE-*)

- [x] **SHAPE-01**: A solid-color rectangle shape layer in PSD produces a UImage with a solid SlateBrush of the correct color after import.
- [x] **SHAPE-02**: Position and size of the shape widget match the PSD layer bounds within 1px.

### Group Effects (GRPFX-*)

- [x] **GRPFX-01**: A group layer with a drop shadow effect produces a container widget with a matching drop shadow applied (`_Shadow` UImage sibling at correct offset, ZOrder = main - 1).
- [x] **GRPFX-02**: A group layer with a color overlay produces a container widget with the overlay color applied (`_ColorOverlay` UImage as last child with fill anchors).

### Rich Text — Multiple Style Runs (RICH-*)

- [x] **RICH-01**: A text layer with two differently-colored spans produces a `URichTextBlock` with both colors represented in inline markup.
- [x] **RICH-02**: A text layer with bold and normal weight runs produces a `URichTextBlock` with correct weight styling per span.

### Typography — Font Matching (FONT-*)

- [x] **FONT-01**: A Photoshop PostScript font name (e.g. "Roboto-Bold") is resolved automatically via AssetRegistry scan over `/Game/` and `/Engine/EngineFonts/` to the correct UFont asset when no explicit `FontMap` entry exists and the asset's base name matches case-insensitively (after `ParseSuffix` strips style suffix). New enum value `EFontResolutionSource::AutoDiscovered` reports the resolution source.
- [x] **FONT-02**: When a PSD font name cannot be resolved via FontMap or AutoDiscovered, `FFontResolver::Resolve` logs a warning (`LogPSD2UMG Warning: Font '%s' not found in FontMap; using DefaultFont`) and falls back to the project's configured `DefaultFont` (or engine default if DefaultFont is unset). Prevents silent "no font" output.

## Traceability

| REQ-ID | Assigned Phase | Status |
|---|---|---|
| HIDDEN-01 | Phase 11 / Phase 18 | Pending (Phase 18 gap closure) |
| HIDDEN-02 | Phase 11 / Phase 16.1 | Complete (verified 2026-04-22 — eye icon from Phase 11, row dimming from Phase 16.1) |
| FILTER-01 | Phase 11 / Phase 18 | Pending (Phase 18 gap closure) |
| FILTER-02 | Phase 11 / Phase 18 | Pending (Phase 18 gap closure) |
| TEXT-F-01 | Phase 12 | Complete |
| TEXT-F-02 | Phase 12 | Complete |
| TEXT-F-03 | Phase 12 | Complete |
| GRAD-01 | Phase 13 / Phase 16.1 | Complete (verified 2026-04-22 — Phase 16.1 FLayerTagParser fix closes the untagged-layer gap) |
| GRAD-02 | Phase 13 | Complete (verified 2026-04-22) |
| SHAPE-01 | Phase 14 / Phase 16.1 | Complete (verified 2026-04-22 — Phase 16.1 FLayerTagParser fix closes the untagged-layer gap) |
| SHAPE-02 | Phase 14 | Complete (verified 2026-04-22) |
| GRPFX-01 | Phase 15 | Complete (verified 2026-04-22) |
| GRPFX-02 | Phase 15 | Complete (verified 2026-04-22) |
| RICH-01 | Phase 16 | Complete (verified 2026-04-22) |
| RICH-02 | Phase 16 | Complete (verified 2026-04-22) |
| FONT-01 | Phase 17 | Complete (verified 2026-04-22 — AssetRegistry scan cache lands in 17-02 Task 1; cache lifecycle hook in 17-02 Task 2) |
| FONT-02 | Phase 17 | Complete (already implemented in FontResolver.cpp DefaultFont fallback; D-06 marks it closed) |

---

*Last updated: 2026-04-22*
