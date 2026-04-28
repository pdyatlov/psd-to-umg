# Domain Pitfalls — v1.3 Advanced Effects

**Domain:** Adding stroke rendering (vstk/frameFXMulti), pattern fills (PtFl), lrFX channel-order verification, and non-ASCII rich text to PSD2UMG.
**Researched:** 2026-04-28
**Codebase revision studied:** Phase 20 complete (master, commit a816180)

> This file supersedes the original v1.0 PITFALLS.md for the v1.3 milestone.
> v1.0 pitfalls (Phase 0-8 porting, UE5 API, Widget Blueprint generation) remain valid
> but are not repeated here. This document covers only pitfalls specific to the
> four v1.3 feature areas.

---

## Critical Pitfalls

These mistakes cause silent wrong output, incorrect visual rendering, or data corruption that is hard to diagnose after the fact.

---

### CP-01: Confusing the vstk descriptor offset with the vscg offset

**What goes wrong:** A new `ScanStrokeData` function for the `vstk` (vecStrokeData) block is written by copying `ScanSolidFillColor` or `ScanShapeFillColor` and uses `TryParseAt(4)` as the primary offset, which is the confirmed primary for `SoCo`. The vstk descriptor starts at a different location.

**Why it happens:** The codebase has two confirmed descriptor patterns that look superficially identical:
- `SoCo` (adjSolidColor): version prefix at `[0..3]`, descriptor at offset 4. Primary = `TryParseAt(4)`.
- `vscg` (vecStrokeContentData): classID `'SoCo'` at `[0..3]`, version at `[4..7]`, descriptor at offset 8. Primary = `TryParseAt(8)`.
- `vstk` (vecStrokeData): per Adobe spec the block carries a raw descriptor with no class-ID or version prefix. The descriptor begins at offset 0. This is NOT the same as either pattern above.

**Consequences:** `TopCount` sanity check (`TopCount == 0 || TopCount > 256`) rejects the correct parse position and falls through to a fallback, producing `StrokeSize = 0` silently. The `bHasVectorStroke` flag (see CP-02) never gets set, so shape layers get no stroke border even when one was clearly authored.

**Prevention:**
1. Add the standard hex-dump diagnostic (mirror of `ScanSolidFillColor` lines 1113-1123) to emit `vstk payload[0..40]=` at Verbose before any other work.
2. Import a known PSD with a vector shape that has a visible border stroke and inspect the log.
3. Try `TryParseAt(0)` first. If `TopCount` is reasonable (2-10), the descriptor is at offset 0.

The offset order for the new function must be: `TryParseAt(0)` primary, `TryParseAt(4)` and `TryParseAt(8)` as defensive fallbacks — the exact reverse of SoCo's order.

**Detection:** Both fallback attempts return false; the Verbose hex dump shows `TopCount` would be correct at offset 0 but not at 4 or 8.

**Phase:** Stroke-rendering parser phase (first parser phase of v1.3).

---

### CP-02: Reusing `bHasStroke` for vstk stroke — double-populate collision with lfx2

**What goes wrong:** The existing `ScanRawLfx2Blocks` / `ParseFrFXDescriptor` pipeline already writes to `Effects.bHasStroke` / `Effects.StrokeSize` / `Effects.StrokeColor` for ALL layer types (PsdParser.cpp:1697-1699, and PsdTypes.h:117: "populated for ALL layer types"). If a new vstk scanner also writes to `bHasStroke`, one of two bugs results:
- Overwrite collision: the vstk value silently replaces the lfx2 value (or vice versa, depending on call order).
- Early skip: the mapper skips vstk because `bHasStroke` is already true from lfx2.

A vector shape layer can have BOTH: an lfx2 "Layer Style Stroke" (panel effect, any layer type) AND a vstk "Shape Stroke" (geometric stroke specific to vector shapes). Photoshop renders both simultaneously.

