# Phase 3: Layer Mapping & Widget Blueprint Generation — Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-09
**Phase:** 03-layer-mapping-widget-blueprint-generation
**Areas discussed:** Mapper architecture, Text properties in Phase 3, Anchor heuristics, Partial import behavior

---

## Mapper Architecture

| Option | Description | Selected |
|--------|-------------|----------|
| Internal-only dispatch | Priority-ordered internal mappers, no public Register() API | ✓ |
| Externally extensible | FLayerMappingRegistry::Register() public for third-party mappers | |

**User's choice:** Internal-only dispatch
**Notes:** Simpler for Phase 3. External extensibility deferred until actually needed.

---

## Text Properties in Phase 3

| Option | Description | Selected |
|--------|-------------|----------|
| Basic: content + DPI size + color | Content + SizePx × 0.75 + Color on UTextBlock | ✓ |
| Minimal: content + raw size only | Just string and raw SizePx, no conversion | |

**User's choice:** Basic: content + DPI size + color
**Notes:** Avoids obviously wrong font sizes in the first end-to-end test. Font mapping, outline, shadow stay in Phase 4.

---

## Anchor Heuristics

| Option | Description | Selected |
|--------|-------------|----------|
| Quadrant-based | 3×3 grid, layer center → anchor preset, ≥80% width/height → stretch | ✓ |
| Edge-proximity | Anchor to nearest edge(s) | |
| Claude's discretion | Implementation picks best approach | |

**User's choice:** Quadrant-based
**Notes:** Simple, predictable, designer-legible. Stretch threshold at 80% canvas dimension.

---

## Partial Import Behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Skip + warn | Log warning per failed layer, continue building blueprint | ✓ |
| Abort | Any failure cancels entire import | |

**User's choice:** Skip + warn
**Notes:** Usable result on complex PSDs even when individual layers fail.

---

## Claude's Discretion

- Exact UE API sequence for Widget Blueprint creation
- Internal FLayerMappingRegistry structure
- FAnchorCalculator as class vs free-function namespace
- UCanvasPanelSlot coordinate math details
- ZOrder assignment strategy

## Deferred Ideas

- External mapper registration (third-party plugin support)
- Abort-on-failure import behavior
