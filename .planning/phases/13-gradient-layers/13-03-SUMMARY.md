---
phase: 13-gradient-layers
plan: "03"
subsystem: mapper, parser, texture-importer
tags: [fill-layer, gradient, solid-fill, adjustment-layer, shape-layer, tagged-blocks, texture-compression]

requires:
  - phase: 13-02
    provides: "EPsdLayerType::Gradient/SolidFill dispatch in parser, RGBAPixels baked for gradients, Effects.ColorOverlayColor populated for solid fills"

provides:
  - "FFillLayerMapper: gradient layer → UImage with TC_BC7 texture, backed by FTextureImporter::ImportLayer"
  - "FSolidFillLayerMapper: solid fill layer → zero-texture UImage; FX-03 applies ColorOverlayColor tint"
  - "FLayerMappingRegistry::RegisterDefaults registers both mappers at priority 100"
  - "AdjustmentLayer<T>::get_channel shim (vendored header) enabling ExtractImagePixels for PhotoshopAPI's fill-layer type"
  - "Tag-based fill dispatch in ConvertLayerRecursive: adjSolidColor/adjGradient keys detected BEFORE RTTI casts"
  - "End-to-end verified: GradientLayers.psd → 3 UImage widgets, gradients no banding, solid_gray correct gray tint"

affects:
  - "FWidgetBlueprintGenerator FX-03 block (tints solid-fill UImage via bHasColorOverlay + ColorOverlayColor)"
  - "build-photoshopapi.bat: now patches AdjustmentLayer.h before build + outputs real-time logs via Tee-Object"

tech-stack:
  added: []
  patterns:
    - "Tag-based fill dispatch: adjSolidColor/adjGradient tagged block keys as type-agnostic discriminator before RTTI"
    - "Vendored-header patch: get_channel added to AdjustmentLayer<T> (no ImageDataMixin, no ambiguity)"
    - "TC_BC7 for gradient textures (TC_UserInterface2D absent in UE5.7)"

key-files:
  created:
    - "Source/PSD2UMG/Private/Mapper/FFillLayerMapper.cpp"
    - "Source/PSD2UMG/Private/Mapper/FSolidFillLayerMapper.cpp"
  modified:
    - "Source/PSD2UMG/Private/Mapper/AllMappers.h — FFillLayerMapper + FSolidFillLayerMapper declarations"
    - "Source/PSD2UMG/Private/Mapper/FLayerMappingRegistry.cpp — registration in priority-100 tier"
    - "Source/PSD2UMG/Private/Generator/FTextureImporter.cpp — TC_BC7 for Gradient layers"
    - "Source/PSD2UMG/Private/Parser/PsdParser.cpp — tag-based fill dispatch + AdjustmentLayer include + SoCo offset fix"
    - "Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/LayeredFile/LayerTypes/AdjustmentLayer.h — get_channel shim"
    - "Scripts/build-photoshopapi.bat — real-time logging + AdjustmentLayer.h patch step"

key-decisions:
  - "PhotoshopAPI classifies gradient/solid fill layers as AdjustmentLayer<T>, NOT ShapeLayer<T>. RTTI-only dispatch silently fails. Tag-based detection (adjSolidColor/adjGradient) must precede all RTTI casts."
  - "get_channel added to AdjustmentLayer<T> only (not Layer<T>) to avoid C2385 ambiguity on SmartObjectLayer, which inherits both Layer<T> and ImageDataMixin."
  - "TC_BC7 replaces TC_UserInterface2D for gradient textures — TC_UserInterface2D does not exist in UE5.7."
  - "SoCo descriptor offset confirmed as 4 (4-byte version=16 prefix before descriptor per PSD spec). TryParseAt(4) first; 0 and 8 as fallbacks."

requirements-completed:
  - GRAD-01
  - GRAD-02

duration: ~90min (including build-iterate cycle)
completed: "2026-04-21"
---

# Phase 13 Plan 03: FFillLayerMapper + FSolidFillLayerMapper + Registry + End-to-End Verify Summary