**Why it happens:** The lfx2 stroke path was designed with a comment "populated for ALL layer types; rendering on non-text is deferred." When vstk rendering is now implemented, the field conflict is invisible without reading that comment.

**Consequences:** Shape layer renders with wrong stroke width (lfx2 value wins or is lost), or the stroke never renders at all.

**Prevention:** Add separate fields to `FPsdLayerEffects` for the vector geometry stroke:
```cpp
// In FPsdLayerEffects (PsdTypes.h):
bool bHasVectorStroke = false;      // from vstk (vecStrokeData) — shape layers only
float VectorStrokeSize = 0.f;
FLinearColor VectorStrokeColor = FLinearColor::Transparent;
```
Do NOT reuse `bHasStroke`. `FShapeLayerMapper::Map` reads `bHasVectorStroke`. The lfx2 `bHasStroke` stays for image/shape fallback effects and the existing text routing path remains untouched.

**Phase:** Stroke-rendering parser phase and `FShapeLayerMapper` update phase.

---

### CP-03: frameFXMulti VlLs stroke — walking a list as if it were a single Objc

**What goes wrong:** Newer Photoshop versions (CC 2014+) write stroke effects under the `"FrFX"` key with ostype `"VlLs"` (a list of effect objects) rather than `"Objc"` (a single sub-descriptor). The existing `ParseFrFXDescriptor` checks `ItemKey == "FrFX" && FCStringAnsi::Strcmp(OsType, "Objc") == 0` (PsdParser.cpp:983). When the block uses VlLs, the OsType match fails. The existing `SkipValueAfterOsType("VlLs")` walker silently skips the entire list, leaving `bFoundStroke = false`.

**Why it happens:** The outer loop has `&& !bFoundStroke`, so it stops at first match. If the file was saved by older Photoshop (Objc format), the existing code works. If saved by newer Photoshop (VlLs format), the entire stroke list is skipped without any log warning.

**Consequences:** PSDs saved with Photoshop CC 2014 through 2025 produce no stroke on any layer type. The silent path makes this extremely hard to diagnose; the developer must know to look for VlLs vs Objc format differences.

**Prevention:** In `ParseFrFXDescriptor`, add a VlLs branch alongside the existing Objc branch. Factor the FrFX Objc body into a named lambda so both call it:
```cpp
// After the existing: if (ItemKey == "FrFX" && Strcmp(OsType, "Objc") == 0) { ... }
else if (ItemKey == "FrFX" && FCStringAnsi::Strcmp(OsType, "VlLs") == 0)
{
    const uint32 N = ReadU32BE();
    for (uint32 k = 0; k < N && CheckRemaining(8) && !bFoundStroke; ++k)
    {
        char ElemOT[5] = {};
        for (int c = 0; c < 4; ++c) ElemOT[c] = static_cast<char>(Data[Pos + c]);
        Pos += 4;
        if (FCStringAnsi::Strcmp(ElemOT, "Objc") == 0)
            ParseFrFXObjc(); // the extracted lambda
        else
            SkipValueAfterOsType(ElemOT);
    }
}
```

**Phase:** frameFXMulti support phase (can be part of the same stroke-rendering phase as CP-01/CP-02).

---

### CP-04: PtFl descriptor — walking for `"Clr "` when the block contains `"Ptrn"` 

**What goes wrong:** `PtFl` (adjPattern, `TaggedBlockKey::adjPattern`) is recognized in PhotoshopAPI's enum (Enum.h:832) but not yet parsed by this codebase. A new `ScanPatternFillData` written by analogy with `ScanSolidFillColor` searches for key `"Clr "` / `"RGBC"` doubles. The PtFl descriptor does NOT contain a `"Clr "` key; it contains `"Ptrn"` (pattern reference Objc with `"Nm  "` and `"ID  "` TEXT fields), `"Scl "` (UntF scale), and `"Algn"` (bool). The color-hunting walker will find nothing and return false.

