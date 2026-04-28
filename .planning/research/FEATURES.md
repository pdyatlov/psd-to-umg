# Feature Landscape

**Domain:** PSD-to-UMG Widget Blueprint import plugin for Unreal Engine 5.7
**Researched:** 2026-04-07 (original); 2026-04-28 (v1.3 Advanced Effects update)

---

## v1.3 Advanced Effects — Research Supplement

This section covers the four target features of the v1.3 milestone. It supersedes older text
where the analysis contradicts this update. Prior competitive landscape sections remain valid.

---

### (1) Stroke on Image/Shape Layers

#### What the PSD stores

Photoshop vector stroke on a shape layer lives in two tagged blocks:
- `vstk` (`vecStrokeData`, key `TaggedBlockKey::vecStrokeData`) — the stroke style: size (px),
  fill type (SolidColor / Gradient / Pattern), position (Inside / Center / Outside), and color
  when fill type is SolidColor.
- `vscg` (`vecStrokeContentData`, `TaggedBlockKey::vecStrokeContentData`) — the fill content
  (already used by FShapeLayerMapper to extract solid fill color via ScanShapeFillColor).

Layer-Style stroke via lfx2/FrFX is the older, non-vector-shape route. That path already
populates `FPsdLayerEffects::bHasStroke`, `StrokeSize`, and `StrokeColor` via
ScanRawLfx2Blocks → ParseFrFXDescriptor → ExtractLfx2Stroke. **This data is already in
FPsdLayer.Effects for all layer types; the gap is only on the rendering (mapper) side for
non-text layers.**

The vstk descriptor uses the same Photoshop descriptor binary format as vscg and lfx2/FrFX.
Key field names (from psd-tools analysis, MEDIUM confidence):
- `strokeStyleLineAlignment` — enum: `strokeStyleInsetFrame` (inside), `strokeStyleOutsetFrame`
  (outside), `strokeStyleDefaultFrame` (center)
