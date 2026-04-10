# Phase 5: Layer Effects & Blend Modes - Context

**Gathered:** 2026-04-10
**Status:** Ready for planning

<domain>
## Phase Boundary

Apply common Photoshop layer effects (opacity, visibility, color overlay, drop shadow) to generated UMG widgets. Layers with unsupported complex effects are rasterized as a single PNG fallback when `bFlattenComplexEffects` is enabled.

**In scope:**
- FX-01: Layer opacity → `SetRenderOpacity` on generated widget
- FX-02: Layer visibility → hidden layers created as Collapsed widgets (not skipped)
- FX-03: Color Overlay effect → `FSlateBrush::TintColor` on UImage widgets only
- FX-04: Drop Shadow → offset semi-transparent duplicate UImage behind the main widget
- FX-05: Flatten fallback for complex effects (inner shadow, gradient overlay, pattern overlay, bevel) → rasterize via PhotoshopAPI composited pixels as single PNG → UImage
- New setting: `bFlattenComplexEffects` on `UPSD2UMGSettings`
- Parser extension: extract layer effect data from PhotoshopAPI into new `FPsdLayer` fields

**Out of scope:**
- Blend modes (multiply, screen, overlay, etc.) — no UMG equivalent; would need materials (future phase or out of scope)
- Inner shadow, gradient overlay, pattern overlay, bevel as individual UMG approximations — these trigger flatten fallback
- Stroke/border effects on non-text layers — out of scope for v1

</domain>

<decisions>
## Implementation Decisions

### Visibility Handling (FX-02)
- **D-01:** Hidden PSD layers are **created as widgets with Visibility=Collapsed**, not skipped. This replaces the current skip behavior in `FWidgetBlueprintGenerator.cpp:37`. Designers can toggle visibility at runtime.
- **D-02:** The generator must be updated: remove the `if (!Layer.bVisible) continue;` skip and instead set `Widget->SetVisibility(ESlateVisibility::Collapsed)` after creation.

### Opacity (FX-01)
- **D-03:** Layer opacity applied via `Widget->SetRenderOpacity(Layer.Opacity)`. Applied to all widget types after creation. Only set when `Opacity < 1.0` to avoid unnecessary calls.