**Why it happens:** SoCo and PtFl are both adjustment fill layers but they describe completely different data. The copy-paste reflex from SoCo is the trap.

**Consequences:** Pattern fill layer produces no data; the mapper falls through to `EPsdLayerType::Unknown`, emitting a warning and generating nothing. The designer's repeating-tile background is completely lost.

**Prevention:**
1. Build a dedicated `ScanPatternFillData` that reads `"Ptrn"` → `"Nm  "` and `"ID  "` for the pattern reference. Store these in a new `FPsdPatternFill` struct on `FPsdLayer` (not in `FPsdLayerEffects` — this is layer-type payload, not a style effect).
2. Add a hex-dump diagnostic and inspect the actual key sequence on a real PSD before writing any parsing logic.
3. For pixel rasterization: verify whether `Layer.RGBAPixels` is populated for AdjustmentLayer pattern fills by PhotoshopAPI (see MP-02). If empty, fall back to the existing `bFlattenComplexEffects` path.

**Phase:** Pattern fill parser phase. Requires empirical offset and key verification before implementation.

---

### CP-05: UTF-16 run-length slicing cuts mid-codepoint for emoji and supplementary-plane characters

**What goes wrong:** `FString::Mid(CharOffset, RunLen)` at PsdParser.cpp:514 uses `RunLen` from `style_run_lengths()` which is a UTF-16 code-unit count per PSD spec. For ASCII and BMP CJK (each 1 UTF-16 code unit, decoded to 1 TCHAR on any UE platform), slicing is correct. For emoji and supplementary-plane characters (e.g. `U+1F600` = `😀` as a surrogate pair), each character is 2 UTF-16 code units but decodes to 1 or 2 TCHARs depending on platform `TCHAR` width (2 bytes on Win64 MSVC, 4 bytes on some Linux builds). The existing `TODO` at PsdParser.cpp:469-478 explicitly flags this and leaves it for a future phase.

**Why it happens:** `Utf8ToFString` converts PSD UTF-8 content to FString. Then `FString::Mid` operates in TCHAR units. PhotoshopAPI's run lengths are in UTF-16 code-unit units. For pure ASCII/BMP these happen to match. For supplementary-plane code points the units diverge.

**Consequences:** For a multi-run text layer containing emoji, `Spans[i].Text` strings have off-by-one TCHAR boundaries. The markup produced by `BuildMarkup` contains a misaligned split — one span has the leading surrogate and the other has the trailing surrogate, or a CJK character is split between two spans. `URichTextBlock` renders a replacement character box or crashes on the malformed surrogate.

**Prevention:** Convert `FullUtf8` to a `std::u16string` before slicing, so that run-length indices operate on the same UTF-16 code-unit granularity as `style_run_lengths()`:
```cpp
// Replace the existing FullText / FString::Mid path in the span extraction block:
std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> Conv;
const std::u16string Utf16Full = Conv.from_bytes(FullUtf8);
// ... slice by code-unit index:
std::u16string SliceU16 = Utf16Full.substr(U16Offset, RunLen);
// Convert slice back to FString:
Span.Text = FString(reinterpret_cast<const TCHAR*>(SliceU16.c_str())); // Win64 TCHAR=wchar_t=UTF-16
```
Verify empirically that `style_run_lengths()` counts surrogate pairs as 2 units (expected per spec) vs 1 before finalizing. Log a hex dump of the UTF-16 slice boundary on a test PSD with emoji.

**Detection:** Import a PSD with two differently-styled runs where the first run ends with an emoji. If `Spans[0].Text` contains a half-surrogate or `Spans[1].Text` begins with one, the offset is misaligned. `URichTextBlock::SetText` will silently render a replacement character.

**Phase:** Non-ASCII rich text phase (v1.3).

---

### CP-06: lrFX sofi handler does not branch on ColorSpace — HSB/CMYK overlay produces wrong color

