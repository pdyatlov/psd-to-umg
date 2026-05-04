# Milestones: PSD2UMG

## v1.3 Advanced Effects (Shipped: 2026-05-04)

**Phases completed:** 3 phases, 8 plans, 14 tasks

**Key accomplishments:**

- Strip trailing `\0` sentinel at both Content scalar and FullUtf8 call sites in PsdParser.cpp; add `FPsdParserCJKSpec` automation spec with 5 It() blocks for empirical validation.
- `ConvertLfx2Color` helper added to `Parser::Internal` namespace; sofi and dsdw callsites routed through it — HSB lrFX colors now convert correctly via `FLinearColor::HSVToLinearRGB`, RGB path byte-identical, CMYK/unknown warn+identity.
- `ParseFrFXObjcItem` lambda extracted from the existing Objc branch body and a new `FrFX/VlLs` branch added to `ParseFrFXDescriptor`'s outer walk — Photoshop CC 2014+ stroke effects stored as VlLs lists now extract `bEnabled`, `SizePx`, and `Color` instead of being silently skipped.
- LFXC-02 visual UAT deferred to pre-v1.3 session; code correctness confirmed by implementation review; UAT log created with deferral rationale.
- ScanVstkStroke binary descriptor walker parses vstk tagged block into separate bHasVectorStroke/VectorStrokeSize/VectorStrokeColor fields on FPsdLayerEffects, with D-03 double-emit guard clearing bHasStroke on Shape layers when vstk wins
- One-liner:
- EPsdLayerType::PatternFill enum value + adjPattern (PtFl) tagged-block detection in ConvertLayerRecursive, routing pattern fill layers via Adj-cast ExtractImagePixels (composited RGBAPixels, CP-04: no Clr key)
- FPatternFillLayerMapper wired at priority 101 with 8-case CanMap spec and D-04 nullptr fallback; pattern fill layers now route to UImage via composited RGBAPixels

---

## v1.2 Layer Fidelity Expansion (Shipped: 2026-04-28)

**Phases completed:** 10 phases (13-20, including 16.1, 17.1, 17.2), 27 plans

**Key accomplishments:**

- Gradient fill layers imported as pre-rendered TC_BC7 UImage widgets (GRAD-01/02)
- Solid-color shape layers imported as UImage with solid SlateBrush (SHAPE-01/02)
- Group drop shadow + color overlay propagated to container widget siblings (GRPFX-01/02)
- Mixed-style text → URichTextBlock with persistent UDataTable companion asset (RICH-01/02)
- PostScript font names auto-resolved via AssetRegistry; reimport cache invalidated on all exit paths (FONT-01/02)
- @button @variants state wiring confirmed; FVariantsSuffixMapper Canvas over-reject fixed (BTN-STATE-01/02)
- @state:* text color diffs → UWidgetAnimation + K2 OnHovered/OnPressed delegates wired (BTN-ANIM-01/02/03)
- Color Overlay wins for text, All Caps → ToUpper, VBox/HBox child order confirmed correct (TXT-FX-01, TXT-CAPS-01, LAYOUT-ORDER-01)
- Mapper priority collision eliminated (FFillLayerMapper/FSolidFillLayerMapper/FShapeLayerMapper → 101)

---

## v1.1 Import Fidelity Fixes (Shipped: 2026-04-17)

**Phases completed:** 2 phases (11-12), 4 plans

**Key accomplishments:**

- Hidden PSD layers now appear unchecked by default in import dialog; eye-closed icon column added
- Checkbox exclusion wiring: unchecked layers/subtrees excluded from generated WBP
- Font size formula validated (PhotoshopAPI 4/3 scaling confirmed; * 0.75f is correct)
- Text paragraph alignment imported correctly (paragraph_normal_justification fallback added)
- Text fill color fix: Color Overlay wins over base fill in RouteTextEffects (TEXT-F-03)

---

## v1.0.1 Panel Child Attachment Hotfix (Shipped: 2026-04-17)

**Phases completed:** 1 phases, 3 plans, 0 tasks

**Key accomplishments:**

- 1. [User Direction] Overlay/Canvas/Nested cases removed from spec

---

| Version | Status | Shipped | Phases | Plans | Archive |
|---------|--------|---------|--------|-------|---------|
| v1.0 | SHIPPED | 2026-04-15 | 10 (01-09 + 04.1) | 33 | [ROADMAP](milestones/v1.0-ROADMAP.md) • [AUDIT](milestones/v1.0-MILESTONE-AUDIT.md) • [REQUIREMENTS](milestones/v1.0-REQUIREMENTS.md) |
| v1.0.1 | SHIPPED | 2026-04-17 | 1 (Phase 10) | 3 | [ROADMAP](milestones/v1.0.1-ROADMAP.md) • [REQUIREMENTS](milestones/v1.0.1-REQUIREMENTS.md) |
| v1.1 | SHIPPED | 2026-04-17 | 2 (Phases 11-12) | 4 | [ROADMAP](milestones/v1.1-ROADMAP.md) • [REQUIREMENTS](milestones/v1.1-REQUIREMENTS.md) |
| v1.2 | SHIPPED | 2026-04-28 | 10 (Phases 13-20) | 27 | [ROADMAP](milestones/v1.2-ROADMAP.md) • [REQUIREMENTS](milestones/v1.2-REQUIREMENTS.md) |
