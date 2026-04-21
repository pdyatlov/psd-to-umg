---
phase: 13-gradient-layers
plan: "02"
subsystem: parser
tags: [photoshopapi, shapelayer, gradient, solidfill, descriptor-walker, soco, tagged-blocks]

requires:
  - phase: 13-01
    provides: "EPsdLayerType::Gradient + EPsdLayerType::SolidFill enums, GradientLayers.psd fixture, 4 RED It() stubs in FPsdParserGradientSpec"

provides:
  - "ShapeLayer<T>::get_channel(Enum::ChannelID id) const shim enabling ExtractImagePixels<ShapeLayer<T>> instantiation"
  - "ScanSolidFillColor static helper that walks SoCo descriptor and populates Effects.ColorOverlayColor + bHasColorOverlay"
  - "ShapeLayer dispatch branch in ConvertLayerRecursive distinguishing Gradient from SolidFill via TaggedBlockKey::adjSolidColor"
  - "9 FPsdParserGradientSpec It() blocks (1 load + 4 type assertions + 4 pixel/color assertions), all GREEN"

affects:
  - "13-03 (FSolidFillLayerMapper consumes Effects.ColorOverlayColor + bHasColorOverlay set here)"
  - "FWidgetBlueprintGenerator FX-03 block (tints UImage when bHasColorOverlay=true)"

tech-stack:
  added: []
  patterns:
    - "Vendored-header patch: add public accessor to PhotoshopAPI ShapeLayer<T> following Phase 12 Layer.h precedent"
    - "TryParseAt(0)/TryParseAt(8) fallback pattern for Photoshop descriptor offset uncertainty"
    - "ShapeLayer dispatch via dynamic_pointer_cast + adjSolidColor tagged block scan"

key-files:
  created:
    - "Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/LayeredFile/LayerTypes/ShapeLayer.h (was untracked, now tracked)"
  modified:
    - "Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/LayeredFile/LayerTypes/ShapeLayer.h — get_channel shim added"
    - "Source/PSD2UMG/Private/Parser/PsdParser.cpp — ScanSolidFillColor helper + ShapeLayer dispatch in ConvertLayerRecursive"
    - "Source/PSD2UMG/Tests/PsdParserSpec.cpp — 4 new It() blocks in FPsdParserGradientSpec"

key-decisions:
  - "SoCo descriptor offset: TryParseAt(0) first, TryParseAt(8) fallback — mirrors the lfx2 8-byte prefix discovery from Phase 04.1 but defensive rather than assumed"
  - "get_channel shim iterates m_UnparsedImageData directly (protected member of Layer<T>) — no upstream API change needed; follows Phase 12 unparsed_tagged_blocks() patch precedent"
  - "SolidFill layers do NOT get pixel extraction — FSolidFillLayerMapper (Plan 13-03) produces a zero-texture UImage; FX-03 tints it via ColorOverlayColor"
  - "ShapeLayer dispatch inserted between ImageLayer and TextLayer branches — safe ordering since fill layers are never text and dynamic_pointer_cast<ImageLayer> fails on ShapeLayer"

patterns-established:
  - "Phase 13 vendored-header pattern: add public accessor to PhotoshopAPI type that lacks one, with detailed comment referencing precedent"
  - "TryParseAt lambda pattern for Photoshop descriptor offset probing"

requirements-completed:
  - GRAD-01
  - GRAD-02

duration: 15min
completed: "2026-04-21"
---

# Phase 13 Plan 02: Parser Dispatch + ShapeLayer Shim + SoCo Scanner (GREEN) Summary

**ShapeLayer.h get_channel shim + ScanSolidFillColor descriptor walker + ConvertLayerRecursive dispatch turn all 4 RED GradientLayers assertions GREEN and add 4 new pixel/color coverage assertions**

## Performance

- **Duration:** ~15 min
- **Started:** 2026-04-21T09:36:43Z
- **Completed:** 2026-04-21T09:50:33Z
- **Tasks:** 3
- **Files modified:** 3

