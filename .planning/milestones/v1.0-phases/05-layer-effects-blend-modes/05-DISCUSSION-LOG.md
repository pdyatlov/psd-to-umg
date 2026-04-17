# Phase 5: Layer Effects & Blend Modes - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-10
**Phase:** 05-layer-effects-blend-modes
**Areas discussed:** Visibility behavior, Drop Shadow approach, Flatten fallback scope, Color Overlay targets

---

## Visibility Behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Create as Collapsed (Recommended) | Create widget with Visibility=Collapsed. Designers can toggle at runtime. | ✓ |
| Skip entirely (current) | Don't create widgets for hidden layers. Simpler but loses hidden layers. | |
| You decide | Claude picks based on UMG conventions | |

**User's choice:** Create as Collapsed
**Notes:** None

---

## Drop Shadow Approach

| Option | Description | Selected |
|--------|-------------|----------|
| Offset dark image duplicate (Recommended) | Semi-transparent copy behind original, offset by shadow distance. Uses shadow color/opacity from PSD. | ✓ |
| Solid color rectangle offset | Solid-color rectangle behind widget. Cheaper but less accurate for non-rectangular shapes. | |
| Rasterize shadow from PSD | Extract layer with effects composited. Most accurate but loses editability. | |
| You decide | Claude picks based on PhotoshopAPI capabilities | |

**User's choice:** Offset dark image duplicate
**Notes:** None

---

## Flatten Fallback Scope

### Which effects trigger flatten

| Option | Description | Selected |
|--------|-------------|----------|
| Inner shadow, gradient overlay, pattern overlay, bevel (Recommended) | Only effects with no UMG approximation. Opacity, color overlay, drop shadow are approximated. | ✓ |
| Everything except opacity and visibility | More aggressive flatten. Simpler code but loses editability. | |
| You decide | Claude determines boundary | |

**User's choice:** Inner shadow, gradient overlay, pattern overlay, bevel

### Rasterized image source

| Option | Description | Selected |
|--------|-------------|----------|
| PhotoshopAPI composited pixels (Recommended) | Extract layer with all effects pre-composited. Most accurate. | ✓ |
| Re-render via pixel data + manual effect application | Composite effects ourselves. Complex and fragile. | |
| You decide | Claude picks based on PhotoshopAPI capabilities | |

**User's choice:** PhotoshopAPI composited pixels
**Notes:** None

---

## Color Overlay Targets

| Option | Description | Selected |
|--------|-------------|----------|
| Image layers only (Recommended) | Apply as FSlateBrush TintColor on UImage. Text has its own color; groups don't have a brush. | ✓ |
| Images and text | UImage gets tint; UTextBlock gets SetColorAndOpacity override. | |
| All widget types | Apply tint everywhere possible. Complex edge cases. | |
| You decide | Claude picks per widget type | |

**User's choice:** Image layers only
**Notes:** None

---

## Claude's Discretion

- Whether FPsdLayerEffects is a nested struct or separate fields on FPsdLayer
- How to detect effects via PhotoshopAPI
- Shadow duplicate image sharing strategy
- Whether to add FEffectsMapper or handle inline in generator
- Exact default for bFlattenComplexEffects

## Deferred Ideas

- Blend mode mapping (multiply, screen, overlay) — needs materials, future phase
- Stroke/border on non-text layers — not in v1
- Per-effect flatten toggle — future enhancement
