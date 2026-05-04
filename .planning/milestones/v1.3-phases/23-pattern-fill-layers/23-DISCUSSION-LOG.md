# Phase 23: Pattern Fill Layers - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-05-04
**Phase:** 23-pattern-fill-layers
**Areas discussed:** Fixture strategy, Empty pixel fallback

---

## Fixture Strategy

| Option | Description | Selected |
|--------|-------------|----------|
| I'll provide PatternFill.psd | Drop a real PSD with pattern fill layer in Tests/Fixtures/ | |
| Spec-only stubs | Synthetic FPsdLayer in spec; asserts type detection + mapper output without PSD | ✓ |
| Deferred fixture, type-detection spec only | Only assert PTFL-01; mapper spec deferred | |

**User's choice:** Spec-only stubs
**Notes:** Mirrors Phase 22 D-04 (ButtonStyles.psd regression only, positive stroke fixture deferred). Positive fixture deferred until user has a real pattern fill PSD.

---

## Empty Pixel Fallback

| Option | Description | Selected |
|--------|-------------|----------|
| Mapper returns nullptr + warn | FPatternFillLayerMapper checks RGBAPixels.Num() == 0, logs Warning, returns nullptr | ✓ |
| Parser sets bHasComplexEffects=true | Parser sets flag; generator FX-05 flatten re-routes — but FX-05 also requires RGBAPixels > 0, so still does nothing when empty | |

**User's choice:** Mapper returns nullptr + warn
**Notes:** Consistent with FFillLayerMapper nullptr-on-failure path. No parser changes required.

---

## Claude's Discretion

- `PatternFill` enum value placement in `EPsdLayerType` (adjacent to `Gradient`/`SolidFill`)
- Spec design: mock tagged block vs real `adjPattern` key
- Plan count/split (single plan likely sufficient)

## Deferred Ideas

- PatternFill.psd fixture — deferred; positive end-to-end test deferred until PSD available
- Material-based tiling — out of v1.3 scope per REQUIREMENTS.md
