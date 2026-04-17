---
phase: 02-c-psd-parser
plan: 03
subsystem: parser
tags: [parser, photoshopapi, pimpl, c++20]
requirements: [PRSR-02, PRSR-03, PRSR-05]
dependency_graph:
  requires:
    - Source/PSD2UMG/Public/Parser/PsdTypes.h
    - Source/PSD2UMG/Public/Parser/PsdDiagnostics.h
    - Source/PSD2UMG/Public/PSD2UMGLog.h
    - Source/ThirdParty/PhotoshopAPI/** (vendored headers and libs from 02-01)
  provides:
    - "PSD2UMG::Parser::ParseFile(const FString&, FPsdDocument&, FPsdParseDiagnostics&)"
  affects:
    - Phase 02 plan 04 (UPsdImportFactory will call ParseFile)
    - Phase 02 plan 05 (automation spec validates ParseFile output)
tech-stack:
  added:
    - "PhotoshopAPI LayeredFile<bpp8_t>::read as the document loader"
  patterns:
    - "PIMPL: third-party headers confined to a single .cpp behind THIRD_PARTY_INCLUDES_START/END"
    - "RTTI layer-type dispatch via std::dynamic_pointer_cast"
    - "try/catch around every vendor call; exceptions -> FPsdDiagnostic::Error"
key-files:
  created:
    - Source/PSD2UMG/Public/Parser/PsdParser.h
    - Source/PSD2UMG/Private/Parser/PsdParser.cpp
    - Source/PSD2UMG/Private/Parser/PsdParserInternal.h
  modified: []
decisions:
  - "Used LayeredFile<bpp8_t>::read(std::filesystem::path) -- the only on-disk read overload the vendor exposes. No stream/buffer overload exists, so plan 02-04 will spill the factory byte buffer to a temp file to bridge in-memory import."
  - "Used style_run_*(0) and paragraph_run_justification(0) instead of the style_normal_* accessors. The run-indexed variants work uniformly whether the PSD has a single implicit run or explicit multiple runs, whereas style_normal_* only applies to layers with a single default style record."
  - "Multi-run detection driven by style_run_lengths()->size() > 1 per 02-CONTEXT.md. When detected, we take the first run's style verbatim, keep the full concatenated TySh text payload (text() already returns it), and emit a Warning diagnostic so callers can surface it."
  - "pt -> px stored as a direct cast (SizePx = static_cast<float>(FontSize)). The x0.75 UMG-DPI conversion is deliberately NOT applied; Phase 4 TEXT-01 owns the full text pipeline per the plan's explicit instruction."
  - "FLinearColor::FromSRGBColor(FColor(r,g,b)) used for color conversion; fill_color doubles are clamped to [0,1] then scaled to 0..255 before construction."
metrics:
  duration: "~15m"
  tasks: 2
  completed: "2026-04-08"
---

# Phase 2 Plan 3: PSD Parser Implementation Summary

One-liner: Native PSD parser reads .psd via PhotoshopAPI LayeredFile<bpp8_t>, walks the layer tree via RTTI dynamic_pointer_cast, populates FPsdDocument with groups/images/text behind a strict PIMPL wall.

## What Shipped

- `PSD2UMG::Parser::ParseFile(Path, OutDoc, OutDiag)` as a stateless free function.
- PIMPL discipline: `Public/Parser/PsdParser.h` has zero references to PhotoshopAPI; the only vendor include lives in `Private/Parser/PsdParser.cpp` inside a `THIRD_PARTY_INCLUDES_START/END` block. `PsdParserInternal.h` is intentionally vendor-free to keep private translation units cheap to compile.
- Layer walker dispatches on `std::dynamic_pointer_cast<GroupLayer<bpp8_t>>`, `ImageLayer<bpp8_t>`, and `TextLayer<bpp8_t>` (enabled by `bUseRTTI = true` in `PSD2UMG.Build.cs`, landed in plan 02-02).
- Image RGBA extraction pulls R/G/B/A via `ImageLayer::get_channel(Enum::ChannelID::*)`, tolerates missing alpha (defaults to 0xFF), interleaves into a packed `TArray<uint8>` of length `PixelWidth * PixelHeight * 4`.
- Text single-run extraction reads content, font postscript name, size, fill color, and paragraph justification from the first run; multi-run layers flatten with a diagnostic warning per 02-CONTEXT.md.
- Every vendor call is guarded. Outer `try { ... } catch (const std::exception&)` + `catch (...)` converts any failure into an `FPsdDiagnostic::Error` and returns `false`. Inner try/catch blocks wrap channel extraction and text reads so a bad image/text layer doesn't abort the whole document.
- `UE_LOG(LogPSD2UMG, Log, ...)` summary line on every parse with canvas size, root layer count, warning/error counts.

## PhotoshopAPI API Surface Used

Discovered from the vendored headers under `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/`:

| API | Header | Usage |
|---|---|---|
| `LayeredFile<T>::read(std::filesystem::path)` | LayeredFile/LayeredFile.h | Document load (path-only overload; no stream variant) |
| `LayeredFile<T>::width()` / `height()` | LayeredFile/LayeredFile.h | Canvas size |
| `LayeredFile<T>::layers()` | LayeredFile/LayeredFile.h | Top-level layer vector |
| `Layer<T>::name()` | LayerTypes/Layer.h | Layer display name |
| `Layer<T>::visible()` | LayerTypes/Layer.h | Visibility flag |
| `Layer<T>::opacity()` | LayerTypes/Layer.h | Already normalised 0..1 float (no manual /255 needed) |
| `Layer<T>::width()` / `height()` / `center_x()` / `center_y()` | LayerTypes/Layer.h | Bounds reconstruction |
| `GroupLayer<T>::layers()` | LayerTypes/GroupLayer.h | Recurse into children |
| `ImageLayer<T>::get_channel(Enum::ChannelID)` | LayerTypes/ImageDataMixins.h | Per-channel RGBA extraction |
| `Enum::ChannelID::{Red,Green,Blue,Alpha}` | Util/Enum.h | Channel identifiers |
| `TextLayer<T>::text()` | LayerTypes/TextLayer/TextLayer.h | UTF-8 content |
| `TextLayer<T>::style_run_lengths()` | LayerTypes/TextLayer/TextLayer.h | Multi-run detection |
| `TextLayer<T>::font_postscript_name(size_t)` | LayerTypes/TextLayer/TextLayerFontMixin.h | Font family (first run) |
| `TextLayer<T>::style_run_font_size(size_t)` | LayerTypes/TextLayer/TextLayerStyleRunMixin.h | Font size (pt) |
| `TextLayer<T>::style_run_fill_color(size_t)` | LayerTypes/TextLayer/TextLayerStyleRunMixin.h | Fill color as `vector<double>` 0..1 sRGB |
| `TextLayer<T>::paragraph_run_justification(size_t)` | LayerTypes/TextLayer/TextLayerParagraphRunMixin.h | Alignment enum (Left/Right/Center/...) |
| `TextLayerEnum::Justification` | LayerTypes/TextLayer/TextLayerEnum.h | Maps to `ETextJustify` |

The `TextLayer` justification accessor was flagged "MEDIUM confidence" in 02-RESEARCH.md. Confirmed: it lives on `TextLayerParagraphRunMixin` as `paragraph_run_justification(size_t)` returning `std::optional<TextLayerEnum::Justification>`. The enum values `{Left=0, Right=1, Center=2, JustifyLastLeft=3, JustifyLastRight=4, JustifyLastCenter=5, JustifyAll=6}` were mapped conservatively: Right/JustifyLastRight -> `ETextJustify::Right`, Center/JustifyLastCenter -> `ETextJustify::Center`, everything else including full-justify variants -> `ETextJustify::Left` (UMG has no native "justify" value in `ETextJustify`; will be revisited in Phase 4 TEXT-01).

## pt -> px Conversion Math

```
SizePx = static_cast<float>(FontSize);   // 1 pt == 1 px at 72 DPI
```

No x0.75 scaling applied. At 72 DPI the PSD-native pt unit is numerically identical to pixels, which matches PhotoshopAPI's coordinate space. The Phase 4 text pipeline (TEXT-01) owns any DPI normalisation for UMG's 96-DPI layout system.

## In-Memory Read: Not Supported by Vendor

`LayeredFile<T>::read` only has two overloads, both taking `const std::filesystem::path&`:

```cpp
static LayeredFile<T> read(const std::filesystem::path& filePath, ProgressCallback& callback);
static LayeredFile<T> read(const std::filesystem::path& filePath);
```

There is no stream or buffer overload. Plan 02-04 (`UPsdImportFactory`) receives a byte buffer from UE's factory pipeline and therefore must spill it to a temporary file on disk before calling `ParseFile`. This was not a decision in this plan but is a hard constraint callers will inherit. Documented here so plan 02-04 plans around it explicitly.

## Deviations from Plan

None. The plan's pseudo-code matched the real API closely; the only adjustments were:

1. Preferring `style_run_*(0)` over `style_normal_*` for robustness (both are public accessors on TextLayer; the run-indexed variant handles both single- and multi-run layers uniformly).
2. Separating the vendor include into `PsdParser.cpp` instead of `PsdParserInternal.h`. Internal header left vendor-free so additional private .cpp files in Phase 2+ can include it without dragging in fmt/eigen/openimageio. The verification grep for `THIRD_PARTY_INCLUDES_START` in `PsdParser.cpp` is satisfied directly rather than transitively.

Neither is a behavioural deviation from 02-CONTEXT.md; both are structural refinements consistent with "Claude's Discretion".

## Commits

- `c45406a` feat(02-03): add ParseFile public header and PIMPL internal header
- `b90af91` feat(02-03): implement ParseFile against PhotoshopAPI

## Verification

- Public header is vendor-clean: `grep -q "PhotoshopAPI" Source/PSD2UMG/Public/Parser/PsdParser.h` -> no match.
- Implementation contains all required markers: namespace, try/catch, THIRD_PARTY_INCLUDES_START, EPsdLayerType::{Group,Image,Text}.
- `PsdParser.cpp` is 320 lines (well over the 150-line floor).
- Functional end-to-end correctness is gated by plan 02-05's automation spec against the fixture PSD.

## Self-Check: PASSED

- FOUND: Source/PSD2UMG/Public/Parser/PsdParser.h
- FOUND: Source/PSD2UMG/Private/Parser/PsdParser.cpp
- FOUND: Source/PSD2UMG/Private/Parser/PsdParserInternal.h
- FOUND commit c45406a
- FOUND commit b90af91