**What goes wrong:** `ExtractLayerEffects` (PsdParser.cpp:692-720) parses the `sofi` key (color overlay) by reading `ColorSpace` (2 bytes) then three 16-bit values `C0, C1, C2` and assigns `FLinearColor(C0, C1, C2, A)` unconditionally. If `ColorSpace == 0` (RGB), this is correct. If `ColorSpace == 1` (HSB) or `ColorSpace == 3` (CMYK), the values are in a different color space and must be converted before constructing the FLinearColor.

**Why it happens:** The existing Verbose log at PsdParser.cpp:713-717 logs `colorSpace=` and the raw values, which is sufficient to diagnose the bug but does not fix it. Test fixtures in `Effects.psd` use RGB-mode documents so the bug is never triggered in automated specs.

**Consequences:** A production PSD with an HSB or CMYK color overlay on a UImage or text widget produces a visually wrong tint. No error is logged. The developer must visually diff the PSD against the widget to notice.

**Prevention:** In `ExtractLayerEffects`, add a branch after reading `ColorSpace`:
```cpp
FLinearColor OverlayLinear = FLinearColor::White;
if (ColorSpace == 0) // RGB
{
    OverlayLinear = FLinearColor(C0, C1, C2, A);
}
else if (ColorSpace == 1) // HSB
{
    // C0=H(0..1 of 360), C1=S(0..1 of 100), C2=B(0..1 of 100)
    FLinearColor HSV(C0 * 360.f, C1 * 100.f, C2 * 100.f, A);
    OverlayLinear = HSV.HSVToLinearRGB(); // UE built-in
}
else
{
    OutDiag.AddWarning(OutLayer.Name,
        FString::Printf(TEXT("sofi: unsupported ColorSpace=%u; color overlay may be wrong"), ColorSpace));
    OverlayLinear = FLinearColor(C0, C1, C2, A); // best-effort
}
OutLayer.Effects.bHasColorOverlay = (Enabled != 0);
OutLayer.Effects.ColorOverlayColor = OverlayLinear;
```
The same fix should be applied to the `dsdw` drop-shadow handler which has the same unconditional channel assignment.

**Phase:** lrFX channel-order verification phase. The v1.3 "lrFX visual confirm" deliverable should include a deliberate HSB overlay test PSD to cover this path.

---

## Moderate Pitfalls

---

### MP-01: UImage has no native outline border — wrong rendering approach for image-layer stroke

**What goes wrong:** A developer implements stroke rendering on image layers by setting a border or outline property on the `UImage` widget. `UImage` has no built-in stroke/outline property; its `FSlateBrush` only exposes `DrawAs` modes (Image, Box, Border, RoundedBox). None of these produce an outline on top of an arbitrary image without a dedicated 9-slice border texture.

**Why it happens:** The text-layer stroke path works because `UTextBlock` exposes `FSlateFontInfo` outline support. The assumption that `UImage` has equivalent functionality is wrong.

**Consequences:** The stroke feature silently does nothing. The mapper compiles, imports without error, but the visual output shows no stroke border. The developer must compare visually against the PSD.

**Prevention:** The correct approximation for image-layer stroke in UMG is one of:
1. `UOverlay` wrapping two widgets: a slightly larger solid-color `UImage` (size = content_size + 2*stroke_px) underneath, and the original `UImage` on top.
2. `DrawAs = ESlateBrushDrawType::RoundedBox` with `OutlineSettings.Color` and `OutlineSettings.Width` on a rectangular shape layer (`FSlateRoundedBoxBrush` supports this).

The existing comment in `FShapeLayerMapper.cpp` line 13 — "Future stroke rendering (vstk -> UMG border/outline) will attach here" — is the correct location. The Overlay approach is more general; RoundedBox is cleaner for axis-aligned rectangles.

**Phase:** Image-layer stroke mapper phase (after the parser correctly populates `bHasVectorStroke` per CP-02).

---

### MP-02: PtFl AdjustmentLayer RGBAPixels may be empty — pixels not rasterized by PhotoshopAPI