### Color Overlay (FX-03)
- **D-04:** Color Overlay applies to **image layers only** — set `FSlateBrush::TintColor` on UImage widgets. Text layers already have their own PSD color; groups don't have a single brush to tint.
- **D-05:** If a non-image layer has a Color Overlay effect, log a warning and ignore it (don't flatten just for color overlay on non-images).

### Drop Shadow (FX-04)
- **D-06:** Drop Shadow approximated as an **offset semi-transparent duplicate UImage** behind the original widget. The duplicate uses the shadow color/opacity from PSD, offset by the shadow distance.
- **D-07:** Shadow UImage is added as a sibling widget in the same canvas, positioned behind (lower z-order) the main widget. The shadow image is a copy of the layer's texture with tint color set to shadow color.
- **D-08:** If shadow data is not available from PhotoshopAPI, FX-04 is delivered as no-op with diagnostic warning (same pattern as TEXT-04 shadow in Phase 4).

### Flatten Fallback (FX-05)
- **D-09:** Effects that trigger flatten: **inner shadow, gradient overlay, pattern overlay, bevel/emboss**. These have no reasonable UMG approximation.
- **D-10:** Effects that are approximated (never trigger flatten): **opacity, visibility, color overlay, drop shadow**.
- **D-11:** Flatten source: **PhotoshopAPI composited pixels** — extract layer with all effects pre-applied as a single RGBA image. The layer becomes a plain UImage.
- **D-12:** Flatten only triggers when `bFlattenComplexEffects = true` in `UPSD2UMGSettings`. When false, complex effects are silently ignored (widget created normally without the effect).
- **D-13:** New setting: `UPROPERTY bFlattenComplexEffects = true` (default on) in `UPSD2UMGSettings`.

### Parser Extension
- **D-14:** New struct `FPsdLayerEffects` added to `FPsdLayer` (not `FPsdTextRun` — effects are layer-level, not text-run-level):
  ```cpp
  struct FPsdLayerEffects
  {
      bool bHasColorOverlay = false;
      FLinearColor ColorOverlayColor = FLinearColor::White;

      bool bHasDropShadow = false;
      FLinearColor DropShadowColor = FLinearColor(0, 0, 0, 0.5f);
      FVector2D DropShadowOffset = FVector2D::ZeroVector;

      bool bHasComplexEffects = false; // inner shadow, gradient, pattern, bevel
  };
  ```
- **D-15:** Struct shape is advisory — planner/researcher may adjust based on what PhotoshopAPI actually exposes. The `bHasComplexEffects` flag is the key decision point for flatten vs approximate.
- **D-16:** ARGB channel order must be verified for effect colors (same quirk as Phase 2 fill color discovery).

### Claude's Discretion
- Whether `FPsdLayerEffects` is a nested struct or separate fields on `FPsdLayer`
- How to detect which effects are present via PhotoshopAPI (descriptor inspection vs specific API calls)
- Whether the shadow duplicate image shares the original texture or creates a new one
- Whether to add an `FEffectsMapper` or handle effects inline in the generator
- Exact default value for `bFlattenComplexEffects`

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Project
- `CLAUDE.md` — UE 5.7.4 / C++20 / no Python / Editor-only / Win64 only
- `.planning/PROJECT.md` — Core value, requirements, key decisions
- `.planning/REQUIREMENTS.md` — FX-01..FX-05 acceptance criteria
- `.planning/ROADMAP.md` — Phase 5 goal and success criteria

### Parser Foundation (Phase 2)
- `Source/PSD2UMG/Public/Parser/PsdTypes.h` — `FPsdLayer` (has `Opacity`, `bVisible`), `FPsdTextRun`, `FPsdDocument`
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` — layer extraction (line 358: `bVisible`, line 360: `Opacity`)
- `Source/PSD2UMG/Private/Parser/PsdParserInternal.h` — PhotoshopAPI PIMPL boundary (all new effect extraction goes here)

### Generator (Phase 3)
- `Source/PSD2UMG/Private/Generator/FWidgetBlueprintGenerator.cpp` — line 37: current invisible-layer skip (must be changed for D-01/D-02)

### Settings
- `Source/PSD2UMG/Public/PSD2UMGSetting.h` — `UPSD2UMGSettings` (add `bFlattenComplexEffects` here)

### Prior Phase Context
- `.planning/phases/04-text-fonts-typography/04-CONTEXT.md` — D-09..D-12: text outline/shadow best-effort pattern; same approach applies to FX-04 if PhotoshopAPI lacks data

### Upstream
- PhotoshopAPI v0.9 — researcher must determine what layer effect data is exposed (color overlay, drop shadow, inner shadow, etc.)

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `FPsdLayer::Opacity` — already parsed (line 72 of PsdTypes.h, line 360 of PsdParser.cpp); just needs application in generator
- `FPsdLayer::bVisible` — already parsed (line 73 of PsdTypes.h, line 358 of PsdParser.cpp); generator behavior must change from skip to Collapsed
- `LogPSD2UMG` — existing log category for all warnings
- `FWidgetBlueprintGenerator` — the main loop that creates widgets and configures slots; effects applied here after widget creation
- Image texture import pipeline — already handles RGBA → UTexture2D (Phase 3); reusable for shadow duplicate and flattened images

### Established Patterns
- PIMPL boundary: all PhotoshopAPI calls inside `Private/Parser/` files, never in public headers
- Diagnostic logging: `UE_LOG(LogPSD2UMG, Warning, ...)` for unsupported/missing data
- Best-effort delivery: if PhotoshopAPI doesn't expose data, deliver as no-op with warning (Phase 4 pattern for TEXT-04)
- ARGB channel order: Phase 2 discovered PhotoshopAPI uses ARGB not RGBA — verify for effect colors

### Integration Points
- `FWidgetBlueprintGenerator::BuildWidgetTree` — apply opacity, visibility, color overlay, drop shadow after widget creation
- `PsdParser.cpp` — extend layer extraction to populate effect data
- `UPSD2UMGSettings` — add `bFlattenComplexEffects` property
- Texture import helper — may need to be called for flattened layer images and shadow duplicates

</code_context>

<specifics>
## Specific Ideas

- Changing invisible layer handling from "skip" to "create as Collapsed" requires updating `FWidgetBlueprintGenerator.cpp` line 37 and any tests that verify the skip behavior (e.g., `FWidgetBlueprintGenSpec.cpp` line 74).
- Drop shadow as offset duplicate: the shadow UImage should be positioned at `(OriginalX + ShadowOffsetX, OriginalY + ShadowOffsetY)` with the layer's texture tinted to the shadow color. Z-order must place it behind the original.
- `bFlattenComplexEffects` defaults to true — the safe choice for designers who expect visual fidelity over editability.

</specifics>

<deferred>
## Deferred Ideas

- **Blend mode mapping** — Multiply, Screen, Overlay etc. have no direct UMG equivalent; would require material-based approach (future phase or out of scope)
- **Stroke/border on non-text layers** — Not covered in v1 requirements
- **Per-effect flatten toggle** — Currently all-or-nothing via `bFlattenComplexEffects`; per-effect granularity is a future enhancement

</deferred>

---

*Phase: 05-layer-effects-blend-modes*
*Context gathered: 2026-04-10 via /gsd:discuss-phase*