## Accomplishments
- Added `get_channel(Enum::ChannelID id) const` to `ShapeLayer<T>` — enables `ExtractImagePixels<ShapeLayer<T>>` template instantiation for gradient baking
- Added `ScanSolidFillColor` static helper that walks the SoCo tagged-block Photoshop descriptor (same lambda pattern as ParseFrFXDescriptor) to extract RGBC color and set `Effects.ColorOverlayColor` + `Effects.bHasColorOverlay`
- Added ShapeLayer dispatch branch between ImageLayer and TextLayer in `ConvertLayerRecursive` — detects Gradient vs SolidFill via `TaggedBlockKey::adjSolidColor` presence
- All 4 RED stubs from Plan 13-01 now GREEN; 4 new GRAD-02/GRAD-01-color assertions added totaling 9 It() blocks

## Task Commits

Each task was committed atomically:

1. **Task 1: Add public get_channel shim to vendored ShapeLayer.h** - `3d0152c` (feat)
2. **Task 2: Add ScanSolidFillColor helper + ShapeLayer dispatch in ConvertLayerRecursive** - `8d7718d` (feat)
3. **Task 3: Extend FPsdParserGradientSpec with GRAD-02 pixel-count and solid_gray colour assertions** - `c55c9b4` (test)

## Files Created/Modified
- `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/LayeredFile/LayerTypes/ShapeLayer.h` — `get_channel` shim inserted after `ShapeLayer() = default;`, Phase 12 comment style, iterates `Layer<T>::m_UnparsedImageData`
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` — `ScanSolidFillColor` inserted after `ParseFrFXDescriptor` (before Raw lfx2 scanner comment); ShapeLayer dispatch branch inserted between ImageLayer and TextLayer branches in `ConvertLayerRecursive`
- `Source/PSD2UMG/Tests/PsdParserSpec.cpp` — 4 new `It()` blocks appended to `FPsdParserGradientSpec::Define()` inside `Describe("GradientLayers fixture", ...)`

## Decisions Made
- **SoCo descriptor offset**: TryParseAt(0) first, TryParseAt(8) fallback — resolves research Open Question 1. The Verbose hex dump log `Layer 'solid_gray' SoCo payload[0..40]=` will reveal which offset wins on first import run.
- **No pixel extraction for SolidFill**: Consistent with Plan 13-03 FSolidFillLayerMapper design — UImage created with no texture, FX-03 tints via `Effects.ColorOverlayColor`.
- **get_channel iterates m_UnparsedImageData**: Protected member of `Layer<T>`, accessible via `Layer<T>::m_UnparsedImageData` qualified name from within the `ShapeLayer<T>` struct body. Matches the `LinkedLayerData.h::get_channel(Enum::ChannelIDInfo)` precedent.

## Deviations from Plan
None - plan executed exactly as written. All exact code blocks from the plan were inserted verbatim.

## Issues Encountered
None. The `TaggedBlock::m_Data` field confirmed as `std::vector<std::byte>` in `TaggedBlock.h` — `std::to_integer<uint32>(Data[i])` usage is correct.

## Known Stubs
None — the SoCo color parser produces real values from the PSD descriptor. The TryParseAt fallback mechanism handles both possible offsets. The actual offset used will be logged on first import run, resolving research Open Question 1.

## Next Phase Readiness
- Plan 13-03 (FSolidFillLayerMapper + FGradientLayerMapper) can now proceed: `Effects.ColorOverlayColor` is populated for solid fills, `RGBAPixels` is populated for gradients
- The FX-03 generator block in `FWidgetBlueprintGenerator.cpp` (lines 326-340) already handles `bHasColorOverlay` on any `UImage` — no generator changes needed for solid fill tinting
- All existing parser specs (MultiLayer, Typography, SimpleHUD, Effects) are unaffected — ShapeLayer dispatch only fires for `ShapeLayer<T>` instances

---
*Phase: 13-gradient-layers*
*Completed: 2026-04-21*
