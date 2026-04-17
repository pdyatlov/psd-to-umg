# Phase 6: Advanced Layout - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-10
**Phase:** 06-advanced-layout
**Areas discussed:** 9-Slice parsing & margins, Smart Object import, Anchor heuristics improvement, Variants & switcher

---

## 9-Slice Parsing & Margins

### Default margins when no syntax provided

| Option | Description | Selected |
|--------|-------------|----------|
| Uniform default (e.g. 16px) | Apply 16,16,16,16 so image looks correct without explicit margins | ✓ |
| Zero margins (require explicit) | Set Box draw mode but 0 margins — designer must always specify | |
| You decide | Claude picks based on UE conventions | |

**User's choice:** Uniform default (e.g. 16px)
**Notes:** None

### Suffix stripping from widget name

| Option | Description | Selected |
|--------|-------------|----------|
| Strip suffix + margins | Widget named "MyButton" — cleaner hierarchy | ✓ |
| Keep full name | Widget named "MyButton_9s" — suffix visible in tree | |
| You decide | Claude picks based on existing suffix handling | |

**User's choice:** Strip suffix + margins
**Notes:** None

### Scope of 9-slice support

| Option | Description | Selected |
|--------|-------------|----------|
| Any image layer (Recommended) | Any layer with _9s/_9slice gets Box draw mode | ✓ |
| Only standalone images | Button state brushes excluded | |

**User's choice:** Any image layer (Recommended)
**Notes:** None

---

## Smart Object Import

### Recursion depth

| Option | Description | Selected |
|--------|-------------|----------|
| One level deep (Recommended) | Import one level, nested SOs rasterize | |
| Unlimited recursion | Recurse until no more SOs — risk of circular refs | |
| Configurable depth | Add MaxSmartObjectDepth setting | ✓ |

**User's choice:** Configurable depth with default value of 2
**Notes:** User specified "Add a setting to let the user control how deep to go + a default value of 2"

### Linked Smart Object handling

| Option | Description | Selected |
|--------|-------------|----------|
| Embedded only, rasterize linked | Only embedded SOs imported as WBPs | |
| Try both, fallback to rasterize | Attempt linked file resolution, rasterize if not found | ✓ |
| You decide | Claude picks based on PhotoshopAPI capabilities | |

**User's choice:** Try both, fallback to rasterize
**Notes:** None

### Child Widget Blueprint reference

| Option | Description | Selected |
|--------|-------------|----------|
| UUserWidget reference (Recommended) | Separate WBP asset, referenced via UUserWidget | ✓ |
| Inline expansion | Flatten SO layers into parent widget tree | |

**User's choice:** UUserWidget reference (Recommended)
**Notes:** None

---

## Anchor Heuristics Improvement

### Auto-wrap vs log suggestion

| Option | Description | Selected |
|--------|-------------|----------|
| Auto-wrap (Recommended) | Auto-create HBox/VBox when row/column detected | ✓ |
| Log suggestion only | Keep CanvasPanel, log suggestion for designer | |
| Configurable | Add bAutoDetectBoxLayout setting | |

**User's choice:** Auto-wrap (Recommended)
**Notes:** None

### Alignment tolerance

| Option | Description | Selected |
|--------|-------------|----------|
| Strict (2-4px) | Nearly perfect alignment required | |
| Relaxed (8-12px) | Forgiving of misalignment | |
| You decide | Claude picks reasonable threshold | |

**User's choice:** Other — "Not exactly strict and not exactly relaxed, maybe 4-8px"
**Notes:** User specified moderate 4-8px range

### Spacing detection

| Option | Description | Selected |
|--------|-------------|----------|
| Yes, detect and apply padding | Detect equal spacing, set HBox/VBox padding | |
| No, just detect alignment | Only row/column detection, spacing via slot offsets | |
| You decide | Claude picks based on complexity | ✓ |

**User's choice:** You decide
**Notes:** None

---

## Variants & Switcher

### Relationship to existing Switcher_ prefix

| Option | Description | Selected |
|--------|-------------|----------|
| Separate behavior (Recommended) | Both _variants suffix and Switcher_ prefix coexist | ✓ |
| Merge into one | Remove Switcher_ prefix, only use _variants suffix | |
| You decide | Claude picks based on conventions | |

**User's choice:** Separate behavior (Recommended)
**Notes:** None

### Slot ordering

| Option | Description | Selected |
|--------|-------------|----------|
| PSD layer order (top to bottom) | First child = slot 0, matches layer panel | ✓ |
| Alphabetical by name | Sorted by name for explicit ordering | |
| You decide | Simplest reliable approach | |

**User's choice:** PSD layer order (top to bottom)
**Notes:** None

### Suffix stripping

| Option | Description | Selected |
|--------|-------------|----------|
| Strip (consistent with _9s) | "States_variants" → widget "States" | ✓ |
| Keep | "States_variants" stays in widget tree | |

**User's choice:** Strip (consistent with _9s)
**Notes:** None

---

## Claude's Discretion

- 9-slice margin parsing implementation (regex vs manual)
- Smart Object child WBP naming convention
- Exact alignment tolerance value within 4-8px
- Whether to implement spacing/padding auto-detection for HBox/VBox
- _variants mapper implementation (new class vs extending existing)
- Smart Object pixel extraction source (composited preview vs linked file data)

## Deferred Ideas

None — discussion stayed within phase scope.
