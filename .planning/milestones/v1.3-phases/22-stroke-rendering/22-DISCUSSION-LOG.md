# Phase 22: Stroke Rendering - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-29
**Phase:** 22-stroke-rendering
**Areas discussed:** STROKE-03 architecture, DrawType::Border rule, Double-emit guard, Stroke fixture

---

## STROKE-03 Architecture

| Option | Description | Selected |
|--------|-------------|----------|
| Generator FX-block | FWidgetBlueprintGenerator gets new FX-05 block; FShapeLayerMapper unchanged; consistent with FX-04 drop shadow pattern | ✓ |
| Mapper emits (literal) | FShapeLayerMapper::Map() creates stroke sibling via side-channel out-param or return wrapper | |

**User's choice:** Generator FX-block
**Notes:** Consistent with existing sibling-creation architecture. "FShapeLayerMapper reads bHasVectorStroke" in REQUIREMENTS interpreted as: the generator reads the field (not the mapper), same as how generator reads bHasDropShadow from FX-04. No mapper API changes.

---

## DrawType::Border Rule

| Option | Description | Selected |
|--------|-------------|----------|
| Always sibling UImage | One consistent pattern for both STROKE-01 and STROKE-03; no Border logic | ✓ |
| Border when no texture | Conditional: use DrawType::Border on main widget when null brush; adds complexity | |
| Border always for shapes | Shape-specific approach; diverges from image-stroke pattern | |

**User's choice:** Always sibling UImage
**Notes:** Shape layers produce UImages with ColorOverlayColor tint (no texture file); the stroke geometry follows the same sized+offset sibling UImage pattern as STROKE-01 (lfx2).

---

## Double-Emit Guard

| Option | Description | Selected |
|--------|-------------|----------|
| vstk wins — clear at parse time | ScanVstkStroke() clears bHasStroke when setting bHasVectorStroke on Shape layers | ✓ |
| Generator checks at emit time | FX-05 block skips lfx2 path if bHasVectorStroke is set | |
| No explicit guard | Layers with both sources are rare; document as limitation | |

**User's choice:** vstk wins — clear at parse time
**Notes:** Mirrors D-13 text outline guard (bHasStroke cleared after routing). Guard scoped to Shape layers only — Image layers with lfx2 stroke are unaffected.

---

## Stroke Fixture

| Option | Description | Selected |
|--------|-------------|----------|
| New Stroke.psd fixture | Positive STROKE-01/03 assertions; parallel to Phase 21 RichTextCJK.psd | |
| ButtonStyles.psd regression only | No new fixture; ButtonStyles covers non-regression (success criterion 4) | ✓ |
| User provides fixture before execution | Same as option 1, fixture provided pre-execution | |

**User's choice:** ButtonStyles.psd regression only
**Notes:** Positive stroke assertions deferred until a fixture PSD is available. Research should determine if spec-level stubs can exercise the generator FX-05 block without a real PSD.

---

## Claude's Discretion

- Whether STROKE-01 and STROKE-03 share a helper or are separate blocks
- Whether to extract a `EmitStrokeSibling()` helper used by both paths
- Plan count and wave structure

## Deferred Ideas

- Stroke.psd positive fixture — deferred (user to provide when available)
- DrawType::Border approach — intentionally rejected per D-02
- frameFXMulti VlLs stroke emission — v1.3+ backlog