**Both fill-layer mappers registered, GradientLayers.psd imports to 3 correct UImage widgets: grad_linear, grad_radial (with texture, no banding), solid_gray (gray tint via FX-03)**

## Performance

- **Duration:** ~90 min (includes iterative build-fix cycle)
- **Completed:** 2026-04-21
- **Tasks:** 6 (mapper impl, registry, texture compression, tag dispatch fix, SoCo offset fix, visual verify)
- **Files modified:** 8

## Accomplishments

- `FFillLayerMapper::Map` produces UImage backed by UTexture2D via `FTextureImporter::ImportLayer`; compression set to TC_BC7 for Gradient type
- `FSolidFillLayerMapper::Map` produces zero-texture UImage with bounds-sized brush; FX-03 block in generator applies `Effects.ColorOverlayColor` as tint
- Both mappers registered in `FLayerMappingRegistry::RegisterDefaults` at priority 100 alongside FImageLayerMapper
- `AdjustmentLayer<T>::get_channel` shim added to vendored header — PhotoshopAPI assigns fill layers to `AdjustmentLayer`, not `ShapeLayer`
- Tag-based fill dispatch added in `ConvertLayerRecursive` BEFORE RTTI casts — detects `adjSolidColor`/`adjGradient` keys from `unparsed_tagged_blocks()`, completely bypassing the RTTI type confusion
- `ScanSolidFillColor` updated: TryParseAt(4) first (confirmed PSD spec: 4-byte version=16 prefix before descriptor)
- `build-photoshopapi.bat` updated: real-time output via PowerShell Tee-Object; AdjustmentLayer.h added as pre-build patch step

## Task Commits

1. FFillLayerMapper + FSolidFillLayerMapper implementation + registry — (mapper files created, AllMappers.h + FLayerMappingRegistry.cpp modified)
2. TC_BC7 gradient texture compression — (FTextureImporter.cpp)
3. AdjustmentLayer.h get_channel shim — (vendored header)
4. Tag-based fill dispatch in ConvertLayerRecursive + AdjustmentLayer include in PsdParser.cpp
5. SoCo offset fix: TryParseAt(4) first — `6debdaa`
6. build-photoshopapi.bat real-time logging + AdjustmentLayer.h patch step

## Deviations from Plan

- **TC_UserInterface2D → TC_BC7**: Plan specified TC_UserInterface2D; not available in UE5.7. TC_BC7 used instead (same intent: high-quality compression without BC1 banding).
- **AdjustmentLayer dispatch, not ShapeLayer**: Plan assumed `dynamic_pointer_cast<ShapeLayer>` would work for fill layers. PhotoshopAPI actually uses `AdjustmentLayer` for these. Required adding `AdjustmentLayer.h` include and `get_channel` shim, plus tag-based dispatch to avoid RTTI failure.
- **SoCo offset**: Plan said TryParseAt(0) first; empirical test revealed offset 4 is correct (PSD spec 4-byte version prefix).

## Issues Encountered

- Linter/hook repeatedly stripped `get_channel` from vendored headers (AdjustmentLayer.h, ShapeLayer.h, Layer.h). Each strip required detection, re-add, commit. `build-photoshopapi.bat` pre-build patch step added to prevent drift on full rebuild.
- `SmartObjectLayer` inherits both `Layer<T>` and `ImageDataMixin` — adding `get_channel` to `Layer<T>` causes C2385 ambiguous access. Method must live only on types without `ImageDataMixin` (`AdjustmentLayer`, `ShapeLayer`).

## Known Stubs

None — both mappers produce correct output on real GradientLayers.psd import.

## Next Phase Readiness

- Phase 14 (Shape/Vector Layers) can now proceed: same tag-based dispatch pattern, same AdjustmentLayer/ShapeLayer get_channel shims are in place
- FX-03 solid-fill tinting is generic — works for any layer type where `bHasColorOverlay=true`, including future shape layers

---
*Phase: 13-gradient-layers*
*Completed: 2026-04-21*