- `strokeStyleLineWidth` — UntF (#Pxl) double giving pixel width
- `strokeStyleFillType` — enum discriminator (`SolidColor`, `Gradient`, `Pattern`)
- `strokeStyleContent` — Objc containing the fill (RGBC for solid color, same structure as
  FrFX's `Clr ` sub-descriptor)
- `strokeStyleLineCapType`, `strokeStyleLineJoinType` — line cap/join, not needed for UMG

#### Designer expectation

Designers expect a stroke to be a visible colored border around the element. In Photoshop, a
shape layer with a 3px outside stroke means 3px of color around the outside of the shape
rectangle. For inside strokes, the color appears inside the shape boundary. Center straddles.

#### UMG representation options — analysis

**Option A: `ESlateBrushDrawType::Border` on the UImage brush.**
`ESlateBrushDrawType::Border` is Slate's nine-slice draw mode that leaves the center transparent
and draws only the outer border sections using the margin values. This is literally a stroke: a
frame of pixels around the edge. Key facts confirmed from UE source research:
- `DrawAs = ESlateBrushDrawType::Border` renders only the margin region; center is hollow.
- Uses `FMargin Margin` on the brush to define the width of each edge strip.
- Requires a texture (the texture provides the source pixels for the border strips).
- Maps cleanly to `strokeStyleLineWidth` → uniform `FMargin(StrokePx / TextureWidth)`.
- **Limitation:** Requires a texture. For a shape layer that renders as a solid-color
  zero-texture UImage, there is no texture to sample from. This makes the Border draw mode
  unusable for zero-texture shapes unless a synthetic 1x1 white texture is manufactured.

**Option B: `ESlateBrushDrawType::Box` (existing 9-slice mode) with padding.**
The Box draw mode also uses nine-slice but fills the center. Not a stroke.

**Option C: Overlay UImage sibling at expanded size (same pattern as drop shadow).**
The drop shadow is implemented as a sibling UImage positioned behind the main image with a
color tint. The same pattern can produce a stroke: insert a sibling UImage at size
`(W + 2*StrokePx, H + 2*StrokePx)`, offset by `(-StrokePx, -StrokePx)`, colored with
`StrokeColor`, placed at ZOrder main-1. This approximates an "outside" stroke without a
texture. For an "inside" stroke the sibling would be smaller at same position, placed at
ZOrder main+1 (in front, clipped by the parent). **This is the recommended approach for
shape layers because it matches the established drop-shadow pattern, requires zero texture,
and is the only approach that does not depend on DrawAs::Border texture sampling.**

**Option D: UBorder widget wrapping.**
UBorder is a container widget with a background brush and padding. It can contain one child.
Using UBorder as a stroke wrapper means re-parenting the existing UImage inside a UBorder,
which restructures the widget tree in a way that breaks positional layout (the UBorder must
be sized and positioned in the canvas slot, not the UImage). This is invasive and fragile.
Not recommended.

#### Recommendation (MEDIUM confidence)

For **image/shape layers with lfx2 stroke** (data already in `Effects.bHasStroke`):
- Use the **sibling overlay pattern** (Option C), consistent with the drop-shadow implementation.
- Sibling is a solid-color zero-texture UImage at `(W + 2*StrokePx) x (H + 2*StrokePx)`,
  offset `(-StrokePx, -StrokePx)` from the main layer position.
- ZOrder = main - 1 for outside stroke (behind main), main + 1 for inside stroke (in front,
  renderer clips naturally when it overdraws the parent).
- Canvas-only (same constraint as drop shadow; warn on non-canvas parent).
- FShapeLayerMapper comment at line 13 already identifies this as the intended attachment point.

For **vstk descriptor** (vector stroke on shape layers, additional to lfx2):
- Add `ScanVstkStroke()` static function in PsdParser.cpp mirroring ScanShapeFillColor/
  ScanRawLfx2Blocks patterns. Walk the vstk descriptor for strokeStyleLineWidth and
  strokeStyleContent (RGBC sub-descriptor).
- Populate `Effects.bHasStroke / StrokeSize / StrokeColor` from vstk when lfx2 does not
  already provide stroke data (lfx2 takes priority for shape layers that have both).
- **Complexity: Medium** — descriptor walk is established pattern; new fields in Effects only.

For **frameFXMulti VlLs** (newer Photoshop CS6+ format storing multiple effects as a VlLs list):
- This is a variant of the lfx2 structure where effects are stored as a `VlLs` (value list)
  within the descriptor instead of a single `FrFX` object. The ParseFrFXDescriptor walker
  already handles `VlLs` skip logic; reading VlLs items would require iterating the list and
  dispatching per-item type.
- **Complexity: Medium-High** — requires extending ParseFrFXDescriptor to walk VlLs items.
  Defer to post-v1.3 unless real-world PSDs require it.

#### D-13 double-render guard

RouteTextEffects already clears `bHasStroke` after routing to `Text.OutlineSize`. The stroke
sibling renderer in FWidgetBlueprintGenerator must NOT fire when `bHasStroke` is false, and
must guard against double-rendering: once the sibling is emitted for an Image layer, the
generator's FX-03 block (color overlay) must not also process the stroke flag.

---

### (2) Pattern Fill Layers

#### What the PSD stores

A pattern fill layer in Photoshop is stored as an adjustment layer with the `PtFl`
(`adjPattern`, `TaggedBlockKey::adjPattern`) tagged block. PhotoshopAPI v0.9 maps `PtFl` to
`TaggedBlockKey::adjPattern` (confirmed from `Enum.h` line 832).

The `PtFl` descriptor (same binary descriptor format as SoCo/GdFl) carries:
- `Ptrn` — Objc with pattern identifier (`ID` uuid string, `Nm  ` display name)
- `Scl ` — UntF (#Prc) double, scale percentage (default 100)
- `Lnkd` — bool, whether pattern is linked to the layer transform
- `phase` — Point2 Objc with `Hrzn`/`Vrtc` doubles for tile offset

The pattern tile pixels are stored separately either inline (in a `lrPatterns`/`Pat2`/`Pat3`
block elsewhere in the PSD file) or embedded. Pattern data is often in the PSD's global
pattern resources section (additional layer info).

The actual rendered appearance of a pattern fill in UMG context has no direct widget
equivalent — UMG has no native tiling texture widget. The realistic approximations are:

**Option A: Flatten to PNG (existing flatten fallback).** Render the pattern layer with its
effects in a PNG via PhotoshopAPI's composited pixel data and import as UTexture2D. Existing
`RGBAPixels` on the layer can already carry composited data; the flatten path handles this.
**Recommended for v1.3.** No new infrastructure needed; just ensure EPsdLayerType::Gradient
(the current fallback for non-solid fill layers) also handles PtFl with the same flatten path.

**Option B: Baked tiled texture UImage with Material.** Create a UMaterial with a Texture
tiling node, set the tile UVs from the `Scl ` field. This is the "correct" UMG approximation
but requires a Material asset to be created and wired up at import time. Very high complexity,
fragile.

**Option C: Set EPsdLayerType::Pattern and route to flatten.** Introduce a distinct layer type
for pattern fills, then in the mapper registry ensure Pattern layers always use the flatten path.
This gives diagnostic clarity without new widget generation logic.

#### Recommendation (MEDIUM confidence)

Classify pattern fill layers as a new `EPsdLayerType::Pattern` (mirrors how Gradient and
SolidFill have distinct types). Route `EPsdLayerType::Pattern` through the existing flatten
fallback: the layer's composited `RGBAPixels` are already the correct tiled appearance from
PhotoshopAPI's compositor. A new `FPatternFillLayerMapper` at priority 101 calls the existing
`FTextureImporter::ImportLayer` and returns a standard UImage. **No new UMG widget type or
material infrastructure needed.**

Parser change: in `ConvertLayerRecursive`, detect `TaggedBlockKey::adjPattern` presence (same
scan pattern as `adjSolidColor` / `adjGradient`) and set `OutLayer.Type = EPsdLayerType::Pattern`.
The `RGBAPixels` already hold the composited appearance — no additional pixel extraction needed.

**Complexity: Low** — one new EPsdLayerType enum value, one scanner check (3 lines), one new
mapper class that is a thin wrapper over FImageLayerMapper.

---

### (3) lrFX RGBC Channel Order — Color Overlay and Drop Shadow

#### What the code currently does (from PsdParser.cpp, verified)

The `sofi` (color overlay) parser at lines 700-720:
- Reads: Version (4), Blend-sig "8BIM" (4), BlendKey (4), ColorSpace uint16 (2), then
  4 × uint16 channels at 16-bit each (8 bytes total), Opacity uint8, Enabled uint8.
- The channels are read as `C0, C1, C2, _` (fourth channel discarded).
- No ARGB swizzle: C0 → R, C1 → G, C2 → B in the resulting FLinearColor.

The `dsdw` (drop shadow) parser at lines 730-775:
- Reads: Version(4), Blur(4), Intensity(4), Angle(4), Distance(4), ColorSpace(2),
  4 × uint16 channels (8 bytes) = C0,C1,C2,C4, Blend-sig "8BIM"(4), BlendKey(4),
  Enabled(1), UseAngle(1), Opacity(1).
- Channels: C0 → R, C1 → G, C2 → B, no swizzle.

#### What the PSD spec says (confirmed from paulbourke.net spec)

The PSD `lrFX` color structure is:
- 2 bytes: ColorSpaceID (0 = RGB, 1 = HSB, 2 = CMYK, 7 = Lab, 8 = Grayscale)
- 8 bytes: four uint16 channel values (each in range 0–65535)

For RGB color space, the four channels are `R, G, B, _` (fourth is padding/unused).
This is **not ARGB**. The existing code (C0=R, C1=G, C2=B) is correct for RGB space.

#### The ARGB path that does exist — text fill color

PhotoshopAPI's `style_run_fill_color()` returns text fill color as **ARGB doubles in [0..1]**:
`[A, R, G, B]`. This was empirically verified at line 316-326 of PsdParser.cpp (comment:
"PhotoshopAPI returns text fill color as ARGB doubles in [0..1], verified empirically against
a pure-red (#FF0000) fixture which came back as [1.0, 1.0, 0.0, 0.0] = (A=1, R=1, G=0, B=0)").

**Key distinction:**
- `lrFX` binary color (sofi/dsdw): `ColorSpace(2) + uint16 R + uint16 G + uint16 B + uint16 pad`
  → divide by 65535 → float R,G,B. **No A prefix.** The `A` comes from a separate `Opacity` byte.
- PhotoshopAPI `style_run_fill_color()`: returns `[A, R, G, B]` as doubles in [0..1]. **A is first.**
- FrFX/lfx2 stroke (ParseFrFXDescriptor): uses RGBC Objc sub-descriptor with named keys
  `Rd  `, `Grn `, `Bl  ` → named-key assignment, no position-based swizzle needed.

#### Is there a bug to fix?

The lrFX `sofi` parser in the existing code is **correct**: it reads C0→R, C1→G, C2→B,
with no ARGB swizzle, consistent with the PSD spec's `R, G, B, pad` layout for RGB colorspace.

The v1.3 "lrFX RGBC channel-order visual confirm" requirement is a **human UAT confirmation**:
visually verify on a real host project that color overlay and drop shadow colors match what
Photoshop shows. There is no known code defect from the static analysis above. The milestone
task is to run the fixture through a real UE 5.7 project and diff the color values visually.

**Confidence: HIGH** (code verified against both PSD spec layout and the empirical test
comment at line 316 that documents the one genuine ARGB quirk — in the text fill path only).

---

### (4) Non-ASCII Rich Text — UTF-16 Run Length Counts vs UTF-8 FString Indexing

#### The problem (from PsdParser.cpp lines 469-514)

PhotoshopAPI's `style_run_lengths()` returns code-unit counts per the PSD spec. The PSD
`TySh` text engine descriptor stores text content as UTF-16 and run lengths as UTF-16
**code-unit counts** (not code-point counts, not UTF-8 byte counts).

PhotoshopAPI exposes the text content via `text()` as a UTF-8 `std::string`. The current
code converts this UTF-8 string to an FString and then slices it with `FString::Mid(CharOffset,
RunLen)`. On Windows, `TCHAR` is 2 bytes (UCS-2/UTF-16LE). On all platforms, `FString::Mid`
counts `TCHAR` units, not Unicode code points.

**For ASCII text:** UTF-16 code unit count = UTF-8 byte count = FString TCHAR count = Unicode
code point count. No mismatch. Current code works.

**For non-ASCII (CJK, emoji, etc.):**
- A CJK character (e.g., U+4E2D) is 1 UTF-16 code unit = 3 UTF-8 bytes.
- An emoji in the BMP (e.g., U+1F600, GRIMACING FACE) is 2 UTF-16 code units (surrogate pair)
  = 4 UTF-8 bytes = 1 Unicode code point.
  - On Windows, FString stores it as 2 TCHAR units.
  - `FString::Mid(offset, 2)` extracts it correctly on Windows.
- The run length from PSD is 2 (UTF-16 code units) for that emoji. `FString::Mid(offset, 2)`
  on Windows extracts exactly the right number of TCHAR units. **On Windows with TCHAR=2 bytes,
  FString::Mid indexing matches UTF-16 code unit indexing for all BMP characters.**
  Surrogate pair emoji (U+1F000+, 2 UTF-16 code units) = 2 TCHAR = 1 visible character.
  Current slicing produces the right TCHAR count from PSD's UTF-16 run length.

**The actual risk:**
1. UTF-8 std::string `text()` → `FString` conversion via `Utf8ToFString()`. This function must
   handle multi-byte UTF-8 correctly. If it truncates or corrupts multi-byte sequences, CJK and
   emoji will be garbled before slicing even starts.
2. PhotoshopAPI's `text()` is the canonical content string — it must correctly decode the PSD's
   UTF-16 text into UTF-8. This is PhotoshopAPI's responsibility; inspect its implementation
   if garbling occurs.
3. `FString::Len()` / `FString::Mid()` on Windows count TCHAR (2-byte) units, matching
   UTF-16 code unit semantics. **This is correct for BMP characters and surrogate pairs.**
4. `CharOffset` arithmetic: `CharOffset += Clipped` advances by TCHAR/UTF-16-unit count, not
   code point count. Because PSD run lengths are also UTF-16 units, this is consistent.

**For non-BMP code points (emoji above U+FFFF, e.g. U+1F600):**
- Stored in PSD as a surrogate pair = 2 UTF-16 code units.
- PhotoshopAPI decodes to UTF-8: 4 bytes.
- `FString` on Windows stores as 2 TCHAR units (surrogate pair preserved).
- PSD run length = 2 UTF-16 code units.
- `FString::Mid(offset, 2)` extracts 2 TCHAR = the full surrogate pair = correct emoji.
- **No bug for surrogate pair emoji on Windows.**

**Real issue — Utf8ToFString conversion:**
The current code calls `Utf8ToFString(FullUtf8)` where `FullUtf8` is a `std::string` UTF-8
from PhotoshopAPI. Unreal's `UTF8_TO_TCHAR` macro (used internally by `Utf8ToFString` or its
equivalent) correctly handles multi-byte UTF-8 and produces TCHAR. The only failure mode is if
`FullUtf8` is malformed UTF-8 (PhotoshopAPI's responsibility) or if `Utf8ToFString` truncates
at the first embedded null (relevant if the text contains `\0` which PSD sometimes appends as
a sentinel).

#### Fix recommendation

The TODO comment at line 469 is overly conservative. On Windows (primary target), TCHAR = 2
bytes and FString::Mid indexing is equivalent to UTF-16 code unit indexing. The fix needed is:

1. **Ensure `Utf8ToFString` handles multi-byte sequences.** Verify it does not use
   `FCStringAnsi::Strlen` (null-terminated, breaks at embedded nulls) but uses explicit byte
   count from `FullUtf8.size()`.
2. **Strip trailing null sentinel from `FullUtf8` before conversion.** PSD text content
   typically ends with `\r` or `\0`. If the last UTF-8 byte is `\0`, `std::string::size()`
   includes it but FString conversion may emit an extra null TCHAR that shifts the span count.
3. **Add a CJK fixture** (RichTextCJK.psd) with at least one Chinese/Japanese character run
   to make the claim empirical rather than theoretical.

**Complexity: Low** — code change is 3-5 lines to use `FullUtf8.size()` explicitly in
conversion, plus a fixture. The existing TCHAR-indexing approach is correct for UTF-16-unit
semantics on Windows; no algorithmic rewrite required.

---

## Table Stakes (for v1.3 scope)

Features users expect if the v1.3 milestone is to be considered complete.

| # | Feature | Why Expected | Complexity | Depends On |
|---|---------|-------------|------------|-----------|
| S-1 | **lrFX color overlay visual confirm** | Existing FX-03 code path; v1.3 closes the gap between "code written" and "confirmed correct in a real project" | Low | Human UAT run |
| S-2 | **Stroke rendering on image layers** (lfx2/FrFX data already populated in `Effects.bHasStroke`) | `D-12 data already populated, needs mapper` is the exact v1.3 scope statement | Medium | Existing `FPsdLayerEffects.bHasStroke/StrokeSize/StrokeColor` |
| S-3 | **Non-ASCII rich text span slicing fix** | Multi-run text with CJK or emoji will misalign runs at import time; crash or visual corruption on first real CJK project | Low | `FString::Mid` UTF-16 semantics confirmed; Utf8ToFString fix |

---

## Differentiators (for v1.3 scope)

Features that make v1.3 stand out beyond the baseline correction work.

| # | Feature | Value Proposition | Complexity | Depends On |
|---|---------|-------------------|------------|-----------|
| D-1 | **vstk descriptor parsing** (vector stroke on shape layers, separate from lfx2) | Closes the other stroke source; shapes drawn with Photoshop's pen tool carry vstk, not lfx2 | Medium | New `ScanVstkStroke()` function; extends `Effects` struct |
| D-2 | **Pattern fill layer import** (PtFl → EPsdLayerType::Pattern → flatten UImage) | Designers use pattern overlays for textures, noise, fabric, etc.; currently silently discarded | Low | Existing flatten path + new enum value |
| D-3 | **frameFXMulti VlLs stroke** (newer Photoshop CS6+ effects format, VlLs list within lfx2 descriptor) | Newer PSDs from CS6+ use this format; `ParseFrFXDescriptor` must walk VlLs items to find FrFX | Medium-High | Extension of `ParseFrFXDescriptor`; adds iteration |
| D-4 | **CJK/emoji rich text fixture** (RichTextCJK.psd with Chinese characters + emoji spans) | Makes non-ASCII correctness testable and verifiable; turns S-3 from a theoretical fix into a proven fix | Low | Fixture authoring |

---

## Anti-Features (for v1.3 scope)

Things that seem useful but should not be built in v1.3.

| Anti-Feature | Why Avoid | What to Do Instead |
|-------------|-----------|-------------------|
| **Material-based tiling for pattern fills** | Requires creating a UMaterial asset at import time, wiring a Tiling UV node, linking a Texture2D — very high complexity for a decorative effect | Use flatten fallback: composited PNG is always correct, zero extra assets |
| **Pattern fill tile offset preservation** (`phase` field in PtFl descriptor) | Tile offset is a sub-pixel aesthetic detail; UMG has no native way to offset a tiled texture without a custom material | Bake into the composited PNG (PhotoshopAPI includes phase in compositing) |
| **Inside vs Outside vs Center stroke precision** | True inside/outside stroke requires clipping masks not available in UMG; the sibling overlay approximation is ±StrokePx off for inside strokes | Document the approximation; note that outside stroke (most common design usage) is pixel-accurate with the sibling pattern |
| **Stroke on non-canvas children** | Drop shadow already has this limitation; adding a sibling requires UCanvasPanel; inside non-canvas panels it is a no-op + warning | Warn, skip, same as drop shadow |
| **Global color-space conversion** (CMYK/Lab lrFX effects) | lrFX ColorSpaceID can be CMYK or Lab; converting those to RGB for UMG requires color profile data not available at import time | Assume RGB (ColorSpaceID == 0) for effects; warn and use white fallback for non-RGB effects |

---

## Feature Dependencies (v1.3 specific)

```
Effects.bHasStroke (already populated by ExtractLfx2Stroke)
    └── Stroke sibling renderer in FWidgetBlueprintGenerator (canvas-only, same as drop shadow)

vstk descriptor scan (new ScanVstkStroke)
    └── populates Effects.bHasStroke when lfx2 not present
    └── feeds same Stroke sibling renderer above

PtFl tagged block detection
    └── EPsdLayerType::Pattern (new enum value)
    └── FPatternFillLayerMapper (thin FImageLayerMapper wrapper)

Utf8ToFString size-explicit conversion
    └── fixes UTF-8→TCHAR for multi-byte characters
    └── span slicing via FString::Mid now correct for CJK / BMP emoji
    └── RichTextCJK.psd fixture verifies end-to-end
```

---

## UMG Widget Choices Justified

| Effect | UMG Widget | Rationale |
|--------|-----------|-----------|
| Stroke on Image/Shape | UImage sibling at expanded size | Matches drop-shadow pattern; zero-texture; canvas-only constraint explicit |
| Stroke alternative (texture) | ESlateBrushDrawType::Border on UImage brush | Valid only when texture present; requires uniform margin; cannot approximate solid-fill shapes |
| Pattern fill | UImage with FlattenedTexture | Composited PNG is always visually correct; no Material needed; DPI-clean |
| Color overlay (existing) | UImage TintColor / UPanelWidget deferred overlay | Confirmed working; v1.3 is UAT confirmation only |

---

## Competing Tools (original section, unchanged)

Before defining features, here is the landscape of existing tools that solve similar problems. This informs what is table stakes vs. differentiating.

### PSD2UMG (durswd) - Marketplace, $14.99
**Engine:** UE4 only (no UE5 version found)
**Approach:** Custom `.psdumg` file extension, C++ plugin
**Supported widgets:** Image, Button, ProgressBar, CanvasPanel, Text (Beta)
**Naming convention:** `LayerName@Button`, state variants via `[Normal]`/`[Hovered]`/`[Pressed]`
**Strengths:**
- Automatic anchor assignment
- Reimport preserves manual edits (only updates layers with matching names)
- Pure C++ (no Python dependency)
**Weaknesses:**
- No layer styles or shape layers
- Only 8-bit RGB
- Text is Beta-quality (position and string only, no font/size/color)
- Requires renaming `.psd` to `.psdumg` manually
- No ListView/TileView/ScrollBox support
- No 9-slice support
- UE4 only, appears unmaintained for UE5
**Confidence:** MEDIUM (based on official docs at GitHub + marketplace listing)

### Summary of Competitive Gap

No existing UE5 PSD tool supports: stroke rendering, pattern fill classification, non-ASCII
multi-run text, or the pipeline correctness level this plugin targets. v1.3 extends the
correctness lead.

---

## Table Stakes (original v1.0 set, still valid)

| # | Feature | Why Expected | Complexity | Existing? |
|---|---------|-------------|------------|-----------|
| 1 | **One-click PSD import** | Core value proposition | Med | Yes (v1.0) |
| 2 | **Layer hierarchy preservation** | Designers structure PSDs with intent | Low | Yes (v1.0) |
| 3 | **Image layer extraction** | Images are 80%+ of any UI mockup | Med | Yes (v1.0) |
| 4 | **Text layer extraction** | Second most common layer type | Med | Yes (v1.0) |
| 5 | **Button widget generation** | Most common interactive widget | Med | Yes (v1.0) |
| 6 | **Correct positioning** | If elements are wrong position, output is useless | Low | Yes (v1.0) |
| 7 | **Font mapping** | Fonts never match 1:1 between PS and UE | Low | Yes (v1.0) |
| 8 | **DPI-correct text sizing** | Without this, all text is wrong size | Low | Yes (v1.0) |
| 9 | **Text stroke/outline and drop shadow** | Most common text effects in game UI | Low | Yes (v1.0) |
| 10 | **Color overlay effect** | Extremely common PSD tinting technique | Low | Yes (v1.0) |
| 11 | **No Python dependency** | Production plugin cannot require Python | High | Yes (v1.0) |
| 12 | **Layer visibility respected** | Hidden layers pollute widget tree | Low | Yes (v1.0) |
| 13 | **Unique widget names** | PSD allows duplicates; UMG does not | Low | Yes (v1.0) |

---

## Sources

- PsdParser.cpp lines 456-614 (multi-run span extraction, UTF-16 TODO comment, ARGB empirical note)
- PsdParser.cpp lines 618-795 (lrFX ExtractLayerEffects — sofi/dsdw color layout)
- PsdParser.cpp lines 826-1074 (ParseFrFXDescriptor — FrFX/lfx2 stroke, RGBC named-key path)
- PsdParser.cpp lines 1094-1296 (ScanSolidFillColor — RGBC sub-descriptor, SoCo)
- PsdParser.cpp lines 1549-1700 (ScanRawLfx2Blocks — raw lfx2 scanner for stroke)
- PhotoshopAPI Enum.h lines 820-901 (vecStrokeData/vecStrokeContentData/adjPattern key mappings)
- FShapeLayerMapper.cpp line 13 (stroke attachment note for vstk)
- FWidgetBlueprintGenerator.cpp lines 305-494 (drop-shadow sibling pattern, color overlay defer)
- FTextLayerMapper.cpp lines 101-110 (FSlateFontInfo::OutlineSettings for text stroke)
- psd-tools effects.py analysis: Stroke class with _ColorMixin/_PatternMixin (MEDIUM confidence)
- psd-tools color.py: Color class = ColorSpaceID + 4 × uint16 channels (MEDIUM confidence)
- psd-tools tagged_blocks.py: VECTOR_STROKE_DATA = DescriptorBlock, no custom color logic
- paulbourke.net PSD spec: vstk/PtFl = "4 bytes version + variable descriptor", minimal detail
- UE forums: Box vs Border DrawAs — "Box fills center, Border leaves center empty" (MEDIUM confidence)
- ESlateBrushDrawType::Type enum: Image/Box/Border/NoDrawType confirmed from UE5.7 docs
