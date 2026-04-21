---
plan: 13-01
phase: 13-gradient-layers
status: complete
completed: 2026-04-21
---

# 13-01 Summary: Fixture + Spec Stubs + Enum Extension (RED)

## What was built

- `Source/PSD2UMG/Tests/Fixtures/GradientLayers.psd` — 3 fill layers: `grad_linear` (linear gradient), `grad_radial` (radial gradient), `solid_gray` (solid color #808080). Saved with Maximize Compatibility ON.
- `EPsdLayerType` extended with `Gradient` (value 4) and `SolidFill` (value 5) inserted between `SmartObject` and `Unknown` (value 6).
- `FPsdParserGradientSpec` appended to `PsdParserSpec.cpp` — registered as `PSD2UMG.Parser.GradientLayers`.

## Fixture

- Path: `Source/PSD2UMG/Tests/Fixtures/GradientLayers.psd`
- Size: 90,420 bytes (> 5 KB threshold; Maximize Compatibility confirmed ON)
- Layers (3 root fill layers, no Background layer): `grad_linear`, `grad_radial`, `solid_gray`
- SHA confirmation: tracked in git at commit 77764d2

## UE Session Frontend — RED state confirmed

Run: `PSD2UMG.Parser.GradientLayers` — 5 It() blocks total:

| Test | Result | Message |
|------|--------|---------|
| loads successfully with 3 root layers | ✅ PASS | bParsed=true, no errors, 3 root layers |
| grad_linear has Type == EPsdLayerType::Gradient | ❌ FAIL | Expected 4, got 6 (Unknown) |
| grad_radial has Type == EPsdLayerType::Gradient | ❌ FAIL | Expected 4, got 6 (Unknown) |
| solid_gray has Type == EPsdLayerType::SolidFill | ❌ FAIL | Expected 5, got 6 (Unknown) |
| solid_gray has Effects.bHasColorOverlay == true | ❌ FAIL | was false |

All 4 failures are wrong-enum-value / flag-not-set shape (NOT BeforeEach fixture-missing errors). RED baseline confirmed.

## Existing spec regressions

None introduced by this plan:
- `PSD2UMG.Parser.MultiLayer` — ✅ all 8 pass
- `PSD2UMG.Parser.Typography` — ✅ 9/10 pass (1 pre-existing `text_overlay_gray` fail from Phase 12 debt)
- `PSD2UMG.Parser.SimpleHUD` — ✅ all 6 pass
- `PSD2UMG.Parser.Effects` — ✅ all 6 pass

Pre-existing failures in PanelAttachment and TextEffects are unrelated to this phase.

## Input contract for Plan 13-02

- Enum values `Gradient=4` and `SolidFill=5` are compilable and present.
- Fixture has 3 ShapeLayer instances with GdFl/SoCo tagged blocks.
- 4 RED assertions in `PSD2UMG.Parser.GradientLayers` waiting to go green.
