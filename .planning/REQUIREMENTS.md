# PSD2UMG — v1.1 Requirements

**Milestone:** v1.1 (patch release)
**Parent:** v1.0.1 (archived 2026-04-17)
**Scope:** Import dialog fidelity (hidden layers, checkbox filtering) + text property correctness (font size, alignment, base color).

## Active Requirements

### Import Dialog — Hidden Layers (HIDDEN-*)

- [ ] **HIDDEN-01**: PSD layers with `bVisible = false` are displayed in the import dialog with their checkbox **unchecked** by default.
- [ ] **HIDDEN-02**: The import dialog visually distinguishes hidden layers from visible ones (e.g., dimmed row, eye-closed icon, or `[hidden]` label).

### Import Dialog — Checkbox Filtering (FILTER-*)

- [ ] **FILTER-01**: Layers whose checkbox is unchecked in the import dialog are **not added** to the generated Widget Blueprint.
- [ ] **FILTER-02**: Children of an unchecked group layer are also excluded from the WBP (unchecking a group implicitly excludes its subtree).

### Text Fidelity — Font Size (TEXT-F-*)

- [ ] **TEXT-F-01**: Font point size from PhotoshopAPI is converted to UMG font size using the correct formula so `FSlateFontInfo.Size` matches PSD design intent (24.45 pt PSD → correct UMG value, not inflated to 30).

### Text Fidelity — Alignment (TEXT-F-*)

- [ ] **TEXT-F-02**: PSD paragraph justification (left/center/right) maps to `ETextJustify::Left/Center/Right` on the generated `UTextBlock` and survives compile+save.

### Text Fidelity — Base Color (TEXT-F-*)

- [ ] **TEXT-F-03**: PSD text fill color is imported as the correct `FSlateColor` on `UTextBlock::ColorAndOpacity` — gray in PSD imports as gray (not red); channel order must be verified.

## Traceability

| REQ-ID | Assigned Phase | Status |
|---|---|---|
| HIDDEN-01 | Phase 11 | Planned |
| HIDDEN-02 | Phase 11 | Planned |
| FILTER-01 | Phase 11 | Planned |
| FILTER-02 | Phase 11 | Planned |
| TEXT-F-01 | Phase 12 | Planned |
| TEXT-F-02 | Phase 12 | Planned |
| TEXT-F-03 | Phase 12 | Planned |

---

*Last updated: 2026-04-17*