**What goes wrong:** The new pattern-fill mapper assumes it can read `Layer.RGBAPixels` to produce a tiled `UTexture2D`, as `EPsdLayerType::Gradient` works by compositing channels into pixels during `ConvertLayerRecursive`. Pattern fill layers are `AdjustmentLayer` instances in PhotoshopAPI; their pixel channels represent the composited output, but PhotoshopAPI may not rasterize the pattern tile into `m_ImageData` the same way it composites gradient fills.

**Why it happens:** Gradient fill has a bespoke code path in the existing parser that handles GdFl channel data. No equivalent path exists for PtFl, so `RGBAPixels` will be empty unless PhotoshopAPI automatically provides composited pixel data for AdjustmentLayers.

**Consequences:** The pattern fill mapper creates a zero-size or blank `UTexture2D` asset. The designer's repeating-tile background appears white or is missing from the widget.

**Prevention:** Before writing the mapper, verify empirically: import any PSD with a `PtFl` layer and log `Layer.RGBAPixels.Num()`, `Layer.PixelWidth`, `Layer.PixelHeight` in `ConvertLayerRecursive`. If they are non-zero, the existing pixel path is sufficient. If zero, use the existing `bFlattenComplexEffects` flatten-rasterize path (bake the layer's composited appearance to PNG) as the v1.3 fallback — consistent with the flatten-fallback design decision already in place.

**Phase:** Pattern fill parser verification phase — must be done empirically before writing the mapper.

---

### MP-03: D-13 guard missing for non-text layer stroke routing

**What goes wrong:** `RouteTextEffects` (PsdParser.cpp:1706) clears `Effects.bHasStroke = false` after routing stroke to `Text.OutlineSize` (D-13 guard). If the new vstk stroke is stored in a separate `bHasVectorStroke` field (per CP-02), this is moot. But if `bHasStroke` is reused, the FX-03 block in `FWidgetBlueprintGenerator` processes `bHasColorOverlay` for shape layers and also iterates effects — it may encounter a residual `bHasStroke=true` from lfx2 on a shape layer and attempt to apply it as if it were the per-widget stroke, causing the effect to be applied twice or producing an unexpected UImage outline.

**Why it happens:** The D-13 guard was built for the text routing path. The generator's FX-03 path (lines 326-340) processes `bHasColorOverlay` on non-text layers but was written before stroke rendering on shape layers existed.

**Prevention:** The cleanest prevention is CP-02's separate-field recommendation. If that is not used, add a `RouteShapeEffects` function mirroring `RouteTextEffects` that clears `bHasStroke` after the shape mapper reads it, exactly as D-13 does for text. The guard must run before `PopulateChildren` in the generator.

**Phase:** Stroke mapper phase.

---

### MP-04: Non-ASCII markup not HTML-escaped at the surrogate-pair level

**What goes wrong:** `EscapeMarkup` in `FRichTextLayerMapper.cpp` (lines 34-40) escapes `&`, `<`, `>`. After the CP-05 fix, non-ASCII characters in `Span.Text` may include surrogate pairs or combining characters that are valid Unicode but would appear as raw byte values in the markup string passed to `URichTextBlock::SetText`. `URichTextBlock`'s markup parser is not a full HTML parser and may misinterpret certain byte sequences as markup delimiters in edge cases.

**Why it happens:** The markup escaping was written assuming ASCII-range span text (the existing RichText.psd fixture uses only ASCII). The behavior with multi-byte TCHAR sequences was never tested.

**Prevention:** After applying the CP-05 fix, add a post-fix sanitizer in `BuildMarkup` that replaces any TCHAR value below U+0020 (control characters) with a space, and replaces any isolated surrogate TCHAR (U+D800–U+DFFF) with the Unicode replacement character U+FFFD. This is defensive hygiene:
```cpp
static FString SanitizeSpanText(const FString& In)
{
    FString Out = In;
    for (TCHAR& Ch : Out)
    {
        if (Ch < 0x0020 && Ch != TEXT('\n')) Ch = TEXT(' ');
        if (Ch >= 0xD800 && Ch <= 0xDFFF) Ch = 0xFFFD; // isolated surrogate
    }
    return Out;
}
```

**Phase:** Non-ASCII rich text phase (same phase as CP-05).

---

## Minor Pitfalls

---

### MiP-01: vstk `Sz  ` UntF tag may be `#Pnt` (points) not `#Pxl` (pixels)

**What goes wrong:** In `ParseFrFXDescriptor`, `"Sz  "` with ostype `"UntF"` skips 4 bytes for the unit tag and reads a double (PsdParser.cpp:1011: `Pos += 4; // skip unit tag (#Pxl)`). In vstk descriptors for shape strokes, Photoshop may write `"#Pnt"` (points) rather than `"#Pxl"` (pixels). At 72 DPI, 1pt = 1px, so both give the same value. At 144 DPI PSD documents the values diverge by 2x.

**Prevention:** Read the 4-byte unit tag explicitly and convert:
```cpp
char UnitTag[5] = {};
for (int k = 0; k < 4; ++k) UnitTag[k] = static_cast<char>(Data[Pos + k]);
Pos += 4;
const double SzRaw = ReadDoubleBE();
// Convert to pixels:
double SzPx = SzRaw;
if (FCStringAnsi::Strcmp(UnitTag, "#Pnt") == 0)
    SzPx = SzRaw * (72.0 / PsdDpi); // where PsdDpi comes from FPsdDocument
else if (FCStringAnsi::Strcmp(UnitTag, "#Prc") == 0)
    SzPx = SzRaw / 100.0 * ReferenceDimension;
```
This same fix applies to any other `UntF` value read in vstk or PtFl descriptors (scale, etc.).

**Phase:** Stroke-rendering parser phase — low urgency for 72 DPI PSDs, but prevents silent 2x errors on high-DPI PSDs.

---

### MiP-02: `ScanRawLfx2Blocks` forward-luni heuristic may associate stroke with the wrong layer in dense PSDs

**What goes wrong:** `ScanRawLfx2Blocks` scans FORWARD from the lfx2 block's end to find the next `8BIM+luni` block and associates the stroke with that layer name (PsdParser.cpp:1646-1651). In dense PSDs (50+ layers), the forward scan may cross a layer record boundary and pick up a different layer's luni, producing a stroke attributed to the wrong layer name.

**Why it happens:** The raw byte scan was the only option because PhotoshopAPI v0.9 silently drops lfx2 blocks. This constraint does NOT apply to vstk: Enum.h:900 confirms `"vstk"` maps to `TaggedBlockKey::vecStrokeData`, meaning vstk IS accessible via `unparsed_tagged_blocks()`.

**Prevention:** The new vstk scanner must use the structured API (not raw byte scanning):
```cpp
for (const auto& Block : InLayer->unparsed_tagged_blocks())
{
    if (!Block || Block->getKey() != NAMESPACE_PSAPI::Enum::TaggedBlockKey::vecStrokeData)
        continue;
    // ... parse Block->m_Data
}
```
This is per-layer, scoped correctly, and cannot misattribute strokes. The raw scan technique should remain only for lfx2 where it is necessary.

**Phase:** Stroke-rendering parser phase.

---

### MiP-03: `EPsdLayerType::PatternFill` missing — PtFl layers fall through to Unknown

**What goes wrong:** In `ConvertLayerRecursive`, the detection block at PsdParser.cpp:1777 checks `TaggedBlockKey::adjGradient` and `TaggedBlockKey::adjSolidColor` but not `TaggedBlockKey::adjPattern`. An AdjustmentLayer with PtFl has none of the checked keys, so it is classified `EPsdLayerType::Unknown`, emits a warning, and produces no widget.

**Prevention:** Add `EPsdLayerType::PatternFill` to the `EPsdLayerType` enum in `PsdTypes.h`, add a `bIsPatternFill` detection branch in `ConvertLayerRecursive` alongside the existing GdFl/SoCo checks, and register `FPatternFillLayerMapper` at priority 101 (matching `FSolidFillLayerMapper` and `FShapeLayerMapper` post-Phase-20) to ensure deterministic dispatch ahead of `FImageLayerMapper`.

**Phase:** Pattern fill mapper phase (follows parser verification in MP-02).

---

## Phase-Specific Warnings

| Phase Topic | Likely Pitfall | Mitigation |
|---|---|---|
| vstk descriptor parser | CP-01: wrong offset (0 not 4 or 8) | Emit hex dump; try `TryParseAt(0)` first |
| vstk / lfx2 stroke coexistence | CP-02: `bHasStroke` double-populated | Add separate `bHasVectorStroke` field |
| frameFXMulti VlLs | CP-03: list silently skipped | Add VlLs branch in `ParseFrFXDescriptor` |
| PtFl descriptor parser | CP-04: walks for `"Clr "`, finds `"Ptrn"` | Hex dump first; build dedicated walker for pattern reference keys |
| Non-ASCII multi-run slicing | CP-05: TCHAR vs UTF-16 code-unit mismatch | Convert to `std::u16string` before slicing by run length |
| lrFX visual confirm | CP-06: HSB/CMYK `sofi` produces wrong FLinearColor | Branch on `ColorSpace` in `sofi` and `dsdw` handlers |
| Image-layer stroke mapper | MP-01: `UImage` has no native outline | Use Overlay + border UImage, or `DrawAs=RoundedBox` with `OutlineSettings` |
| PtFl pixel rasterization | MP-02: `RGBAPixels` empty for AdjustmentLayer | Verify empirically; use flatten fallback if empty |
| Non-text D-13 guard | MP-03: generator FX-03 processes residual `bHasStroke` | Use separate field (CP-02) or add `RouteShapeEffects` |
| Non-ASCII markup | MP-04: half-surrogates in markup string | Sanitize `Span.Text` after CP-05 fix |
| vstk UntF size units | MiP-01: `#Pnt` silently treated as `#Pxl` | Read and branch on unit tag |
| lfx2 raw scan attribution | MiP-02: forward-luni misattributes in dense PSDs | Use `unparsed_tagged_blocks()` for vstk (structured API) |
| PtFl dispatch | MiP-03: falls through to `EPsdLayerType::Unknown` | Add `EPsdLayerType::PatternFill` + detection branch |

---

## Sources

All findings derived from direct codebase analysis:
- `Source/PSD2UMG/Private/Parser/PsdParser.cpp` — lrFX walker (lines 618-804), lfx2 raw scanner (lines 806-1697), SoCo walker (lines 1076-1295), vscg walker (lines 1298-1547), UTF-16 span TODO (lines 469-478), multi-run slicing (lines 480-596)
- `Source/PSD2UMG/Public/Parser/PsdTypes.h` — `FPsdLayerEffects`, `FPsdTextRunSpan`, `FPsdLayer` struct definitions
- `Source/PSD2UMG/Private/Mapper/FRichTextLayerMapper.cpp` — `EscapeMarkup`, `BuildMarkup`, `CreateStyleTableAsset`
- `Source/PSD2UMG/Private/Mapper/FShapeLayerMapper.cpp` — line 13 comment confirming vstk attachment point
- `Source/ThirdParty/PhotoshopAPI/Win64/include/PhotoshopAPI/Util/Enum.h` — `TaggedBlockKey` enum (lines 751-925): `"vstk"` = `vecStrokeData` (line 900), `"vscg"` = `vecStrokeContentData` (line 901), `"PtFl"` = `adjPattern` (line 832), `"lrFX"` = `fxLayer` (line 850)
- `.planning/PROJECT.md` — v1.3 scope definition, D-12/D-13 decisions log, Phase 20 state
